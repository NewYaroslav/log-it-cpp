# AGENTS

## Header inclusion
This project provides dedicated entry-point headers that include internal dependencies in the correct order. Use these headers instead of forward-declaring library types.

* `include/logit_cpp/logit.hpp` – main entry for the entire library.
* `include/logit_cpp/logit/utils.hpp` – aggregates the utilities module.

Including through these headers ensures dependencies are resolved without manual forward declarations.

## Commit Messages
Use Conventional Commits format: type(scope): summary.
The header must be in English.
Include a body that describes the change.

## Changes
Keep diffs minimal and focused.
Do not refactor or apply style changes beyond the lines you directly touch.

## Header naming conventions
When adding or renaming headers, follow these rules:

* If the file defines a single primary class (with optional supporting enums or helper functions dedicated to that class), use a PascalCase filename.
* If the file hosts multiple classes, macros, free functions, or otherwise represents a mixed collection of declarations, use a snake_case filename.
