#include <logit/loggers/prometheus/PrometheusMetricBuilders.hpp>

#ifdef LOGIT_WITH_PROMETHEUS

#include <cassert>
#include <vector>

int main() {
    // Test 1: make_prometheus_label
    {
        auto label = logit::make_prometheus_label("env", "production");
        assert(label.name == "env");
        assert(label.value == "production");
    }

    // Test 2: make_prometheus_sample (unlabeled)
    {
        auto sample = logit::make_prometheus_sample("cpu_seconds_total", 123.45);
        assert(sample.name == "cpu_seconds_total");
        assert(sample.value == 123.45);
        assert(sample.labels.empty());
        assert(sample.timestamp_ms == 0);
    }

    // Test 3: make_prometheus_sample (with labels and timestamp)
    {
        auto sample = logit::make_prometheus_sample(
            "cpu_seconds_total", 67.89,
            {logit::make_prometheus_label("cpu", "0")},
            1710000000123LL);
        assert(sample.name == "cpu_seconds_total");
        assert(sample.value == 67.89);
        assert(sample.labels.size() == 1);
        assert(sample.labels[0].name == "cpu");
        assert(sample.labels[0].value == "0");
        assert(sample.timestamp_ms == 1710000000123LL);
    }

    // Test 4: make_prometheus_counter (unlabeled)
    {
        auto mf = logit::make_prometheus_counter("requests_total", "Total requests", 42.0);
        assert(mf.name == "requests_total");
        assert(mf.help == "Total requests");
        assert(mf.type == logit::PrometheusMetricType::Counter);
        assert(mf.samples.size() == 1);
        assert(mf.samples[0].name == "requests_total");
        assert(mf.samples[0].value == 42.0);
        assert(mf.samples[0].labels.empty());
    }

    // Test 5: make_prometheus_counter (labeled)
    {
        auto mf = logit::make_prometheus_counter(
            "requests_total", "Total requests", 7.0,
            {logit::make_prometheus_label("method", "GET")});
        assert(mf.samples[0].labels.size() == 1);
        assert(mf.samples[0].labels[0].name == "method");
        assert(mf.samples[0].labels[0].value == "GET");
    }

    // Test 6: make_prometheus_gauge (unlabeled)
    {
        auto mf = logit::make_prometheus_gauge("queue_size", "Current queue size", 3.0);
        assert(mf.name == "queue_size");
        assert(mf.help == "Current queue size");
        assert(mf.type == logit::PrometheusMetricType::Gauge);
        assert(mf.samples[0].value == 3.0);
    }

    // Test 7: make_prometheus_gauge (labeled)
    {
        auto mf = logit::make_prometheus_gauge(
            "temperature_celsius", "Room temperature", 22.5,
            {logit::make_prometheus_label("room", "server_room_a")});
        assert(mf.type == logit::PrometheusMetricType::Gauge);
        assert(mf.samples[0].labels[0].value == "server_room_a");
    }

    // Test 8: make_prometheus_untyped (unlabeled)
    {
        auto mf = logit::make_prometheus_untyped("raw_value", "Some raw value", 99.0);
        assert(mf.name == "raw_value");
        assert(mf.type == logit::PrometheusMetricType::Untyped);
        assert(mf.samples[0].value == 99.0);
    }

    // Test 9: add_prometheus_counter via vector
    {
        std::vector<logit::PrometheusMetricFamily> families;
        logit::add_prometheus_counter(families, "errors_total", "Total errors", 5.0);
        assert(families.size() == 1);
        assert(families[0].name == "errors_total");
        assert(families[0].type == logit::PrometheusMetricType::Counter);

        logit::add_prometheus_gauge(
            families, "active_sessions", "Active sessions", 12.0,
            {logit::make_prometheus_label("region", "eu-west")});
        assert(families.size() == 2);
        assert(families[1].name == "active_sessions");
        assert(families[1].samples[0].labels[0].value == "eu-west");
    }

    // Test 10: add_prometheus_untyped via vector
    {
        std::vector<logit::PrometheusMetricFamily> families;
        logit::add_prometheus_untyped(families, "misc", "Misc value", 0.0);
        assert(families.size() == 1);
        assert(families[0].type == logit::PrometheusMetricType::Untyped);
    }

    return 0;
}

#else

int main() {
    return 0;
}

#endif
