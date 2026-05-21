#include <logit.hpp>

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

struct PayloadCollector {
    std::mutex mutex;
    std::condition_variable cv;
    std::atomic<int> count{0};
    std::vector<std::string> payloads;
};

} // namespace

int main() {
    // Test a: sync mode callback receives valid JSON with "resourceLogs"
    {
        PayloadCollector collector;

        logit::OtlpPayloadLoggerConfig config;
        config.async = false;
        config.format.service_name = "sync-test";
        config.on_payload = [&collector](std::string payload) {
            std::lock_guard<std::mutex> lock(collector.mutex);
            collector.payloads.push_back(std::move(payload));
            collector.count.fetch_add(1);
            collector.cv.notify_all();
        };

        LOGIT_ADD_LOGGER(
            logit::OtlpPayloadLogger,
            (config),
            logit::SimpleLogFormatter,
            ("%v")
        );

        LOGIT_WARN("sync payload test");

        assert(collector.count.load() == 1);
        assert(!collector.payloads.empty());
        assert(collector.payloads[0].find("\"resourceLogs\"") != std::string::npos);

        LOGIT_SHUTDOWN();
    }

    // Test a2: sync mode throwing callback increments failed exports
    {
        std::atomic<int> call_count{0};

        logit::OtlpPayloadLoggerConfig config;
        config.async = false;
        config.format.service_name = "sync-throw-test";
        config.on_payload = [&call_count](std::string) {
            ++call_count;
            throw std::runtime_error("sync payload rejected");
        };

        LOGIT_ADD_LOGGER(
            logit::OtlpPayloadLogger,
            (config),
            logit::SimpleLogFormatter,
            ("%v")
        );

        LOGIT_WARN("sync throw test");

        assert(call_count.load() == 1);
        uint64_t failed = static_cast<uint64_t>(LOGIT_GET_INT_PARAM(0, logit::LoggerParam::FailedExportCount));
        assert(failed >= 1);

        LOGIT_SHUTDOWN();
    }

    // Test b: async mode with batching - log 5 messages, verify callback receives 1 call with all 5 bodies
    {
        PayloadCollector collector;

        logit::OtlpPayloadLoggerConfig config;
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

        LOGIT_ADD_LOGGER(
            logit::OtlpPayloadLogger,
            (config),
            logit::SimpleLogFormatter,
            ("%v")
        );

        LOGIT_WARN("batch msg 1");
        LOGIT_WARN("batch msg 2");
        LOGIT_WARN("batch msg 3");
        LOGIT_WARN("batch msg 4");
        LOGIT_WARN("batch msg 5");

        LOGIT_WAIT();

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

        LOGIT_SHUTDOWN();
    }

    // Test c: async mode queue overflow with drop_on_overflow=true
    {
        PayloadCollector collector;

        logit::OtlpPayloadLoggerConfig config;
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

        LOGIT_ADD_LOGGER(
            logit::OtlpPayloadLogger,
            (config),
            logit::SimpleLogFormatter,
            ("%v")
        );

        for (int i = 0; i < 100; ++i) {
            LOGIT_WARN("overflow msg");
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        uint64_t dropped = static_cast<uint64_t>(LOGIT_GET_INT_PARAM(0, logit::LoggerParam::DroppedLogCount));
        assert(dropped > 0);

        LOGIT_SHUTDOWN();
    }

    // Test d: wait() blocks until queue drain
    {
        PayloadCollector collector;

        logit::OtlpPayloadLoggerConfig config;
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

        LOGIT_ADD_LOGGER(
            logit::OtlpPayloadLogger,
            (config),
            logit::SimpleLogFormatter,
            ("%v")
        );

        LOGIT_WARN("wait test message");
        LOGIT_WAIT();

        {
            std::unique_lock<std::mutex> lock(collector.mutex);
            collector.cv.wait_for(lock, std::chrono::seconds(3), [&collector]() {
                return collector.count.load() >= 1;
            });
        }

        assert(collector.count.load() >= 1);

        LOGIT_SHUTDOWN();
    }

    // Test e: shutdown() stops worker cleanly without deadlocks
    {
        PayloadCollector collector;

        logit::OtlpPayloadLoggerConfig config;
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

        LOGIT_ADD_LOGGER(
            logit::OtlpPayloadLogger,
            (config),
            logit::SimpleLogFormatter,
            ("%v")
        );

        LOGIT_WARN("shutdown test message");

        auto start = std::chrono::steady_clock::now();
        LOGIT_SHUTDOWN();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();

        assert(elapsed < 5000);

        LOGIT_SHUTDOWN();
    }

    // Test f: wait() blocks until slow callback finishes
    {
        PayloadCollector collector;

        logit::OtlpPayloadLoggerConfig config;
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

        LOGIT_ADD_LOGGER(
            logit::OtlpPayloadLogger,
            (config),
            logit::SimpleLogFormatter,
            ("%v")
        );

        LOGIT_WARN("slow callback test");

        auto start = std::chrono::steady_clock::now();
        LOGIT_WAIT();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();

        assert(elapsed >= 400);
        assert(collector.count.load() >= 1);

        LOGIT_SHUTDOWN();
    }

    // Test g: throwing callback increments failed export count
    {
        std::atomic<int> call_count{0};

        logit::OtlpPayloadLoggerConfig config;
        config.async = true;
        config.format.service_name = "throw-test";
        config.max_batch_size = 256;
        config.export_interval_ms = 50;
        config.on_payload = [&call_count](std::string) {
            ++call_count;
            throw std::runtime_error("payload rejected");
        };

        LOGIT_ADD_LOGGER(
            logit::OtlpPayloadLogger,
            (config),
            logit::SimpleLogFormatter,
            ("%v")
        );

        LOGIT_WARN("throw test 1");
        LOGIT_WARN("throw test 2");

        LOGIT_WAIT();
        LOGIT_SHUTDOWN();

        // both logs should have been attempted (maybe in one batch, maybe two)
        assert(call_count.load() >= 1);

        uint64_t failed = static_cast<uint64_t>(LOGIT_GET_INT_PARAM(0, logit::LoggerParam::FailedExportCount));
        assert(failed >= static_cast<uint64_t>(call_count.load()));
    }

    return 0;
}

#else

int main() {
    return 0;
}

#endif
