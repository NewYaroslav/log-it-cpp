#include <atomic>

namespace {
std::atomic<long long> g_now_ms(0);

long long test_now_ms() {
    return g_now_ms.load();
}
} // namespace

#define LOGIT_CURRENT_TIMESTAMP_MS() test_now_ms()

#include <logit.hpp>

namespace {
logit::LogRecord make_record(logit::LogLevel level, int64_t ts, int line, const char* function) {
    return logit::LogRecord(level, ts, "memory_logger_backend_test.cpp", line, function, "", "", -1, false);
}
} // namespace

int main() {
    logit::MemoryLogger logger(logit::MemoryLogger::Config{3, 10, 50});

    g_now_ms = 100;
    logger.log(make_record(logit::LogLevel::LOG_LVL_INFO, 100, 10, "first"), "12345");

    g_now_ms = 130;
    logger.log(make_record(logit::LogLevel::LOG_LVL_WARN, 130, 20, "second"), "6789");

    g_now_ms = 140;
    logger.log(make_record(logit::LogLevel::LOG_LVL_ERROR, 140, 30, "third"), "abcdef");

    const auto strings = logger.get_buffered_strings();
    if (strings.size() != 2 || strings[0] != "6789" || strings[1] != "abcdef") {
        return 1;
    }

    const auto entries = logger.get_buffered_entries();
    if (entries.size() != 2) {
        return 1;
    }
    if (entries[0].level != logit::LogLevel::LOG_LVL_WARN ||
        entries[0].timestamp_ms != 130 ||
        entries[0].line != 20 ||
        entries[0].function != "second" ||
        entries[0].message != "6789") {
        return 1;
    }
    if (entries[1].level != logit::LogLevel::LOG_LVL_ERROR ||
        entries[1].timestamp_ms != 140 ||
        entries[1].line != 30 ||
        entries[1].function != "third" ||
        entries[1].message != "abcdef") {
        return 1;
    }

    g_now_ms = 160;
    logger.log(make_record(logit::LogLevel::LOG_LVL_INFO, 160, 40, "fourth"), "xy");

    const auto strings_after_insert = logger.get_buffered_strings();
    if (strings_after_insert.size() != 2 ||
        strings_after_insert[0] != "abcdef" ||
        strings_after_insert[1] != "xy") {
        return 1;
    }

    g_now_ms = 211;
    const auto strings_after_age_cleanup = logger.get_buffered_strings();
    if (!strings_after_age_cleanup.empty()) {
        return 1;
    }

    logit::MemoryLogger unlimited(logit::MemoryLogger::Config{0, 0, 0});
    g_now_ms = 500;
    unlimited.log(make_record(logit::LogLevel::LOG_LVL_INFO, 500, 50, "u1"), "alpha");
    g_now_ms = 600;
    unlimited.log(make_record(logit::LogLevel::LOG_LVL_INFO, 600, 60, "u2"), "beta");
    g_now_ms = 10000;
    const auto unlimited_entries = unlimited.get_buffered_entries();
    if (unlimited_entries.size() != 2 ||
        unlimited_entries[0].message != "alpha" ||
        unlimited_entries[1].message != "beta") {
        return 1;
    }

    logit::MemoryLogger oversized(logit::MemoryLogger::Config{10, 4, 0});
    g_now_ms = 700;
    oversized.log(make_record(logit::LogLevel::LOG_LVL_INFO, 700, 70, "big"), "12345");
    if (!oversized.get_buffered_strings().empty()) {
        return 1;
    }

    return 0;
}
