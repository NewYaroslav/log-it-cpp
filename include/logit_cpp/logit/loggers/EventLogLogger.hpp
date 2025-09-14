#pragma once
#ifndef LOGIT_EVENT_LOG_LOGGER_HPP_INCLUDED
#define LOGIT_EVENT_LOG_LOGGER_HPP_INCLUDED

#include "ILogger.hpp"
#include <atomic>
#include <string>

#if defined(_WIN32) && defined(LOGIT_HAS_WIN_EVENT_LOG)
#include <windows.h>
#define LOGIT_WIN_EVENT_ENABLED 1
#else
#define LOGIT_WIN_EVENT_ENABLED 0
#endif

namespace logit {

#if LOGIT_WIN_EVENT_ENABLED

class EventLogLogger : public ILogger {
public:
    struct Config {
        const wchar_t* source;
        bool async;
        Config(const wchar_t* s = L"LogIt", bool a = true) : source(s), async(a) {}
    };
    EventLogLogger() : EventLogLogger(Config()) {}
    explicit EventLogLogger(const Config& c) : cfg(c) {
        hsrc = RegisterEventSourceW(nullptr, cfg.source);
    }
    EventLogLogger(const wchar_t* source, bool async) : EventLogLogger(Config(source, async)) {}
    ~EventLogLogger() override { if (hsrc) DeregisterEventSource(hsrc); }

    void log(const LogRecord& rec, const std::string& msg) override {
        LogLevel lvl = rec.log_level;
        std::string s = msg;
        auto task = [this, lvl, s]() {
            if ((int)lvl < level.load()) return;
            if (!hsrc) return;
            WORD type = map(lvl);
            int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
            std::wstring wmsg; wmsg.resize(n);
            MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &wmsg[0], n);
            LPCWSTR arr[1] = { wmsg.c_str() };
            ReportEventW(hsrc, type, 0, 0, nullptr, 1, 0, arr, nullptr);
        };
        if (cfg.async) { detail::TaskExecutor::get_instance().add_task(task); }
        else { task(); }
        last_ts.store(rec.timestamp_ms);
    }

    std::string get_string_param(const LoggerParam&) const override { return {}; }
    int64_t get_int_param(const LoggerParam&) const override { return 0; }
    double get_float_param(const LoggerParam&) const override { return 0.0; }
    void set_log_level(LogLevel l) override { level.store((int)l); }
    LogLevel get_log_level() const override { return (LogLevel)level.load(); }
    void wait() override { if (cfg.async) detail::TaskExecutor::get_instance().wait(); }

private:
    static WORD map(LogLevel l) {
        switch (l) {
            case LogLevel::LOG_LVL_TRACE:
            case LogLevel::LOG_LVL_DEBUG:
            case LogLevel::LOG_LVL_INFO:  return EVENTLOG_INFORMATION_TYPE;
            case LogLevel::LOG_LVL_WARN:  return EVENTLOG_WARNING_TYPE;
            case LogLevel::LOG_LVL_ERROR: return EVENTLOG_ERROR_TYPE;
            case LogLevel::LOG_LVL_FATAL: return EVENTLOG_ERROR_TYPE;
        } return EVENTLOG_INFORMATION_TYPE;
    }
    Config cfg{};
    HANDLE hsrc = nullptr;
    std::atomic<int> level{(int)LogLevel::LOG_LVL_TRACE};
    std::atomic<int64_t> last_ts{0};
};

#else // stub
class EventLogLogger : public ILogger {
public:
    struct Config {
        const wchar_t* source;
        bool async;
        Config(const wchar_t* s = L"", bool a = false) : source(s), async(a) {}
    };
    EventLogLogger() {}
    explicit EventLogLogger(const Config&) {}
    EventLogLogger(const wchar_t*, bool) {}
    void log(const LogRecord&, const std::string&) override {}
    std::string get_string_param(const LoggerParam&) const override { return {}; }
    int64_t get_int_param(const LoggerParam&) const override { return 0; }
    double get_float_param(const LoggerParam&) const override { return 0.0; }
    void set_log_level(LogLevel) override {}
    LogLevel get_log_level() const override { return LogLevel::LOG_LVL_TRACE; }
    void wait() override {}
};
#endif

} // namespace logit
#endif

