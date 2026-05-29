#include <logit.hpp>

#include <cstdlib>
#include <string>

namespace {

std::string env_or(const char* name, const char* fallback) {
    const char* value = std::getenv(name);
    return (value && *value) ? std::string(value) : std::string(fallback);
}

} // namespace

int main() {
#ifndef LOGIT_WITH_OTLP
    LOGIT_ADD_CONSOLE_DEFAULT();
    LOGIT_WARN("OTLP example requires LOGIT_WITH_OTLP=ON");
    LOGIT_WAIT();
    return 0;
#else
    logit::OtlpHttpLogger::Config config;
    config.host = env_or("LOGIT_OTLP_ENDPOINT", "http://localhost:4318");
    config.path = env_or("LOGIT_OTLP_PATH", "/v1/logs");
    config.format.service_name = env_or("LOGIT_SERVICE_NAME", "checkout-service");
    config.format.service_namespace = "examples";
    config.format.service_instance_id = "local-dev";
    config.format.deployment_environment = env_or("LOGIT_ENVIRONMENT", "dev");
    config.max_queue_size = 1024;
    config.max_batch_size = 64;
    config.max_in_flight_requests = 2;
    config.export_interval_ms = 250;
    config.request_timeout_sec = 3;
    config.retry_attempts = 1;
    config.retry_delay_ms = 100;
    config.cancel_on_shutdown = false;

#if defined(LOGIT_HAS_ZSTD)
    config.compression = logit::OtlpCompression::Zstd;
    config.compression_level = 3;
#elif defined(LOGIT_HAS_ZLIB)
    config.compression = logit::OtlpCompression::Gzip;
    config.compression_level = 6;
#endif

    LOGIT_ADD_LOGGER(
        logit::OtlpHttpLogger,
        (config),
        logit::SimpleLogFormatter,
#ifdef LOGIT_WITH_CONTEXT
        ("[%l] trace=%K{trace_id} span=%K{span_id} %v")
#else
        ("[%l] %v")
#endif
    );

#ifdef LOGIT_WITH_CONTEXT
    LOGIT_MDC_PUT("trace_id", "7b3f1c8a2e914a99");
    LOGIT_MDC_PUT("span_id", "checkout-001");
    LOGIT_NDC_PUSH("checkout");
#endif

    LOGIT_INFO("OTLP logger started");
    LOGIT_WARN("payment provider latency is above threshold");

    {
#ifdef LOGIT_WITH_CONTEXT
        LOGIT_NDC_GUARD("submit-order");
#endif
        LOGIT_ERROR("order export failed; collector will count failed exports if HTTP fails");
    }

    LOGIT_WAIT();
#ifdef LOGIT_WITH_CONTEXT
    LOGIT_MDC_CLEAR();
    LOGIT_NDC_CLEAR();
#endif
    LOGIT_SHUTDOWN();
    return 0;
#endif
}
