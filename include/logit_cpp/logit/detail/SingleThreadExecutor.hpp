#pragma once
#ifndef _LOGIT_DETAIL_SINGLE_THREAD_EXECUTOR_HPP_INCLUDED
#define _LOGIT_DETAIL_SINGLE_THREAD_EXECUTOR_HPP_INCLUDED

/// \file SingleThreadExecutor.hpp
/// \brief Per-instance single-thread executor for isolated async logging.

#include "QueuePolicy.hpp"
#include <functional>
#include <deque>
#include <mutex>
#include <atomic>
#include <memory>
#if !defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__)
#include <condition_variable>
#include <thread>
#else
#include <emscripten/emscripten.h>
#endif

namespace logit {
namespace detail {

/// \class SingleThreadExecutor
/// \brief Simplified per-logger task executor.
/// \details Provides the same public API as TaskExecutor (add_task, wait, shutdown,
/// set_max_queue_size, set_queue_policy, dropped_tasks, reset_dropped_tasks) so
/// loggers can use either executor interchangeably. Unlike the global TaskExecutor
/// singleton, each native instance owns its own worker thread, providing isolation
/// between loggers. Single-threaded Emscripten builds use a per-instance
/// cooperative queue drained through the browser event loop.
#if !defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__)
class SingleThreadExecutor {
public:
    /// \brief Construct and immediately start the worker thread.
    SingleThreadExecutor()
        : m_stop(false)
        , m_shutdown_done(false)
        , m_max_queue_size(0)
        , m_overflow_policy(QueuePolicy::Block)
        , m_dropped_tasks(0)
        , m_active_tasks(0)
    {
        m_worker = std::thread(&SingleThreadExecutor::worker_loop, this);
    }

    /// \brief Destructor drains and joins the worker thread.
    ~SingleThreadExecutor() {
        shutdown();
    }

    SingleThreadExecutor(const SingleThreadExecutor&) = delete;
    SingleThreadExecutor& operator=(const SingleThreadExecutor&) = delete;
    SingleThreadExecutor(SingleThreadExecutor&&) = delete;
    SingleThreadExecutor& operator=(SingleThreadExecutor&&) = delete;

    /// \brief Enqueue a task for asynchronous execution.
    void add_task(std::function<void()> task) {
        if (!task) return;
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_stop.load(std::memory_order_acquire)) return;
        if (m_max_queue_size > 0 && m_queue.size() >= m_max_queue_size) {
            switch (m_overflow_policy) {
                case QueuePolicy::DropNewest:
                    ++m_dropped_tasks;
                    return;
                case QueuePolicy::DropOldest:
                    if (!m_queue.empty()) {
                        m_queue.pop_front();
                        ++m_dropped_tasks;
                    }
                    break;
                case QueuePolicy::Block:
                    m_cv.wait(lock, [this]() {
                        return m_max_queue_size == 0 ||
                               m_queue.size() < m_max_queue_size ||
                               m_stop.load(std::memory_order_acquire);
                    });
                    if (m_stop.load(std::memory_order_acquire)) return;
                    break;
            }
        }
        m_queue.push_back(std::move(task));
        lock.unlock();
        m_cv.notify_one();
    }

    /// \brief Block until the queue is empty and no active tasks remain.
    void wait() {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [this]() {
            return m_queue.empty() &&
                   m_active_tasks.load(std::memory_order_relaxed) == 0;
        });
    }

    /// \brief Stop accepting new tasks, drain remaining, and join the worker thread.
    void shutdown() {
        bool notify_worker = false;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (!m_shutdown_done.load(std::memory_order_acquire)) {
                m_shutdown_done.store(true, std::memory_order_release);
                m_stop.store(true, std::memory_order_release);
                notify_worker = true;
            }
        }
        if (notify_worker) {
            m_cv.notify_all();
        }
        if (m_worker.joinable() && m_worker.get_id() != std::this_thread::get_id()) {
            m_worker.join();
        }
    }

    /// \brief Change the maximum queue size (0 disables the limit).
    void set_max_queue_size(std::size_t size) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_stop.load(std::memory_order_acquire)) return;
            m_max_queue_size = size;
        }
        m_cv.notify_all();
    }

    /// \brief Change the queue overflow policy.
    void set_queue_policy(QueuePolicy policy) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_stop.load(std::memory_order_acquire)) return;
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
    std::deque<std::function<void()>> m_queue;
    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    std::thread m_worker;
    std::atomic<bool> m_stop;
    std::atomic<bool> m_shutdown_done;
    std::size_t m_max_queue_size;
    QueuePolicy m_overflow_policy;
    std::atomic<std::size_t> m_dropped_tasks;
    std::atomic<std::size_t> m_active_tasks;

    void worker_loop() {
        for (;;) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_cv.wait(lock, [this]() {
                    return !m_queue.empty() || m_stop.load(std::memory_order_acquire);
                });
                if (m_stop.load(std::memory_order_acquire) && m_queue.empty()) {
                    break;
                }
                if (m_queue.empty()) continue;
                task = std::move(m_queue.front());
                m_queue.pop_front();
                m_active_tasks.fetch_add(1, std::memory_order_relaxed);
            }
            m_cv.notify_all();

            try {
                task();
            } catch (...) {
                // Suppress exceptions from user tasks.
            }

            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_active_tasks.fetch_sub(1, std::memory_order_relaxed);
                if (m_queue.empty() && m_active_tasks.load(std::memory_order_relaxed) == 0) {
                    m_cv.notify_all();
                }
            }
        }
    }
};

#else // single-threaded Emscripten: cooperative per-instance queue
class SingleThreadExecutor {
public:
    SingleThreadExecutor()
        : m_state(new State()) {}

    ~SingleThreadExecutor() {
        shutdown();
    }

    SingleThreadExecutor(const SingleThreadExecutor&) = delete;
    SingleThreadExecutor& operator=(const SingleThreadExecutor&) = delete;
    SingleThreadExecutor(SingleThreadExecutor&&) = delete;
    SingleThreadExecutor& operator=(SingleThreadExecutor&&) = delete;

    void add_task(std::function<void()> task) {
        if (!task) return;
        const std::shared_ptr<State> state = m_state;
        bool schedule = false;

        for (;;) {
            bool drain_for_capacity = false;
            {
                std::lock_guard<std::mutex> lock(state->mutex);
                if (state->shutdown_requested) return;

                if (state->max_queue_size > 0 &&
                    state->queue.size() >= state->max_queue_size) {
                    switch (state->overflow_policy) {
                        case QueuePolicy::DropNewest:
                            ++state->dropped_tasks;
                            return;
                        case QueuePolicy::DropOldest:
                            if (!state->queue.empty()) {
                                state->queue.pop_front();
                                ++state->dropped_tasks;
                            }
                            break;
                        case QueuePolicy::Block:
                            drain_for_capacity = true;
                            break;
                    }
                }

                if (drain_for_capacity &&
                    state->max_queue_size > 0 &&
                    state->queue.size() >= state->max_queue_size) {
                    // Drain outside the lock to emulate producer backpressure
                    // without dropping the incoming task.
                } else {
                    state->queue.push_back(std::move(task));
                    schedule = !state->scheduled;
                    state->scheduled = state->scheduled || schedule;
                    break;
                }
            }

            drain_state(state);
        }

        if (schedule) {
            schedule_drain(state);
        }
    }

    void wait() {
        drain_state(m_state);
    }

    void shutdown() {
        const std::shared_ptr<State> state = m_state;
        {
            std::lock_guard<std::mutex> lock(state->mutex);
            if (state->shutdown_requested) return;
            state->shutdown_requested = true;
        }
        drain_state(state);
    }

    void set_max_queue_size(std::size_t size) {
        std::lock_guard<std::mutex> lock(m_state->mutex);
        if (m_state->shutdown_requested) return;
        m_state->max_queue_size = size;
    }

    void set_queue_policy(QueuePolicy policy) {
        std::lock_guard<std::mutex> lock(m_state->mutex);
        if (m_state->shutdown_requested) return;
        m_state->overflow_policy = policy;
    }

    std::size_t dropped_tasks() const noexcept {
        return m_state->dropped_tasks.load(std::memory_order_relaxed);
    }

    void reset_dropped_tasks() noexcept {
        m_state->dropped_tasks.store(0, std::memory_order_relaxed);
    }

private:
    struct State {
        State()
            : max_queue_size(0)
            , overflow_policy(QueuePolicy::Block)
            , dropped_tasks(0)
            , scheduled(false)
            , shutdown_requested(false) {}

        std::deque<std::function<void()>> queue;
        std::mutex mutex;
        std::size_t max_queue_size;
        QueuePolicy overflow_policy;
        std::atomic<std::size_t> dropped_tasks;
        bool scheduled;
        bool shutdown_requested;
    };

    std::shared_ptr<State> m_state;

    static void schedule_drain(const std::shared_ptr<State>& state) {
        std::shared_ptr<State>* token = new std::shared_ptr<State>(state);
        emscripten_async_call(&SingleThreadExecutor::drain_thunk, token, 0);
    }

    static void drain_thunk(void* arg) {
        std::unique_ptr<std::shared_ptr<State>> token(
                static_cast<std::shared_ptr<State>*>(arg));
        drain_state(*token);
    }

    static void drain_state(const std::shared_ptr<State>& state) {
        for (;;) {
            std::function<void()> task;
            {
                std::lock_guard<std::mutex> lock(state->mutex);
                if (state->queue.empty()) {
                    state->scheduled = false;
                    break;
                }
                task = std::move(state->queue.front());
                state->queue.pop_front();
            }

            try {
                task();
            } catch (...) {
                // Suppress exceptions from user tasks.
            }
        }
    }
};
#endif // !defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__)

} // namespace detail
} // namespace logit

#endif // _LOGIT_DETAIL_SINGLE_THREAD_EXECUTOR_HPP_INCLUDED
