# Codebase Orientation

Use this file when you need a fast, project-specific mental model before
changing `log-it-cpp`. It complements the narrower playbooks in `guides/` and
does not replace them.

## Quick Model

`log-it-cpp` is a header-only C++ logging library. Its public surface is a
macro-first facade (`include/logit_cpp/logit/log_macros.hpp`) backed by a
singleton dispatcher (`include/logit_cpp/logit/Logger.hpp`), formatter
strategies (`include/logit_cpp/logit/formatter/`), logger backends
(`include/logit_cpp/logit/loggers/`), utility DTOs/helpers
(`include/logit_cpp/logit/utils/`), and private execution internals
(`include/logit_cpp/logit/detail/`).

The project is not a DDD application. There are no HTTP/WebSocket endpoints,
database layers, event buses, repositories, or product domains. Treat the
"domains" below as library subsystems.

## Public Entry Points

Prefer these umbrella headers instead of recreating include order manually:

| Header | Purpose |
| --- | --- |
| `include/logit_cpp/logit.hpp` | Full public entry point: config, enums, utilities, formatters, loggers, `Logger`, and macros. |
| `include/logit_cpp/logit/utils.hpp` | Public utilities module and DTOs such as `LogRecord`, `BufferedLogEntry`, `LogFileInfo`, and `VariableValue`. |
| `include/logit_cpp/logit/formatter.hpp` | Formatter interfaces, `SimpleLogFormatter`, and pattern compiler support. |
| `include/logit_cpp/logit/loggers.hpp` | Logger backend umbrella plus required shared internals. |

Installed consumers include from `include/logit_cpp`, so examples use
`#include <logit.hpp>` or `#include <logit/loggers.hpp>`.

Leaf headers follow the aggregate-first NHR rule. If direct leaf access is
needed, include the nearest umbrella first:

```cpp
#include <logit/loggers.hpp>
#include <logit/loggers/MemoryLogger.hpp>
```

See `guides/header-impl.md` before changing include structure.

## Subsystems

| Subsystem | Main files | Responsibility |
| --- | --- | --- |
| Macro facade | `log_macros.hpp` | Public logging, setup, query, queue, and short-name macros. Normal application code should stay here. |
| Dispatcher | `Logger.hpp` | Owns logger/formatter strategies, filtering, single-mode routing, targeted logging, snapshots, shutdown. |
| Logger interfaces | `loggers/ILogger.hpp` | Backend contract for sinks, parameters, buffered snapshots, persisted file access, and flush/wait. |
| Logger backends | `ConsoleLogger.hpp`, `FileLogger.hpp`, `UniqueFileLogger.hpp`, `MemoryLogger.hpp`, `SyslogLogger.hpp`, `EventLogLogger.hpp`, `CrashLogger.hpp` | Concrete sinks. Platform-specific backends provide stubs or aliases where unsupported. |
| Formatter interfaces | `formatter/ILogFormatter.hpp`, `formatter/SimpleLogFormatter.hpp` | Strategy interface and default pattern/JSON formatter. |
| Pattern compiler | `formatter/compiler/PatternCompiler.hpp` | Parses formatting patterns into `FormatInstruction` objects used by `SimpleLogFormatter`. |
| Utilities and DTOs | `utils/*.hpp`, `enums.hpp`, `config.hpp` | Public data structures, value formatting, argument parsing, paths, tags, encoding, config macros. |
| Async internals | `detail/TaskExecutor.hpp`, `detail/MpscRingAny.hpp` | Shared task queue, backpressure, MPSC variant, Emscripten variant. |
| File compression internals | `detail/CompressionWorker.hpp` | Optional rotated-file compression using zlib, zstd, or external commands. |
| Tests | `tests/` | Runtime behavior, include contracts, optional features, ODR checks, Emscripten smoke tests. |
| Examples | `examples/` | User-facing usage patterns for macros, memory/file/custom/system loggers. |
| Benchmarks | `bench/` | Adapter-based latency benchmark harness. Not normal library API. |

## Layer Rules

The practical dependency direction is:

1. `config.hpp` and `enums.hpp` provide shared public constants and enums.
2. `utils.hpp` aggregates utility DTOs/helpers.
3. `formatter.hpp` depends on utilities and exposes formatter strategies.
4. `loggers.hpp` depends on utilities and detail helpers, then exposes
   backend implementations.
5. `Logger.hpp` combines `ILogger` and `ILogFormatter` strategies.
6. `log_macros.hpp` is the macro facade over `Logger` and `TaskExecutor`.

Avoid these dependency shapes:

- Public leaf headers directly including sibling modules when the nearest
  umbrella exists.
- `logit/detail/*` including public `logit/...` headers.
- Consumer examples or application-style tests depending on `logit/detail/*`.
- New code that requires users to include files in a fragile custom order.
- Vendor or generated paths leaking into public headers.

Namespaces:

- Public API lives in `namespace logit`.
- Private implementation details use `namespace logit::detail`, written in the
  existing C++11-compatible style `namespace logit { namespace detail { ... } }`.
- Benchmarks use `namespace logit_bench`.

## Design Patterns In Use

- **Facade**: `log_macros.hpp` hides low-level `Logger` and `LogRecord`
  construction from normal users.
- **Singleton**: `Logger::get_instance()` and `detail::TaskExecutor::get_instance()`
  are process-wide coordinators. Both intentionally use static pointer
  singletons to survive shutdown/static-destruction ordering.
- **Strategy**: `Logger` stores an `ILogger` sink and an `ILogFormatter`
  formatter per strategy.
- **Interface-based extension**: custom sinks implement `ILogger`; custom
  formatters implement `ILogFormatter`.
- **Adapter**: `bench/adapters/*` adapts LogIt and spdlog to the benchmark
  harness. Do not copy benchmark shortcuts into public API without review.
- **Task executor**: async loggers enqueue lambdas into `detail::TaskExecutor`.
  Queue semantics are documented in `docs/TaskExecutor.md`.
- **Compiler/interpreter**: `PatternCompiler` turns pattern strings into
  `FormatInstruction` objects that are applied during formatting.
- **DTOs**: public snapshot/file records are small structs in `utils/`.

Preferred for new code:

- Add extension points through `ILogger`, `ILogFormatter`, public DTOs, and
  macros rather than exposing internal `LoggerStrategy` state.
- Prefer macro-level examples for user documentation.
- Keep async behavior behind `TaskExecutor` unless the backend is explicitly
  synchronous, like `MemoryLogger`.

Be cautious copying:

- Manual `LogRecord` construction in `bench/adapters/LogItAdapter.cpp`; it is a
  benchmark-only low-level adapter.
- Platform stubs in `FileLogger`, `UniqueFileLogger`, `SyslogLogger`, and
  `EventLogLogger`; those exist to keep unsupported targets buildable.
- C++17-only conveniences unless a C++11 fallback is also maintained.

## Style Notes

The canonical style rules live in `guides/cpp_style.md`.
Important project-specific points:

- Classes, structs, and enum types use `CamelCase`; methods use `snake_case`.
- Member fields use `m_`.
- Constants and macros use `UPPER_SNAKE_CASE`.
- Public enum values currently use a mix: `LogLevel` has legacy `LOG_LVL_*`,
  `TextColor` and `RotationNaming` use `CamelCase`, and `CompressType` uses
  names such as `GZIP` and `EXTERNAL_CMD`. Preserve existing public names unless
  a task explicitly requires a breaking rename.
- Headers begin with `#pragma once` and an include guard.
- Doxygen comments are English and usually use `/// \brief`.
- Project headers appear before system headers when adding include lists.
- Keep documentation and sources in UTF-8. Use `u8"..."` for non-ASCII C++
  string literals.
- The default standard is C++11 (`CMakeLists.txt`). Guard C++17-only code with
  `#if __cplusplus >= 201703L` as in `VariableValue.hpp`, `Logger.hpp`, and
  `log_macros.hpp`.

Error handling style:

- Logging backends usually avoid throwing through the logging path. File and
  console paths report internal failures to `std::cerr`.
- Unsupported feature APIs return neutral values (`""`, `0`, `0.0`, empty
  vectors, or `LogFileReadResult{ok=false}`) rather than throwing.
- File APIs only expose persisted files owned by the backend; they do not drain
  async queues.

Pointer ownership:

- Logger registration transfers ownership with `std::unique_ptr`.
- `Logger` stores strategies as `std::shared_ptr<LoggerStrategy>` only so log
  dispatch can copy a stable snapshot outside the logger-list lock.
- Avoid adding `std::shared_ptr` ownership to public APIs unless shared lifetime
  is genuinely required.
- C++17 macros use `std::make_unique`; C++11 macro branches use
  `std::unique_ptr<T>(new T(...))`. Keep both branches in sync.

## Utility Inventory

Reuse these helpers instead of writing local equivalents:

| Need | Reuse |
| --- | --- |
| Current timestamps and monotonic time | `LOGIT_CURRENT_TIMESTAMP_MS()`, `LOGIT_MONOTONIC_MS()` from `config.hpp`; TimeShield for conversions. |
| Formatting `printf`-style strings | `logit::format()` in `utils/format.hpp`. |
| Capturing macro arguments | `split_arguments()` and `args_to_array()` in `utils/argument_utils.hpp`; `VariableValue` in `utils/VariableValue.hpp`. |
| Structured memory snapshots | `BufferedLogEntry` and `MemoryLogger`. |
| Persisted log file metadata/results | `LogFileInfo`, `LogFileReadResult`, and `ILogger` file APIs. |
| Path handling for file loggers | `utils/path_utils.hpp` helpers. |
| Tags | `utils/tag_utils.hpp` and `LOGIT_*_TAG` macros. |
| Encoding conversions | `utils/encoding_utils.hpp`. |
| Async work and backpressure | `detail::TaskExecutor` through public macros when possible. |
| Rotation compression | `detail::CompressionWorker` through `FileLogger::Config`. |

There are no JSON, HTTP, WebSocket, database, or serialization utility layers
beyond the simple JSON string formatting inside `SimpleLogFormatter`.

## Extension Recipes

### Add a Public Data Struct

Use this for DTO-style data exposed by logger APIs.

1. Add a PascalCase header in `include/logit_cpp/logit/utils/`, for example
   `LogArchiveInfo.hpp`.
2. Put the struct in `namespace logit`, give fields safe defaults, and document
   it with `/// \brief`.
3. Include it from `include/logit_cpp/logit/utils.hpp`.
4. Add or update an include-contract test like
   `tests/include_log_file_info_nhr_test.cpp`.
5. If the DTO affects public APIs, update `README.md`, `README-RU.md`, and
   `docs/mainpage.dox`.

Compact style example:

```cpp
#pragma once
#ifndef _LOGIT_LOG_ARCHIVE_INFO_HPP_INCLUDED
#define _LOGIT_LOG_ARCHIVE_INFO_HPP_INCLUDED

#include <cstdint>
#include <string>

namespace logit {

    /// \brief Metadata for an archived log artifact.
    struct LogArchiveInfo {
        std::string path;
        int64_t created_ms = 0;
        bool compressed = false;
    };

} // namespace logit

#endif // _LOGIT_LOG_ARCHIVE_INFO_HPP_INCLUDED
```

### Add a Logger Backend

Use this when adding a new sink such as a network, ring-buffer, or platform
backend. Do not add HTTP/WebSocket endpoints unless the user explicitly asks for
that feature; none exist today.

1. Add `include/logit_cpp/logit/loggers/NewLogger.hpp`.
2. Derive from `logit::ILogger` and implement all pure virtual methods:
   `log`, `get_string_param`, `get_int_param`, `get_float_param`,
   `set_log_level`, `get_log_level`, and `wait`.
3. If the backend is async, enqueue work through `detail::TaskExecutor` and make
   the destructor/wait path flush or stop safely.
4. Add it to `include/logit_cpp/logit/loggers.hpp`.
5. Add registration macros in both C++17 and C++11 branches of
   `log_macros.hpp` if it should be user-facing.
6. Add tests under `tests/` and an example under `examples/` if the backend is a
   public feature.

Minimal skeleton:

```cpp
class NewLogger : public logit::ILogger {
public:
    void log(const logit::LogRecord& record, const std::string& message) override {
        if (!record.raw_mode &&
            static_cast<int>(record.log_level) < m_log_level.load(std::memory_order_relaxed)) {
            return;
        }
        // Write message to the sink.
    }

    std::string get_string_param(const logit::LoggerParam&) const override { return std::string(); }
    int64_t get_int_param(const logit::LoggerParam&) const override { return 0; }
    double get_float_param(const logit::LoggerParam&) const override { return 0.0; }
    void set_log_level(logit::LogLevel level) override { m_log_level.store(static_cast<int>(level)); }
    logit::LogLevel get_log_level() const override {
        return static_cast<logit::LogLevel>(m_log_level.load());
    }
    void wait() override {}

private:
    std::atomic<int> m_log_level = ATOMIC_VAR_INIT(static_cast<int>(logit::LogLevel::LOG_LVL_TRACE));
};
```

### Extend Formatting Pattern Handling

Use this when adding a new `%` token to `SimpleLogFormatter`.

1. Add a new `FormatInstruction::FormatType` value in
   `formatter/compiler/PatternCompiler.hpp`.
2. Update parser logic in `PatternCompiler::compile(...)` so the token creates
   that instruction.
3. Update `FormatInstruction::apply(...)` to render the token from
   `LogRecord`, TimeShield date/time data, or another existing source.
4. Add tests that format a record through `SimpleLogFormatter`; avoid testing
   only the parser in isolation.
5. Document the pattern token in README/mainpage if it is public.

## Safety Invariants

- Do not mutate `LogRecord` fields except for its intentional mutable
  `args_array` cache inside `Logger::print()`.
- Preserve macro-first usage. Ordinary examples/tests should not manually build
  `LogRecord` or call low-level `Logger::log()` unless they test internals,
  adapters, or extension contracts.
- Keep `Logger` thread-safety: copy strategy snapshots under `m_loggers_mx`,
  then invoke backends under each strategy's `exec_mx`.
- Backend snapshot APIs (`MemoryLogger`, file APIs) must be safe while `log()`
  may run concurrently.
- Async lambdas capturing `this` require destructor or `wait()` logic that
  drains pending tasks before members disappear.
- `TaskExecutor` semantics differ by build: deque, MPSC, and single-threaded
  Emscripten. Read `docs/TaskExecutor.md` before changing queue behavior.
- `QueuePolicy::DropOldest` intentionally drops incoming tasks in MPSC builds;
  do not "fix" this without updating docs and tests.
- Keep Emscripten support buildable. Unsupported file/system backends use stubs
  or disabled CMake options.
- Optional dependency features must stay behind compile definitions:
  `LOGIT_WITH_FMT`, `LOGIT_HAS_ZLIB`, `LOGIT_HAS_ZSTD`,
  `LOGIT_HAS_SYSLOG`, `LOGIT_HAS_WIN_EVENT_LOG`, and `LOGIT_EMSCRIPTEN`.
- Preserve public include paths and public macro names for backward
  compatibility.
- Do not edit `external/` vendored submodules except for an explicit dependency
  update task.

## Testing Patterns

- Runtime behavior tests live as one executable per `tests/*.cpp`; the test name
  is the file stem from `tests/CMakeLists.txt`.
- Include-contract tests use the NHR pattern, for example
  `include_formatter_nhr_test.cpp` and `include_loggers_nhr_test.cpp`.
- Optional feature tests are conditionally excluded when flags are off:
  `fmt_macros_test.cpp`, gzip/zstd compression tests.
- ODR tests in `tests/odr/` are compiled manually in CI to catch header-only
  singleton and inline issues.
- Emscripten tests are separate under `tests/ems/`.
- Backpressure tests include TSAN labels for queue behavior.

When changing public include structure, add an include-contract test. When
changing async behavior, run the relevant backpressure tests and prefer a full
`ctest --output-on-failure` pass.

## Project Map

| Path | Purpose | Edit when |
| --- | --- | --- |
| `AGENTS.md` | Root agent rules and playbook index. | Agent workflow or repository-wide guidance changes. |
| `guides/` | Detailed agent playbooks. | A recurring agent workflow needs documented project-specific rules. |
| `include/logit_cpp/logit.hpp` | Full umbrella header. | Public entry-point composition changes. |
| `include/logit_cpp/logit/config.hpp` | Config macros and default paths/patterns. | Adding compile-time knobs or default settings. |
| `include/logit_cpp/logit/enums.hpp` | Public enums and enum-to-string helpers. | Adding public enum values or logger parameters. |
| `include/logit_cpp/logit/utils.hpp` | Utilities umbrella. | Adding/removing public utility headers. |
| `include/logit_cpp/logit/utils/` | DTOs, argument/value/path/tag/encoding helpers. | Adding public data structures or reusable helper functions. |
| `include/logit_cpp/logit/formatter.hpp` | Formatter umbrella. | Adding formatter-facing public headers. |
| `include/logit_cpp/logit/formatter/` | Formatter interfaces and pattern compiler. | Changing output formatting or formatter extension points. |
| `include/logit_cpp/logit/loggers.hpp` | Logger backend umbrella. | Adding/removing public backend headers. |
| `include/logit_cpp/logit/loggers/` | Concrete sinks and `ILogger`. | Adding or changing backend behavior. |
| `include/logit_cpp/logit/detail/` | Private executor, compression, stream, scope internals. | Internal library mechanics only; avoid from consumer code. |
| `include/logit_cpp/logit/Logger.hpp` | Singleton dispatcher and strategy management. | Changing routing, filtering, snapshot, or shutdown behavior. |
| `include/logit_cpp/logit/log_macros.hpp` | Public macro API. | Adding macro families, setup/query macros, or C++11/17 macro branches. |
| `tests/` | Unit, integration, include, ODR, optional feature tests. | Any behavior or include-contract change. |
| `examples/` | User-facing examples. | Public workflow or new backend examples. |
| `bench/` | Benchmark harness and adapters. | Performance scenarios or benchmark-specific adapter changes. |
| `docs/` | Doxygen mainpage and design notes. | Public documentation or deeper design notes. |
| `cmake/` | Package config and pkg-config templates. | Install/export metadata changes. |
| `vcpkg-overlay/` | Local vcpkg port. | Packaging behavior changes. |
| `.github/workflows/` | CI and publishing workflows. | Build matrix, verification, or release automation changes. |
| `external/` | Vendored submodules. | Explicit dependency updates only. |

## Usually Avoid Editing Without Need

- `external/` vendor trees.
- Generated or local build trees such as `build/`, `build-mingw*/`, and `tmp/`.
- `.github/doxygen-awesome-css/` unless updating the docs theme submodule.
- Public macro names, enum values, and include paths unless the task is a
  deliberate breaking change.
- Benchmark internals when changing normal library behavior, except to update
  benchmark coverage for a performance-related feature.

## Final Checklist For Agents

- Read the narrow playbook that matches the task:
  `guides/logging-macros.md`, `guides/header-impl.md`,
  `guides/build.md`, or `guides/commits.md`.
- Use umbrella headers and preserve include policy.
- Keep C++11 compatibility unless the code path is explicitly C++17-gated.
- Keep async and snapshot APIs thread-safe.
- Add tests matching the changed surface.
- Update README, Doxygen, examples, or vcpkg metadata when public behavior
  changes.
