#pragma once
#ifndef _LOGIT_DETAIL_TASK_EXECUTOR_HPP_INCLUDED
#define _LOGIT_DETAIL_TASK_EXECUTOR_HPP_INCLUDED

/// \file TaskExecutor.hpp
/// \brief Defines the TaskExecutor class, which manages task execution in a separate thread.

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
// Optional: enable rare DropOldest slow-path coordination:
//   #define LOGIT_ENABLE_DROP_OLDEST_SLOWPATH

#if !defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__)
#   ifdef LOGIT_USE_MPSC_RING
#       include "MpscRingAny.hpp"
#   endif
#endif

namespace logit { namespace detail {

    /// \brief Queue overflow handling policy.
    enum class QueuePolicy { DropNewest, DropOldest, Block };

#if defined(__EMSCRIPTEN__) && !defined(__EMSCRIPTEN_PTHREADS__)

    /// \class TaskExecutor
    /// \brief Simplified task executor for single-threaded Emscripten builds.
    /// \thread_safety Not thread-safe.
    class TaskExecutor {
    public:
        /// \brief Obtain singleton instance.
        /// \return Global executor.
        static TaskExecutor& get_instance() {
            static TaskExecutor instance;
            return instance;
        }

        /// \brief Enqueue task for later execution.
        /// \param task Callable to execute.
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

        /// \brief Run all queued tasks.
        void wait() { drain(); }

        /// \brief Drain queue without scheduling new tasks.
        void shutdown() { drain(); }

        /// \brief Set maximum queue size.
        /// \param size Number of tasks allowed (0 for unlimited).
        void set_max_queue_size(std::size_t size) {
            std::lock_guard<std::mutex> lk(m_mutex);
            m_max_queue_size = size;
        }

        /// \brief Set overflow handling policy.
        /// \param policy Policy to apply.
        void set_queue_policy(QueuePolicy policy) {
            std::lock_guard<std::mutex> lk(m_mutex);
            m_overflow_policy = policy;
        }

        /// \brief Retrieve the number of tasks dropped due to overflow.
        /// \return Total count of dropped tasks.
        std::size_t dropped_tasks() const noexcept {
            return m_dropped_tasks.load(std::memory_order_relaxed);
        }

        /// \brief Reset the dropped tasks counter back to zero.
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

#else

    /// \class TaskExecutor
    /// \brief A thread-safe task executor that processes tasks in a dedicated worker thread.
    /// \thread_safety Thread-safe.
    class TaskExecutor {
    public:
        /// \brief Get the singleton instance of the TaskExecutor.
        /// \return A reference to the single instance of `TaskExecutor`.
        static TaskExecutor& get_instance() {
            static TaskExecutor* instance = new TaskExecutor();
            return *instance;
        }

        /// \brief Adds a task to the queue in a thread-safe manner.
        /// \param task A function or lambda with no arguments to be executed asynchronously.
        void add_task(std::function<void()> task) {
            if (!task) return;
#ifdef LOGIT_USE_MPSC_RING
            if (m_stop_flag.load(std::memory_order_acquire)) {
                return;
            }

            std::function<void()> local_task = std::move(task);

            if (m_mpsc_queue.try_push(local_task)) {
                m_cv.notify_one();
                return;
            }

            switch (m_overflow_policy.load(std::memory_order_relaxed)) {
                case QueuePolicy::DropNewest:
                    ++m_dropped_tasks;
                    return;
                case QueuePolicy::Block: {
                    for (;;) {
                        if (m_mpsc_queue.try_push(local_task)) {
                            m_cv.notify_one();
                            return;
                        }

                        if (m_stop_flag.load(std::memory_order_acquire)) {
                            return;
                        }

                        std::unique_lock<std::mutex> lk(m_cv_mutex);
                        m_cv.wait_for(lk, std::chrono::microseconds(50));
                    }
                }
                case QueuePolicy::DropOldest:
#ifdef LOGIT_ENABLE_DROP_OLDEST_SLOWPATH
                {
                    std::size_t target = 0;
                    bool request_completed = false;
                    {
                        std::unique_lock<std::mutex> lk(m_drop_mutex);
                        target = ++m_drop_requested;
                        m_cv.notify_one();
                        request_completed = m_drop_cv.wait_for(
                            lk, std::chrono::milliseconds(2),
                            [this, target] { return m_drop_done >= target; });
                    }

                    auto finalize_request = [this, target]() {
                        std::unique_lock<std::mutex> lk(m_drop_mutex);
                        if (m_drop_done < target) {
                            m_drop_done = target;
                            lk.unlock();
                            m_drop_cv.notify_all();
                        }
                    };

                    if (m_mpsc_queue.try_push(local_task)) {
                        if (!request_completed) {
                            finalize_request();
                        }
                        m_cv.notify_one();
                        return;
                    }

                    if (!request_completed) {
                        finalize_request();
                    }

                    ++m_dropped_tasks;
                    return;
                }
#else
                    ++m_dropped_tasks;
                    return;
#endif
            }
#else
            std::unique_lock<std::mutex> lock(m_queue_mutex);
            if (m_stop_flag.load(std::memory_order_relaxed)) return;
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
                                m_stop_flag.load(std::memory_order_relaxed);
                        });
                        if (m_stop_flag.load(std::memory_order_relaxed)) return;
                        break;
                }
            }
            m_tasks_queue.push_back(std::move(task));
            lock.unlock();
            m_queue_condition.notify_one();
#endif
        }

        /// \brief Waits for all tasks in the queue to be processed.
        void wait() {
#ifdef LOGIT_USE_MPSC_RING
            std::unique_lock<std::mutex> lock(m_queue_mutex);
            m_queue_condition.wait(lock, [this]() {
                return ((queue_empty_() &&
                        m_active_tasks.load(std::memory_order_relaxed) == 0) ||
                    m_stop_flag.load(std::memory_order_relaxed));
            });
#else
            std::unique_lock<std::mutex> lock(m_queue_mutex);
            m_queue_condition.wait(lock, [this]() {
                return ((m_tasks_queue.empty() &&
                        m_active_tasks.load(std::memory_order_relaxed) == 0) ||
                    m_stop_flag.load(std::memory_order_relaxed));
            });
#endif
        }

        /// \brief Shuts down the TaskExecutor by stopping the worker thread.
        /// \details This method signals the worker thread to stop and then joins it.
        void shutdown() {
#ifdef LOGIT_USE_MPSC_RING
            {
                std::lock_guard<std::mutex> lock(m_queue_mutex);
                m_stop_flag.store(true, std::memory_order_relaxed);
            }
            m_cv.notify_all();
            m_queue_condition.notify_all();
            if (m_worker_thread.joinable()) {
                m_worker_thread.join();
            }
#else
            std::unique_lock<std::mutex> lock(m_queue_mutex);
            m_stop_flag.store(true, std::memory_order_relaxed);
            lock.unlock();
            m_queue_condition.notify_all();
            if (m_worker_thread.joinable()) {
                m_worker_thread.join();
            }
#endif
        }

        /// \brief Sets the maximum size of the task queue.
        /// \param size Maximum number of tasks in the queue (0 for unlimited).
        void set_max_queue_size(std::size_t size) {
#ifdef LOGIT_USE_MPSC_RING
            std::lock_guard<std::mutex> lk(m_queue_mutex);
            m_max_queue_size = size;
            if (queue_empty_() && m_active_tasks.load(std::memory_order_relaxed) == 0) {
                std::size_t cap = (m_max_queue_size == 0 ? m_default_ring_cap : m_max_queue_size);
                m_mpsc_queue = MpscRingAny<std::function<void()>>(cap);
            }
#else
            std::lock_guard<std::mutex> lock(m_queue_mutex);
            m_max_queue_size = size;
#endif
        }

        /// \brief Sets the behavior when the queue is full.
        /// \param policy QueuePolicy::DropNewest to discard the incoming task,
        /// QueuePolicy::DropOldest to discard the oldest task,
        /// or QueuePolicy::Block to wait.
        void set_queue_policy(QueuePolicy policy) {
            std::lock_guard<std::mutex> lock(m_queue_mutex);
            m_overflow_policy.store(policy, std::memory_order_relaxed);
        }

        /// \brief Returns the number of tasks dropped because of overflow.
        /// \return Total dropped tasks observed so far.
        std::size_t dropped_tasks() const noexcept {
            return m_dropped_tasks.load(std::memory_order_relaxed);
        }

        /// \brief Resets the dropped tasks counter back to zero.
        void reset_dropped_tasks() noexcept {
            m_dropped_tasks.store(0, std::memory_order_relaxed);
        }

    private:
#ifndef LOGIT_USE_MPSC_RING
        std::deque<std::function<void()>> m_tasks_queue;  ///< Queue holding tasks to be executed.
        mutable std::mutex m_queue_mutex;                 ///< Mutex to protect access to the task queue.
        std::condition_variable m_queue_condition;        ///< Condition variable to signal task availability.
        std::thread m_worker_thread;                      ///< Worker thread for executing tasks.
        std::atomic<bool> m_stop_flag;                    ///< Flag indicating if the worker thread should stop.
        std::size_t m_max_queue_size;                     ///< Maximum number of tasks in the queue (0 for unlimited).
        std::atomic<QueuePolicy> m_overflow_policy;       ///< Policy for handling queue overflow.
        std::atomic<std::size_t> m_dropped_tasks;         ///< Number of discarded tasks due to overflow.
        std::atomic<std::size_t> m_active_tasks;          ///< Number of tasks currently running.
#else
        mutable std::mutex m_queue_mutex;                 ///< Used only for wait()/policy changes.
        std::condition_variable m_queue_condition;        ///< Notifies waiters on full drain.

        std::condition_variable m_cv;                     ///< Wake-up for worker on push.
        std::mutex m_cv_mutex;                            ///< Sleep mutex for worker waits.

        std::thread m_worker_thread;                      ///< Worker thread for executing tasks.
        std::atomic<bool> m_stop_flag;                    ///< Flag indicating if the worker thread should stop.
        std::size_t m_max_queue_size;                     ///< Maximum number of tasks requested by user.
        std::atomic<QueuePolicy> m_overflow_policy;       ///< Policy for handling queue overflow.
        std::atomic<std::size_t> m_dropped_tasks;         ///< Number of discarded tasks due to overflow.
        std::atomic<std::size_t> m_active_tasks;          ///< Number of tasks currently running.

        const std::size_t m_default_ring_cap = LOGIT_TASK_EXECUTOR_DEFAULT_RING_CAPACITY; ///< Default capacity when unlimited requested.
        MpscRingAny<std::function<void()>> m_mpsc_queue;  ///< Lock-free bounded MPSC ring.

#ifdef LOGIT_ENABLE_DROP_OLDEST_SLOWPATH
        std::mutex m_drop_mutex;                          ///< Coordinates DropOldest slow-path.
        std::condition_variable m_drop_cv;                ///< Producer waits for confirmation.
        std::size_t m_drop_requested;                     ///< Number of requested drops.
        std::size_t m_drop_done;                          ///< Number of drops completed.
#endif
#endif

        /// \brief The worker thread function that processes tasks from the queue.
        void worker_function() {
#ifndef LOGIT_USE_MPSC_RING
            for (;;) {
                std::function<void()> task;
                std::unique_lock<std::mutex> lock(m_queue_mutex);
                m_queue_condition.wait(lock, [this]() {
                    return !m_tasks_queue.empty() || m_stop_flag.load(std::memory_order_relaxed);
                });
                if (m_stop_flag.load(std::memory_order_relaxed) && m_tasks_queue.empty()) {
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

                int budget = 2048;
                while (budget-- && m_mpsc_queue.try_pop(task)) {
                    drained_any = true;
                    m_active_tasks.fetch_add(1, std::memory_order_relaxed);
                    task();
                    m_active_tasks.fetch_sub(1, std::memory_order_relaxed);
                }

#ifdef LOGIT_ENABLE_DROP_OLDEST_SLOWPATH
                handle_drop_requests_();
#endif

                if (queue_empty_() && m_active_tasks.load(std::memory_order_relaxed) == 0) {
                    std::unique_lock<std::mutex> lock(m_queue_mutex);
                    m_queue_condition.notify_all();
                    if (m_stop_flag.load(std::memory_order_relaxed)) {
                        break;
                    }
                }

                if (!drained_any) {
                    std::unique_lock<std::mutex> lk(m_cv_mutex);
                    if (m_stop_flag.load(std::memory_order_relaxed) && queue_empty_()) {
                        break;
                    }
                    m_cv.wait_for(lk, std::chrono::milliseconds(1));
                }
            }
#endif
        }

#ifdef LOGIT_USE_MPSC_RING
        /// \brief Return true if ring appears empty for current consumer position.
        bool queue_empty_() const noexcept {
            return m_mpsc_queue.empty();
        }

#ifdef LOGIT_ENABLE_DROP_OLDEST_SLOWPATH
        /// \brief Perform requested drops of oldest items (rare path).
        void handle_drop_requests_() {
            {
                std::lock_guard<std::mutex> g(m_drop_mutex);
                if (m_drop_done >= m_drop_requested) {
                    return;
                }
            }
            std::unique_lock<std::mutex> lk(m_drop_mutex);
            while (m_drop_done < m_drop_requested) {
                std::function<void()> dummy;
                if (m_mpsc_queue.try_pop(dummy)) {
                    ++m_drop_done;
                    m_dropped_tasks.fetch_add(1, std::memory_order_relaxed);
                } else {
                    break;
                }
            }
            m_drop_cv.notify_all();
        }
#endif
#endif

        /// \brief Private constructor to enforce the singleton pattern.
        TaskExecutor()
#ifndef LOGIT_USE_MPSC_RING
            : m_stop_flag(false),
              m_max_queue_size(0),
              m_overflow_policy(QueuePolicy::Block),
              m_dropped_tasks(0),
              m_active_tasks(0)
#else
            : m_stop_flag(false),
              m_max_queue_size(0),
              m_overflow_policy(QueuePolicy::Block),
              m_dropped_tasks(0),
              m_active_tasks(0),
              m_mpsc_queue(m_default_ring_cap)
#ifdef LOGIT_ENABLE_DROP_OLDEST_SLOWPATH
            , m_drop_requested(0),
              m_drop_done(0)
#endif
#endif
        {
            m_worker_thread = std::thread(&TaskExecutor::worker_function, this);
        }

        /// \brief Destructor that stops the worker thread and cleans up resources.
        ~TaskExecutor() {
            shutdown();
        }

        // Delete copy constructor and assignment operators to enforce singleton usage.
        TaskExecutor(const TaskExecutor&) = delete;
        TaskExecutor& operator=(const TaskExecutor&) = delete;
        TaskExecutor(TaskExecutor&&) = delete;
        TaskExecutor& operator=(TaskExecutor&&) = delete;
    };

#endif // defined(__EMSCRIPTEN__) && !defined(__EMSCRIPTEN_PTHREADS__)

}} // namespace logit::detail

#endif // _LOGIT_DETAIL_TASK_EXECUTOR_HPP_INCLUDED
