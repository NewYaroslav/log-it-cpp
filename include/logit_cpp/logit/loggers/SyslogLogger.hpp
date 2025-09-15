#pragma once
#ifndef LOGIT_SYSLOG_LOGGER_HPP_INCLUDED
#define LOGIT_SYSLOG_LOGGER_HPP_INCLUDED

#include "ILogger.hpp"
#include <atomic>
#include <string>

#if (defined(__unix__) || defined(__APPLE__)) && defined(LOGIT_HAS_SYSLOG)
#include <syslog.h>
#define LOGIT_SYSLOG_ENABLED 1
#else
#define LOGIT_SYSLOG_ENABLED 0
#endif

namespace logit {

#   if LOGIT_SYSLOG_ENABLED

    class SyslogLogger : public ILogger {
    public:
        struct Config {
            const char* ident;
            int facility;
            bool async;
            Config(const char* i = "log-it", int f = LOG_USER, bool a = true)
                : ident(i), facility(f), async(a) {}
        };
        SyslogLogger() : SyslogLogger(Config()) {}

        explicit SyslogLogger(const Config& c) : m_cfg(c) { 
            openlog(m_cfg.ident, LOG_PID | LOG_NDELAY, m_cfg.facility); 
        }
        
        SyslogLogger(const char* ident, int facility, bool async) : SyslogLogger(Config(ident, facility, async)) {}
        ~SyslogLogger() override { closelog(); }

        void log(const LogRecord& rec, const std::string& msg) override {
            LogLevel lvl = rec.log_level;
            std::string s = msg;
            auto task = [this, lvl, s]() {
                if ((int)lvl < m_level.load()) return;
                syslog(m_map(lvl), "%s", s.c_str());
            };
            if (m_cfg.async) { detail::TaskExecutor::get_instance().add_task(task); }
            else { task(); }
            m_last_ts.store(rec.timestamp_ms);
        }
        std::string get_string_param(const LoggerParam&) const override { return {}; }
        int64_t get_int_param(const LoggerParam&) const override { return 0; }
        double  get_float_param(const LoggerParam&) const override { return 0.0; }
        void set_log_level(LogLevel l) override { m_level.store((int)l); }
        LogLevel get_log_level() const override { return (LogLevel)m_level.load(); }
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
        std::atomic<int> m_level{(int)LogLevel::LOG_LVL_TRACE};
        std::atomic<int64_t> m_last_ts{0};
    };

#   else // stub on unsupported
    class SyslogLogger : public ILogger {
    public:
        struct Config {
            const char* ident;
            int facility;
            bool async;
            Config(const char* i="", int f=0, bool a=false) : ident(i), facility(f), async(a) {}
        };
        SyslogLogger() {}
        explicit SyslogLogger(const Config&) {}
        SyslogLogger(const char*,int,bool) {}
        void log(const LogRecord&, const std::string&) override {}
        std::string get_string_param(const LoggerParam&) const override { return {}; }
        int64_t get_int_param(const LoggerParam&) const override { return 0; }
        double get_float_param(const LoggerParam&) const override { return 0.0; }
        void set_log_level(LogLevel) override {}
        LogLevel get_log_level() const override { return LogLevel::LOG_LVL_TRACE; }
        void wait() override {}
    };
#   endif

} // namespace logit
#endif

