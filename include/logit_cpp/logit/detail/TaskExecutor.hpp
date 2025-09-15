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
                                m_tasks.pop_front();
                                ++m_dropped_tasks;
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
    ///
    /// This class provides a mechanism for queuing tasks (functions or lambdas) and executing them
    /// asynchronously in a background thread. It follows the singleton design pattern.
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
            std::unique_lock<std::mutex> lock(m_queue_mutex);
            if (m_stop_flag) return;
            if (m_max_queue_size > 0 && m_tasks_queue.size() >= m_max_queue_size) {
                switch (m_overflow_policy) {
                    case QueuePolicy::DropNewest:
                        ++m_dropped_tasks;
                        return;
                    case QueuePolicy::DropOldest:
                        m_tasks_queue.pop_front();
                        ++m_dropped_tasks;
                        break;
                    case QueuePolicy::Block:
                        m_queue_condition.wait(lock, [this]() {
                            return m_tasks_queue.size() < m_max_queue_size || m_stop_flag;
                        });
                        if (m_stop_flag) return;
                        break;
                }
            }
            m_tasks_queue.push_back(std::move(task));
            lock.unlock();
            m_queue_condition.notify_one();
        }

        /// \brief Waits for all tasks in the queue to be processed.
        void wait() {
            std::unique_lock<std::mutex> lock(m_queue_mutex);
            m_queue_condition.wait(lock, [this]() {
                return (m_tasks_queue.empty() && m_active_tasks.load() == 0) || m_stop_flag;
            });
        }

        /// \brief Shuts down the TaskExecutor by stopping the worker thread.
        /// \details This method signals the worker thread to stop and then joins it.
        void shutdown() {
            std::unique_lock<std::mutex> lock(m_queue_mutex);
            m_stop_flag = true;
            lock.unlock();
            m_queue_condition.notify_all();
            if (m_worker_thread.joinable()) {
                m_worker_thread.join();
            }
        }

        /// \brief Sets the maximum size of the task queue.
        /// \param size Maximum number of tasks in the queue (0 for unlimited).
        void set_max_queue_size(std::size_t size) {
            std::lock_guard<std::mutex> lock(m_queue_mutex);
            m_max_queue_size = size;
        }

        /// \brief Sets the behavior when the queue is full.
        /// \param policy QueuePolicy::DropNewest to discard the incoming task,
        /// QueuePolicy::DropOldest to discard the oldest task,
        /// or QueuePolicy::Block to wait.
        void set_queue_policy(QueuePolicy policy) {
            std::lock_guard<std::mutex> lock(m_queue_mutex);
            m_overflow_policy = policy;
        }

    private:
        std::deque<std::function<void()>> m_tasks_queue;  ///< Queue holding tasks to be executed.
        mutable std::mutex m_queue_mutex;                 ///< Mutex to protect access to the task queue.
        std::condition_variable m_queue_condition;        ///< Condition variable to signal task availability.
        std::thread m_worker_thread;                      ///< Worker thread for executing tasks.
        bool m_stop_flag;                                 ///< Flag indicating if the worker thread should stop.
        std::size_t m_max_queue_size;                     ///< Maximum number of tasks in the queue (0 for unlimited).
        QueuePolicy m_overflow_policy;                    ///< Policy for handling queue overflow.
        std::atomic<std::size_t> m_dropped_tasks;         ///< Number of discarded tasks due to overflow.
        std::atomic<std::size_t> m_active_tasks;          ///< Number of tasks currently running.

        /// \brief The worker thread function that processes tasks from the queue.
        void worker_function() {
            for (;;) {
                std::function<void()> task;
                std::unique_lock<std::mutex> lock(m_queue_mutex);
                m_queue_condition.wait(lock, [this]() {
                    return !m_tasks_queue.empty() || m_stop_flag;
                });
                if (m_stop_flag && m_tasks_queue.empty()) {
                    break;
                }
                task = std::move(m_tasks_queue.front());
                m_tasks_queue.pop_front();
                ++m_active_tasks;
                lock.unlock();
                m_queue_condition.notify_one();
                task();
                lock.lock();
                --m_active_tasks;
                if (m_tasks_queue.empty() && m_active_tasks == 0) {
                    m_queue_condition.notify_all();
                }
                lock.unlock();
            }
        }

        /// \brief Private constructor to enforce the singleton pattern.
        TaskExecutor() : m_stop_flag(false), m_max_queue_size(0), m_overflow_policy(QueuePolicy::Block), m_dropped_tasks(0), m_active_tasks(0) {
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
