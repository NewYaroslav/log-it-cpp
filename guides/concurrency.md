# Concurrency and Thread-Safety

Use this file when a task changes callback dispatch, mutex usage, shutdown
behaviour, or any code running on `detail::TaskExecutor` or
`detail::CompressionWorker`.

## Execution Model

`log-it-cpp` uses two internal background workers:

- `detail::TaskExecutor` — a singleton worker thread that runs async log tasks
  and user callbacks dispatched from `ConsoleLogger` and other backends.
- `detail::CompressionWorker` — a dedicated thread that performs optional
  rotated-file compression.

Both workers use `std::thread` + `std::condition_variable` + atomic
`m_stop_flag`. User callbacks and internal `process()` routines run on these
threads.

## Invariant: Do Not Block the Worker

No heavy computation, synchronous I/O, or blocking waits are allowed inside
`TaskExecutor` or `CompressionWorker` callbacks. Keep callbacks short; offload
heavy work to your own threads. Blocking the worker delays all other queued log
operations and can stall the compression queue.

## Invariant: Mutex and Callback Ordering

Never call user callbacks while holding an internal mutex. Release internal
locks before invoking any callback, lambda, or virtual method that can re-enter
the library or invoke user code. This prevents deadlocks when user code chains
back into the logging system (for example, logging again from inside a completion
handler).

Example pattern:

```cpp
{
    std::lock_guard<std::mutex> lock(m_mutex);
    snapshot = m_data;   // copy state under lock
}                         // drop lock before the call
user_callback(snapshot);  // safe: callback sees consistent data, no deadlock
```

## Invariant: Exception Safety in Callbacks

Callbacks may throw. Background workers must catch exceptions, route them to the
error path, and leave the process in a consistent state. RAII guards inside the
worker loop must survive exceptions without leaking state or leaving a mutex
locked.

## Invariant: Cancellation and Shutdown

Cancellation must prevent new tasks from being enqueued and safely complete or
cancel existing ones. Use atomic `m_stop_flag` together with
`condition_variable::notify_all` so the worker wakes promptly. Destructors of
async backends must call `wait()` (or an equivalent drain) before their members
are destroyed, ensuring pending lambdas that capture `this` do not access
freed memory.

## Lambda Capture Safety

When dispatching work to a background thread, avoid `[&]` captures that silently
grab class members by reference. A lambda queued into `TaskExecutor` outlives
the function that created it, so references to local variables or class members
that are not `this` can become dangling.

Prefer one of these patterns:

- Copy the needed value:

  ```cpp
  int level = static_cast<int>(record.log_level);
  TaskExecutor::get_instance().add_task([level, message]() {
      // safe: level and message are copied
  });
  ```

- Capture `this` explicitly only when the object lifetime is guaranteed to exceed
  the task lifetime (typically via `wait()` in the destructor):

  ```cpp
task_executor.add_task([this, record, message]() {
      this->sink->log(record, message);
  });
  ```

- If multiple members are needed, capture a snapshot struct by value rather than
  capturing `this`.

Never capture a local variable by reference `[&]` and enqueue it into a worker
thread.

## Cross-References

- `guides/orientation.md` — project map, subsystem model, and extension recipes.
- `guides/build.md` — CMake flags that affect async behaviour (`LOGIT_FORCE_ASYNC_OFF`,
  `LOGIT_USE_MPSC_RING`).
