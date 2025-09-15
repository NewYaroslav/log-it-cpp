// detail/MpscRingAny.hpp
#ifndef _LOGIT_DETAIL_MPSC_RING_ANY_HPP_INCLUDED
#define _LOGIT_DETAIL_MPSC_RING_ANY_HPP_INCLUDED

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

namespace logit { namespace detail {

    /// \brief Bounded MPSC ring buffer with arbitrary capacity (C++11).
    /// \tparam T Stored type.
    template <class T>
    class MpscRingAny {
    private:
        /// \brief Single cell storing sequence number and raw storage for T.
        struct Cell {
            std::atomic<std::size_t> m_seq;
            typename std::aligned_storage<sizeof(T), alignof(T)>::type m_storage;
        };
    
    public:
        /// \brief Construct ring with given capacity (>= 2).
        explicit MpscRingAny(std::size_t capacity)
            : m_cap(capacity < 2 ? 2 : capacity),
              m_cells(new Cell[m_cap]),
              m_enqueue_pos(0),
              m_dequeue_pos(0) {
            for (std::size_t i = 0; i < m_cap; ++i) {
                m_cells[i].m_seq.store(i, std::memory_order_relaxed);
            }
        }
    
        /// \brief Destroy remaining elements, if any.
        ~MpscRingAny() {
            destroy_all_();
        }

        MpscRingAny(const MpscRingAny&) = delete;
        MpscRingAny& operator=(const MpscRingAny&) = delete;

        /// \brief Move construct ring, adopting producer/consumer indices.
        MpscRingAny(MpscRingAny&& other) noexcept
            : m_cap(other.m_cap),
              m_cells(std::move(other.m_cells)),
              m_enqueue_pos(other.m_enqueue_pos.load(std::memory_order_relaxed)),
              m_dequeue_pos(other.m_dequeue_pos.load(std::memory_order_relaxed)) {
            other.m_cap = 0;
            other.m_enqueue_pos.store(0, std::memory_order_relaxed);
            other.m_dequeue_pos.store(0, std::memory_order_relaxed);
        }

        /// \brief Move assign ring, draining existing payload.
        MpscRingAny& operator=(MpscRingAny&& other) noexcept {
            if (this != &other) {
                destroy_all_();
                m_cap = other.m_cap;
                m_cells = std::move(other.m_cells);
                m_enqueue_pos.store(other.m_enqueue_pos.load(std::memory_order_relaxed),
                    std::memory_order_relaxed);
                m_dequeue_pos.store(other.m_dequeue_pos.load(std::memory_order_relaxed),
                    std::memory_order_relaxed);
                other.m_cap = 0;
                other.m_enqueue_pos.store(0, std::memory_order_relaxed);
                other.m_dequeue_pos.store(0, std::memory_order_relaxed);
            }
            return *this;
        }
    
        /// \brief Capacity of the ring.
        std::size_t capacity() const noexcept { return m_cap; }
    
        /// \brief Try to enqueue value. Non-blocking.
        /// \return true on success; false if queue is full.
        template <class U>
        bool try_push(U&& v) noexcept {
            std::size_t pos = m_enqueue_pos.load(std::memory_order_relaxed);
            for (;;) {
                Cell& c = m_cells[pos % m_cap];
                std::size_t seq = c.m_seq.load(std::memory_order_acquire);
    
                // When free, seq == pos
                std::intptr_t diff =
                    static_cast<std::intptr_t>(seq) - static_cast<std::intptr_t>(pos);
    
                if (diff == 0) {
                    if (m_enqueue_pos.compare_exchange_weak(
                            pos, pos + 1,
                            std::memory_order_relaxed,
                            std::memory_order_relaxed)) {
                        // We own the cell; construct T in-place.
                        new (&c.m_storage) T(std::forward<U>(v));
                        // Publish element.
                        c.m_seq.store(pos + 1, std::memory_order_release);
                        return true;
                    }
                    // CAS failed; 'pos' updated, retry.
                } else if (diff < 0) {
                    // Full.
                    return false;
                } else {
                    // Another producer advanced 'pos'; reload.
                    pos = m_enqueue_pos.load(std::memory_order_relaxed);
                }
            }
        }
    
        /// \brief Try to dequeue value into out. Non-blocking.
        /// \return true on success; false if queue is empty.
        bool try_pop(T& out) noexcept {
            std::size_t pos = m_dequeue_pos.load(std::memory_order_relaxed);
            Cell& c = m_cells[pos % m_cap];
            std::size_t seq = c.m_seq.load(std::memory_order_acquire);
    
            // When ready, seq == pos + 1
            std::intptr_t diff =
                static_cast<std::intptr_t>(seq) - static_cast<std::intptr_t>(pos + 1);
    
            if (diff == 0) {
                if (!m_dequeue_pos.compare_exchange_strong(
                        pos, pos + 1,
                        std::memory_order_relaxed,
                        std::memory_order_relaxed)) {
                    return false; // Single consumer: should be rare.
                }
    
                T* p = reinterpret_cast<T*>(&c.m_storage);
                out = std::move(*p);
                p->~T();
    
                // Mark cell free for next cycle.
                c.m_seq.store(pos + m_cap, std::memory_order_release);
                return true;
            }
    
            return false; // Empty or not yet published.
        }
    
        /// \brief Lightweight emptiness check for current consumer position.
        bool empty() const noexcept {
            if (!m_cells || m_cap == 0) {
                return true;
            }
            std::size_t pos = m_dequeue_pos.load(std::memory_order_acquire);
            const Cell& c = m_cells[pos % m_cap];
            std::size_t seq = c.m_seq.load(std::memory_order_acquire);
            std::intptr_t diff =
                static_cast<std::intptr_t>(seq) - static_cast<std::intptr_t>(pos + 1);
            return diff != 0;
        }

    private:
        void destroy_all_() noexcept {
            if (!m_cells || m_cap == 0) {
                return;
            }
            T tmp;
            while (try_pop(tmp)) {
                // Element destroyed via move-from tmp
            }
        }

        std::size_t                     m_cap;
        std::unique_ptr<Cell[]>         m_cells;
        alignas(64) std::atomic<std::size_t> m_enqueue_pos;
        alignas(64) std::atomic<std::size_t> m_dequeue_pos;
    };

}} // namespace logit::detail

#endif // _LOGIT_DETAIL_MPSC_RING_ANY_HPP_INCLUDED
