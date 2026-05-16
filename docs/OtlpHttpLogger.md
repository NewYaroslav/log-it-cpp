# OTLP/HTTP logger

`OtlpHttpLogger` is an optional LogIt++ backend that exports log records to an OpenTelemetry-compatible OTLP/HTTP endpoint.

The backend is disabled by default and requires the optional [`kurlyk`](https://github.com/NewYaroslav/kurlyk) dependency.

## CMake

```bash
cmake -S . -B build \
  -DLOGIT_WITH_OTLP=ON \
  -DLOGIT_USE_SUBMODULES=ON
```

When `kurlyk` is placed at `external/kurlyk`, LogIt++ adds it only when `LOGIT_WITH_OTLP=ON`.

For Windows MinGW builds, the CMake integration enables kurlyk fallback options for curl, OpenSSL, and Asio when `LOGIT_USE_SUBMODULES=ON`. This keeps OTLP optional while still allowing a ready-made MinGW dependency path through kurlyk.

## Usage

```cpp
#include <logit.hpp>

int main() {
    logit::OtlpHttpLoggerConfig config;
    config.host = "http://localhost:4318";
    config.path = "/v1/logs";
    config.service_name = "trade-bot";
    config.deployment_environment = "dev";

    LOGIT_ADD_LOGGER(
        logit::OtlpHttpLogger,
        (config),
        logit::SimpleLogFormatter,
        ("%v")
    );

    LOGIT_INFO("bot started");
    LOGIT_WARN("spread too high");

    LOGIT_WAIT();
}
```

## Export model

The backend sends OTLP/HTTP JSON requests:

```text
LogIt++ -> OtlpHttpLogger -> kurlyk::HttpClient -> OpenTelemetry Collector / Loki
```

`LogRecord` fields are mapped to OTLP attributes:

| LogIt++ field | OTLP field |
| --- | --- |
| `timestamp_ms` | `timeUnixNano` |
| `log_level` | `severityText`, `severityNumber` |
| formatted message | `body.stringValue` |
| `file`, `line`, `function` | `code.file.path`, `code.line.number`, `code.function.name` |
| `thread_id` | `thread.id` |
| `format` | `logit.format` |
| `arg_names` | `logit.arg_names` |

Resource attributes are configured through `OtlpHttpLoggerConfig`, including `service.name`, `service.namespace`, `service.instance.id`, and `deployment.environment.name`.

## Notes

- `OtlpHttpLogger` is an outbound client/exporter, not a server.
- The first implementation uses OTLP/HTTP JSON. Protobuf can be added later as a second encoding mode.
- The backend uses its own queue and worker thread, so logging calls do not synchronously perform HTTP requests when `async=true`.
- Export errors are counted internally and are not logged through LogIt++ to avoid recursive logging loops.
