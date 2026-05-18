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

/// \brief Probe logger that records task count.
class CountingLogger : public logit::ILogger {
public:
    struct Config {
        bool async = true;
        bool use_dedicated_executor = false;
    };

    explicit CountingLogger(const Config& cfg) : m_cfg(cfg) {
        if (m_cfg.async && m_cfg.use_dedicated_executor) {
            m_executor.reset(new logit::detail::SingleThreadExecutor());
        }
    }

    ~CountingLogger() override {
        shutdown();
    }

    void log(const LogRecord& record, const std::string& message) override {
        m_last_ts.store(record.timestamp_ms);
        if (!m_cfg.async) {
            m_count.fetch_add(1, std::memory_order_relaxed);
            return;
        }
        if (m_executor) {
            m_executor->add_task([this]() { m_count.fetch_add(1, std::memory_order_relaxed); });
        } else {
            logit::detail::TaskExecutor::get_instance().add_task([this]() { m_count.fetch_add(1, std::memory_order_relaxed); });
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

    void shutdown() override {
        wait();
        if (m_executor) m_executor->shutdown();
    }

    std::size_t count() const { return m_count.load(std::memory_order_relaxed); }

private:
    Config m_cfg;
    std::atomic<int> m_level{static_cast<int>(LogLevel::LOG_LVL_TRACE)};
    std::atomic<int64_t> m_last_ts{0};
    std::atomic<std::size_t> m_count{0};
    std::unique_ptr<logit::detail::SingleThreadExecutor> m_executor;
};

static bool test_mixed_mode_drain() {
    // Logger A: dedicated executor
    CountingLogger::Config cfg_a;
    cfg_a.async = true;
    cfg_a.use_dedicated_executor = true;
    CountingLogger logger_a(cfg_a);

    // Logger B: global executor
    CountingLogger::Config cfg_b;
    cfg_b.async = true;
    cfg_b.use_dedicated_executor = false;
    CountingLogger logger_b(cfg_b);

    LogRecord rec(LogLevel::LOG_LVL_INFO, 0, "", 0, "", "", "", -1, false);

    const int N = 20;
    for (int i = 0; i < N; ++i) {
        logger_a.log(rec, "a");
        logger_b.log(rec, "b");
    }

    logger_a.wait();
    logger_b.wait();

    bool a_ok = logger_a.count() == N;
    bool b_ok = logger_b.count() == N;

    if (!a_ok) std::cout << "Logger A: expected " << N << " got " << logger_a.count() << std::endl;
    if (!b_ok) std::cout << "Logger B: expected " << N << " got " << logger_b.count() << std::endl;

    return a_ok && b_ok;
}

static bool test_mixed_mode_shutdown() {
    // Ensure destructor properly drains both executors
    std::size_t a_count = 0;
    std::size_t b_count = 0;

    {
        CountingLogger::Config cfg_a;
        cfg_a.async = true;
        cfg_a.use_dedicated_executor = true;
        CountingLogger logger_a(cfg_a);

        CountingLogger::Config cfg_b;
        cfg_b.async = true;
        cfg_b.use_dedicated_executor = false;
        CountingLogger logger_b(cfg_b);

        LogRecord rec(LogLevel::LOG_LVL_INFO, 0, "", 0, "", "", "", -1, false);

        for (int i = 0; i < 10; ++i) {
            logger_a.log(rec, "a");
            logger_b.log(rec, "b");
        }

        // Destructors call shutdown/wait
    }

    // If we reach here without hang, the test passes
    return true;
}

int main() {
    int passed = 0;
    int failed = 0;

    auto run = [&](const char* name, bool result) {
        if (result) { ++passed; std::cout << "PASS: " << name << std::endl; }
        else        { ++failed; std::cout << "FAIL: " << name << std::endl; }
    };

    run("mixed_mode_drain", test_mixed_mode_drain());
    run("mixed_mode_shutdown", test_mixed_mode_shutdown());

    std::cout << "\n" << passed << " passed, " << failed << " failed" << std::endl;
    return failed > 0 ? 1 : 0;
}

#else // __EMSCRIPTEN__
int main() { return 0; }
#endif
