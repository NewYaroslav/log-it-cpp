#pragma once
#ifndef LOGIT_EVENT_LOG_LOGGER_HPP_INCLUDED
#define LOGIT_EVENT_LOG_LOGGER_HPP_INCLUDED

#include "ILogger.hpp"
#include <atomic>
#include <string>

/// \file EventLogLogger.hpp
/// \brief Logger writing to Windows Event Log.

#if defined(_WIN32) && defined(LOGIT_HAS_WIN_EVENT_LOG)
#include <windows.h>
#define LOGIT_WIN_EVENT_ENABLED 1
#else
#define LOGIT_WIN_EVENT_ENABLED 0
#endif

namespace logit {

#   if LOGIT_WIN_EVENT_ENABLED

    /// \class EventLogLogger
    /// \brief Logger forwarding messages to Windows Event Log.
    /// \thread_safety Thread-safe.
    class EventLogLogger : public ILogger {
    public:
        /// \brief Runtime configuration.
        struct Config {
            const wchar_t* source; ///< Event source name.
            bool async;           ///< Use TaskExecutor when true.
            /// \brief Initialize configuration.
            /// \param s Source name.
            /// \param a Run asynchronously.
            Config(const wchar_t* s = L"LogIt", bool a = true) : source(s), async(a) {}
        };

        /// \brief Construct with default configuration.
        EventLogLogger() : EventLogLogger(Config()) {}

        /// \brief Construct with explicit configuration.
        /// \param c Configuration options.
        explicit EventLogLogger(const Config& c) : m_cfg(c) {
            m_hsrc = RegisterEventSourceW(nullptr, m_cfg.source);
        }

        /// \brief Construct with explicit parameters.
        /// \param source Event source name.
        /// \param async Run asynchronously.
        EventLogLogger(const wchar_t* source, bool async)
            : EventLogLogger(Config(source, async)) {}

        /// \brief Deregister event source on destruction.
        ~EventLogLogger() override { if (m_hsrc) DeregisterEventSource(m_hsrc); }

        /// \brief Send message to Event Log.
        /// \param rec Log metadata.
        /// \param msg UTF-8 message text.
        void log(const LogRecord& rec, const std::string& msg) override {
            LogLevel lvl = rec.log_level;
            std::string s = msg;
            auto task = [this, lvl, s]() {
                if ((int)lvl < m_level.load()) return;
                if (!m_hsrc) return;
                WORD type = m_map(lvl);
                int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
                std::wstring wmsg; wmsg.resize(n);
                MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &wmsg[0], n);
                LPCWSTR arr[1] = { wmsg.c_str() };
                ReportEventW(m_hsrc, type, 0, 0, nullptr, 1, 0, arr, nullptr);
            };
            if (m_cfg.async) { detail::TaskExecutor::get_instance().add_task(task); }
            else { task(); }
            m_last_ts.store(rec.timestamp_ms);
        }

        /// \brief Get string parameter.
        /// \param param Parameter identifier.
        /// \return Parameter value or empty string.
        std::string get_string_param(const LoggerParam& param) const override { return {}; }

        /// \brief Get integer parameter.
        /// \param param Parameter identifier.
        /// \return Parameter value or 0.
        int64_t get_int_param(const LoggerParam& param) const override { return 0; }

        /// \brief Get floating-point parameter.
        /// \param param Parameter identifier.
        /// \return Parameter value or 0.0.
        double get_float_param(const LoggerParam& param) const override { return 0.0; }

        /// \brief Set minimal log level.
        /// \param l New level.
        void set_log_level(LogLevel l) override { m_level.store((int)l); }

        /// \brief Get current log level.
        /// \return Minimal log level.
        LogLevel get_log_level() const override { return (LogLevel)m_level.load(); }

        /// \brief Wait for asynchronous tasks to finish.
        void wait() override { if (m_cfg.async) detail::TaskExecutor::get_instance().wait(); }

    private:
        static WORD m_map(LogLevel l) {
            switch (l) {
                case LogLevel::LOG_LVL_TRACE:
                case LogLevel::LOG_LVL_DEBUG:
                case LogLevel::LOG_LVL_INFO:  return EVENTLOG_INFORMATION_TYPE;
                case LogLevel::LOG_LVL_WARN:  return EVENTLOG_WARNING_TYPE;
                case LogLevel::LOG_LVL_ERROR: return EVENTLOG_ERROR_TYPE;
                case LogLevel::LOG_LVL_FATAL: return EVENTLOG_ERROR_TYPE;
            } return EVENTLOG_INFORMATION_TYPE;
        }
        Config m_cfg{};
        HANDLE m_hsrc = nullptr;
        std::atomic<int> m_level{(int)LogLevel::LOG_LVL_TRACE};
        std::atomic<int64_t> m_last_ts{0};
    };

#   else // stub
    /// \class EventLogLogger
    /// \brief Stub logger when Windows Event Log is unavailable.
    class EventLogLogger : public ILogger {
    public:
        /// \brief Stub configuration.
        struct Config {
            const wchar_t* source; ///< Unused source name.
            bool async;           ///< Unused flag.
            Config(const wchar_t* s = L"", bool a = false) : source(s), async(a) {}
        };

        /// \brief Construct stub logger.
        EventLogLogger() {}

        /// \brief Construct with configuration.
        /// \param c Ignored configuration.
        explicit EventLogLogger(const Config& c) { (void)c; }

        /// \brief Construct with parameters.
        /// \param source Ignored source name.
        /// \param async Ignored flag.
        EventLogLogger(const wchar_t* source, bool async) { (void)source; (void)async; }

        /// \brief Ignore log request.
        /// \param rec Log metadata.
        /// \param msg Message text.
        void log(const LogRecord& rec, const std::string& msg) override { (void)rec; (void)msg; }

        /// \brief Return empty string parameter.
        /// \param param Parameter identifier.
        /// \return Empty string.
        std::string get_string_param(const LoggerParam& param) const override { (void)param; return {}; }

        /// \brief Return zero integer parameter.
        /// \param param Parameter identifier.
        /// \return Zero.
        int64_t get_int_param(const LoggerParam& param) const override { (void)param; return 0; }

        /// \brief Return zero float parameter.
        /// \param param Parameter identifier.
        /// \return Zero.
        double get_float_param(const LoggerParam& param) const override { (void)param; return 0.0; }

        /// \brief Ignore level change.
        /// \param l Desired level.
        void set_log_level(LogLevel l) override { (void)l; }

        /// \brief Return default log level.
        /// \return LogLevel::LOG_LVL_TRACE.
        LogLevel get_log_level() const override { return LogLevel::LOG_LVL_TRACE; }

        /// \brief No-op wait.
        void wait() override {}
    };
#   endif

} // namespace logit
#endif

