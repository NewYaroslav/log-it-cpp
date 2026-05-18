#include <logit.hpp>

#include <atomic>
#include <iostream>
#include <memory>

class ShutdownProbeLogger : public logit::ILogger {
public:
    ShutdownProbeLogger()
        : m_executor(new logit::detail::SingleThreadExecutor()) {}

    void log(const logit::LogRecord&, const std::string&) override {
        m_executor->add_task([this]() {
            m_count.fetch_add(1, std::memory_order_relaxed);
        });
    }

    std::string get_string_param(const logit::LoggerParam&) const override { return std::string(); }
    int64_t get_int_param(const logit::LoggerParam&) const override { return 0; }
    double get_float_param(const logit::LoggerParam&) const override { return 0.0; }
    void set_log_level(logit::LogLevel level) override { m_level.store(static_cast<int>(level)); }
    logit::LogLevel get_log_level() const override {
        return static_cast<logit::LogLevel>(m_level.load());
    }

    void wait() override {
        m_executor->wait();
    }

    void shutdown() override {
        m_shutdown_called.store(true, std::memory_order_relaxed);
        m_executor->shutdown();
    }

    bool shutdown_called() const {
        return m_shutdown_called.load(std::memory_order_relaxed);
    }

    std::size_t count() const {
        return m_count.load(std::memory_order_relaxed);
    }

private:
    std::unique_ptr<logit::detail::SingleThreadExecutor> m_executor;
    std::atomic<int> m_level{static_cast<int>(logit::LogLevel::LOG_LVL_TRACE)};
    std::atomic<bool> m_shutdown_called{false};
    std::atomic<std::size_t> m_count{0};
};

int main() {
    ShutdownProbeLogger* raw_logger = new ShutdownProbeLogger();
    logit::Logger::get_instance().add_logger(
            std::unique_ptr<logit::ILogger>(raw_logger),
            std::unique_ptr<logit::ILogFormatter>(
                    new logit::SimpleLogFormatter(LOGIT_CONSOLE_PATTERN)));

    const int message_count = 16;
    for (int i = 0; i < message_count; ++i) {
        LOGIT_INFO("shutdown probe");
    }

    LOGIT_SHUTDOWN();

    const bool ok = raw_logger->shutdown_called() &&
                    raw_logger->count() == static_cast<std::size_t>(message_count);
    std::cout << (ok ? "PASS" : "FAIL") << ": dedicated_executor_shutdown" << std::endl;
    return ok ? 0 : 1;
}
