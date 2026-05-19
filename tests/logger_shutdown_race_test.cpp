#include <logit.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace {

class ShutdownRaceLogger final : public logit::ILogger {
public:
    void log(const logit::LogRecord&, const std::string&) override {
        if (m_shutdown_started.load(std::memory_order_acquire)) {
            m_logs_after_shutdown.fetch_add(1, std::memory_order_relaxed);
        }
        m_logs.fetch_add(1, std::memory_order_relaxed);
    }

    std::string get_string_param(const logit::LoggerParam&) const override {
        return std::string();
    }

    int64_t get_int_param(const logit::LoggerParam&) const override {
        return 0;
    }

    double get_float_param(const logit::LoggerParam&) const override {
        return 0.0;
    }

    void set_log_level(logit::LogLevel level) override {
        m_level.store(static_cast<int>(level), std::memory_order_relaxed);
    }

    logit::LogLevel get_log_level() const override {
        return static_cast<logit::LogLevel>(m_level.load(std::memory_order_relaxed));
    }

    void wait() override {}

    void shutdown() override {
        m_shutdown_started.store(true, std::memory_order_release);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::size_t logs_after_shutdown() const {
        return m_logs_after_shutdown.load(std::memory_order_relaxed);
    }

    bool shutdown_started() const {
        return m_shutdown_started.load(std::memory_order_acquire);
    }

private:
    std::atomic<int> m_level{static_cast<int>(logit::LogLevel::LOG_LVL_TRACE)};
    std::atomic<std::size_t> m_logs{0};
    std::atomic<std::size_t> m_logs_after_shutdown{0};
    std::atomic<bool> m_shutdown_started{false};
};

} // namespace

int main() {
    ShutdownRaceLogger* raw_logger = new ShutdownRaceLogger();
    logit::Logger::get_instance().add_logger(
            std::unique_ptr<logit::ILogger>(raw_logger),
            std::unique_ptr<logit::ILogFormatter>(
                    new logit::SimpleLogFormatter("%v")));

    std::atomic<bool> start(false);
    std::atomic<bool> stop(false);
    std::vector<std::thread> producers;
    for (int i = 0; i < 4; ++i) {
        producers.emplace_back([&start, &stop]() {
            while (!start.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            while (!stop.load(std::memory_order_acquire)) {
                LOGIT_INFO("shutdown race");
            }
        });
    }

    start.store(true, std::memory_order_release);
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    LOGIT_SHUTDOWN();
    stop.store(true, std::memory_order_release);

    for (std::size_t i = 0; i < producers.size(); ++i) {
        producers[i].join();
    }

    LOGIT_INFO("after shutdown");

    const bool ok = raw_logger->shutdown_started() &&
                    raw_logger->logs_after_shutdown() == 0;
    std::cout << (ok ? "PASS" : "FAIL") << ": logger_shutdown_race" << std::endl;
    return ok ? 0 : 1;
}
