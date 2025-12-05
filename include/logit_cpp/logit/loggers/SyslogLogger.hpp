#pragma once
#ifndef LOGIT_SYSLOG_LOGGER_HPP_INCLUDED
#define LOGIT_SYSLOG_LOGGER_HPP_INCLUDED

#include "ILogger.hpp"
#include <atomic>
#include <string>

/// \file SyslogLogger.hpp
/// \brief Logger writing to system syslog.

#if (defined(__unix__) || defined(__APPLE__)) && defined(LOGIT_HAS_SYSLOG)
#include <syslog.h>
#define LOGIT_SYSLOG_ENABLED 1
#else
#define LOGIT_SYSLOG_ENABLED 0
#endif

namespace logit {

#   if LOGIT_SYSLOG_ENABLED

    /// \class SyslogLogger
    /// \brief Logger forwarding messages to syslog.
    /// \thread_safety Thread-safe.
    class SyslogLogger : public ILogger {
    public:
        /// \brief Runtime configuration.
        struct Config {
            const char* ident;  ///< Identifier passed to openlog.
            int facility;       ///< Syslog facility.
            bool async;         ///< Use TaskExecutor when true.
            /// \brief Initialize configuration.
            /// \param i Identifier string.
            /// \param f Facility code.
            /// \param a Run asynchronously.
            Config(const char* i = "log-it", int f = LOG_USER, bool a = true)
                : ident(i), facility(f), async(a) {}
        };

        /// \brief Construct with default configuration.
        SyslogLogger() : SyslogLogger(Config()) {}

        /// \brief Construct with explicit configuration.
        /// \param c Configuration options.
        explicit SyslogLogger(const Config& c) : m_cfg(c) {
            openlog(m_cfg.ident, LOG_PID | LOG_NDELAY, m_cfg.facility);
        }

        /// \brief Construct with explicit parameters.
        /// \param ident Identifier string.
        /// \param facility Syslog facility.
        /// \param async Run asynchronously.
        SyslogLogger(const char* ident, int facility, bool async)
            : SyslogLogger(Config(ident, facility, async)) {}

        /// \brief Close syslog on destruction.
        ~SyslogLogger() override { closelog(); }

        /// \brief Send message to syslog.
        /// \param rec Log metadata.
        /// \param msg Formatted message.
        void log(const LogRecord& rec, const std::string& msg) override {
            LogLevel lvl = rec.log_level;
            std::string s = msg;
            auto task = [this, lvl, s]() {
                if (static_cast<int>(lvl) < m_level.load()) return;
                syslog(m_map(lvl), "%s", s.c_str());
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
        double  get_float_param(const LoggerParam& param) const override { return 0.0; }

        /// \brief Set minimal log level.
        /// \param l New level.
        void set_log_level(LogLevel l) override { m_level.store(static_cast<int>(l)); }

        /// \brief Get current log level.
        /// \return Minimal log level.
        LogLevel get_log_level() const override { return static_cast<LogLevel>(m_level.load()); }

        /// \brief Wait for asynchronous tasks to finish.
        void wait() override { if (m_cfg.async) detail::TaskExecutor::get_instance().wait(); }

    private:
        static int m_map(LogLevel l) {
            switch (l) {
                case LogLevel::LOG_LVL_TRACE:
                case LogLevel::LOG_LVL_DEBUG: return LOG_DEBUG;
                case LogLevel::LOG_LVL_INFO:  return LOG_INFO;
                case LogLevel::LOG_LVL_WARN:  return LOG_WARNING;
                case LogLevel::LOG_LVL_ERROR: return LOG_ERR;
                case LogLevel::LOG_LVL_FATAL: return LOG_CRIT;
            } return LOG_INFO;
        }
        Config m_cfg{};
        std::atomic<int> m_level{static_cast<int>(LogLevel::LOG_LVL_TRACE)};
        std::atomic<int64_t> m_last_ts{0};
    };

#   else // stub on unsupported
    /// \class SyslogLogger
    /// \brief Stub logger when syslog is unavailable.
    class SyslogLogger : public ILogger {
    public:
        /// \brief Stub configuration.
        struct Config {
            const char* ident;  ///< Unused identifier.
            int facility;       ///< Unused facility.
            bool async;         ///< Unused flag.
            Config(const char* i="", int f=0, bool a=false) : ident(i), facility(f), async(a) {}
        };

        /// \brief Construct stub logger.
        SyslogLogger() {}

        /// \brief Construct with configuration.
        /// \param c Ignored configuration.
        explicit SyslogLogger(const Config& c) { (void)c; }

        /// \brief Construct with parameters.
        /// \param ident Ignored identifier.
        /// \param facility Ignored facility.
        /// \param async Ignored flag.
        SyslogLogger(const char* ident,int facility,bool async) { (void)ident; (void)facility; (void)async; }

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

