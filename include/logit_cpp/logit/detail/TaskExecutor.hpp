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

#if !defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__)
  #ifdef LOGIT_USE_MPSC_RING
    #include "MpscRingAny.hpp"
  #endif
#endif

namespace logit { namespace detail {

    /// \brief Queue overflow handling policy.
    enum class QueuePolicy { DropNewest, DropOldest, Block };
    
#   if defined(__EMSCRIPTEN__) && !defined(__EMSCRIPTEN_PTHREADS__)
    
    /// \class TaskExecutor
    /// \brief Simplified task executor for single-threaded Emscripten builds.
    /// \thread_safety Not thread-safe.
    class TaskExecutor {
    public:
        static TaskExecutor& get_instance() {
            static TaskExecutor instance;
            return instance;
        }
    
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
    
        void wait() { drain(); }
        void shutdown() { drain(); }
    
        void set_max_queue_size(std::size_t size) {
            std::lock_guard<std::mutex> lk(m_mutex);
            m_max_queue_size = size;
        }
    
        void set_queue_policy(QueuePolicy policy) {
            std::lock_guard<std::mutex> lk(m_mutex);
            m_overflow_policy = policy;
        }
    
        std::size_t dropped_tasks() const noexcept {
            return m_dropped_tasks.load(std::memory_order_relaxed);
        }
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
    /// \brief A thread-safe task executor that processes tasks in a dedicated worker thread.
    /// \thread_safety Thread-safe.
    class TaskExecutor {
    public:
        /// Singleton (сохраняем вашу реализацию с new).
        static TaskExecutor& get_instance() {
            static TaskExecutor* instance = new TaskExecutor();
            return *instance;
        }
    
        /// Добавить задачу.
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
            // Барьер ресайза: пережидаем «горячее» изменение кольца.
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
    
                // Реальное backpressure: учитываем "висящие" задачи.
                if (policy == QueuePolicy::Block &&
                    m_max_queue_size > 0 &&
                    m_active_tasks.load(std::memory_order_relaxed) >= m_max_queue_size)
                {
                    std::unique_lock<std::mutex> lk(m_cv_mutex);
                    m_cv.wait_for(lk, std::chrono::microseconds(200));
                    continue;
                }
    
                // Пытаемся положить в кольцо.
                if (m_mpsc_queue.try_push(local_task)) {
                    m_cv.notify_one(); // разбудить воркера
                    return;
                }
    
                // Переполнение — применяем политику.
                switch (policy) {
                    case QueuePolicy::DropNewest:
                        m_dropped_tasks.fetch_add(1, std::memory_order_relaxed);
                        return;
    
                    case QueuePolicy::DropOldest:
                        // Безопасная реализация под MPSC: дропаем входящий.
                        // Это сохраняет порядок и исключает дедлоки при gate.
                        m_dropped_tasks.fetch_add(1, std::memory_order_relaxed);
                        return;
    
                    case QueuePolicy::Block: {
                        std::unique_lock<std::mutex> lk(m_cv_mutex);
                        m_cv.wait_for(lk, std::chrono::microseconds(200));
                        break;
                    }
                }
            }
#        endif
        }
    
        /// Дождаться опустошения.
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
    
        /// Остановить воркер.
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
    
        /// Изменить ёмкость очереди.
        void set_max_queue_size(std::size_t size) {
#       ifdef LOGIT_USE_MPSC_RING
            // Сигналим продюсерам, чтобы переждали ресайз (до любых ожиданий/стопов).
            m_resizing.store(true, std::memory_order_release);

            // Дождаться опустошения очереди
            wait();
        
            // Акуратно остановить воркер и дождаться его завершения, чтобы он не трогал m_mpsc_queue, пока мы его меняем.
            std::unique_lock<std::mutex> lk(m_queue_mutex);
            m_stop_flag.store(true, std::memory_order_relaxed);
        	lk.unlock();
        
            m_cv.notify_all();
            m_queue_condition.notify_all();
            if (m_worker_thread.joinable()) {
                m_worker_thread.join();
            }
        
            // Переинициализировать параметры и само кольцо в единственном потоке.
        	lk.lock();
        	m_max_queue_size = size;
        	const std::size_t cap =
        		(m_max_queue_size == 0 ? m_default_ring_cap : m_max_queue_size);
        	m_mpsc_queue = MpscRingAny<std::function<void()>>(cap);
        	// обнулить счётчики (не обязательно, но логично при "чистой" очереди).
        	m_active_tasks.store(0, std::memory_order_relaxed);
        	// m_dropped_tasks оставляем как есть — тесты его сами сбрасывают макросом.
        	lk.unlock();
        
            // Снимаем стоп-флаг, перезапускаем воркер…
            m_stop_flag.store(false, std::memory_order_relaxed);
            m_worker_thread = std::thread(&TaskExecutor::worker_function, this);

            // Открываем барьер для продюсеров.
            m_resizing.store(false, std::memory_order_release);
            m_resize_cv.notify_all();
#       else
            std::lock_guard<std::mutex> lock(m_queue_mutex);
            m_max_queue_size = size;
#       endif
        }
    
        /// Политика переполнения.
        void set_queue_policy(QueuePolicy policy) {
            std::lock_guard<std::mutex> lock(m_queue_mutex);
            m_overflow_policy.store(policy, std::memory_order_relaxed);
        }
    
        std::size_t dropped_tasks() const noexcept {
            return m_dropped_tasks.load(std::memory_order_relaxed);
        }
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
        mutable std::mutex m_queue_mutex;          ///< Для wait()/смены политики.
        std::condition_variable m_queue_condition; ///< Будим wait() на полном drain.
    
        std::condition_variable m_cv;              ///< Будим воркер / продюсеров.
        std::mutex m_cv_mutex;                     ///< Сон продюсеров/воркера.

        std::atomic<bool> m_resizing;              ///< true — идёт ресайз кольца.
        std::condition_variable m_resize_cv;       ///< Продюсеры ждут окончания ресайза.
    
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
    
                int budget = 2048;
                while (budget-- && m_mpsc_queue.try_pop(task)) {
                    drained_any = true;
                    m_active_tasks.fetch_add(1, std::memory_order_relaxed);
    
                    task();
    
                    m_active_tasks.fetch_sub(1, std::memory_order_relaxed);
                    m_cv.notify_one(); // освободили in-flight слот
                }
    
                if (queue_empty_() && m_active_tasks.load(std::memory_order_relaxed) == 0) {
                    std::unique_lock<std::mutex> lock(m_queue_mutex);
                    m_queue_condition.notify_all(); // для wait()
                    m_cv.notify_all();              // разбудить продюсеров Block
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

