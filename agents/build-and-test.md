# Build and Test Playbook

Use this file when a task requires configuring, building, testing, or running
benchmarks for `log-it-cpp`.

## Submodules first

Before configuring or building, make sure submodules are present:

```bash
git submodule update --init --recursive
```

This repository depends on bundled submodules such as `libs/time-shield-cpp`,
and optional builds may also use `libs/fmt`, `libs/zlib`, or `libs/zstd`.

## General build expectations

- `log-it-cpp` is header-only, but CMake is still used for tests, examples,
  benchmarks, packaging metadata, and optional dependency toggles.
- Do not commit local build artifacts unless the user explicitly asks for that.
- Prefer reusing an existing compatible build tree when it is clearly tied to
  this repository and the current toolchain.

## Common CMake options

From `CMakeLists.txt` and `README.md`, the most relevant toggles are:

- `LOGIT_CPP_BUILD_TESTS` - build the test suite.
- `LOGIT_CPP_BUILD_EXAMPLES` - build example programs.
- `LOGIT_BENCH_ENABLE` - build benchmark targets.
- `LOGIT_BENCH_WITH_SPDLOG` - include the spdlog comparison benchmarks.
- `LOGIT_WITH_FMT` - enable `{}`-style formatting support.
- `LOGIT_WITH_GZIP` / `LOGIT_WITH_ZSTD` - enable rotated-file compression.
- `LOGIT_USE_SUBMODULES` - allow bundled dependency fallbacks when system
  packages are missing.
- `LOGIT_WITH_SYSLOG` - enable the POSIX syslog backend on supported targets.
- `LOGIT_WITH_WIN_EVENT_LOG` - enable the Windows Event Log backend on Windows.
- `LOGIT_FORCE_ASYNC_OFF` - force synchronous logging.
- `LOGIT_USE_MPSC_RING` - enable the lock-free task queue.
- `LOGIT_ENABLE_DROP_OLDEST_SLOWPATH` - compile the ring slow-path for
  `DropOldest`.

## Typical flows

### Configure tests

```bash
cmake -S . -B build -DLOGIT_CPP_BUILD_TESTS=ON
```

### Build tests

```bash
cmake --build build
```

### Run tests

```bash
ctest --test-dir build --output-on-failure
```

### Build examples

```bash
cmake -S . -B build -DLOGIT_CPP_BUILD_EXAMPLES=ON
cmake --build build
```

### Build benchmarks

```bash
cmake -S . -B build -DLOGIT_BENCH_ENABLE=ON -DLOGIT_BENCH_WITH_SPDLOG=ON
cmake --build build --target logit_bench
```

Benchmark output is described in `README.md`; the benchmark binary lives under
the build tree, typically `build/bench/logit_bench`.

## Verification notes

- Use `ctest --output-on-failure` for the default verification pass when tests
  are enabled.
- When a task changes public include structure, include-contract coverage matters
  as much as runtime tests.
- If optional dependency flags are changed, verify both the option wiring in
  CMake and the resulting include/link expectations.
