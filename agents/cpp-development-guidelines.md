# C++ Development Guidelines

## Variable naming

- Class fields should use the `m_` prefix when adding new members.
- Prefixes like `p_` or `str_` are optional and should not be introduced mechanically.
- Boolean names should start with `is`, `has`, `use`, `enable`, or member-field forms like `m_is_`, `m_has_`.
- Do not use `b_`, `n_`, or `f_` prefixes.

## Doxygen comments

- All comments and Doxygen text must be in English.
- Use `/// \brief` before classes and functions.
- Do not start brief descriptions with `The`.

## File naming

- If a file contains one primary class, use `PascalCase` file names.
- If a file contains multiple classes, helpers, macros, or utility code, use `snake_case`.

## Entity naming

- Class, struct, and enum names use `CamelCase`.
- Method names use `snake_case`.

## Method naming

- Methods should be named in `snake_case`.
- Getter names may omit `get_` when they return a direct property, reference, or value with no substantial computation.
- Use `get_` when the method computes a result or when omitting `get_` would make semantics unclear.

## Code style

- Use 4 spaces for indentation; do not use tabs.
- Indent declarations and definitions inside `namespace` blocks by one level.
- Keep opening braces on the same line for classes, methods, and namespaces.
- Do not use `using namespace`; always qualify names such as `std::`.
- Keep project headers before system headers in include lists.
- Header files must start with `#pragma once`; if an include guard is also used, prefer a `LOGIT_*_HPP_INCLUDED` style guard that matches the file.
- Keep source and documentation files in UTF-8.
- Write non-ASCII C++ string literals as `u8"..."`.
- Preserve existing public API names unless the task explicitly requires renaming them.

## Header / implementation ownership

- Use `header-implementation-guidelines.md` for `.hpp` / `.ipp` / `.tpp` ownership and include-structure policy.
- Keep this file focused on naming and formatting rather than duplicating the full header policy.

## Constants and macros

- Constants and macro names use `UPPER_SNAKE_CASE`.
- Enum values use `CamelCase`.
- Mark non-inheritable classes with `final`.
- Use `override` for overridden virtual methods.
