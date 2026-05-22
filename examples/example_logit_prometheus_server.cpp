#include <logit.hpp>

#ifdef LOGIT_WITH_PROMETHEUS_SERVER
#include <logit/loggers/prometheus/PrometheusMetricBuilders.hpp>
#endif

int main() {
#ifndef LOGIT_WITH_PROMETHEUS_SERVER
    LOGIT_ADD_CONSOLE_DEFAULT();
    LOGIT_WARN("Prometheus server example requires LOGIT_WITH_PROMETHEUS_SERVER=ON");
    LOGIT_WAIT();
    return 0;
#else
    logit::PrometheusHttpServerLogger::Config config;
    config.port = 9090;
    config.path = "/metrics";
    config.format.metric_prefix = "myapp_";
    config.format.include_build_info = true;

    // Optional: add custom metrics on each scrape
    config.on_collect = [](std::vector<logit::PrometheusMetricFamily>& families) {
        logit::add_prometheus_gauge(
            families,
            "myapp_uptime_seconds",
            "Application uptime in seconds",
            42.0);
    };

    LOGIT_ADD_LOGGER(
        logit::PrometheusHttpServerLogger,
        (config),
        logit::SimpleLogFormatter,
        ("%v")
    );

    LOGIT_INFO("Prometheus HTTP server started on port 9090");
    LOGIT_WARN("Scrape metrics at http://localhost:9090/metrics");

    for (int i = 0; i < 5; ++i) {
        LOGIT_INFO("Logging iteration %d", (i + 1));
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    LOGIT_SHUTDOWN();
    return 0;
#endif
}
