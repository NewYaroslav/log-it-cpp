#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <condition_variable>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <vector>
#include <cmath>

namespace logit_bench {

    /**
     * Lock-free-ish recorder for latency samples:
     *  - begin(record=true) reserves a slot and captures t0 (steady_clock) AFTER reservation.
     *  - complete(token) stores (t1 - t0) into that slot, deduplicated (at most once per slot).
     *  - complete_slot(slot) stores (now - t0[slot]) into that slot, deduplicated.
     *  - finalize() returns p50/p99/p99.9 using nearest-rank (ceil) on a sorted copy.
     *
     * Thread-safety: concurrent writers store into distinct preallocated slots.
     * Dedup: per-slot atomic flag prevents double completion from counting twice.
     */
    class LatencyRecorder {
    public:
        struct Token {
            std::uint64_t slot = invalid_slot();
            std::uint64_t t0_ns = 0;
            bool active = false;
        };
    
        struct Summary {
            std::uint64_t p50_ns  = 0;
            std::uint64_t p99_ns  = 0;
            std::uint64_t p999_ns = 0;
        };
    
        explicit LatencyRecorder(std::size_t total)
            : m_values(total),
              m_t0_ns(total),
              m_expected(total),
              m_next_slot(0),
              m_completed(0),
              m_slot_done(total) {
            for (auto& flag : m_slot_done) {
                flag.store(0, std::memory_order_relaxed);
            }
            // m_values/m_t0_ns are POD; leave uninitialized if you want,
            // but it's safer to keep deterministic state.
            std::fill(m_values.begin(), m_values.end(), 0);
            std::fill(m_t0_ns.begin(), m_t0_ns.end(), 0);
        }
    
        /**
         * Reserve a slot (if record==true) and capture t0 using steady_clock.
         * We take t0 **after** the slot reservation to minimize skew before log().
         *
         * Also stores t0 into internal t0 array, enabling complete_slot(slot).
         */
        Token begin(bool record) {
            Token token;
            token.active = record;
            if (!record) return token;
    
            const auto slot = m_next_slot.fetch_add(1, std::memory_order_relaxed);
            if (slot >= m_expected) {
                throw std::out_of_range("LatencyRecorder capacity exceeded");
            }
    
            token.slot = static_cast<std::uint64_t>(slot);
            token.t0_ns = now();
    
            // Store t0 for "slot-only" completion path (spdlog-friendly).
            // Relaxed is fine: slot is unique per begin(); consumer uses the same slot.
            m_t0_ns[slot] = token.t0_ns;
    
            return token;
        }
    
        /// Capture t1 and store (t1 - t0) into the reserved slot (deduplicated).
        void complete(const Token& token) {
            if (!token.active) return;
            complete_impl(token.slot, token.t0_ns);
        }
    
        /**
         * Complete by slot only (uses t0 stored in begin()).
         * Useful when transport only carries the slot (e.g. spdlog payload "slot|text" or source_loc.line).
         */
        void complete_slot(std::uint64_t slot) {
            if (slot >= m_expected) {
                // In bench code it's OK to ignore silently; for debugging prefer throw.
                throw std::out_of_range("LatencyRecorder capacity exceeded");
            }
            const auto t0 = m_t0_ns[static_cast<std::size_t>(slot)];
            complete_impl(slot, t0);
        }
    
        std::size_t recorded() const {
            return m_next_slot.load(std::memory_order_relaxed);
        }
    
        std::size_t completed() const {
            return m_completed.load(std::memory_order_acquire);
        }
    
        void wait_for_all() const {
            std::unique_lock<std::mutex> lk(m_wait_mx);
            m_wait_cv.wait(lk, [&] {
                return m_completed.load(std::memory_order_acquire) >= m_expected;
            });
        }
    
        Summary finalize() const {
            if (recorded() != m_expected ||
                m_completed.load(std::memory_order_acquire) != m_expected) {
                throw std::runtime_error("Incomplete latency capture");
            }
    
            std::vector<std::uint64_t> sorted = m_values;
            std::sort(sorted.begin(), sorted.end());
    
            Summary summary;
            summary.p50_ns  = pick(sorted, 0.50);
            summary.p99_ns  = pick(sorted, 0.99);
            summary.p999_ns = pick(sorted, 0.999);
            return summary;
        }
    
        static std::uint64_t invalid_slot() {
            return std::numeric_limits<std::uint64_t>::max();
        }
    
        static std::uint64_t now() {
            const auto tp = std::chrono::steady_clock::now().time_since_epoch();
            return static_cast<std::uint64_t>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(tp).count()
            );
        }
    
    private:
        // Nearest-rank percentile with ceil(p * N), clamped to [0..N-1].
        static std::uint64_t pick(const std::vector<std::uint64_t>& data, double p) {
            if (data.empty()) return 0;
            const double r = std::ceil(p * static_cast<double>(data.size()));
            std::size_t idx = (r <= 1.0) ? 0 : (static_cast<std::size_t>(r) - 1);
            if (idx >= data.size()) idx = data.size() - 1;
            return data[idx];
        }
    
        void complete_impl(std::uint64_t slot_u64, std::uint64_t t0_ns) {
            if (slot_u64 >= m_expected) {
                throw std::out_of_range("LatencyRecorder capacity exceeded");
            }
    
            const auto slot = static_cast<std::size_t>(slot_u64);
    
            // Dedup per slot.
            std::uint8_t expected = 0;
            if (!m_slot_done[slot].compare_exchange_strong(
                    expected, 1, std::memory_order_acq_rel)) {
                return; // duplicate completion -> ignore
            }
    
            const auto t1_ns = now();
            m_values[slot] = t1_ns - t0_ns;
    
            const auto done = m_completed.fetch_add(1, std::memory_order_acq_rel) + 1;
            if (done == m_expected) {
                // Need the mutex for CV correctness (avoid lost wakeups).
                std::lock_guard<std::mutex> lk(m_wait_mx);
                m_wait_cv.notify_all();
            }
        }
    
        std::vector<std::uint64_t> m_values; // latency per slot
        std::vector<std::uint64_t> m_t0_ns;  // t0 per slot (for complete_slot)
        const std::size_t          m_expected;
        std::atomic<std::size_t>   m_next_slot;
        std::atomic<std::size_t>   m_completed;
    
        // 0 -> not done, 1 -> done.
        std::vector<std::atomic<std::uint8_t>> m_slot_done;
    
        mutable std::condition_variable m_wait_cv;
        mutable std::mutex m_wait_mx;
    };

} // namespace logit_bench
