#include <logit/utils.hpp>
#include <logit/loggers/OtlpPayloadLogger.hpp>

#ifdef LOGIT_WITH_OTLP

#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace {

struct Collector {
    std::mutex mutex;
    std::condition_variable cv;
    std::atomic<int> count{0};
    std::vector<std::string> payloads;
};

} // namespace

int main() {
    // Test a: sync mode callback receives valid JSON with "resourceLogs"
    {
        std::atomic<int> count{0};
        std::vector<std::string> payloads;

        logit::OtlpPayloadLogger::Config config;
        config.async = false;
        config.format.service_name = "sync-test";
        config.on_payload = [&count, &payloads](std::string payload) {
            payloads.push_back(std::move(payload));
            ++count;
        };

        auto logger = std::unique_ptr<logit::OtlpPayloadLogger>(new logit::OtlpPayloadLogger(config));

        logit::LogRecord record(
            logit::LogLevel::LOG_LVL_WARN, 1710000000123LL,
            "test.cpp", 10, "test_func", "sync payload test", "",
            -1, false, false, false);
        logger->log(record, "sync payload test");

        assert(count.load() == 1);
        assert(!payloads.empty());
        assert(payloads[0].find("\"resourceLogs\"") != std::string::npos);

        logger->shutdown();
    }

    // Test a2: sync mode throwing callback increments failed exports
    {
        std::atomic<int> call_count{0};

        logit::OtlpPayloadLogger::Config config;
        config.async = false;
        config.format.service_name = "sync-throw-test";
        config.on_payload = [&call_count](std::string) {
            ++call_count;
            throw std::runtime_error("sync payload rejected");
        };

        auto logger = std::unique_ptr<logit::OtlpPayloadLogger>(new logit::OtlpPayloadLogger(config));

        logit::LogRecord record(
            logit::LogLevel::LOG_LVL_WARN, 1710000000123LL,
            "test.cpp", 20, "test_func", "sync throw test", "",
            -1, false, false, false);
        logger->log(record, "sync throw test");

        assert(call_count.load() == 1);
        uint64_t failed = static_cast<uint64_t>(logger->get_int_param(logit::LoggerParam::FailedExportCount));
        assert(failed >= 1);

        logger->shutdown();
    }

    // Test b: async mode with batching - log 5 messages, verify callback receives 1 call with all 5 bodies
    {
        Collector collector;

        logit::OtlpPayloadLogger::Config config;
        config.async = true;
        config.format.service_name = "batch-test";
        config.max_batch_size = 256;
        config.export_interval_ms = 50;
        config.on_payload = [&collector](std::string payload) {
            std::lock_guard<std::mutex> lock(collector.mutex);
            collector.payloads.push_back(std::move(payload));
            collector.count.fetch_add(1);
            collector.cv.notify_all();
        };

        auto logger = std::unique_ptr<logit::OtlpPayloadLogger>(new logit::OtlpPayloadLogger(config));

        for (int i = 1; i <= 5; ++i) {
            logit::LogRecord record(
                logit::LogLevel::LOG_LVL_WARN, 1710000000123LL + i,
                "test.cpp", 30 + i, "test_func", "batch msg", "",
                -1, false, false, false);
            logger->log(record, "batch msg " + std::to_string(i));
        }

        logger->wait();

        {
            std::unique_lock<std::mutex> lock(collector.mutex);
            collector.cv.wait_for(lock, std::chrono::seconds(3), [&collector]() {
                return collector.count.load() >= 1;
            });
        }

        assert(collector.count.load() >= 1);

        int body_count = 0;
        for (const auto& p : collector.payloads) {
            std::size_t pos = 0;
            while ((pos = p.find("\"body\"", pos)) != std::string::npos) {
                ++body_count;
                ++pos;
            }
        }
        assert(body_count == 5);

        logger->shutdown();
    }

    // Test c: async mode queue overflow with drop_on_overflow=true
    {
        Collector collector;

        logit::OtlpPayloadLogger::Config config;
        config.async = true;
        config.format.service_name = "overflow-test";
        config.max_queue_size = 2;
        config.max_batch_size = 1;
        config.drop_on_overflow = true;
        config.export_interval_ms = 500;
        config.on_payload = [&collector](std::string payload) {
            std::lock_guard<std::mutex> lock(collector.mutex);
            collector.payloads.push_back(std::move(payload));
            collector.count.fetch_add(1);
            collector.cv.notify_all();
        };

        auto logger = std::unique_ptr<logit::OtlpPayloadLogger>(new logit::OtlpPayloadLogger(config));

        for (int i = 0; i < 100; ++i) {
            logit::LogRecord record(
                logit::LogLevel::LOG_LVL_WARN, 1710000000123LL,
                "test.cpp", 40, "test_func", "overflow msg", "",
                -1, false, false, false);
            logger->log(record, "overflow msg");
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        uint64_t dropped = static_cast<uint64_t>(logger->get_int_param(logit::LoggerParam::DroppedLogCount));
        assert(dropped > 0);

        logger->shutdown();
    }

    // Test d: wait() blocks until queue drain
    {
        Collector collector;

        logit::OtlpPayloadLogger::Config config;
        config.async = true;
        config.format.service_name = "wait-test";
        config.max_batch_size = 256;
        config.export_interval_ms = 50;
        config.on_payload = [&collector](std::string payload) {
            std::lock_guard<std::mutex> lock(collector.mutex);
            collector.payloads.push_back(std::move(payload));
            collector.count.fetch_add(1);
            collector.cv.notify_all();
        };

        auto logger = std::unique_ptr<logit::OtlpPayloadLogger>(new logit::OtlpPayloadLogger(config));

        logit::LogRecord record(
            logit::LogLevel::LOG_LVL_WARN, 1710000000123LL,
            "test.cpp", 50, "test_func", "wait test message", "",
            -1, false, false, false);
        logger->log(record, "wait test message");

        logger->wait();

        {
            std::unique_lock<std::mutex> lock(collector.mutex);
            collector.cv.wait_for(lock, std::chrono::seconds(3), [&collector]() {
                return collector.count.load() >= 1;
            });
        }

        assert(collector.count.load() >= 1);

        logger->shutdown();
    }

    // Test e: shutdown() stops worker cleanly without deadlocks
    {
        Collector collector;

        logit::OtlpPayloadLogger::Config config;
        config.async = true;
        config.format.service_name = "shutdown-test";
        config.max_batch_size = 256;
        config.export_interval_ms = 50;
        config.on_payload = [&collector](std::string payload) {
            std::lock_guard<std::mutex> lock(collector.mutex);
            collector.payloads.push_back(std::move(payload));
            collector.count.fetch_add(1);
            collector.cv.notify_all();
        };

        auto logger = std::unique_ptr<logit::OtlpPayloadLogger>(new logit::OtlpPayloadLogger(config));

        logit::LogRecord record(
            logit::LogLevel::LOG_LVL_WARN, 1710000000123LL,
            "test.cpp", 60, "test_func", "shutdown test message", "",
            -1, false, false, false);
        logger->log(record, "shutdown test message");

        auto start = std::chrono::steady_clock::now();
        logger->shutdown();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();

        assert(elapsed < 5000);
    }

    // Test f: wait() blocks until slow callback finishes
    {
        Collector collector;

        logit::OtlpPayloadLogger::Config config;
        config.async = true;
        config.format.service_name = "slow-callback-test";
        config.max_batch_size = 256;
        config.export_interval_ms = 50;
        config.on_payload = [&collector](std::string payload) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            std::lock_guard<std::mutex> lock(collector.mutex);
            collector.payloads.push_back(std::move(payload));
            collector.count.fetch_add(1);
            collector.cv.notify_all();
        };

        auto logger = std::unique_ptr<logit::OtlpPayloadLogger>(new logit::OtlpPayloadLogger(config));

        logit::LogRecord record(
            logit::LogLevel::LOG_LVL_WARN, 1710000000123LL,
            "test.cpp", 70, "test_func", "slow callback test", "",
            -1, false, false, false);
        logger->log(record, "slow callback test");

        auto start = std::chrono::steady_clock::now();
        logger->wait();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();

        assert(elapsed >= 400);
        assert(collector.count.load() >= 1);

        logger->shutdown();
    }

    // Test g: throwing callback increments failed export count
    {
        std::atomic<int> call_count{0};

        logit::OtlpPayloadLogger::Config config;
        config.async = true;
        config.format.service_name = "throw-test";
        config.max_batch_size = 256;
        config.export_interval_ms = 50;
        config.on_payload = [&call_count](std::string) {
            ++call_count;
            throw std::runtime_error("payload rejected");
        };

        auto logger = std::unique_ptr<logit::OtlpPayloadLogger>(new logit::OtlpPayloadLogger(config));

        for (int i = 1; i <= 2; ++i) {
            logit::LogRecord record(
                logit::LogLevel::LOG_LVL_WARN, 1710000000123LL + i,
                "test.cpp", 80 + i, "test_func", "throw test", "",
                -1, false, false, false);
            logger->log(record, "throw test " + std::to_string(i));
        }

        logger->wait();
        logger->shutdown();

        assert(call_count.load() >= 1);

        uint64_t failed = static_cast<uint64_t>(logger->get_int_param(logit::LoggerParam::FailedExportCount));
        assert(failed >= static_cast<uint64_t>(call_count.load()));
    }

    // Test h: async mode with payload splitting - small max_payload_bytes forces multiple chunks
    {
        Collector collector;

        logit::OtlpPayloadLogger::Config config;
        config.async = true;
        config.format.service_name = "async-split-test";
        config.max_batch_size = 256;
        config.max_payload_bytes = 1024;
        config.export_interval_ms = 50;
        config.on_payload = [&collector](std::string payload) {
            std::lock_guard<std::mutex> lock(collector.mutex);
            collector.payloads.push_back(std::move(payload));
            collector.count.fetch_add(1);
            collector.cv.notify_all();
        };

        auto logger = std::unique_ptr<logit::OtlpPayloadLogger>(new logit::OtlpPayloadLogger(config));

        for (int i = 0; i < 50; ++i) {
            logit::LogRecord record(
                logit::LogLevel::LOG_LVL_WARN, 1710000000123LL + i,
                "test.cpp", 90 + i, "test_func", "async split test", "",
                -1, false, false, false);
            logger->log(record, "async split payload test message number " + std::to_string(i));
        }

        logger->wait();

        {
            std::unique_lock<std::mutex> lock(collector.mutex);
            collector.cv.wait_for(lock, std::chrono::seconds(3), [&collector]() {
                return collector.count.load() >= 2;
            });
        }

        assert(collector.count.load() > 1);

        int body_count = 0;
        for (const auto& p : collector.payloads) {
            std::size_t pos = 0;
            while ((pos = p.find("\"body\"", pos)) != std::string::npos) {
                ++body_count;
                ++pos;
            }
        }
        assert(body_count == 50);

        logger->shutdown();
    }

    // Test i: sync mode with payload splitting - small max_payload_bytes forces multiple chunks
    {
        std::atomic<int> count{0};
        std::vector<std::string> payloads;

        logit::OtlpPayloadLogger::Config config;
        config.async = false;
        config.format.service_name = "sync-split-test";
        config.max_payload_bytes = 1024;
        config.on_payload = [&count, &payloads](std::string payload) {
            payloads.push_back(std::move(payload));
            ++count;
        };

        auto logger = std::unique_ptr<logit::OtlpPayloadLogger>(new logit::OtlpPayloadLogger(config));

        for (int i = 0; i < 50; ++i) {
            logit::LogRecord record(
                logit::LogLevel::LOG_LVL_WARN, 1710000000123LL + i,
                "test.cpp", 150 + i, "test_func", "sync split test", "",
                -1, false, false, false);
            logger->log(record, "sync split payload test message number " + std::to_string(i));
        }

        assert(count.load() > 1);

        int body_count = 0;
        for (const auto& p : payloads) {
            std::size_t pos = 0;
            while ((pos = p.find("\"body\"", pos)) != std::string::npos) {
                ++body_count;
                ++pos;
            }
        }
        assert(body_count == 50);

        logger->shutdown();
    }

    return 0;
}

#else

int main() {
    return 0;
}

#endif
