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

The `metric_prefix` config option (default: `logit_`) is applied to all metric names.

## CMake Options

```cmake
option(LOGIT_WITH_PROMETHEUS "Enable Prometheus text payload support" OFF)
option(LOGIT_WITH_PROMETHEUS_SERVER "Enable Prometheus HTTP server backend" OFF)
```

`LOGIT_WITH_PROMETHEUS_SERVER` implies `LOGIT_WITH_PROMETHEUS` and requires C++17
(Simple-Web-Server dependency).

## Usage: PrometheusPayloadLogger

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

Use the `on_collect` callback to add application-specific metrics on each scrape:

```cpp
config.on_collect = [](std::vector<logit::PrometheusMetricFamily>& families) {
    logit::PrometheusMetricFamily mf;
    mf.name = "myapp_queue_size";
    mf.help = "Current queue depth";
    mf.type = logit::PrometheusMetricType::Gauge;
    logit::PrometheusSample s;
    s.name = "myapp_queue_size";
    s.value = get_queue_depth();
    mf.samples.push_back(s);
    families.push_back(mf);
};
```

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
