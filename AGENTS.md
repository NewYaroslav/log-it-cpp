# AGENTS

## Header inclusion
This project provides dedicated entry-point headers that include internal dependencies in the correct order. Use these headers instead of forward-declaring library types.

* `include/logit_cpp/LogIt.hpp` – main entry for the entire library.
* `include/logit_cpp/logit/utils.hpp` – aggregates the utilities module.

Including through these headers ensures dependencies are resolved without manual forward declarations.
