# Prometheus Logger

## Overview

LogIt++ provides two Prometheus backends for exposing internal log metrics in the
[Prometheus text exposition format](https://prometheus.io/docs/instrumenting/exposition_formats/):

- **PrometheusPayloadLogger** -- callback-based; delivers the serialized payload to a
  user-provided function. Useful when you have your own HTTP server or want to push to
  a Prometheus Pushgateway.

- **PrometheusHttpServerLogger** -- embedded HTTP server; serves `/metrics` on a
  configurable port using Simple-Web-Server. Ideal for simple services without a
  separate metrics endpoint.

## Built-in Metrics

| Metric | Type | Description |
|--------|------|-------------|
| `logit_log_records_total` | counter | Total log records processed |
| `logit_dropped_logs_total` | counter | Dropped log records |
| `logit_failed_exports_total` | counter | Failed export/callback attempts |
| `logit_last_log_timestamp_ms` | gauge | Timestamp of last log (ms) |
| `logit_time_since_last_log_ms` | gauge | Time since last log (ms) |
| `logit_build_info` | gauge | Build info (value=1, labels: version, compiler) |

The `metric_prefix` config option (default: `logit_`) is applied to built-in logger metric names.

`PrometheusHttpServerLogger` also exposes scrape diagnostics:

| Metric | Type | Description |
|--------|------|-------------|
| `logit_prometheus_scrapes_total` | counter | Total `/metrics` scrape requests |
| `logit_prometheus_scrape_errors_total` | counter | Failed scrape requests |
| `logit_prometheus_last_scrape_timestamp_ms` | gauge | Timestamp of the last scrape request |
| `logit_prometheus_collect_duration_seconds` | gauge | Duration of the last metrics collection |

## CMake Options

```cmake
option(LOGIT_WITH_PROMETHEUS "Enable Prometheus text payload support" OFF)
option(LOGIT_WITH_PROMETHEUS_SERVER "Enable Prometheus HTTP server backend" OFF)
```

`LOGIT_WITH_PROMETHEUS_SERVER` implies `LOGIT_WITH_PROMETHEUS` and requires C++17
(Simple-Web-Server dependency).

## Usage: PrometheusPayloadLogger

For a runnable callback example with custom application metrics, see
`examples/example_logit_prometheus_payload.cpp`.

```cpp
#include <logit.hpp>

logit::PrometheusPayloadLogger::Config config;
config.format.metric_prefix = "myapp_";
config.emit_on_wait = true;
config.on_payload = [](std::string payload) {
    // Send to your HTTP endpoint or Pushgateway
};

LOGIT_ADD_LOGGER(
    logit::PrometheusPayloadLogger,
    (config),
    logit::SimpleLogFormatter,
    ("%v")
);

LOGIT_INFO("Application started");
LOGIT_WAIT(); // triggers on_payload with current metrics
```

## Usage: PrometheusHttpServerLogger

For a runnable embedded `/metrics` server example, see
`examples/example_logit_prometheus_server.cpp`.

```cpp
#include <logit.hpp>

logit::PrometheusHttpServerLogger::Config config;
config.port = 9090;
config.path = "/metrics";

LOGIT_ADD_LOGGER(
    logit::PrometheusHttpServerLogger,
    (config),
    logit::SimpleLogFormatter,
    ("%v")
);

LOGIT_INFO("Server started");
// Scrape http://localhost:9090/metrics
```

## Custom Metrics

Use `PrometheusRegistry` with the `on_collect` callback to register
application-specific metrics once and collect them on each scrape:

```cpp
#include <logit/loggers/prometheus/PrometheusRegistry.hpp>

logit::PrometheusRegistry registry("myapp_");

registry.set_gauge(
    "queue_size",
    "Current queue depth",
    []() { return get_queue_depth(); });

config.on_collect = [&registry](std::vector<logit::PrometheusMetricFamily>& families) {
    registry.collect(families);
};
```

`PrometheusTextFormatConfig::metric_prefix` applies only to LogIt++ built-in
metrics. Custom metric names are written as supplied by the registry or manual
builders, so use the registry prefix for application metric namespaces.

If a registry value callback throws, the registry skips that sample, keeps
collecting later metrics, appends the healthy samples, and then rethrows the
first exception. The Prometheus loggers catch that exception from `on_collect`
and increment their failed export counter.

For low-level control, `on_collect` can still append `PrometheusMetricFamily`
objects directly or use helpers such as `add_prometheus_gauge()`.

## Prometheus Scrape Config

```yaml
scrape_configs:
  - job_name: 'logit-app'
    scrape_interval: 15s
    static_configs:
      - targets: ['localhost:9090']
    metrics_path: /metrics
```

## Limitations

- Text exposition format only (no protobuf, no OpenMetrics `# EOF`).
- No histograms or summaries -- use `on_collect` for custom metric types.
- No TLS or authentication on the HTTP server.
- No metric renaming conflicts resolution -- user must ensure unique names.
