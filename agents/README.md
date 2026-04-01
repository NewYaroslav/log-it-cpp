# agents/

Agent-specific instruction files for `log-it-cpp`.

## Files

- `commit-conventions.md` - Conventional Commit format and commit-body rules.
- `cpp-development-guidelines.md` - naming, comments, file naming, include
  ordering, and small style rules for this library.
- `header-implementation-guidelines.md` - ownership rules for `.hpp`, `.ipp`,
  and `.tpp` files, with the project umbrella-header policy.
- `header-implementation-guidelines-RU.md` - Russian localization of the header
  ownership playbook.
- `logging-macro-guidelines.md` - macro-first logging guidance for agents and
  maintainers working above the low-level internals.
- `build-and-test.md` - submodule setup and common CMake build, test, example,
  and benchmark flows.

## Notes

- English is the canonical language for agent instructions.
- Russian localization is provided only for selected files when it helps with
  day-to-day use.
- Keep this folder specific to `log-it-cpp`; do not add unrelated product,
  application-layer, or third-party playbooks here.
