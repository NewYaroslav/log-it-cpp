#include <logit.hpp>

int main() {
#ifndef LOGIT_WITH_PROMETHEUS
    LOGIT_ADD_CONSOLE_DEFAULT();
    LOGIT_WARN("Prometheus payload example requires LOGIT_WITH_PROMETHEUS=ON");
    LOGIT_WAIT();
    return 0;
#else
    logit::PrometheusPayloadLogger::Config config;
    config.format.metric_prefix = "myapp_";
    config.format.include_build_info = true;
    config.emit_on_wait = true;
    config.on_payload = [](std::string payload) {
        // In a real application, send payload to your Prometheus push gateway
        // or expose it via your own HTTP endpoint.
        (void)payload;
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

    LOGIT_WAIT();
    LOGIT_SHUTDOWN();
    return 0;
#endif
}
