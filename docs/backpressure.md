# Queue Back-Pressure Controls

The asynchronous task executor backs every logger and can be tuned to handle
high-load bursts. Use the following helpers from
`<logit_cpp/logit/log_macros.hpp>` when preparing stress tests or
long-running services:

- `LOGIT_SET_MAX_QUEUE(size)` sets the maximum number of queued tasks. Use a
  small `size` to emulate a constrained environment or `0` to remove the
  bound completely.
- `LOGIT_SET_QUEUE_POLICY(mode)` switches the overflow strategy. Available
  options are `LOGIT_QUEUE_BLOCK`, `LOGIT_QUEUE_DROP_NEWEST`, and
  `LOGIT_QUEUE_DROP_OLDEST`.
- `LOGIT_GET_DROPPED_TASKS()` returns the number of tasks that were discarded
  under the current configuration. The value is safe to read concurrently with
  producers.
- `LOGIT_RESET_DROPPED_TASKS()` clears the drop counter. Call it between
  scenarios so each run measures its own losses.

The drop counter is maintained inside the executor and is updated every time a
publishing policy decides to discard work. Combining the counter with
`TaskExecutor::wait()` makes it easy to assert the expected throughput for each
policy without inspecting private state.

The queue limits apply globally to every logger instance. After finishing a
burst test remember to restore the capacity or shut down the logging subsystem
with `LOGIT_WAIT()` and `LOGIT_SHUTDOWN()` to avoid interfering with other
scenarios.
