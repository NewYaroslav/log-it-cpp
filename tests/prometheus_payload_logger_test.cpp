#include <logit/utils.hpp>
#include <logit/loggers/PrometheusPayloadLogger.hpp>

#ifdef LOGIT_WITH_PROMETHEUS

#include <cassert>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

int main() {
    // Test 1: collect_payload() contains logit_* metrics after a log
    {
        logit::PrometheusPayloadLogger::Config config;
        config.format.metric_prefix = "logit_";
        config.format.include_build_info = true;

        auto logger = std::unique_ptr<logit::PrometheusPayloadLogger>(new logit::PrometheusPayloadLogger(config));

        logit::LogRecord record(
            logit::LogLevel::LOG_LVL_WARN, 1710000000123LL,
            "test.cpp", 10, "test_func", "test message", "",
            -1, false, false, false);
        logger->log(record, "test message");

        std::string payload = logger->collect_payload();

        assert(payload.find("# HELP logit_log_records_total") != std::string::npos);
        assert(payload.find("# TYPE logit_log_records_total counter") != std::string::npos);
        assert(payload.find("logit_log_records_total") != std::string::npos);
        assert(payload.find("# TYPE logit_last_log_timestamp_ms gauge") != std::string::npos);
        assert(payload.find("# TYPE logit_time_since_last_log_ms gauge") != std::string::npos);
        assert(payload.find("# TYPE logit_build_info gauge") != std::string::npos);
        assert(payload.find("version=") != std::string::npos);
        assert(payload.find("compiler=") != std::string::npos);

        logger->shutdown();
    }

    // Test 2: on_payload called on wait() when emit_on_wait=true
    {
        std::atomic<int> payload_count{0};
        std::string last_payload;

        logit::PrometheusPayloadLogger::Config config;
        config.emit_on_wait = true;
        config.on_payload = [&payload_count, &last_payload](std::string payload) {
            last_payload = payload;
            ++payload_count;
        };

        auto logger = std::unique_ptr<logit::PrometheusPayloadLogger>(new logit::PrometheusPayloadLogger(config));

        logit::LogRecord record(
            logit::LogLevel::LOG_LVL_WARN, 1710000000123LL,
            "test.cpp", 20, "test_func", "wait test", "",
            -1, false, false, false);
        logger->log(record, "wait test");

        logger->wait();

        assert(payload_count.load() >= 1);
        assert(!last_payload.empty());
        assert(last_payload.find("logit_log_records_total") != std::string::npos);

        logger->shutdown();
    }

    // Test 3: callback exception increments FailedExportCount
    {
        logit::PrometheusPayloadLogger::Config config;
        config.emit_on_wait = true;
        config.on_payload = [](std::string) {
            throw std::runtime_error("callback failed");
        };

        auto logger = std::unique_ptr<logit::PrometheusPayloadLogger>(new logit::PrometheusPayloadLogger(config));

        logit::LogRecord record(
            logit::LogLevel::LOG_LVL_WARN, 1710000000123LL,
            "test.cpp", 30, "test_func", "exception test", "",
            -1, false, false, false);
        logger->log(record, "exception test");
        logger->wait();

        int64_t failed = logger->get_int_param(logit::LoggerParam::FailedExportCount);
        assert(failed >= 1);

        logger->shutdown();
    }

    // Test 4: no callback does not crash
    {
        logit::PrometheusPayloadLogger::Config config;

        auto logger = std::unique_ptr<logit::PrometheusPayloadLogger>(new logit::PrometheusPayloadLogger(config));

        logit::LogRecord record(
            logit::LogLevel::LOG_LVL_WARN, 1710000000123LL,
            "test.cpp", 40, "test_func", "no callback test", "",
            -1, false, false, false);
        logger->log(record, "no callback test");
        logger->wait();

        logger->shutdown();
    }

    // Test 5: custom on_collect adds user metric
    {
        logit::PrometheusPayloadLogger::Config config;
        config.on_collect = [](std::vector<logit::PrometheusMetricFamily>& families) {
            logit::PrometheusMetricFamily mf;
            mf.name = "custom_metric";
            mf.help = "A custom metric";
            mf.type = logit::PrometheusMetricType::Gauge;
            logit::PrometheusSample s;
            s.name = "custom_metric";
            s.value = 99.0;
            mf.samples.push_back(s);
            families.push_back(mf);
        };

        auto logger = std::unique_ptr<logit::PrometheusPayloadLogger>(new logit::PrometheusPayloadLogger(config));

        logit::LogRecord record(
            logit::LogLevel::LOG_LVL_WARN, 1710000000123LL,
            "test.cpp", 50, "test_func", "custom collect test", "",
            -1, false, false, false);
        logger->log(record, "custom collect test");

        std::string payload = logger->collect_payload();

        assert(payload.find("# HELP custom_metric A custom metric") != std::string::npos);
        assert(payload.find("# TYPE custom_metric gauge") != std::string::npos);
        assert(payload.find("custom_metric 99") != std::string::npos);

        logger->shutdown();
    }

    // Test 6: metric_prefix is applied
    {
        logit::PrometheusPayloadLogger::Config config;
        config.format.metric_prefix = "app_";

        auto logger = std::unique_ptr<logit::PrometheusPayloadLogger>(new logit::PrometheusPayloadLogger(config));

        logit::LogRecord record(
            logit::LogLevel::LOG_LVL_WARN, 1710000000123LL,
            "test.cpp", 60, "test_func", "prefix test", "",
            -1, false, false, false);
        logger->log(record, "prefix test");

        std::string payload = logger->collect_payload();

        assert(payload.find("app_log_records_total") != std::string::npos);
        assert(payload.find("app_build_info") != std::string::npos);

        logger->shutdown();
    }

    // Test 7: emit_on_log triggers callback on each log
    {
        std::atomic<int> payload_count{0};

        logit::PrometheusPayloadLogger::Config config;
        config.emit_on_log = true;
        config.on_payload = [&payload_count](std::string) {
            ++payload_count;
        };

        auto logger = std::unique_ptr<logit::PrometheusPayloadLogger>(new logit::PrometheusPayloadLogger(config));

        logit::LogRecord record1(
            logit::LogLevel::LOG_LVL_WARN, 1710000000123LL,
            "test.cpp", 70, "test_func", "emit on log 1", "",
            -1, false, false, false);
        logger->log(record1, "emit on log 1");

        logit::LogRecord record2(
            logit::LogLevel::LOG_LVL_WARN, 1710000000124LL,
            "test.cpp", 71, "test_func", "emit on log 2", "",
            -1, false, false, false);
        logger->log(record2, "emit on log 2");

        assert(payload_count.load() >= 2);

        logger->shutdown();
    }

    // Test 8: get_string_param / get_float_param
    {
        logit::PrometheusPayloadLogger::Config config;

        auto logger = std::unique_ptr<logit::PrometheusPayloadLogger>(new logit::PrometheusPayloadLogger(config));

        logit::LogRecord record(
            logit::LogLevel::LOG_LVL_INFO, 1710000000123LL,
            "test.cpp", 80, "test_func", "param test", "",
            -1, false, false, false);
        logger->log(record, "param test");

        std::string ts_str = logger->get_string_param(logit::LoggerParam::LastLogTimestamp);
        assert(!ts_str.empty());
        assert(ts_str == "1710000000123");

        double ts_f = logger->get_float_param(logit::LoggerParam::LastLogTimestamp);
        assert(ts_f > 0.0);

        int64_t ts_i = logger->get_int_param(logit::LoggerParam::LastLogTimestamp);
        assert(ts_i == 1710000000123LL);

        logger->shutdown();
    }

    return 0;
}

#else

int main() {
    return 0;
}

#endif
