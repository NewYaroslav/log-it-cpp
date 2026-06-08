# CI Environment Parity

Use this file when a task changes CI workflows, CMake/build configuration,
tests, examples, public headers, dependency setup, packaging, or anything that
could compile differently across the supported matrix.

## Version Compatibility

- Check `cmake_minimum_required` before adding CMake syntax. This project
  currently declares CMake 3.18, so avoid commands, options, or policy-dependent
  behavior that require newer CMake unless the minimum is intentionally raised.
- Keep the C++ standard matrix in mind. The default build supports C++11, while
  MDBX, OTLP, and Prometheus server paths may require C++17. Do not use C++17
  language/library features in C++11 paths.
- When adding dependency declarations, prefer syntax supported by the declared
  minimum tool versions. If newer tool behavior is needed, update the minimum
  version and document why.

## Compiler Matrix

- Treat GCC, Clang, and MSVC as distinct compilers, not interchangeable local
  substitutes. A test that passes on one compiler can still fail on another.
- Write portable C++ for public headers and tests. Be explicit in lambda
  captures when there is no default capture mode; MSVC diagnoses cases that
  some local GCC builds may miss.
- Include the headers that provide the standard library types you use. Avoid
  relying on transitive includes that may differ between standard libraries.
- Assume CI may promote warnings or tool diagnostics that are quiet locally.
  Keep casts, conversions, and unused values minimal and intentional.

## Tests Under Instrumentation

- Prefer deterministic synchronization over sleeps, wall-clock timing, or
  scheduler assumptions. If a timing-sensitive test is unavoidable, keep the
  tolerance generous and document why the timing is part of the contract.
- Sanitizers and heavy instrumentation slow execution and can change scheduling.
  Timing-sensitive or stress tests should be labelled or filtered in CMake/CI
  when they are not meaningful under those modes.
- Put CI behavior in executable configuration, not only in documentation. If a
  test must not run under a sanitizer or platform, express that through CMake
  labels, test properties, or workflow filters.

## Clean Environment Assumptions

- Do not assume local utilities are available in CI images. If a workflow,
  packaging step, or script needs a tool such as `file`, `git`, `curl`, or an
  archiver, install or check for it explicitly.
- For optional dependencies, verify both enabled and disabled paths when the
  changed code has compile-time branches.
- Keep generated build directories, downloaded artifacts, and local cache files
  out of commits unless the task explicitly asks to change checked-in fixtures.

## CI Failure Workflow

- Inspect real job logs before changing code. Start with `gh pr checks`, then
  open the failing GitHub Actions run or job log.
- Identify whether the failure is from compile errors, tests, environment setup,
  packaging, or an external service. Keep the fix scoped to the observed cause.
- Reproduce locally when a matching toolchain is available. If it is not, make a
  conservative compatibility fix and rely on the rerun CI signal.
- After pushing a CI fix, recheck the relevant jobs. Record any jobs still
  pending or failing separately from the fixed failure.

## Cross-References

- `guides/build.md` - configure/build/test commands and common options.
- `guides/concurrency.md` - callback dispatch, mutex ordering, and async tests.
- `guides/cpp_style.md` - small C++ style rules that reduce compiler drift.
