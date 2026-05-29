#include <logit.hpp>

#ifdef LOGIT_WITH_PROMETHEUS
#include <logit/loggers/prometheus/PrometheusRegistry.hpp>
#endif

#include <iostream>
#include <string>
#include <vector>

int main() {
#ifndef LOGIT_WITH_PROMETHEUS
    LOGIT_ADD_CONSOLE_DEFAULT();
    LOGIT_WARN("Prometheus payload example requires LOGIT_WITH_PROMETHEUS=ON");
    LOGIT_WAIT();
    return 0;
#else
    int queue_depth = 3;
    unsigned long long jobs_processed = 41;

    logit::PrometheusRegistry registry("myapp_");
    registry.set_gauge(
        "queue_depth",
        "Current application queue depth",
        [&queue_depth]() { return static_cast<double>(queue_depth); },
        {{"queue", "orders"}});
    registry.set_counter(
        "jobs_processed_total",
        "Total processed jobs",
        [&jobs_processed]() { return static_cast<double>(jobs_processed); });

    logit::PrometheusPayloadLogger::Config config;
    config.format.metric_prefix = "myapp_";
    config.format.include_build_info = true;
    config.emit_on_wait = true;
    config.on_collect = [&registry](std::vector<logit::PrometheusMetricFamily>& families) {
        registry.collect(families);
    };
    config.on_payload = [](std::string payload) {
        std::cout << payload << std::endl;
    };

    LOGIT_ADD_LOGGER(
        logit::PrometheusPayloadLogger,
        (config),
        logit::SimpleLogFormatter,
        ("%v")
    );

    LOGIT_INFO("Prometheus payload logger started");
    LOGIT_WARN("Example warning message");
    LOGIT_ERROR("Example error message");
    queue_depth = 1;
    ++jobs_processed;

    LOGIT_WAIT();
    LOGIT_SHUTDOWN();
    return 0;
#endif
}
