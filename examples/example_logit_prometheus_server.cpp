#include <logit.hpp>

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
        logit::PrometheusMetricFamily mf;
        mf.name = "myapp_uptime_seconds";
        mf.help = "Application uptime in seconds";
        mf.type = logit::PrometheusMetricType::Gauge;
        logit::PrometheusSample s;
        s.name = "myapp_uptime_seconds";
        s.value = 42.0;
        mf.samples.push_back(s);
        families.push_back(mf);
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
