#include <logit.hpp>

#ifdef LOGIT_WITH_PROMETHEUS_SERVER
#include <logit/loggers/prometheus/PrometheusRegistry.hpp>
#endif

#include <chrono>
#include <atomic>
#include <thread>
#include <vector>

int main() {
#ifndef LOGIT_WITH_PROMETHEUS_SERVER
    LOGIT_ADD_CONSOLE_DEFAULT();
    LOGIT_WARN("Prometheus server example requires LOGIT_WITH_PROMETHEUS_SERVER=ON");
    LOGIT_WAIT();
    return 0;
#else
    const auto started_at = std::chrono::steady_clock::now();
    std::atomic<unsigned long long> jobs_processed{0};
    std::atomic<int> queue_depth{0};

    logit::PrometheusRegistry registry("myapp_");
    registry.set_gauge(
        "uptime_seconds",
        "Application uptime in seconds",
        [started_at]() {
            return std::chrono::duration<double>(
                std::chrono::steady_clock::now() - started_at).count();
        });
    registry.set_gauge(
        "queue_depth",
        "Current application queue depth",
        [&queue_depth]() { return static_cast<double>(queue_depth.load()); },
        {{"queue", "orders"}});
    registry.set_counter(
        "jobs_processed_total",
        "Total processed jobs",
        [&jobs_processed]() { return static_cast<double>(jobs_processed.load()); });

    logit::PrometheusHttpServerLogger::Config config;
    config.port = 9090;
    config.path = "/metrics";
    config.format.metric_prefix = "myapp_";
    config.format.include_build_info = true;

    config.on_collect = [&registry](std::vector<logit::PrometheusMetricFamily>& families) {
        registry.collect(families);
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
        queue_depth.store(5 - i);
        jobs_processed.fetch_add(1);
        LOGIT_INFO("Logging iteration %d", (i + 1));
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    LOGIT_SHUTDOWN();
    return 0;
#endif
}
