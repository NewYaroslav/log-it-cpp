#include <logit.hpp>
#include <cassert>
#include <chrono>
#include <future>
#include <string>
#include <thread>
#include <vector>

namespace {

std::string make_unique_directory_name(const std::string& prefix) {
    const long long stamp = static_cast<long long>(
        std::chrono::steady_clock::now().time_since_epoch().count());
    return prefix + "_" + std::to_string(stamp);
}

logit::LogRecord make_record(logit::LogLevel level, int64_t timestamp_ms, int line) {
    return logit::LogRecord(
        level,
        timestamp_ms,
        "logger_clear_api_test.cpp",
        line,
        "make_record",
        "message",
        "",
        -1,
        false,
        false,
        false);
}

void test_memory_logger_clear_direct() {
    logit::MemoryLogger logger(logit::MemoryLogger::Config{0, 0, 0});
    logger.log(make_record(logit::LogLevel::LOG_LVL_INFO, 100, 10), "first");
    logger.log(make_record(logit::LogLevel::LOG_LVL_WARN, 101, 11), "second");
    assert(logger.get_buffered_entries().size() == 2);

    logit::LogClearResult result = logger.clear_logs();
    assert(result.ok);
    assert(result.status == logit::LogClearStatus::Cleared);
    assert(result.cleared_records == 2);
    assert(logger.get_buffered_entries().empty());
    assert(logger.get_int_param(logit::LoggerParam::LastLogTimestamp) == 0);

    logger.log(make_record(logit::LogLevel::LOG_LVL_ERROR, 102, 12), "after-clear");
    const auto entries = logger.get_buffered_entries();
    assert(entries.size() == 1);
    assert(entries[0].message == "after-clear");
}

void test_logger_clear_specific_and_all() {
    logit::Logger& logger = logit::Logger::get_instance();
    const int memory_index = static_cast<int>(logger.logger_count());
    logger.add_logger(
        std::unique_ptr<logit::ILogger>(new logit::MemoryLogger(logit::MemoryLogger::Config{0, 0, 0})),
        std::unique_ptr<logit::ILogFormatter>(new logit::SimpleLogFormatter("%v")));
    const int console_index = static_cast<int>(logger.logger_count());
    logger.add_logger(
        std::unique_ptr<logit::ILogger>(new logit::ConsoleLogger()),
        std::unique_ptr<logit::ILogFormatter>(new logit::SimpleLogFormatter("%v")));
    const int second_memory_index = static_cast<int>(logger.logger_count());
    logger.add_logger(
        std::unique_ptr<logit::ILogger>(new logit::MemoryLogger(logit::MemoryLogger::Config{0, 0, 0})),
        std::unique_ptr<logit::ILogFormatter>(new logit::SimpleLogFormatter("%v")));

    LOGIT_RAW_TO(memory_index, "memory-one");
    LOGIT_RAW_TO(second_memory_index, "memory-two");
    assert(LOGIT_GET_BUFFERED_STRINGS(memory_index).size() == 1);
    assert(LOGIT_GET_BUFFERED_STRINGS(second_memory_index).size() == 1);

    logit::LogClearResult one = LOGIT_CLEAR_LOGGER(memory_index);
    assert(one.ok);
    assert(one.status == logit::LogClearStatus::Cleared);
    assert(one.cleared_records == 1);
    assert(LOGIT_GET_BUFFERED_STRINGS(memory_index).empty());
    assert(LOGIT_GET_BUFFERED_STRINGS(second_memory_index).size() == 1);

    logit::LogClearResult unsupported = LOGIT_CLEAR_LOGGER(console_index);
    assert(!unsupported.ok);
    assert(unsupported.status == logit::LogClearStatus::Unsupported);

    logit::LogClearResult all = LOGIT_CLEAR_ALL_LOGGERS();
    assert(all.ok);
    assert(all.status == logit::LogClearStatus::Cleared);
    assert(all.cleared_records >= 1);
    assert(LOGIT_GET_BUFFERED_STRINGS(second_memory_index).empty());

    LOGIT_RAW_TO(memory_index, "after-clear");
    assert(LOGIT_GET_BUFFERED_STRINGS(memory_index).size() == 1);
}

void test_file_loggers_clear_and_continue() {
    logit::FileLogger::Config file_config;
    file_config.directory = make_unique_directory_name("clear_file_logs");
    file_config.async = false;
    logit::FileLogger file_logger(file_config);
    file_logger.log(make_record(logit::LogLevel::LOG_LVL_INFO, LOGIT_CURRENT_TIMESTAMP_MS(), 20), "file-before-clear");
    assert(!file_logger.list_log_files().empty());

    logit::LogClearResult file_clear = file_logger.clear_logs();
    assert(file_clear.ok);
    assert(file_clear.status == logit::LogClearStatus::Cleared);
    assert(file_clear.cleared_records >= 1);
    std::vector<logit::LogFileInfo> file_logs = file_logger.list_log_files();
    assert(file_logs.size() == 1);
    logit::LogFileReadResult empty_current = file_logger.read_log_file(file_logs[0].path);
    assert(empty_current.ok);
    assert(empty_current.content.empty());

    file_logger.log(make_record(logit::LogLevel::LOG_LVL_INFO, LOGIT_CURRENT_TIMESTAMP_MS(), 21), "file-after-clear");
    file_logs = file_logger.list_log_files();
    assert(!file_logs.empty());
    logit::LogFileReadResult after_file = file_logger.read_log_file(file_logs[0].path);
    assert(after_file.ok);
    assert(after_file.content.find("file-after-clear") != std::string::npos);

    logit::UniqueFileLogger::Config unique_config;
    unique_config.directory = make_unique_directory_name("clear_unique_logs");
    unique_config.async = false;
    logit::UniqueFileLogger unique_logger(unique_config);
    unique_logger.log(make_record(logit::LogLevel::LOG_LVL_INFO, LOGIT_CURRENT_TIMESTAMP_MS(), 30), "unique-before-clear");
    assert(!unique_logger.list_log_files().empty());

    logit::LogClearResult unique_clear = unique_logger.clear_logs();
    assert(unique_clear.ok);
    assert(unique_clear.status == logit::LogClearStatus::Cleared);
    assert(unique_clear.cleared_records >= 1);
    assert(unique_logger.list_log_files().empty());

    unique_logger.log(make_record(logit::LogLevel::LOG_LVL_INFO, LOGIT_CURRENT_TIMESTAMP_MS(), 31), "unique-after-clear");
    assert(unique_logger.list_log_files().size() == 1);
}

class BlockingClearLogger final : public logit::ILogger {
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
    void set_log_level(logit::LogLevel level) override { m_level.store(static_cast<int>(level), std::memory_order_relaxed); }
    logit::LogLevel get_log_level() const override { return static_cast<logit::LogLevel>(m_level.load(std::memory_order_relaxed)); }
    void wait() override {}

    logit::LogClearResult clear_logs(const logit::LogClearOptions& options = logit::LogClearOptions()) override {
        (void)options;
        logit::LogClearResult result;
        result.ok = true;
        result.status = logit::LogClearStatus::Cleared;
        result.message = "cleared";
        return result;
    }

    std::atomic<bool> entered = ATOMIC_VAR_INIT(false);
    std::atomic<bool> allow_exit = ATOMIC_VAR_INIT(false);

private:
    std::atomic<int> m_level = ATOMIC_VAR_INIT(static_cast<int>(logit::LogLevel::LOG_LVL_TRACE));
};

void test_clear_uses_strategy_snapshot() {
    logit::Logger& logger = logit::Logger::get_instance();
    BlockingClearLogger* blocking = new BlockingClearLogger();
    const int blocking_index = static_cast<int>(logger.logger_count());
    logger.add_logger(
        std::unique_ptr<logit::ILogger>(blocking),
        std::unique_ptr<logit::ILogFormatter>(new logit::SimpleLogFormatter("%v")));
    const int memory_index = static_cast<int>(logger.logger_count());
    logger.add_logger(
        std::unique_ptr<logit::ILogger>(new logit::MemoryLogger(logit::MemoryLogger::Config{0, 0, 0})),
        std::unique_ptr<logit::ILogFormatter>(new logit::SimpleLogFormatter("%v")));

    std::thread writer([blocking_index]() {
        LOGIT_RAW_TO(blocking_index, "held-by-blocking-logger");
    });

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(1);
    while (!blocking->entered.load(std::memory_order_acquire)) {
        if (std::chrono::steady_clock::now() >= deadline) {
            blocking->allow_exit.store(true, std::memory_order_release);
            writer.join();
            assert(false);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    auto clear_future = std::async(std::launch::async, [memory_index]() {
        return LOGIT_CLEAR_LOGGER(memory_index);
    });
    assert(clear_future.wait_for(std::chrono::milliseconds(100)) == std::future_status::ready);
    assert(clear_future.get().ok);

    blocking->allow_exit.store(true, std::memory_order_release);
    writer.join();
}

} // namespace

int main() {
    test_memory_logger_clear_direct();
    test_logger_clear_specific_and_all();
    test_file_loggers_clear_and_continue();
    test_clear_uses_strategy_snapshot();
    return 0;
}
