# TaskExecutor Implementation Notes

The asynchronous task executor powers every non-blocking logger. It accepts
work from multiple producer threads and drains it on a dedicated worker. This
document describes how the executor behaves across build configurations,
provides guidance on tuning the backpressure policies, and explains the
lifetime guarantees that logger integrations rely on.

## 1. Implementation variants

### Default deque worker (without `LOGIT_USE_MPSC_RING`)

* Structure: one worker thread (`m_worker_thread`) consumes a `std::deque`
  protected by `m_queue_mutex`.
* Synchronisation: producers and the worker coordinate through
  `m_queue_condition` and the `m_stop_flag` atomic.
* Backpressure policies are implemented literally on the protected deque.
* Intended for environments where a simple mutex-protected queue is sufficient
  or where the lock-free ring cannot be used.

### Lock-free MPSC ring (`LOGIT_USE_MPSC_RING`)

* Structure: producers push tasks into `m_mpsc_queue`, a lock-free
  `MpscRingAny<std::function<void()>>` with a single consumer thread.
* Synchronisation primitives:
  * `m_cv` + `m_cv_mutex` coordinate sleepers for both the worker and producers
    that wait for capacity during `QueuePolicy::Block`.
  * `m_queue_condition` wakes `wait()` callers once the queue drains.
  * `m_active_tasks` tracks in-flight work so that `Block` limits concurrent
    execution and `wait()` can determine quiescence.
  * `m_stop_flag` terminates the worker and stops accepting new tasks.
* Enables very low producer overhead while maintaining FIFO ordering on the
  consumer side.

### Emscripten builds without pthreads

* Structure: single-threaded `std::deque` guarded by `m_mutex`.
* No dedicated worker thread is created. Instead, tasks are drained via
  `emscripten_async_call` scheduled from the main loop.
* Not thread-safe — intended for WebAssembly builds where pthreads are not
  available.

## 2. Backpressure semantics

`QueuePolicy` controls what happens when the queue reaches `max_queue_size`
(`0` means "unbounded").

* `Block`
  * Uses `m_active_tasks` to count in-flight work. If the counter reaches the
    limit, producers wait. The non-MPSC build waits on
    `m_queue_condition`. The MPSC build parks on `m_cv` with short sleeps while
    the worker drains tasks. The sleep interval defaults to
    `LOGIT_TASK_EXECUTOR_BLOCK_WAIT_USEC` microseconds (200 by default) and can
    be overridden at compile time. This policy avoids loss at the expense of
    producer-side backpressure.
* `DropNewest`
  * Non-MPSC: the incoming task is discarded when the deque is full.
  * MPSC: identical semantics — the incoming task is dropped and
    `m_dropped_tasks` is incremented.
* `DropOldest`
  * Non-MPSC: the oldest dequeued element is removed, then the incoming task is
    enqueued, providing literal "drop the oldest" behaviour.
  * MPSC: **drop-incoming semantics**. The executor rejects the incoming task
    instead of racing to remove an old element. This preserves the order of
    tasks already accepted by the consumer, avoids lock-step coordination
    between multiple producers and the worker, and keeps the implementation
    TSAN-clean. `m_dropped_tasks` still counts these rejections.

The drop counter is observable via `TaskExecutor::dropped_tasks()` and exposed to
end users through `LOGIT_GET_DROPPED_TASKS()`.

## 3. Hot queue resize (`LOGIT_USE_MPSC_RING`)

`set_max_queue_size()` performs a "hot" resize without tearing down the
application.

1. `m_resizing` is set to `true` with release semantics.
2. `wait()` drains the queue and ensures `m_active_tasks == 0`.
3. The worker is stopped by setting `m_stop_flag`, notifying sleepers, and
   joining the thread so it no longer touches `m_mpsc_queue`.
4. In a single thread the ring is rebuilt with the new capacity. The resize
   keeps `m_dropped_tasks` intact but resets `m_active_tasks` to 0 because the
   queue is empty.
5. The worker thread is restarted and the stop flag cleared.
6. `m_resizing` flips back to `false` and `m_resize_cv.notify_all()` wakes
   producers that parked at the start of `add_task()`.

While the resize is in progress, producers briefly wait on `m_resize_cv`. No
accepted tasks are lost, and the consumer thread never observes partially
initialised ring buffers.

## 4. Ordering and completion guarantees

* Exactly one consumer thread executes tasks, so work is processed in the order
  accepted by the consumer.
* When the ring build is enabled, `DropNewest` and `DropOldest` both drop the
  incoming task; accepted tasks keep their order.
* `wait()` returns once the queue is empty and `m_active_tasks == 0`, or when a
  shutdown is requested.
* `shutdown()` blocks until the worker thread terminates. It is safe to call
  multiple times.

## 5. Singleton and lifetime management

`TaskExecutor::get_instance()` intentionally stores the singleton inside a
`static TaskExecutor* instance = new TaskExecutor();`. This lets the executor
outlive static destructors inside logger components. Applications may call
`shutdown()` explicitly (for example during test teardown), but the singleton
remains valid until the process terminates.

## 6. Emscripten (no pthreads)

When targeting Emscripten without pthread support:

* The executor remains single-threaded and therefore not thread-safe.
* `Block` is approximated by invoking `drain()` from the producer path until the
  deque has room. `DropNewest`/`DropOldest` mirror the deque operations exactly.
* Tasks are executed by `emscripten_async_call`, which schedules a drain on the
  browser event loop. This keeps logging compatible with the cooperative
  execution model used in WebAssembly UI scenarios.
* Typical use cases: browser-hosted tools or demos that need asynchronous-style
  logging without pulling in pthread support.

## 7. API surface and macros

Public methods exposed by `TaskExecutor`:

* `set_max_queue_size(std::size_t size)` — change the queue capacity (`0`
  disables the limit). Trigger a hot resize on MPSC builds.
* `set_queue_policy(QueuePolicy policy)` — change overflow behaviour.
* `add_task(std::function<void()> fn)` — enqueue work for the background worker.
* `wait()` — block until the queue drains or stop is requested.
* `shutdown()` — stop the worker thread and release resources.
* `dropped_tasks()` and `reset_dropped_tasks()` — inspect or reset the overflow
  counter.

Macros in `<logit_cpp/logit/log_macros.hpp>` map directly onto these calls:

* `LOGIT_SET_MAX_QUEUE(size)` → `set_max_queue_size(size)`
* `LOGIT_SET_QUEUE_POLICY(mode)` → `set_queue_policy(mode)`
* `LOGIT_QUEUE_BLOCK`, `LOGIT_QUEUE_DROP_NEWEST`, `LOGIT_QUEUE_DROP_OLDEST`
  select the enum value.
* `LOGIT_GET_DROPPED_TASKS()` and `LOGIT_RESET_DROPPED_TASKS()` forward to the
  counter helpers.

### Examples

Basic setup using macros:

```cpp
#include <logit.hpp>

int main() {
    LOGIT_ADD_CONSOLE_DEFAULT();
    LOGIT_SET_QUEUE_POLICY(LOGIT_QUEUE_BLOCK);

    LOGIT_INFO("async logging is live");
    LOGIT_WAIT();
}
```

Hot resize while the system is running (only with `LOGIT_USE_MPSC_RING`):

```cpp
auto& executor = logit::detail::TaskExecutor::get_instance();
LOGIT_SET_QUEUE_POLICY(LOGIT_QUEUE_BLOCK);

// Later, increase the capacity without losing accepted tasks.
LOGIT_SET_MAX_QUEUE(1024); // producers briefly wait for resize to finish
```

Inspecting drops under `DropNewest`:

```cpp
LOGIT_SET_QUEUE_POLICY(LOGIT_QUEUE_DROP_NEWEST);
LOGIT_SET_MAX_QUEUE(16);
LOGIT_RESET_DROPPED_TASKS();

for (int i = 0; i < 1000; ++i) {
    LOGIT_INFO("burst", i);
}

LOGIT_WAIT();
const auto lost = LOGIT_GET_DROPPED_TASKS();
```

## 8. Thread-safety and TSAN considerations

* All public methods on non-Emscripten builds are thread-safe. Producers may
  call `add_task()` concurrently with `set_max_queue_size()` and
  `set_queue_policy()`.
* The hot-resize barrier uses `m_resizing` and `m_resize_cv` so producers never
  touch a ring buffer that is being rebuilt. This eliminates the data races that
  TSAN previously reported on `try_pop()` vs. buffer assignment. The barrier
  only drops once the worker thread fully stops and the queue drains; if a sink
  blocks the worker or `QueuePolicy::Block` keeps `m_active_tasks` above the
  limit for more than one second, `set_max_queue_size()` abandons the hot
  resize, clears `m_resizing`, and leaves the existing ring untouched so
  producers cannot wait indefinitely. Non-MPSC builds perform the resize as an
  atomic update of `m_max_queue_size`, so they are not subject to this stall.
* Non-MPSC builds rely solely on mutexes and had no known data races.
* The Emscripten path is single-threaded and should not be used concurrently.

## 9. Performance and tuning

* `QueuePolicy::Block` limits the number of in-flight tasks tracked by
  `m_active_tasks`. Use it to introduce producer-side backpressure when the
  downstream sinks are expensive.
* The worker drains up to `LOGIT_TASK_EXECUTOR_DRAIN_BUDGET` tasks per iteration
  when the ring is enabled. Increase this "budget" in
  `TaskExecutor::worker_function()` if your workload generates extremely large
  bursts and the worker sleeps too often. Reducing it can lower per-iteration
  latency for latency-sensitive applications.
* Adjust `LOGIT_TASK_EXECUTOR_DEFAULT_RING_CAPACITY` at compile time to select a
  different default capacity when `LOGIT_USE_MPSC_RING` is active.
* Monitor `dropped_tasks()` during load testing to verify that the chosen policy
  matches the application's tolerance for loss.
