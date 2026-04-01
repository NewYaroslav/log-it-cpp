# AGENTS

This repository is a header-only C++ logging library with optional tests,
examples, and benchmarks. Keep agent instructions focused on `log-it-cpp`
itself rather than on generic product or application architecture.

## Always Follow

- Keep diffs minimal and focused.
- Do not refactor or apply style changes beyond the lines you directly touch.
- Use Conventional Commits: `type(scope): summary`.
- Commit headers must be in English and include a descriptive body.

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

Read the matching file in `agents/` when the task needs more detail:

- `agents/README.md` - index of available agent instructions.
- `agents/commit-conventions.md` - commit message rules.
- `agents/cpp-development-guidelines.md` - naming and style guidance.
- `agents/header-implementation-guidelines.md` - `.hpp` / `.ipp` / `.tpp`
  ownership and include-structure rules.
- `agents/logging-macro-guidelines.md` - macro-first logging guidance for
  normal application code and agent behavior.
- `agents/build-and-test.md` - submodules, configure/build/test/bench flow.
