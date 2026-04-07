#include <atomic>
#include <string>
#include <thread>
#include <vector>

namespace {
std::atomic<long long> g_now_ms(0);

long long test_now_ms() {
    return g_now_ms.load();
}
} // namespace

#define LOGIT_CURRENT_TIMESTAMP_MS() test_now_ms()

#include <logit.hpp>

namespace {
logit::LogRecord make_record(logit::LogLevel level, int64_t ts, int line) {
    return logit::LogRecord(level, ts, "memory_logger_concurrency_test.cpp", line, "writer", "", "", -1, false);
}
} // namespace

int main() {
    logit::MemoryLogger logger(logit::MemoryLogger::Config{0, 0, 0});

    std::atomic<int64_t> seq(1);
    std::atomic<bool> failed(false);
    std::atomic<bool> done(false);

    std::thread reader([&]() {
        while (!done.load()) {
            const auto entries = logger.get_buffered_entries();
            for (size_t i = 1; i < entries.size(); ++i) {
                if (entries[i - 1].timestamp_ms > entries[i].timestamp_ms) {
                    failed = true;
                    done = true;
                    return;
                }
            }
            (void)logger.get_buffered_strings();
        }
    });

    std::thread writer([&logger, &seq, &failed]() {
        for (int i = 0; i < 800; ++i) {
            const int64_t ts = seq.fetch_add(1);
            g_now_ms = ts;
            logger.log(make_record(logit::LogLevel::LOG_LVL_INFO, ts, static_cast<int>(ts)),
                       std::to_string(ts));
            if (failed.load()) {
                return;
            }
        }
    });

    writer.join();
    done = true;
    reader.join();

    if (failed.load()) {
        return 1;
    }

    const auto entries = logger.get_buffered_entries();
    if (entries.size() != 800) {
        return 1;
    }
    if (entries.front().timestamp_ms != 1 || entries.back().timestamp_ms != 800) {
        return 1;
    }

    const auto strings = logger.get_buffered_strings();
    if (strings.size() != 800 || strings.front() != "1" || strings.back() != "800") {
        return 1;
    }

    return 0;
}
