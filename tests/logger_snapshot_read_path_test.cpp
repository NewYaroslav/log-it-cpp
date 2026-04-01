#include <atomic>
#include <chrono>
#include <future>
#include <thread>
#include <vector>

#include <logit.hpp>

namespace {
class ProbeLogger final : public logit::ILogger {
public:
    void log(const logit::LogRecord&, const std::string&) override {
        entered.store(true, std::memory_order_release);
        while (!allow_exit.load(std::memory_order_acquire)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    std::string get_string_param(const logit::LoggerParam&) const override { return std::string(); }
    int64_t get_int_param(const logit::LoggerParam&) const override { return 0; }
    double get_float_param(const logit::LoggerParam&) const override { return 0.0; }

    void set_log_level(logit::LogLevel level) override {
        m_level.store(static_cast<int>(level), std::memory_order_relaxed);
    }

    logit::LogLevel get_log_level() const override {
        return static_cast<logit::LogLevel>(m_level.load(std::memory_order_relaxed));
    }

    std::vector<std::string> get_buffered_strings() const override {
        return std::vector<std::string>(1, "snapshot");
    }

    std::vector<logit::BufferedLogEntry> get_buffered_entries() const override {
        return std::vector<logit::BufferedLogEntry>(1, logit::BufferedLogEntry{});
    }

    void wait() override {}

    std::atomic<bool> entered = ATOMIC_VAR_INIT(false);
    std::atomic<bool> allow_exit = ATOMIC_VAR_INIT(false);

private:
    std::atomic<int> m_level = ATOMIC_VAR_INIT(static_cast<int>(logit::LogLevel::LOG_LVL_TRACE));
};
} // namespace

int main() {
    ProbeLogger* probe = new ProbeLogger();
    logit::Logger::get_instance().add_logger(
        std::unique_ptr<logit::ILogger>(probe),
        std::unique_ptr<logit::ILogFormatter>(new logit::SimpleLogFormatter("%v")));

    std::thread writer([]() {
        LOGIT_INFO("held-by-probe");
    });

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(1);
    while (!probe->entered.load(std::memory_order_acquire)) {
        if (std::chrono::steady_clock::now() >= deadline) {
            probe->allow_exit.store(true, std::memory_order_release);
            writer.join();
            return 1;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    auto strings_future = std::async(std::launch::async, []() {
        return logit::Logger::get_instance().get_buffered_strings(0);
    });
    auto entries_future = std::async(std::launch::async, []() {
        return logit::Logger::get_instance().get_buffered_entries(0);
    });
    auto level_future = std::async(std::launch::async, []() {
        return logit::Logger::get_instance().get_log_level(0);
    });

    if (strings_future.wait_for(std::chrono::milliseconds(100)) != std::future_status::ready) {
        probe->allow_exit.store(true, std::memory_order_release);
        writer.join();
        return 1;
    }
    if (entries_future.wait_for(std::chrono::milliseconds(100)) != std::future_status::ready) {
        probe->allow_exit.store(true, std::memory_order_release);
        writer.join();
        return 1;
    }
    if (level_future.wait_for(std::chrono::milliseconds(100)) != std::future_status::ready) {
        probe->allow_exit.store(true, std::memory_order_release);
        writer.join();
        return 1;
    }

    const auto strings = strings_future.get();
    const auto entries = entries_future.get();
    const auto level = level_future.get();

    probe->allow_exit.store(true, std::memory_order_release);
    writer.join();

    if (strings.size() != 1 || strings[0] != "snapshot") {
        return 1;
    }
    if (entries.size() != 1) {
        return 1;
    }
    if (level != logit::LogLevel::LOG_LVL_TRACE) {
        return 1;
    }

    return 0;
}
