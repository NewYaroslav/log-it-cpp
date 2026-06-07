# AGENTS

This repository is a header-only C++ logging library with optional tests,
examples, and benchmarks. Keep agent instructions focused on `log-it-cpp`
itself rather than on generic product or application architecture.

## Always Follow

- Keep diffs minimal and focused.
- Do not refactor or apply style changes beyond the lines you directly touch.
- Use Conventional Commits: `type(scope): summary`.
- Commit headers must be in English and include a descriptive body.

## Codebase Discovery

For non-trivial codebase investigation, architecture questions, cross-file edits,
refactors, call chains, or unknown implementation locations, use Codebase Memory
before Grep/Glob/Read/LSP.

Preferred sequence:
`index_status` → `search_graph` / `trace_path` / `get_code_snippet` → targeted Read/LSP.

Grep/Glob are fallback or precision-confirmation tools, not first-pass architecture discovery.

## Public Entry Headers

Use the project umbrella headers instead of recreating include order manually:

- `include/logit_cpp/logit.hpp` - full library entry point.
- `include/logit_cpp/logit/utils.hpp` - utilities module umbrella.
- `include/logit_cpp/logit/formatter.hpp` - formatter module umbrella.
- `include/logit_cpp/logit/loggers.hpp` - logger backend umbrella.

These headers prepare internal dependencies in the intended order.

## Include Policy

- Do not use `../` in `#include` directives.
- Within a module (`logit/utils/*`, `logit/formatter/*`, `logit/loggers/*`)
  include only headers from the same sub-tree using forward paths.
- Cross-module dependencies must come through the nearest umbrella header
  instead of direct sibling includes.
- Files under `logit/detail/` may include other detail headers, but must not
  include public `logit/...` headers.
- Prefer the self-contained public entry points in `include/logit_cpp`.

## Header Naming

- If a header defines one primary class, use a PascalCase filename.
- If a header contains mixed declarations, helpers, macros, or multiple types,
  use a snake_case filename.

## Repository Setup

Before configuring or building, initialize submodules:

```bash
git submodule update --init --recursive
```

## Detailed Playbooks

Read the matching file in `guides/` when the task needs more detail:

- `guides/README.md` - index of available agent instructions.
- `guides/orientation.md` - project map, subsystem model, extension
  recipes, and safety invariants for AI agents.
- `guides/commits.md` - commit message rules.
- `guides/cpp_style.md` - naming and style guidance.
- `guides/header-impl.md` - `.hpp` / `.ipp` / `.tpp`
  ownership and include-structure rules.
- `guides/header-impl-RU.md` - Russian localization of the header ownership playbook.
- `guides/logging-macros.md` - macro-first logging guidance for
  normal application code and agent behavior.
- `guides/concurrency.md` - thread-safety contracts, callback dispatch,
  mutex ordering, and shutdown invariants.
- `guides/build.md` - submodules, configure/build/test/bench flow.
- `guides/ci.md` - CI environment parity, toolchain compatibility,
  sanitizer/timing test guidance, and CI-failure workflow.
