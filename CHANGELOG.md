# Changelog

All notable changes to this project will be documented in this file.

## [v1.0.2] - 2026-04-25
- Added raw and section logging macros for unformatted diagnostic snapshots that bypass level filters while still using configured backends, queues, routing, and file rotation.
- Added in-memory snapshot logging APIs, buffered entry retrieval, runtime logger snapshots, and examples for control-plane style diagnostics.
- Added persisted file access APIs for listing and reading current and rotated file logs.
- Added system logging backends for POSIX syslog and Windows Event Log, plus POSIX/Windows crash logger backends and registration macros.
- Added compile-time log-level filtering, runtime log-level controls, conditional logging helpers, frequency controls, tagging macros, stream/printf/scope macro coverage, and default no-op handling for disabled macro families.
- Added file logger size-based rotation, rotation naming policies, retention coverage, gzip/zstd/external-command compression support, and idempotent rotation tests.
- Added configurable async backpressure controls, queue policies, lock-free MPSC task execution, hot queue resizing, and TSAN-oriented regression coverage.
- Added Emscripten build support, CMake package installation metadata, pkg-config generation, vcpkg overlay updates, and install-consumer coverage.
- Added benchmark coverage and refreshed benchmark adapters, latency snapshots, and CI benchmark gating.
- Expanded CI coverage with sanitizer, Emscripten, ODR, install-consumer, compression, and platform-specific regression checks.
- Reorganized public include entry points, moved internal helpers under `detail`, hardened header-only ODR behavior, and fixed utility/header dependency issues.
- Refreshed README, README-RU, Doxygen, agent guidance, macro references, examples, and architecture/task-executor documentation.
- Updated bundled dependency pins, including TimeShield through `v1.0.5` and compression dependency pins.
- Fixed Windows crash-filter naming collisions, FileLogger rotation ordering and error handling, benchmark async flushing, MPSC/drop-policy accounting, fmt-disabled macro handling, and `LOGIT_SCOPE_*` duration logging with unnamed messages.

## [v1.0.1] - 2025-08-05
- Added initial CMake integration for building, installing, and consuming the header-only package.
- Added Emscripten build support with browser console output and runtime export wiring.
- Added minimum log-level control macros and related logger configuration coverage.
- Improved Doxygen mainpage and macro documentation, including version tag injection during documentation publishing.
- Added docs grouping support and updated publish workflow paths.
- Cleaned up public include dependencies and redundant includes while preserving compatibility with existing entry headers.
- Restored and updated the TimeShield submodule reference used by the release.
- Fixed path utility comments, Doxygen comments, `argument_utils` unused parameter warnings, formatter fallthrough warnings, and `VariableValue` overload ambiguity for `bool`.

## [v1.0.0] - 2025-07-17
- Initial public release of the header-only LogIt++ logging library.
- Added console, file, and unique-file logger backends with asynchronous task execution.
- Added configurable log formatting with pattern compiler support and formatter interfaces.
- Added logging macros for trace, debug, info, warn, error, and fatal levels.
- Added stream-style logging and variable argument formatting helpers.
- Added path, encoding, formatting, and argument parsing utilities.
- Added examples for basic logging, custom backends, customized settings, and short macro names.
- Added README, README-RU, Doxygen configuration, generated documentation styling, and MIT license metadata.
