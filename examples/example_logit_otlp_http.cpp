#include <logit.hpp>

int main() {
#ifndef LOGIT_WITH_OTLP
    LOGIT_ADD_CONSOLE_DEFAULT();
    LOGIT_WARN("OTLP example requires LOGIT_WITH_OTLP=ON");
    LOGIT_WAIT();
    return 0;
#else
    logit::OtlpHttpLogger::Config config;
    config.host = "http://localhost:4318";
    config.path = "/v1/logs";
    config.format.service_name = "logit-otlp-example";
    config.format.deployment_environment = "dev";
    config.max_batch_size = 32;
    config.export_interval_ms = 500;

    LOGIT_ADD_LOGGER(
        logit::OtlpHttpLogger,
        (config),
        logit::SimpleLogFormatter,
        ("%v")
    );

    LOGIT_INFO("OTLP logger started");
    LOGIT_WARN("Example warning message");
    LOGIT_ERROR("Example error message");

    LOGIT_WAIT();
    return 0;
#endif
}
