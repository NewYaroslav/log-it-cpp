#include <logit.hpp>

#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#ifndef __EMSCRIPTEN__

using logit::LogLevel;
using logit::LogRecord;

/// \brief Probe logger that records timing and can inject a delay.
class ProbeLogger : public logit::ILogger {
public:
    struct Config {
        bool async = true;
        bool use_dedicated_executor = false;
        std::chrono::milliseconds task_delay{0};
    };

    explicit ProbeLogger(const Config& cfg) : m_cfg(cfg) {
        if (m_cfg.async && m_cfg.use_dedicated_executor) {
            m_executor.reset(new logit::detail::SingleThreadExecutor());
        }
    }

    ~ProbeLogger() override {
        if (m_executor) m_executor->shutdown();
    }

    void log(const LogRecord& record, const std::string& message) override {
        m_last_ts.store(record.timestamp_ms);
        if (!m_cfg.async) {
            run_task(message);
            return;
        }
        if (m_executor) {
            m_executor->add_task([this, message]() { run_task(message); });
        } else {
            logit::detail::TaskExecutor::get_instance().add_task([this, message]() { run_task(message); });
        }
    }

    std::string get_string_param(const logit::LoggerParam&) const override { return {}; }
    int64_t get_int_param(const logit::LoggerParam&) const override { return 0; }
    double get_float_param(const logit::LoggerParam&) const override { return 0.0; }
    void set_log_level(LogLevel l) override { m_level.store(static_cast<int>(l)); }
    LogLevel get_log_level() const override { return static_cast<LogLevel>(m_level.load()); }

    void wait() override {
        if (!m_cfg.async) return;
        if (m_executor) m_executor->wait();
        else logit::detail::TaskExecutor::get_instance().wait();
    }

    std::size_t count() const { return m_count.load(std::memory_order_relaxed); }
    void reset_count() { m_count.store(0, std::memory_order_relaxed); }

private:
    void run_task(const std::string&) {
        if (m_cfg.task_delay.count() > 0) {
            std::this_thread::sleep_for(m_cfg.task_delay);
        }
        m_count.fetch_add(1, std::memory_order_relaxed);
    }

    Config m_cfg;
    std::atomic<int> m_level{static_cast<int>(LogLevel::LOG_LVL_TRACE)};
    std::atomic<int64_t> m_last_ts{0};
    std::atomic<std::size_t> m_count{0};
    std::unique_ptr<logit::detail::SingleThreadExecutor> m_executor;
};

static bool test_isolation() {
    // Slow logger with dedicated executor
    ProbeLogger::Config slow_cfg;
    slow_cfg.async = true;
    slow_cfg.use_dedicated_executor = true;
    slow_cfg.task_delay = std::chrono::milliseconds(50);
    ProbeLogger slow_logger(slow_cfg);

    // Fast logger with dedicated executor
    ProbeLogger::Config fast_cfg;
    fast_cfg.async = true;
    fast_cfg.use_dedicated_executor = true;
    fast_cfg.task_delay = std::chrono::milliseconds(0);
    ProbeLogger fast_logger(fast_cfg);

    // Enqueue a slow task
    LogRecord rec(LogLevel::LOG_LVL_INFO, 0, "", 0, "", "", "", -1, false);

    slow_logger.log(rec, "slow1");

    // Enqueue a fast task -- should complete quickly even though slow is blocked
    const auto start = std::chrono::steady_clock::now();
    fast_logger.log(rec, "fast1");
    fast_logger.wait();
    const auto fast_duration = std::chrono::steady_clock::now() - start;

    slow_logger.wait();

    // Fast should complete well under 50ms (it has its own thread)
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(fast_duration).count();
    bool fast_was_quick = ms < 40;
    bool counts_ok = slow_logger.count() == 1 && fast_logger.count() == 1;

    if (!fast_was_quick) {
        std::cout << "Fast logger took " << ms << "ms (expected < 40)" << std::endl;
    }

    return fast_was_quick && counts_ok;
}

int main() {
    int passed = 0;
    int failed = 0;

    auto run = [&](const char* name, bool result) {
        if (result) { ++passed; std::cout << "PASS: " << name << std::endl; }
        else        { ++failed; std::cout << "FAIL: " << name << std::endl; }
    };

    run("per_logger_isolation", test_isolation());

    std::cout << "\n" << passed << " passed, " << failed << " failed" << std::endl;
    return failed > 0 ? 1 : 0;
}

#else // __EMSCRIPTEN__
int main() { return 0; }
#endif
