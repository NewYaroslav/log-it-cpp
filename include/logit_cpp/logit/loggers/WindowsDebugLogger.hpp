#pragma once
#ifndef _LOGIT_WINDOWS_DEBUG_LOGGER_HPP_INCLUDED
#define _LOGIT_WINDOWS_DEBUG_LOGGER_HPP_INCLUDED

/// \file WindowsDebugLogger.hpp
/// \brief Logger that writes to the Windows debug output (OutputDebugStringW) or stderr on other platforms.

#include "ILogger.hpp"
#include <atomic>
#include <string>
#include <iostream>
#include <memory>
#include <mutex>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace logit {

    /// \class WindowsDebugLogger
    /// \ingroup LogBackends
    /// \brief Outputs formatted log messages to the Windows debug console or stderr.
    ///
    /// On Windows this logger forwards messages to `OutputDebugStringW()`, making them
    /// visible in Visual Studio Output, DebugView (Sysinternals), WinDbg and other
    /// debuggers. On non-Windows platforms it falls back to `std::cerr`.
    ///
    /// **Key Features:**
    /// - No-op when no debugger is attached (OutputDebugStringW cost is minimal).
    /// - Synchronous or asynchronous operation.
    /// - Thread-safe.
    class WindowsDebugLogger : public ILogger {
    public:

        /// \struct Config
        /// \brief Configuration for the Windows debug logger.
        struct Config {
#ifdef __EMSCRIPTEN__
            /// \brief Initializes configuration.
            explicit Config(bool async_value = false)
#else
            /// \brief Initializes configuration.
            explicit Config(bool async_value = true)
#endif
                : async(async_value)
                , use_dedicated_executor(false)
                , queue_capacity(0)
                , queue_policy(detail::QueuePolicy::Block) {}

            bool async; ///< Flag indicating whether logging should be asynchronous.
            bool use_dedicated_executor; ///< Use a dedicated executor instead of the global TaskExecutor; native builds create one worker thread per logger.
            std::size_t queue_capacity;       ///< Maximum queue size for the dedicated executor (0 = unlimited).
            detail::QueuePolicy queue_policy; ///< Overflow policy for the dedicated executor.
        };

        /// \brief Default constructor that uses default configuration.
        WindowsDebugLogger() : WindowsDebugLogger(Config()) {}

        /// \brief Constructor with custom configuration.
        /// \param config The configuration for the logger.
        explicit WindowsDebugLogger(const Config& config) : m_config(config) {
            if (m_config.async && m_config.use_dedicated_executor) {
                m_executor.reset(new detail::SingleThreadExecutor());
                m_executor->set_max_queue_size(m_config.queue_capacity);
                m_executor->set_queue_policy(m_config.queue_policy);
            }
        }

        /// \brief Constructor with asynchronous and dedicated executor options.
        WindowsDebugLogger(
                bool async,
                bool use_dedicated_executor,
                std::size_t queue_capacity,
                detail::QueuePolicy queue_policy)
            : WindowsDebugLogger(make_config(
                    async,
                    use_dedicated_executor,
                    queue_capacity,
                    queue_policy)) {}

        /// \brief Update executor-related configuration at runtime.
        ///
        /// Only these Config fields are applied:
        ///   async, use_dedicated_executor, queue_capacity, queue_policy.
        void set_config(const Config& config) {
            std::unique_ptr<detail::SingleThreadExecutor> old_executor;
            {
                std::unique_lock<std::mutex> lock(m_lifecycle_mutex);
                if (m_shutdown.load(std::memory_order_acquire)) return;
                if (m_config.async) {
                    if (m_executor) {
                        m_executor->wait();
                    } else {
                        detail::TaskExecutor::get_instance().wait();
                    }
                }
                m_config.async = config.async;
                m_config.use_dedicated_executor = config.use_dedicated_executor;
                m_config.queue_capacity = config.queue_capacity;
                m_config.queue_policy = config.queue_policy;
                if (m_config.async && m_config.use_dedicated_executor) {
                    if (!m_executor) {
                        m_executor.reset(new detail::SingleThreadExecutor());
                    }
                    m_executor->set_max_queue_size(m_config.queue_capacity);
                    m_executor->set_queue_policy(m_config.queue_policy);
                } else if (m_executor) {
                    old_executor = std::move(m_executor);
                    m_executor.reset();
                }
            }
            if (old_executor) {
                old_executor->shutdown();
            }
        }

        /// \brief Logs a message to the Windows debug output or stderr.
        ///
        /// If asynchronous logging is enabled, the message is added to the task queue;
        /// otherwise, it is logged directly.
        ///
        /// \param record The log record containing log information.
        /// \param message The formatted log message.
        void log(const LogRecord& record, const std::string& message) override {
            std::lock_guard<std::mutex> lifecycle_lock(m_lifecycle_mutex);
            if (m_shutdown.load(std::memory_order_acquire)) return;
            if (m_config.async) {
                if (m_executor) {
                    m_executor->add_task([message]() {
                        write_impl(message);
                    });
                } else {
                    detail::TaskExecutor::get_instance().add_task([message]() {
                        write_impl(message);
                    });
                }
            } else {
                write_impl(message);
            }
            m_last_log_ts.store(record.timestamp_ms);
        }

        /// \brief Retrieves a string parameter from the logger.
        /// \param param The parameter type to retrieve.
        /// \return A string representing the requested parameter.
        std::string get_string_param(const LoggerParam& param) const override {
            switch (param) {
            case LoggerParam::LastLogTimestamp:
                return std::to_string(m_last_log_ts.load());
            default:
                break;
            }
            return {};
        }

        /// \brief Retrieves an integer parameter from the logger.
        /// \param param The parameter type to retrieve.
        /// \return An integer representing the requested parameter, or 0 if unsupported.
        int64_t get_int_param(const LoggerParam& param) const override {
            switch (param) {
            case LoggerParam::LastLogTimestamp:
                return m_last_log_ts.load();
            default:
                break;
            }
            return 0;
        }

        /// \brief Retrieves a floating-point parameter from the logger.
        /// \param param The parameter type to retrieve.
        /// \return A double representing the requested parameter, or 0.0 if unsupported.
        double get_float_param(const LoggerParam& param) const override {
            switch (param) {
            case LoggerParam::LastLogTimestamp:
                return static_cast<double>(m_last_log_ts.load()) / 1000.0;
            default:
                break;
            }
            return 0.0;
        }

        /// \brief Sets the minimal log level for this logger.
        void set_log_level(LogLevel level) override {
            m_log_level = static_cast<int>(level);
        }

        /// \brief Gets the minimal log level for this logger.
        LogLevel get_log_level() const override {
            return static_cast<LogLevel>(m_log_level.load());
        }

        /// \brief Waits for all asynchronous tasks to complete.
        void wait() override {
            if (!m_config.async) return;
            if (m_executor) {
                m_executor->wait();
            } else {
                detail::TaskExecutor::get_instance().wait();
            }
        }

        /// \brief Stops logger-owned asynchronous resources after draining pending messages.
        void shutdown() override {
            {
                std::lock_guard<std::mutex> lifecycle_lock(m_lifecycle_mutex);
                if (m_shutdown.exchange(true, std::memory_order_acq_rel)) return;
            }
            if (m_executor) {
                m_executor->shutdown();
            } else if (m_config.async) {
                detail::TaskExecutor::get_instance().wait();
            }
        }

    private:
        Config m_config;
        std::mutex m_lifecycle_mutex;
        std::atomic<int> m_log_level{static_cast<int>(LogLevel::LOG_LVL_TRACE)};
        std::atomic<int64_t> m_last_log_ts{0};
        std::atomic<bool> m_shutdown{false};
        std::unique_ptr<detail::SingleThreadExecutor> m_executor;

        static Config make_config(
                bool async,
                bool use_dedicated_executor,
                std::size_t queue_capacity,
                detail::QueuePolicy queue_policy) {
            Config config(async);
            config.use_dedicated_executor = use_dedicated_executor;
            config.queue_capacity = queue_capacity;
            config.queue_policy = queue_policy;
            return config;
        }

        static void write_impl(const std::string& message) {
#if defined(_WIN32)
            // Convert UTF-8 message to wide string for OutputDebugStringW.
            if (message.empty()) {
                OutputDebugStringW(L"\n");
                return;
            }
            const int len = static_cast<int>(message.size());
            const int wlen = MultiByteToWideChar(CP_UTF8, 0, message.c_str(), len, nullptr, 0);
            if (wlen > 0) {
                std::wstring wmsg;
                wmsg.resize(static_cast<size_t>(wlen));
                MultiByteToWideChar(CP_UTF8, 0, message.c_str(), len, &wmsg[0], wlen);
                wmsg += L'\n';
                OutputDebugStringW(wmsg.c_str());
            } else {
                OutputDebugStringA((message + "\n").c_str());
            }
#else
            std::cerr << message << '\n';
#endif
        }
    };

} // namespace logit

#endif // _LOGIT_WINDOWS_DEBUG_LOGGER_HPP_INCLUDED
