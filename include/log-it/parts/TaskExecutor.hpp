#pragma once
#ifndef _LOGIT_TASK_EXECUTOR_HPP_INCLUDED
#define _LOGIT_TASK_EXECUTOR_HPP_INCLUDED
/// \file TaskExecutor.hpp
/// \brief Defines the TaskExecutor class, which manages task execution in a separate thread.

#include <thread>
#include <queue>
#include <mutex>
#include <functional>
#include <condition_variable>

namespace logit {

    /// \class TaskExecutor
    /// \brief A thread-safe task executor that processes tasks in a dedicated worker thread.
    ///
    /// This class provides a mechanism for queuing tasks (functions or lambdas) and executing them
    /// asynchronously in a background thread. It follows the singleton design pattern.
    class TaskExecutor {
    public:
        /// \brief Get the singleton instance of the TaskExecutor.
        /// \return A reference to the single instance of `TaskExecutor`.
        static TaskExecutor& get_instance() {
            static TaskExecutor instance;
            return instance;
        }

        /// \brief Adds a task to the queue in a thread-safe manner.
        /// \param task A function or lambda with no arguments to be executed asynchronously.
        void add_task(std::function<void()> task) {
            std::unique_lock<std::mutex> lock(m_queue_mutex);
            m_tasks_queue.push(std::move(task));
            lock.unlock();
            m_queue_condition.notify_one();
        }

        /// \brief Waits for all tasks in the queue to be processed.
        void wait() {
            for (;;) {
                std::unique_lock<std::mutex> lock(m_queue_mutex);
                if (m_tasks_queue.empty()) return;
                lock.unlock();
                std::this_thread::yield();
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }

    private:
        std::queue<std::function<void()>> m_tasks_queue;  ///< Queue holding tasks to be executed.
        mutable std::mutex m_queue_mutex;                 ///< Mutex to protect access to the task queue.
        std::condition_variable m_queue_condition;        ///< Condition variable to signal task availability.
        std::thread m_worker_thread;                      ///< Worker thread for executing tasks.
        bool m_stop_flag;                                 ///< Flag indicating if the worker thread should stop.

        /// \brief The worker thread function that processes tasks from the queue.
        void worker_function() {
            for (;;) {
                std::function<void()> task;
                std::unique_lock<std::mutex> lock(m_queue_mutex);
                m_queue_condition.wait(lock, [this]() {
                    return !m_tasks_queue.empty() || m_stop_flag;
                });
                if (m_stop_flag && m_tasks_queue.empty()) {
                    return;
                }
                task = std::move(m_tasks_queue.front());
                m_tasks_queue.pop();
                lock.unlock();
                task();
            }
        }

        /// \brief Private constructor to enforce the singleton pattern.
        TaskExecutor() : m_stop_flag(false) {
            m_worker_thread = std::thread(&TaskExecutor::worker_function, this);
        }

        /// \brief Destructor that stops the worker thread and cleans up resources.
        ~TaskExecutor() {
            std::unique_lock<std::mutex> lock(m_queue_mutex);
            m_stop_flag = true;
            lock.unlock();
            m_queue_condition.notify_one();
            if (m_worker_thread.joinable()) {
                m_worker_thread.join();
            }
        }

        // Delete copy constructor and assignment operators to enforce singleton usage.
        TaskExecutor(const TaskExecutor&) = delete;
        TaskExecutor& operator=(const TaskExecutor&) = delete;
        TaskExecutor(TaskExecutor&&) = delete;
        TaskExecutor& operator=(TaskExecutor&&) = delete;
    };

}; // namespace logit

#endif // _LOGIT_TASK_EXECUTOR_HPP_INCLUDED
