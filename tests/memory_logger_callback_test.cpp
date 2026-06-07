#include <atomic>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

#include <logit.hpp>

namespace {
logit::LogRecord make_record(int64_t ts, int line, const char* function = "writer") {
    return logit::LogRecord(
        logit::LogLevel::LOG_LVL_INFO,
        ts,
        "memory_logger_callback_test.cpp",
        line,
        function,
        "",
        "",
        -1,
        false);
}

bool test_add_remove_and_view() {
    logit::MemoryLogger logger(logit::MemoryLogger::Config{0, 0, 0});
    std::vector<logit::LogRecordView> received;

    const uint64_t callback_id = logger.add_log_callback(
        [&received](const logit::LogRecordView& view) {
            received.push_back(view);
        });

    logger.log(make_record(100, 10, "first"), "alpha");

    if (received.size() != 1 ||
        received[0].timestamp_ms != 100 ||
        received[0].level != logit::LogLevel::LOG_LVL_INFO ||
        received[0].file != "memory_logger_callback_test.cpp" ||
        received[0].line != 10 ||
        received[0].function != "first" ||
        received[0].message != "alpha") {
        return false;
    }

    if (!logger.remove_log_callback(callback_id)) {
        return false;
    }
    if (logger.remove_log_callback(callback_id)) {
        return false;
    }

    logger.log(make_record(101, 11, "after_remove"), "beta");
    return received.size() == 1;
}

bool test_callback_order() {
    logit::MemoryLogger logger(logit::MemoryLogger::Config{0, 0, 0});
    std::vector<int> order;

    logger.add_log_callback([&order](const logit::LogRecordView&) { order.push_back(1); });
    logger.add_log_callback([&order](const logit::LogRecordView&) { order.push_back(2); });
    logger.add_log_callback([&order](const logit::LogRecordView&) { order.push_back(3); });

    logger.log(make_record(200, 20, "ordered"), "ordered");

    const std::vector<int> expected{1, 2, 3};
    return order == expected;
}

bool test_callbacks_can_reenter_logger() {
    logit::MemoryLogger logger(logit::MemoryLogger::Config{0, 0, 0});
    std::atomic<int> calls(0);
    std::atomic<bool> failed(false);
    uint64_t callback_id = 0;

    callback_id = logger.add_log_callback(
        [&logger, &calls, &failed, &callback_id](const logit::LogRecordView& view) {
            const auto entries = logger.get_buffered_entries();
            if (entries.empty() || entries.back().message != view.message) {
                failed = true;
            }
            if (!logger.remove_log_callback(callback_id)) {
                failed = true;
            }
            calls.fetch_add(1, std::memory_order_relaxed);
        });

    logger.log(make_record(300, 30, "reenter"), "self-remove");
    logger.log(make_record(301, 31, "reenter_after_remove"), "after");

    return !failed.load() && calls.load() == 1;
}

bool test_thread_safety() {
    logit::MemoryLogger logger(logit::MemoryLogger::Config{0, 0, 0});
    const int writer_count = 4;
    const int logs_per_writer = 250;
    const int expected_logs = writer_count * logs_per_writer;

    std::atomic<int64_t> sequence(1);
    std::atomic<int> primary_calls(0);
    std::atomic<int> transient_calls(0);
    std::atomic<bool> start(false);
    std::atomic<bool> failed(false);

    const uint64_t primary_id = logger.add_log_callback(
        [&primary_calls, &failed](const logit::LogRecordView& view) {
            if (view.message.empty()) {
                failed = true;
            }
            primary_calls.fetch_add(1, std::memory_order_relaxed);
        });

    std::vector<std::thread> writers;
    for (int t = 0; t < writer_count; ++t) {
        writers.push_back(std::thread([&logger, &sequence, &start, logs_per_writer, t]() {
            while (!start.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            for (int i = 0; i < logs_per_writer; ++i) {
                const int64_t ts = sequence.fetch_add(1, std::memory_order_relaxed);
                logger.log(make_record(ts, t * logs_per_writer + i), std::to_string(ts));
            }
        }));
    }

    std::thread churner([&logger, &transient_calls, &start, &failed]() {
        while (!start.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        for (int i = 0; i < 500; ++i) {
            const uint64_t id = logger.add_log_callback(
                [&transient_calls](const logit::LogRecordView&) {
                    transient_calls.fetch_add(1, std::memory_order_relaxed);
                });
            if (!logger.remove_log_callback(id)) {
                failed = true;
            }
        }
    });

    start.store(true, std::memory_order_release);
    for (auto& writer : writers) {
        writer.join();
    }
    churner.join();

    if (failed.load()) {
        return false;
    }
    if (primary_calls.load() != expected_logs) {
        return false;
    }

    const auto entries = logger.get_buffered_entries();
    if (entries.size() != static_cast<std::size_t>(expected_logs)) {
        return false;
    }

    if (!logger.remove_log_callback(primary_id)) {
        return false;
    }
    const int calls_after_remove = primary_calls.load();
    logger.log(make_record(sequence.fetch_add(1, std::memory_order_relaxed), 9999), "after-remove");
    return primary_calls.load() == calls_after_remove;
}
} // namespace

int main() {
    if (!test_add_remove_and_view()) {
        return 1;
    }
    if (!test_callback_order()) {
        return 1;
    }
    if (!test_callbacks_can_reenter_logger()) {
        return 1;
    }
    if (!test_thread_safety()) {
        return 1;
    }
    return 0;
}
