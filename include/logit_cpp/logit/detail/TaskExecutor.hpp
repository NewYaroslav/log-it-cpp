#pragma once
#ifndef _LOGIT_DETAIL_TASK_EXECUTOR_HPP_INCLUDED
#define _LOGIT_DETAIL_TASK_EXECUTOR_HPP_INCLUDED

/// \file TaskExecutor.hpp
/// \brief Task executor used by asynchronous loggers.
/// \details Detailed design notes are available in docs/TaskExecutor.md.

#include <functional>
#include <atomic>
#if defined(__EMSCRIPTEN__) && !defined(__EMSCRIPTEN_PTHREADS__)
  #include <deque>
  #include <mutex>
  #include <emscripten/emscripten.h>
    #else
        #include <thread>
        #include <deque>
        #include <mutex>
        #include <condition_variable>
        #include <chrono>
#endif

// Enable lock-free MPSC ring integration (non-Emscripten) by defining:
//   #define LOGIT_USE_MPSC_RING

#if !defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__)
  #ifdef LOGIT_USE_MPSC_RING
    #include "MpscRingAny.hpp"
  #endif
#endif

namespace logit { namespace detail {

    /// \brief Queue overflow handling policy used by TaskExecutor.
    enum class QueuePolicy {
        DropNewest, ///< Reject the incoming task when the queue is full.
        DropOldest, ///< Drop the oldest queued task (or the incoming one in MPSC builds).
        Block       ///< Producers wait until capacity is available.
    };
    
#   if defined(__EMSCRIPTEN__) && !defined(__EMSCRIPTEN_PTHREADS__)
    
    /// \class TaskExecutor
    /// \brief Simplified task executor for single-threaded Emscripten builds.
    /// \details The Emscripten variant keeps behaviour compatible with the
    /// browser event loop. See docs/TaskExecutor.md for the high level design.
    /// \thread_safety Not thread-safe.
    class TaskExecutor {
    public:
        /// \brief Returns the singleton executor instance.
        static TaskExecutor& get_instance() {
            static TaskExecutor instance;
            return instance;
        }

        /// \brief Enqueue a task to be executed on the async drain.
        /// \note Backpressure policies mirror the deque implementation described
        /// in docs/TaskExecutor.md.
        void add_task(std::function<void()> task) {
            if (!task) return;
            bool schedule = false;
            for (;;) {
                schedule = false;
                {
                    std::lock_guard<std::mutex> lk(m_mutex);
                    if (m_max_queue_size > 0 && m_tasks.size() >= m_max_queue_size) {
                        switch (m_overflow_policy) {
                            case QueuePolicy::DropNewest:
                                ++m_dropped_tasks;
                                return;
                            case QueuePolicy::DropOldest:
                                if (!m_tasks.empty()) {
                                    m_tasks.pop_front();
                                    ++m_dropped_tasks;
                                }
                                break;
                            case QueuePolicy::Block:
                                break; // handled after unlocking
                        }
                        if (m_overflow_policy == QueuePolicy::Block && m_tasks.size() >= m_max_queue_size) {
                            // fall through to drain outside lock
                        } else {
                            m_tasks.emplace_back(std::move(task));
                            schedule = !m_scheduled;
                            m_scheduled = m_scheduled || schedule;
                            break;
                        }
                    } else {
                        m_tasks.emplace_back(std::move(task));
                        schedule = !m_scheduled;
                        m_scheduled = m_scheduled || schedule;
                        break;
                    }
                }
                drain();
            }
            if (schedule) {
                emscripten_async_call(&TaskExecutor::drain_thunk, this, 0);
            }
        }

        /// \brief Drain the queue synchronously.
        void wait() { drain(); }
        /// \brief Shut down the executor by draining all queued work.
        void shutdown() { drain(); }

        /// \brief Change the maximum queue size (`0` disables the limit).
        void set_max_queue_size(std::size_t size) {
            std::lock_guard<std::mutex> lk(m_mutex);
            m_max_queue_size = size;
        }

        /// \brief Update the queue overflow policy.
        void set_queue_policy(QueuePolicy policy) {
            std::lock_guard<std::mutex> lk(m_mutex);
            m_overflow_policy = policy;
        }

        /// \brief Return the number of tasks dropped by the overflow policy.
        std::size_t dropped_tasks() const noexcept {
            return m_dropped_tasks.load(std::memory_order_relaxed);
        }
        /// \brief Reset the drop counter to zero.
        void reset_dropped_tasks() noexcept {
            m_dropped_tasks.store(0, std::memory_order_relaxed);
        }
    
    private:
        TaskExecutor()
            : m_max_queue_size(0),
              m_overflow_policy(QueuePolicy::Block),
              m_dropped_tasks(0),
              m_scheduled(false) {}
        ~TaskExecutor() = default;
        TaskExecutor(const TaskExecutor&) = delete;
        TaskExecutor& operator=(const TaskExecutor&) = delete;
        TaskExecutor(TaskExecutor&&) = delete;
        TaskExecutor& operator=(TaskExecutor&&) = delete;
    
        std::deque<std::function<void()>> m_tasks;
        std::mutex m_mutex;
        std::size_t m_max_queue_size;
        QueuePolicy m_overflow_policy;
        std::atomic<std::size_t> m_dropped_tasks;
        bool m_scheduled;
    
        static void drain_thunk(void* arg) {
            static_cast<TaskExecutor*>(arg)->drain();
        }
    
        void drain() {
            for (;;) {
                std::function<void()> task;
                {
                    std::lock_guard<std::mutex> lk(m_mutex);
                    if (m_tasks.empty()) {
                        m_scheduled = false;
                        break;
                    }
                    task = std::move(m_tasks.front());
                    m_tasks.pop_front();
                }
                task();
            }
        }
    };
    
#   else // !Emscripten or pthreads
    
    /// \class TaskExecutor
    /// \brief Thread-safe task executor backed by a dedicated worker thread.
    /// \details The full design, including backpressure semantics and hot
    /// resizing, is described in docs/TaskExecutor.md.
    /// \thread_safety Thread-safe.
    class TaskExecutor {
    public:
        /// \brief Returns the global executor instance.
        /// \note The pointer-based singleton intentionally outlives static
        /// logger destructors so logging remains available during process
        /// shutdown.
        static TaskExecutor& get_instance() {
            static TaskExecutor* instance = new TaskExecutor();
            return *instance;
        }

        /// \brief Enqueue a task for asynchronous execution.
        /// \note `QueuePolicy::DropOldest` drops the incoming task when
        /// `LOGIT_USE_MPSC_RING` is defined. See docs/TaskExecutor.md for the
        /// rationale.
        void add_task(std::function<void()> task) {
            if (!task) return;
#        ifndef LOGIT_USE_MPSC_RING
            std::unique_lock<std::mutex> lock(m_queue_mutex);
            if (m_stop_flag.load(std::memory_order_acquire)) return;
            if (m_max_queue_size > 0 && m_tasks_queue.size() >= m_max_queue_size) {
                switch (m_overflow_policy.load(std::memory_order_relaxed)) {
                    case QueuePolicy::DropNewest:
                        ++m_dropped_tasks;
                        return;
                    case QueuePolicy::DropOldest:
                        if (!m_tasks_queue.empty()) {
                            m_tasks_queue.pop_front();
                            ++m_dropped_tasks;
                        }
                        break;
                    case QueuePolicy::Block:
                        m_queue_condition.wait(lock, [this]() {
                            return m_tasks_queue.size() < m_max_queue_size ||
                                   m_stop_flag.load(std::memory_order_acquire);
                        });
                        if (m_stop_flag.load(std::memory_order_acquire)) return;
                        break;
                }
            }
            m_tasks_queue.push_back(std::move(task));
            lock.unlock();
            m_queue_condition.notify_one();
#        else
            // Hot-resize barrier: wait until the ring rebuild is finished.
            if (m_resizing.load(std::memory_order_acquire)) {
                std::unique_lock<std::mutex> lk(m_cv_mutex);
                m_resize_cv.wait(lk, [this]{ return !m_resizing.load(std::memory_order_acquire); });
            }

            if (m_stop_flag.load(std::memory_order_acquire)) {
                return;
            }
    
            std::function<void()> local_task = std::move(task);
    
            for (;;) {
                if (m_stop_flag.load(std::memory_order_acquire)) {
                    return;
                }
    
                const auto policy = m_overflow_policy.load(std::memory_order_relaxed);
    
                // Backpressure relies on the count of in-flight tasks.
                if (policy == QueuePolicy::Block &&
                    m_max_queue_size > 0 &&
                    m_active_tasks.load(std::memory_order_relaxed) >= m_max_queue_size)
                {
                    std::unique_lock<std::mutex> lk(m_cv_mutex);
                    m_cv.wait_for(lk, std::chrono::microseconds(LOGIT_TASK_EXECUTOR_BLOCK_WAIT_USEC));
                    continue;
                }
    
                // Try to push into the ring buffer.
                if (m_mpsc_queue.try_push(local_task)) {
                    m_cv.notify_one(); // wake the worker
                    return;
                }

                // Apply the configured overflow policy when the ring is full.
                switch (policy) {
                    case QueuePolicy::DropNewest:
                        m_dropped_tasks.fetch_add(1, std::memory_order_relaxed);
                        return;

                    case QueuePolicy::DropOldest:
                        // Safe MPSC behaviour: drop the incoming task.
                        // Preserves ordering and avoids producer/consumer deadlocks.
                        m_dropped_tasks.fetch_add(1, std::memory_order_relaxed);
                        return;
    
                    case QueuePolicy::Block: {
                        std::unique_lock<std::mutex> lk(m_cv_mutex);
                        m_cv.wait_for(lk, std::chrono::microseconds(LOGIT_TASK_EXECUTOR_BLOCK_WAIT_USEC));
                        break;
                    }
                }
            }
#        endif
        }
    
        /// \brief Block until the queue is empty or a shutdown is requested.
        void wait() {
#        ifndef LOGIT_USE_MPSC_RING
            std::unique_lock<std::mutex> lock(m_queue_mutex);
            m_queue_condition.wait(lock, [this]() {
                return ((m_tasks_queue.empty() &&
                        m_active_tasks.load(std::memory_order_relaxed) == 0) ||
                        m_stop_flag.load(std::memory_order_acquire));
            });
#        else
            std::unique_lock<std::mutex> lock(m_queue_mutex);
            m_queue_condition.wait(lock, [this]() {
                return ((queue_empty_() &&
                        m_active_tasks.load(std::memory_order_relaxed) == 0) ||
                        m_stop_flag.load(std::memory_order_acquire));
            });
#        endif
        }
    
        /// \brief Stop the worker thread and drain outstanding tasks.
        void shutdown() {
#        ifndef LOGIT_USE_MPSC_RING
            std::unique_lock<std::mutex> lock(m_queue_mutex);
            m_stop_flag.store(true, std::memory_order_release);
            lock.unlock();
            m_queue_condition.notify_all();
            if (m_worker_thread.joinable()) {
                m_worker_thread.join();
            }
#        else
            {
                std::lock_guard<std::mutex> lock(m_queue_mutex);
                m_stop_flag.store(true, std::memory_order_release);
            }
            m_cv.notify_all();
            m_queue_condition.notify_all();
            if (m_worker_thread.joinable()) {
                m_worker_thread.join();
            }
#        endif
        }
    
        /// \brief Update the maximum queue size (`0` disables the limit).
        /// \details On MPSC builds this performs the "hot" resize described in
        /// docs/TaskExecutor.md.
        void set_max_queue_size(std::size_t size) {
#       ifdef LOGIT_USE_MPSC_RING
            // Tell producers to pause before any wait()/stop conditions run.
            m_resizing.store(true, std::memory_order_release);

            // Drain the queue completely, but do not wait forever if the worker
            // is stalled (e.g., blocked sink or backpressure keeping
            // m_active_tasks > 0). If we fail to drain before the deadline,
            // abort the resize and re-open the barrier so producers can
            // continue.
            const auto deadline = std::chrono::steady_clock::now() +
                                  std::chrono::seconds(1);
            if (!wait_until_idle_(deadline)) {
                m_resizing.store(false, std::memory_order_release);
                m_resize_cv.notify_all();
                return;
            }

            // Stop the worker so it cannot touch m_mpsc_queue during the resize.
            std::unique_lock<std::mutex> lk(m_queue_mutex);
            m_stop_flag.store(true, std::memory_order_relaxed);
                lk.unlock();

            m_cv.notify_all();
            m_queue_condition.notify_all();
            if (m_worker_thread.joinable()) {
                m_worker_thread.join();
            }

            // Reinitialise the parameters and the ring on a single thread.
                lk.lock();
                m_max_queue_size = size;
                const std::size_t cap =
                        (m_max_queue_size == 0 ? m_default_ring_cap : m_max_queue_size);
                m_mpsc_queue = MpscRingAny<std::function<void()>>(cap);
                // Reset counters (except drops) because the queue is empty.
                m_active_tasks.store(0, std::memory_order_relaxed);
                // Keep m_dropped_tasks untouched — tests manage it via macros.
                lk.unlock();

            // Clear the stop flag and restart the worker thread.
            m_stop_flag.store(false, std::memory_order_relaxed);
            m_worker_thread = std::thread(&TaskExecutor::worker_function, this);

            // Re-open the barrier so producers can continue.
            m_resizing.store(false, std::memory_order_release);
            m_resize_cv.notify_all();
#       else
            std::lock_guard<std::mutex> lock(m_queue_mutex);
            m_max_queue_size = size;
#       endif
        }
    
        /// \brief Change the overflow policy for newly submitted tasks.
        void set_queue_policy(QueuePolicy policy) {
            std::lock_guard<std::mutex> lock(m_queue_mutex);
            m_overflow_policy.store(policy, std::memory_order_relaxed);
        }

        /// \brief Return the number of tasks dropped by the overflow policy.
        std::size_t dropped_tasks() const noexcept {
            return m_dropped_tasks.load(std::memory_order_relaxed);
        }
        /// \brief Reset the overflow counter to zero.
        void reset_dropped_tasks() noexcept {
            m_dropped_tasks.store(0, std::memory_order_relaxed);
        }
    
    private:
    #ifndef LOGIT_USE_MPSC_RING
        std::deque<std::function<void()>> m_tasks_queue;
        mutable std::mutex m_queue_mutex;
        std::condition_variable m_queue_condition;
        std::thread m_worker_thread;
        std::atomic<bool> m_stop_flag;
        std::size_t m_max_queue_size;
        std::atomic<QueuePolicy> m_overflow_policy;
        std::atomic<std::size_t> m_dropped_tasks;
        std::atomic<std::size_t> m_active_tasks;
    #else
        mutable std::mutex m_queue_mutex;          ///< Guards wait() and policy changes.
        std::condition_variable m_queue_condition; ///< Notifies wait() once the queue drains.

        std::condition_variable m_cv;              ///< Wakes the worker or producers.
        std::mutex m_cv_mutex;                     ///< Protects producer/worker sleeps.

        std::atomic<bool> m_resizing;              ///< true while a hot resize is in flight.
        std::condition_variable m_resize_cv;       ///< Producers wait here during a resize.
    
        std::thread m_worker_thread;
        std::atomic<bool> m_stop_flag;
        std::size_t m_max_queue_size;
        std::atomic<QueuePolicy> m_overflow_policy;
        std::atomic<std::size_t> m_dropped_tasks;
        std::atomic<std::size_t> m_active_tasks;
    
        const std::size_t m_default_ring_cap = LOGIT_TASK_EXECUTOR_DEFAULT_RING_CAPACITY;
        MpscRingAny<std::function<void()>> m_mpsc_queue;
    #endif
    
        void worker_function() {
    #ifndef LOGIT_USE_MPSC_RING
            for (;;) {
                std::function<void()> task;
                std::unique_lock<std::mutex> lock(m_queue_mutex);
                m_queue_condition.wait(lock, [this]() {
                    return !m_tasks_queue.empty() || m_stop_flag.load(std::memory_order_acquire);
                });
                if (m_stop_flag.load(std::memory_order_acquire) && m_tasks_queue.empty()) {
                    break;
                }
                task = std::move(m_tasks_queue.front());
                m_tasks_queue.pop_front();
                m_active_tasks.fetch_add(1, std::memory_order_relaxed);
                lock.unlock();
                m_queue_condition.notify_one();
    
                task();
    
                lock.lock();
                m_active_tasks.fetch_sub(1, std::memory_order_relaxed);
                if (m_tasks_queue.empty() && m_active_tasks.load(std::memory_order_relaxed) == 0) {
                    m_queue_condition.notify_all();
                }
                lock.unlock();
            }
    #else
            for (;;) {
                bool drained_any = false;
                std::function<void()> task;
    
                int budget = LOGIT_TASK_EXECUTOR_DRAIN_BUDGET;
                while (budget-- && m_mpsc_queue.try_pop(task)) {
                    drained_any = true;
                    m_active_tasks.fetch_add(1, std::memory_order_relaxed);
    
                    task();
    
                    m_active_tasks.fetch_sub(1, std::memory_order_relaxed);
                    m_cv.notify_one(); // freed an in-flight slot
                }
    
                if (queue_empty_() && m_active_tasks.load(std::memory_order_relaxed) == 0) {
                    std::unique_lock<std::mutex> lock(m_queue_mutex);
                    m_queue_condition.notify_all(); // notify wait()
                    m_cv.notify_all();              // wake producers blocked on Block
                    if (m_stop_flag.load(std::memory_order_acquire)) {
                        break;
                    }
                }
    
                if (!drained_any) {
                    std::unique_lock<std::mutex> lk(m_cv_mutex);
                    if (m_stop_flag.load(std::memory_order_acquire) && queue_empty_()) {
                        break;
                    }
                    m_cv.wait_for(lk, std::chrono::milliseconds(1));
                }
            }
    #endif
        }
    
    #ifdef LOGIT_USE_MPSC_RING
        bool queue_empty_() const noexcept {
            return m_mpsc_queue.empty();
        }

        bool wait_until_idle_(std::chrono::steady_clock::time_point deadline) {
            std::unique_lock<std::mutex> lock(m_queue_mutex);
            return m_queue_condition.wait_until(lock, deadline, [this]() {
                return ((queue_empty_() &&
                         m_active_tasks.load(std::memory_order_relaxed) == 0) ||
                        m_stop_flag.load(std::memory_order_acquire));
            });
        }
    #endif
    
        TaskExecutor()
    #ifndef LOGIT_USE_MPSC_RING
            : m_stop_flag(false),
              m_max_queue_size(0),
              m_overflow_policy(QueuePolicy::Block),
              m_dropped_tasks(0),
              m_active_tasks(0)
    #else
            : m_resizing(false),
              m_worker_thread(),
              m_stop_flag(false),
              m_max_queue_size(0),
              m_overflow_policy(QueuePolicy::Block),
              m_dropped_tasks(0),
              m_active_tasks(0),
              m_mpsc_queue(m_default_ring_cap)
    #endif
        {
            m_worker_thread = std::thread(&TaskExecutor::worker_function, this);
        }
    
        ~TaskExecutor() {
            shutdown();
        }
    
        TaskExecutor(const TaskExecutor&) = delete;
        TaskExecutor& operator=(const TaskExecutor&) = delete;
        TaskExecutor(TaskExecutor&&) = delete;
        TaskExecutor& operator=(TaskExecutor&&) = delete;
    };

#endif // Emscripten split

}} // namespace logit::detail

#endif // _LOGIT_DETAIL_TASK_EXECUTOR_HPP_INCLUDED

