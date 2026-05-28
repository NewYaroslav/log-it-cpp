#include <logit/loggers/prometheus/PrometheusRegistry.hpp>
#include <logit/loggers/prometheus/PrometheusTextSerializer.hpp>

#ifdef LOGIT_WITH_PROMETHEUS

#include <cassert>
#include <stdexcept>
#include <string>
#include <vector>

int main() {
    // Test 1: gauge callback is collected with the current value
    {
        int queue_size = 3;
        logit::PrometheusRegistry registry;
        registry.set_gauge(
            "app_queue_size",
            "Current application queue size",
            [&queue_size]() { return static_cast<double>(queue_size); });

        std::vector<logit::PrometheusMetricFamily> families;
        registry.collect(families);

        assert(families.size() == 1);
        assert(families[0].name == "app_queue_size");
        assert(families[0].type == logit::PrometheusMetricType::Gauge);
        assert(families[0].samples.size() == 1);
        assert(families[0].samples[0].value == 3.0);

        queue_size = 7;
        families.clear();
        registry.collect(families);
        assert(families[0].samples[0].value == 7.0);
    }

    // Test 2: counter callback and labels are preserved
    {
        logit::PrometheusRegistry registry;
        registry.set_counter(
            "requests_total",
            "Total requests",
            []() { return 12.0; },
            {{"method", "GET"}});

        std::vector<logit::PrometheusMetricFamily> families;
        registry.collect(families);

        assert(families.size() == 1);
        assert(families[0].type == logit::PrometheusMetricType::Counter);
        assert(families[0].samples[0].labels.size() == 1);
        assert(families[0].samples[0].labels[0].name == "method");
        assert(families[0].samples[0].labels[0].value == "GET");
    }

    // Test 3: same raw name and labels replace the existing series
    {
        logit::PrometheusRegistry registry;
        registry.set_gauge("worker_load", "Worker load", []() { return 1.0; }, {{"worker", "a"}});
        registry.set_gauge("worker_load", "Worker load", []() { return 2.0; }, {{"worker", "a"}});

        std::vector<logit::PrometheusMetricFamily> families;
        registry.collect(families);

        assert(families.size() == 1);
        assert(families[0].samples.size() == 1);
        assert(families[0].samples[0].value == 2.0);
    }

    // Test 4: same raw name with different labels is grouped into one family
    {
        logit::PrometheusRegistry registry;
        registry.set_gauge("worker_load", "Worker load", []() { return 1.0; }, {{"worker", "a"}});
        registry.set_gauge("worker_load", "Worker load", []() { return 2.0; }, {{"worker", "b"}});

        std::vector<logit::PrometheusMetricFamily> families;
        registry.collect(families);

        assert(families.size() == 1);
        assert(families[0].samples.size() == 2);
        assert(families[0].samples[0].labels[0].value == "a");
        assert(families[0].samples[1].labels[0].value == "b");
    }

    // Test 5: registry prefix is independent from serializer config
    {
        logit::PrometheusRegistry registry("myapp_");
        registry.set_metric_prefix("myapp_");
        assert(registry.metric_prefix() == "myapp_");

        registry.set_gauge(
            "queue_size",
            "Queue size",
            []() { return 5.0; },
            {{"component", "api"}});

        std::vector<logit::PrometheusMetricFamily> families;
        registry.collect(families);

        assert(families.size() == 1);
        assert(families[0].name == "myapp_queue_size");
        assert(families[0].samples[0].name == "myapp_queue_size");

        logit::PrometheusTextFormatConfig config;
        config.metric_prefix = "logit_";
        std::string payload = logit::build_prometheus_text_payload(families, config);

        assert(payload.find("# HELP myapp_queue_size Queue size") != std::string::npos);
        assert(payload.find("# TYPE myapp_queue_size gauge") != std::string::npos);
        assert(payload.find("myapp_queue_size{component=\"api\"} 5") != std::string::npos);
        assert(payload.find("logit_myapp_queue_size") == std::string::npos);
    }

    // Test 6: value callback exceptions propagate to caller
    {
        logit::PrometheusRegistry registry;
        registry.set_gauge("broken_metric", "Broken metric", []() -> double {
            throw std::runtime_error("broken metric");
        });

        std::vector<logit::PrometheusMetricFamily> families;
        bool caught = false;
        try {
            registry.collect(families);
        } catch (const std::runtime_error&) {
            caught = true;
        }

        assert(caught);
    }

    return 0;
}

#else

int main() {
    return 0;
}

#endif
