#include <logit/utils.hpp>
#include <logit/loggers/PrometheusHttpServerLogger.hpp>

#if defined(LOGIT_WITH_PROMETHEUS_SERVER)

#include <cassert>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

#include <client_http.hpp>

int main() {
    // Test 1: Construction and shutdown without deadlock
    {
        logit::PrometheusHttpServerLogger::Config config;
        config.port = 43191;
        config.path = "/metrics";
        config.start_immediately = false;

        logit::PrometheusHttpServerLogger logger(config);

        logit::LogRecord record(
            logit::LogLevel::LOG_LVL_WARN, 1710000000123LL,
            "test.cpp", 10, "test_func", "shutdown test", "",
            -1, false, false, false);
        logger.log(record, "shutdown test");

        auto start = std::chrono::steady_clock::now();
        logger.shutdown();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();

        assert(elapsed < 5000);
    }

    // Test 2: collect_payload() returns valid Prometheus text format
    {
        logit::PrometheusHttpServerLogger::Config config;
        config.port = 43192;
        config.path = "/metrics";
        config.format.metric_prefix = "logit_";
        config.format.include_build_info = true;
        config.start_immediately = false;

        logit::PrometheusHttpServerLogger logger(config);

        logit::LogRecord record(
            logit::LogLevel::LOG_LVL_WARN, 1710000000123LL,
            "test.cpp", 20, "test_func", "payload test", "",
            -1, false, false, false);
        logger.log(record, "payload test");

        std::string payload = logger.collect_payload();

        assert(payload.find("# HELP logit_log_records_total") != std::string::npos);
        assert(payload.find("# TYPE logit_log_records_total counter") != std::string::npos);
        assert(payload.find("logit_log_records_total") != std::string::npos);
        assert(payload.find("# TYPE logit_last_log_timestamp_ms gauge") != std::string::npos);
        assert(payload.find("# TYPE logit_build_info gauge") != std::string::npos);
        assert(payload.find("version=") != std::string::npos);
        assert(payload.find("compiler=") != std::string::npos);

        logger.shutdown();
    }

    // Test 3: metric_prefix applied
    {
        logit::PrometheusHttpServerLogger::Config config;
        config.port = 43193;
        config.format.metric_prefix = "app_";
        config.start_immediately = false;

        logit::PrometheusHttpServerLogger logger(config);

        logit::LogRecord record(
            logit::LogLevel::LOG_LVL_INFO, 1710000000123LL,
            "test.cpp", 30, "test_func", "prefix test", "",
            -1, false, false, false);
        logger.log(record, "prefix test");

        std::string payload = logger.collect_payload();

        assert(payload.find("app_log_records_total") != std::string::npos);
        assert(payload.find("app_build_info") != std::string::npos);

        logger.shutdown();
    }

    // Test 4: custom on_collect adds user metric
    {
        logit::PrometheusHttpServerLogger::Config config;
        config.port = 43194;
        config.on_collect = [](std::vector<logit::PrometheusMetricFamily>& families) {
            logit::PrometheusMetricFamily mf;
            mf.name = "custom_metric";
            mf.help = "A custom metric";
            mf.type = logit::PrometheusMetricType::Gauge;
            logit::PrometheusSample s;
            s.name = "custom_metric";
            s.value = 77.0;
            mf.samples.push_back(s);
            families.push_back(mf);
        };
        config.start_immediately = false;

        logit::PrometheusHttpServerLogger logger(config);

        std::string payload = logger.collect_payload();

        assert(payload.find("# HELP custom_metric A custom metric") != std::string::npos);
        assert(payload.find("# TYPE custom_metric gauge") != std::string::npos);
        assert(payload.find("custom_metric 77") != std::string::npos);

        logger.shutdown();
    }

    // Test 5: get_string_param / get_int_param / get_float_param
    {
        logit::PrometheusHttpServerLogger::Config config;
        config.port = 43195;
        config.start_immediately = false;

        logit::PrometheusHttpServerLogger logger(config);

        logit::LogRecord record(
            logit::LogLevel::LOG_LVL_ERROR, 1710000000123LL,
            "test.cpp", 40, "test_func", "param test", "",
            -1, false, false, false);
        logger.log(record, "param test");

        std::string ts_str = logger.get_string_param(logit::LoggerParam::LastLogTimestamp);
        assert(!ts_str.empty());
        assert(ts_str == "1710000000123");

        int64_t ts_i = logger.get_int_param(logit::LoggerParam::LastLogTimestamp);
        assert(ts_i == 1710000000123LL);

        double ts_f = logger.get_float_param(logit::LoggerParam::LastLogTimestamp);
        assert(ts_f > 0.0);

        logger.shutdown();
    }

    // Test 6: start/stop lifecycle
    {
        logit::PrometheusHttpServerLogger::Config config;
        config.port = 43196;
        config.start_immediately = false;

        logit::PrometheusHttpServerLogger logger(config);

        logger.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        logit::LogRecord record(
            logit::LogLevel::LOG_LVL_INFO, 1710000000123LL,
            "test.cpp", 50, "test_func", "start/stop test", "",
            -1, false, false, false);
        logger.log(record, "start/stop test");

        auto start = std::chrono::steady_clock::now();
        logger.shutdown();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();

        assert(elapsed < 5000);
    }

    // Test 7: HTTP GET /metrics returns valid payload
    {
        logit::PrometheusHttpServerLogger::Config config;
        config.port = 43197;
        config.path = "/metrics";
        config.format.metric_prefix = "logit_";
        config.start_immediately = false;

        logit::PrometheusHttpServerLogger logger(config);

        logit::LogRecord record(
            logit::LogLevel::LOG_LVL_INFO, 1710000000123LL,
            "test.cpp", 60, "test_func", "http metrics test", "",
            -1, false, false, false);
        logger.log(record, "http metrics test");

        logger.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(300));

        using HttpClient = SimpleWeb::Client<SimpleWeb::HTTP>;
        HttpClient client("localhost:43197");
        auto response = client.request("GET", "/metrics");

        assert(response->status_code.find("200") != std::string::npos);

        auto ct_it = response->header.find("Content-Type");
        assert(ct_it != response->header.end());
        assert(ct_it->second.find("text/plain") != std::string::npos);

        std::string body = response->content.string();
        assert(body.find("# HELP logit_log_records_total") != std::string::npos);
        assert(body.find("# TYPE logit_log_records_total counter") != std::string::npos);
        assert(body.find("logit_log_records_total") != std::string::npos);

        logger.shutdown();
    }

    // Test 8: HTTP GET /health returns 200
    {
        logit::PrometheusHttpServerLogger::Config config;
        config.port = 43198;
        config.enable_health_endpoint = true;
        config.start_immediately = false;

        logit::PrometheusHttpServerLogger logger(config);
        logger.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(300));

        using HttpClient = SimpleWeb::Client<SimpleWeb::HTTP>;
        HttpClient client("localhost:43198");
        auto response = client.request("GET", "/health");

        assert(response->status_code.find("200") != std::string::npos);
        assert(response->content.string() == "ok");

        logger.shutdown();
    }

    // Test 9: Unknown path returns 404
    {
        logit::PrometheusHttpServerLogger::Config config;
        config.port = 43199;
        config.start_immediately = false;

        logit::PrometheusHttpServerLogger logger(config);
        logger.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(300));

        using HttpClient = SimpleWeb::Client<SimpleWeb::HTTP>;
        HttpClient client("localhost:43199");
        auto response = client.request("GET", "/unknown");

        assert(response->status_code.find("404") != std::string::npos);

        logger.shutdown();
    }

    // Test 10: Custom metrics path works via HTTP
    {
        logit::PrometheusHttpServerLogger::Config config;
        config.port = 43200;
        config.path = "/custom_metrics";
        config.format.metric_prefix = "app_";
        config.start_immediately = false;

        logit::PrometheusHttpServerLogger logger(config);

        logit::LogRecord record(
            logit::LogLevel::LOG_LVL_WARN, 1710000000123LL,
            "test.cpp", 70, "test_func", "custom path test", "",
            -1, false, false, false);
        logger.log(record, "custom path test");

        logger.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(300));

        using HttpClient = SimpleWeb::Client<SimpleWeb::HTTP>;
        HttpClient client("localhost:43200");
        auto response = client.request("GET", "/custom_metrics");

        assert(response->status_code.find("200") != std::string::npos);

        std::string body = response->content.string();
        assert(body.find("# HELP app_log_records_total") != std::string::npos);
        assert(body.find("app_log_records_total") != std::string::npos);

        logger.shutdown();
    }

    return 0;
}

#else

int main() {
    return 0;
}

#endif
