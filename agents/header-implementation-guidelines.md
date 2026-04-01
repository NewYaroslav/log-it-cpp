# Header / Implementation Ownership Playbook

Use this document when you are changing `.hpp`, `.ipp`, or `.tpp` structure.

English is the canonical version.

Russian localization:
- `agents/header-implementation-guidelines-RU.md`

## Decide the subsystem model first

Choose one primary model before editing headers:

- **aggregate-first**
  - normal consumers include one aggregate entry header first
  - shared prerequisites may be centralized there
  - leaf headers may rely on that prepared include context
- **standalone-leaf-first**
  - leaf headers are expected to work independently
  - each leaf owns the dependencies required by its public contract

Do not mix both models casually inside one small subsystem.

For `log-it-cpp`, public modules are intentionally aggregate-first:

- include `<logit.hpp>`, `<logit/utils.hpp>`, `<logit/formatter.hpp>`, or
  `<logit/loggers.hpp>` before the matching leaf header when the public include
  contract expects it
- do not bypass that contract with direct sibling includes

## Place code by ownership

### `.hpp`

Put in `.hpp`:

- public declarations
- public aliases and config types
- the include contract for the module

### `.tpp`

Put in `.tpp`:

- template-visible implementation

Rules:

- if a consumer can instantiate it, the definition must be visible from the header
- include `.tpp` from `.hpp` unconditionally when needed
- do not hide template definitions behind implementation-only paths

### `.ipp`

Put in `.ipp`:

- non-template implementation kept near the header
- shared implementation for header-only and compiled-library modes
- inline-capable helper bodies

Decide whether the `.ipp` is:

- always header-owned
- or ownership-controlled by a compiled ownership TU

## Keep ownership explicit

Use always-included implementation only for:

- templates
- tiny helpers
- small inline methods
- truly leaf-level header-owned logic

Use ownership-controlled implementation for:

- large non-template methods
- code that should be emitted once in compiled-library mode
- implementation shared between header-only and compiled-library builds

## Place dependencies intentionally

Centralize shared includes only when the subsystem intentionally uses an
aggregate entry header.

Otherwise:

- keep dependencies with the leaf header that owns the public contract

Implementation-only helpers such as build-mode macros should stay at the
narrowest valid boundary:

- preferably in the `.ipp` that uses them
- otherwise in the direct owning `.hpp`

Do not spread those helpers through broad aggregators unless that is the real
contract.

## Respect project include boundaries

- Do not use `../` in `#include` directives.
- Within one module sub-tree, include local headers with forward paths such as
  `#include "compiler/PatternCompiler.hpp"`.
- Cross-module dependencies should come from the nearest umbrella header, not
  from direct sibling-module includes.
- Files under `logit/detail/` may include other detail headers, but must not
  include public `logit/...` headers.
- Consumers must not include `logit/detail/...` directly.

## Use forward declarations sparingly

Add forward declarations only when they give a real structural benefit:

- breaking a cycle
- reducing unavoidable coupling
- stabilizing a genuine leaf interface

Do not add them just to make a header look lighter if that:

- hides dependencies
- increases include-order sensitivity
- makes the subsystem harder to read

If the subsystem is aggregate-first, prefer a clear and flat include structure
over mechanical forward-declaration cleanup.

## Respect private boundaries

Do not include private or vendor `.ipp` files from consumer code to bypass
public APIs.

If public headers do not expose what is needed:

- extend the dependency properly
- or keep the needed state and ownership local to the caller

## Test the intended contract

For aggregate-first subsystems:

- test the public path through the aggregate entry header
- prefer include coverage similar to the NHR tests already used in this repository

For standalone-leaf-first subsystems:

- test direct inclusion of leaf headers where that is part of the contract

For white-box tests:

- direct leaf-header inclusion is acceptable
- the test must declare its own prerequisites explicitly
- internal tests must not redefine the public include policy by accident

## Quick review checklist

Before finalizing:

- subsystem model is explicit
- `.tpp` contains all template-visible definitions that consumers need
- `.ipp` ownership is explicit and consistent
- shared includes are centralized only when the aggregate model is intentional
- implementation-only helpers stay local
- forward declarations are justified
- tests validate the intended include contract
