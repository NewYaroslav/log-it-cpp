# Logging Macro Guidelines

Use this file when a task touches how application code emits logs.

## Default rule

`log-it-cpp` is macro-first for normal application logging.

For ordinary logging in examples, tests, applications, and integration code:

- include `<logit.hpp>` and use the public logging macros;
- choose the macro family that matches the desired behavior;
- keep low-level logger internals out of normal call sites.

## Prefer public macro families

Use the public macro layer instead of hand-assembling lower-level structures:

- `LOGIT_<LEVEL>(...)` for regular argument-aware logging;
- `LOGIT_PRINT_<LEVEL>(...)`, `LOGIT_PRINTF_<LEVEL>(...)`,
  `LOGIT_FORMAT_<LEVEL>(...)`, and `LOGIT_FMT_<LEVEL>(...)` for the matching
  formatting style;
- `LOGIT_STREAM_<LEVEL>()` for stream-style logging;
- `LOGIT_*_TO(index, ...)` and `LOGIT_STREAM_*_TO(index)` when targeting a
  specific logger or a single-mode backend;
- `LOGIT_*_IF(...)`, `LOGIT_*_ONCE(...)`, `LOGIT_*_EVERY_N(...)`,
  `LOGIT_*_THROTTLE(...)`, and `LOGIT_*_TAG(...)` for conditional or filtered
  logging;
- `LOGIT_SCOPE_<LEVEL>(...)`, `LOGIT_SCOPE_PRINTF_<LEVEL>(...)`, and
  `LOGIT_SCOPE_FMT_<LEVEL>(...)` for scope-duration logging;
- `LOGIT_ADD_*`, `LOGIT_SET_*`, `LOGIT_GET_*`, `LOGIT_IS_*`, `LOGIT_WAIT()`,
  and `LOGIT_SHUTDOWN()` for setup and control.

## Avoid in normal logging code

Do not use these lower-level paths just to emit an ordinary log message:

- manually constructing `logit::LogRecord`;
- filling `record.args_array` or splitting argument names manually;
- calling `logit::Logger::get_instance().log(...)`,
  `print(...)`, or `log_and_return(...)` directly;
- building your own logger snapshot or dispatch path around the singleton.

These patterns bypass the intended public API and make code harder to maintain.

## Allowed low-level exceptions

Direct low-level usage is acceptable when the task is explicitly about:

- implementing a custom `ILogger` or `ILogFormatter`;
- registering custom backends or formatters as extension code;
- tests that intentionally validate low-level contracts or include behavior;
- benchmark and adapter code that intentionally works below the macro layer;
- internal library maintenance inside `Logger.hpp`, `log_macros.hpp`, or
  related implementation files.

Even in extension code, prefer `LOGIT_ADD_LOGGER(...)` over raw
`logit::Logger::get_instance().add_logger(...)` when the macro already matches
the needed behavior.

## Repo evidence

The current repository already demonstrates the intended pattern:

- `examples/example_logit_basic.cpp` uses public macros as the default logging
  path;
- `examples/example_logit_custom_backend.cpp` shows the extension path via
  `LOGIT_ADD_LOGGER(...)`;
- `tests/compiled_level_test.cpp` exercises the public macro matrix directly;
- `include/logit_cpp/logit/Logger.hpp` contains the low-level internals that
  the macro layer is meant to shield from normal call sites.
