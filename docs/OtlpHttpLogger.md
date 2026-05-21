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
    logit::OtlpHttpLogger::Config config;
    config.host = "http://localhost:4318";
    config.path = "/v1/logs";
    config.format.service_name = "trade-bot";
    config.format.deployment_environment = "dev";

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
| `arg_names` | `logit.arg_names` (legacy, deprecated) |
| `args_array` elements | typed attributes under `logit.arg.*` prefix |

## Structured typed attributes

When `include_args = true` (the default), each element of `args_array` is emitted as a separate OTLP attribute with a typed value. The key is built from `args_prefix` + the sanitized argument name.

### Configuration flags

| Flag | Default | Description |
| --- | --- | --- |
| `include_args` | `true` | Emit structured typed arg attributes. |
| `include_arg_names` | `false` | Emit legacy `logit.arg_names` string attribute. |
| `args_prefix` | `"logit.arg."` | Key prefix for structured arg attributes. |

### Type mapping

| VariableValue type | OTLP AnyValue field |
| --- | --- |
| `BOOL_VAL` | `boolValue` |
| `INT8_VAL`..`INT64_VAL` | `intValue` |
| `UINT8_VAL`..`UINT32_VAL` | `intValue` |
| `UINT64_VAL` | `intValue` if <= INT64_MAX, else `stringValue` |
| `FLOAT_VAL`, `DOUBLE_VAL`, `LONG_DOUBLE_VAL` | `doubleValue` if finite, else `stringValue` |
| All other types | `stringValue` (via `to_string()`) |

### Name sanitization and deduplication

Argument names are sanitized: characters that are not alphanumeric, `_`, `.`, or `-` are replaced with `_`. If the sanitized name is empty or consists only of underscores, a positional key (`args_prefix` + index) is used instead.

Duplicate keys are resolved by appending `.N` (starting at 1 for the second occurrence). For example, two args both named `px` produce keys `logit.arg.px` and `logit.arg.px.1`.

### Reserved prefix

The default prefix `logit.arg.` is reserved. Changing `args_prefix` is supported but may cause attribute collisions with other OTLP receivers.

### Cardinality warning

Avoid putting unique values (timestamps, request IDs, UUIDs) into arg attributes. High-cardinality attributes increase memory and storage costs in OTLP collectors and backends (Loki, Tempo, etc.). Use structured attributes for low-cardinality dimensions like `symbol`, `side`, or `region`.

### Deprecation notice

The `logit.arg_names` OTLP attribute (controlled by `include_arg_names`) is deprecated. It emits argument names as a single comma-separated string with no type information. Prefer `include_args = true` for typed, queryable attributes.

Resource attributes are configured through `OtlpHttpLogger::Config.format`, including `service.name`, `service.namespace`, `service.instance.id`, and `deployment.environment.name`.

## Diagnostics

`OtlpHttpLogger` exposes its internal counters through the generic logger parameter API:

```cpp
const auto dropped = LOGIT_GET_INT_PARAM(otlp_index, logit::LoggerParam::DroppedLogCount);
const auto failed = LOGIT_GET_INT_PARAM(otlp_index, logit::LoggerParam::FailedExportCount);
```

Available diagnostic params:

| Parameter | Meaning |
| --- | --- |
| `LoggerParam::DroppedLogCount` | Number of records dropped because the backend queue overflowed or the logger was stopping. |
| `LoggerParam::FailedExportCount` | Number of failed OTLP export attempts. |

These values are also available through `get_string_param()` and `get_float_param()`. Integer retrieval saturates at `int64_t` max if the underlying unsigned counter ever exceeds that range.

## Notes

- `OtlpHttpLogger` is an outbound client/exporter, not a server.
- The first implementation uses OTLP/HTTP JSON. Protobuf can be added later as a second encoding mode.
- The backend uses its own queue and worker thread, so logging calls do not synchronously perform HTTP requests when `async=true`.
- Export errors are counted internally and are not logged through LogIt++ to avoid recursive logging loops.
