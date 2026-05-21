#include <logit/loggers/prometheus/PrometheusTextSerializer.hpp>

#ifdef LOGIT_WITH_PROMETHEUS

#include <cassert>
#include <cmath>
#include <string>
#include <vector>

int main() {
    // Test 1: prometheus_escape_help
    {
        assert(logit::prometheus_escape_help("hello") == "hello");
        assert(logit::prometheus_escape_help("back\\slash") == "back\\\\slash");
        assert(logit::prometheus_escape_help("new\nline") == "new\\nline");
        assert(logit::prometheus_escape_help("both\\\n") == "both\\\\\\n");
    }

    // Test 2: prometheus_escape_label_value
    {
        assert(logit::prometheus_escape_label_value("simple") == "simple");
        assert(logit::prometheus_escape_label_value("with\"quote") == "with\\\"quote");
        assert(logit::prometheus_escape_label_value("back\\slash") == "back\\\\slash");
        assert(logit::prometheus_escape_label_value("new\nline") == "new\\nline");
    }

    // Test 3: prometheus_sanitize_metric_name
    {
        assert(logit::prometheus_sanitize_metric_name("valid_name") == "valid_name");
        assert(logit::prometheus_sanitize_metric_name("valid:name") == "valid:name");
        assert(logit::prometheus_sanitize_metric_name("123bad") == "_23bad");
        assert(logit::prometheus_sanitize_metric_name("my-metric") == "my_metric");
        assert(logit::prometheus_sanitize_metric_name("a.b") == "a_b");
        assert(logit::prometheus_sanitize_metric_name("") == "_");
    }

    // Test 4: prometheus_sanitize_label_name
    {
        assert(logit::prometheus_sanitize_label_name("valid_name") == "valid_name");
        assert(logit::prometheus_sanitize_label_name("123bad") == "_23bad");
        assert(logit::prometheus_sanitize_label_name("my-label") == "my_label");
        assert(logit::prometheus_sanitize_label_name("") == "_");
    }

    // Test 5: HELP and TYPE output for counter
    {
        logit::PrometheusMetricFamily mf;
        mf.name = "http_requests_total";
        mf.help = "Total HTTP requests";
        mf.type = logit::PrometheusMetricType::Counter;
        logit::PrometheusSample s;
        s.name = "http_requests_total";
        s.value = 42.0;
        mf.samples.push_back(s);

        logit::PrometheusTextFormatConfig config;
        std::string payload = logit::build_prometheus_text_payload({mf}, config);

        assert(payload.find("# HELP http_requests_total Total HTTP requests") != std::string::npos);
        assert(payload.find("# TYPE http_requests_total counter") != std::string::npos);
        assert(payload.find("http_requests_total 42") != std::string::npos);
    }

    // Test 6: Gauge type
    {
        logit::PrometheusMetricFamily mf;
        mf.name = "temperature";
        mf.help = "Current temperature";
        mf.type = logit::PrometheusMetricType::Gauge;
        logit::PrometheusSample s;
        s.name = "temperature";
        s.value = 23.5;
        mf.samples.push_back(s);

        logit::PrometheusTextFormatConfig config;
        std::string payload = logit::build_prometheus_text_payload({mf}, config);

        assert(payload.find("# TYPE temperature gauge") != std::string::npos);
        assert(payload.find("temperature 23.") != std::string::npos);
    }

    // Test 7: Untyped metric
    {
        logit::PrometheusMetricFamily mf;
        mf.name = "mystery";
        mf.help = "";
        mf.type = logit::PrometheusMetricType::Untyped;
        logit::PrometheusSample s;
        s.name = "mystery";
        s.value = 7.0;
        mf.samples.push_back(s);

        logit::PrometheusTextFormatConfig config;
        std::string payload = logit::build_prometheus_text_payload({mf}, config);

        assert(payload.find("# TYPE mystery untyped") != std::string::npos);
    }

    // Test 8: Labels with escaping
    {
        logit::PrometheusMetricFamily mf;
        mf.name = "test_metric";
        mf.help = "test";
        mf.type = logit::PrometheusMetricType::Counter;
        logit::PrometheusSample s;
        s.name = "test_metric";
        s.value = 1.0;
        s.labels.push_back({"method", "GET"});
        s.labels.push_back({"path", "/api/\"test\""});
        mf.samples.push_back(s);

        logit::PrometheusTextFormatConfig config;
        std::string payload = logit::build_prometheus_text_payload({mf}, config);

        assert(payload.find("method=\"GET\"") != std::string::npos);
        assert(payload.find("path=\"/api/\\\"test\\\"\"") != std::string::npos);
    }

    // Test 9: NaN, +Inf, -Inf value formatting
    {
        logit::PrometheusMetricFamily mf;
        mf.name = "special_values";
        mf.help = "Special float values";
        mf.type = logit::PrometheusMetricType::Gauge;

        logit::PrometheusSample s1;
        s1.name = "special_values";
        s1.value = std::numeric_limits<double>::quiet_NaN();
        s1.labels.push_back({"case", "nan"});
        mf.samples.push_back(s1);

        logit::PrometheusSample s2;
        s2.name = "special_values";
        s2.value = std::numeric_limits<double>::infinity();
        s2.labels.push_back({"case", "pos_inf"});
        mf.samples.push_back(s2);

        logit::PrometheusSample s3;
        s3.name = "special_values";
        s3.value = -std::numeric_limits<double>::infinity();
        s3.labels.push_back({"case", "neg_inf"});
        mf.samples.push_back(s3);

        logit::PrometheusTextFormatConfig config;
        std::string payload = logit::build_prometheus_text_payload({mf}, config);

        assert(payload.find("NaN") != std::string::npos);
        assert(payload.find("+Inf") != std::string::npos);
        assert(payload.find("-Inf") != std::string::npos);
    }

    // Test 10: const_labels
    {
        logit::PrometheusMetricFamily mf;
        mf.name = "with_const";
        mf.help = "With const labels";
        mf.type = logit::PrometheusMetricType::Counter;
        logit::PrometheusSample s;
        s.name = "with_const";
        s.value = 5.0;
        s.labels.push_back({"dynamic", "val"});
        mf.samples.push_back(s);

        logit::PrometheusTextFormatConfig config;
        config.const_labels.push_back({"job", "test_job"});
        std::string payload = logit::build_prometheus_text_payload({mf}, config);

        assert(payload.find("job=\"test_job\"") != std::string::npos);
        assert(payload.find("dynamic=\"val\"") != std::string::npos);
        // const_labels come before sample labels
        size_t job_pos = payload.find("job=\"test_job\"");
        size_t dyn_pos = payload.find("dynamic=\"val\"");
        assert(job_pos < dyn_pos);
    }

    // Test 11: timestamp optional
    {
        logit::PrometheusMetricFamily mf;
        mf.name = "ts_metric";
        mf.help = "With timestamp";
        mf.type = logit::PrometheusMetricType::Gauge;
        logit::PrometheusSample s;
        s.name = "ts_metric";
        s.value = 10.0;
        s.timestamp_ms = 1710000000123LL;
        mf.samples.push_back(s);

        logit::PrometheusTextFormatConfig config_with_ts;
        config_with_ts.include_timestamp = true;
        std::string payload_ts = logit::build_prometheus_text_payload({mf}, config_with_ts);
        assert(payload_ts.find("1710000000123") != std::string::npos);

        logit::PrometheusTextFormatConfig config_no_ts;
        config_no_ts.include_timestamp = false;
        std::string payload_no_ts = logit::build_prometheus_text_payload({mf}, config_no_ts);
        assert(payload_no_ts.find("1710000000123") == std::string::npos);
    }

    // Test 12: include_help=false and include_type=false
    {
        logit::PrometheusMetricFamily mf;
        mf.name = "bare_metric";
        mf.help = "Should not appear";
        mf.type = logit::PrometheusMetricType::Counter;
        logit::PrometheusSample s;
        s.name = "bare_metric";
        s.value = 1.0;
        mf.samples.push_back(s);

        logit::PrometheusTextFormatConfig config;
        config.include_help = false;
        config.include_type = false;
        std::string payload = logit::build_prometheus_text_payload({mf}, config);

        assert(payload.find("# HELP") == std::string::npos);
        assert(payload.find("# TYPE") == std::string::npos);
        assert(payload.find("bare_metric 1") != std::string::npos);
    }

    // Test 13: finite double precision
    {
        logit::PrometheusMetricFamily mf;
        mf.name = "precision_metric";
        mf.help = "Precision test";
        mf.type = logit::PrometheusMetricType::Gauge;
        logit::PrometheusSample s;
        s.name = "precision_metric";
        s.value = 0.1 + 0.2; // classic floating-point imprecision
        mf.samples.push_back(s);

        logit::PrometheusTextFormatConfig config;
        std::string payload = logit::build_prometheus_text_payload({mf}, config);
        // Should contain enough digits to round-trip
        assert(payload.find("0.30000000000000004") != std::string::npos);
    }

    return 0;
}

#else

int main() {
    return 0;
}

#endif
