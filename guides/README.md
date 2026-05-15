# Guides

These files are shared AI-agent instruction files for `log-it-cpp`.

## Reading Order

1. `orientation.md` — project map, subsystem model, extension recipes,
   safety invariants, and utility inventory.
2. `concurrency.md` — thread-safety contracts, callback/mutex ordering,
   exception safety, and lambda capture rules.
3. Task-specific playbooks for the change at hand.

## Files

| File | Purpose |
| --- | --- |
| `orientation.md` | Project map, public API, source layout, layer rules, and safety invariants. |
| `concurrency.md` | Thread-safety contracts, callback dispatch rules, mutex ordering, shutdown invariants, and lambda capture safety. |
| `cpp_style.md` | C++ naming, comments, formatting, and small style rules. |
| `header-impl.md` | Ownership rules for `.hpp`, `.ipp`, `.tpp` files and include-structure policy. |
| `header-impl-RU.md` | Russian localization of the header ownership playbook. |
| `logging-macros.md` | Macro-first logging guidance for agents and maintainers. |
| `build.md` | Build, test, example, and benchmark flows. |
| `commits.md` | Commit message conventions and grouping rules. |

## Maintenance

Keep each file focused on one concern. Update the canonical topic file instead
of copying rules into `AGENTS.md`. No tool-specific rules unless genuinely
different for that tool.
