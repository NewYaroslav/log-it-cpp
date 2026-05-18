#include <logit.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <thread>
#include <vector>
#include <iostream>

using logit::detail::SingleThreadExecutor;
using logit::detail::QueuePolicy;

static bool test_basic_enqueue_execute() {
    SingleThreadExecutor ex;
    std::atomic<int> counter{0};
    ex.add_task([&counter]() { counter.fetch_add(1, std::memory_order_relaxed); });
    ex.wait();
    return counter.load() == 1;
}

static bool test_fifo_ordering() {
    SingleThreadExecutor ex;
    std::vector<int> order;
    std::mutex m;
    for (int i = 0; i < 10; ++i) {
        ex.add_task([&order, &m, i]() {
            std::lock_guard<std::mutex> lk(m);
            order.push_back(i);
        });
    }
    ex.wait();
    if (order.size() != 10) return false;
    for (int i = 0; i < 10; ++i) {
        if (order[i] != i) return false;
    }
    return true;
}

static bool test_wait_blocks_until_empty() {
    SingleThreadExecutor ex;
    std::atomic<int> counter{0};
    for (int i = 0; i < 5; ++i) {
        ex.add_task([&counter]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            counter.fetch_add(1, std::memory_order_relaxed);
        });
    }
    ex.wait();
    return counter.load() == 5;
}

static bool test_shutdown_drains_remaining() {
    std::atomic<int> counter{0};
    {
        SingleThreadExecutor ex;
        for (int i = 0; i < 5; ++i) {
            ex.add_task([&counter]() {
                counter.fetch_add(1, std::memory_order_relaxed);
            });
        }
        // shutdown() should drain before joining
        ex.shutdown();
    }
    return counter.load() == 5;
}

static bool test_drop_newest_policy() {
    SingleThreadExecutor ex;
    ex.set_max_queue_size(2);
    ex.set_queue_policy(QueuePolicy::DropNewest);

    // Fill with slow tasks to block the worker
    std::atomic<int> processed{0};
    std::mutex gate_mutex;
    std::condition_variable gate_cv;
    bool gate_open = false;

    // First task holds the worker
    ex.add_task([&]() {
        std::unique_lock<std::mutex> lk(gate_mutex);
        gate_cv.wait(lk, [&]{ return gate_open; });
        processed.fetch_add(1, std::memory_order_relaxed);
    });

    // Give worker time to start the first task
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    // Second task enters the queue (size 1)
    ex.add_task([&]() {
        processed.fetch_add(1, std::memory_order_relaxed);
    });

    // Third task fills queue (size 2)
    ex.add_task([&]() {
        processed.fetch_add(1, std::memory_order_relaxed);
    });

    // Fourth task should be dropped (queue full)
    ex.add_task([&]() {
        processed.fetch_add(1, std::memory_order_relaxed);
    });

    // Release the gate
    {
        std::lock_guard<std::mutex> lk(gate_mutex);
        gate_open = true;
    }
    gate_cv.notify_all();

    ex.wait();
    bool dropped_ok = ex.dropped_tasks() > 0;
    ex.reset_dropped_tasks();
    bool reset_ok = ex.dropped_tasks() == 0;
    return dropped_ok && reset_ok;
}

static bool test_drop_oldest_policy() {
    SingleThreadExecutor ex;
    ex.set_max_queue_size(2);
    ex.set_queue_policy(QueuePolicy::DropOldest);

    std::atomic<int> processed{0};
    std::mutex gate_mutex;
    std::condition_variable gate_cv;
    bool gate_open = false;

    // Hold the worker
    ex.add_task([&]() {
        std::unique_lock<std::mutex> lk(gate_mutex);
        gate_cv.wait(lk, [&]{ return gate_open; });
        processed.fetch_add(1, std::memory_order_relaxed);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    // Fill queue to capacity
    ex.add_task([&]() { processed.fetch_add(1, std::memory_order_relaxed); });
    ex.add_task([&]() { processed.fetch_add(1, std::memory_order_relaxed); });

    // This should drop the oldest queued task
    ex.add_task([&]() { processed.fetch_add(1, std::memory_order_relaxed); });

    {
        std::lock_guard<std::mutex> lk(gate_mutex);
        gate_open = true;
    }
    gate_cv.notify_all();
    ex.wait();

    return ex.dropped_tasks() > 0;
}

static bool test_block_policy() {
    SingleThreadExecutor ex;
    ex.set_max_queue_size(2);
    ex.set_queue_policy(QueuePolicy::Block);

    std::atomic<int> processed{0};
    std::mutex gate_mutex;
    std::condition_variable gate_cv;
    bool gate_open = false;

    // Hold the worker
    ex.add_task([&]() {
        std::unique_lock<std::mutex> lk(gate_mutex);
        gate_cv.wait(lk, [&]{ return gate_open; });
        processed.fetch_add(1, std::memory_order_relaxed);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    // Fill queue
    ex.add_task([&]() { processed.fetch_add(1, std::memory_order_relaxed); });
    ex.add_task([&]() { processed.fetch_add(1, std::memory_order_relaxed); });

    // This enqueue should block until queue drains
    std::thread blocker([&]() {
        ex.add_task([&]() { processed.fetch_add(1, std::memory_order_relaxed); });
    });

    // Wait a bit, then release the gate
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    {
        std::lock_guard<std::mutex> lk(gate_mutex);
        gate_open = true;
    }
    gate_cv.notify_all();

    blocker.join();
    ex.wait();

    // All 4 tasks should have been processed, no drops
    return processed.load() == 4 && ex.dropped_tasks() == 0;
}

static bool test_block_policy_wakes_on_capacity() {
    SingleThreadExecutor ex;
    ex.set_max_queue_size(1);
    ex.set_queue_policy(QueuePolicy::Block);

    std::mutex gate_mutex;
    std::condition_variable gate_cv;
    bool first_gate_open = false;
    bool second_gate_open = false;

    std::mutex done_mutex;
    std::condition_variable done_cv;
    bool producer_done = false;

    ex.add_task([&]() {
        std::unique_lock<std::mutex> lk(gate_mutex);
        gate_cv.wait(lk, [&]() { return first_gate_open; });
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    ex.add_task([&]() {
        std::unique_lock<std::mutex> lk(gate_mutex);
        gate_cv.wait(lk, [&]() { return second_gate_open; });
    });

    std::thread producer([&]() {
        ex.add_task([]() {});
        {
            std::lock_guard<std::mutex> lk(done_mutex);
            producer_done = true;
        }
        done_cv.notify_all();
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    {
        std::lock_guard<std::mutex> lk(gate_mutex);
        first_gate_open = true;
    }
    gate_cv.notify_all();

    bool woke_before_second_task_finished = false;
    {
        std::unique_lock<std::mutex> lk(done_mutex);
        woke_before_second_task_finished =
                done_cv.wait_for(lk, std::chrono::milliseconds(200), [&]() {
                    return producer_done;
                });
    }

    {
        std::lock_guard<std::mutex> lk(gate_mutex);
        second_gate_open = true;
    }
    gate_cv.notify_all();

    producer.join();
    ex.wait();

    return woke_before_second_task_finished;
}

static bool test_exception_in_task() {
    SingleThreadExecutor ex;
    std::atomic<int> counter{0};

    ex.add_task([&counter]() {
        counter.fetch_add(1, std::memory_order_relaxed);
    });
    ex.add_task([&counter]() {
        throw std::runtime_error("test exception");
    });
    ex.add_task([&counter]() {
        counter.fetch_add(1, std::memory_order_relaxed);
    });

    ex.wait();
    return counter.load() == 2;
}

static bool test_concurrent_producers() {
    SingleThreadExecutor ex;
    std::atomic<int> counter{0};
    const int num_threads = 4;
    const int tasks_per_thread = 25;

    std::vector<std::thread> producers;
    for (int t = 0; t < num_threads; ++t) {
        producers.emplace_back([&ex, &counter, tasks_per_thread]() {
            for (int i = 0; i < tasks_per_thread; ++i) {
                ex.add_task([&counter]() {
                    counter.fetch_add(1, std::memory_order_relaxed);
                });
            }
        });
    }
    for (auto& t : producers) t.join();
    ex.wait();

    return counter.load() == num_threads * tasks_per_thread;
}

static bool test_post_shutdown_rejection() {
    SingleThreadExecutor ex;
    std::atomic<int> counter{0};
    ex.add_task([&counter]() { counter.fetch_add(1, std::memory_order_relaxed); });
    ex.shutdown();
    ex.add_task([&counter]() { counter.fetch_add(100, std::memory_order_relaxed); });
    ex.add_task([&counter]() { counter.fetch_add(100, std::memory_order_relaxed); });
    return counter.load() == 1;
}

int main() {
    int passed = 0;
    int failed = 0;

    auto run = [&](const char* name, bool result) {
        if (result) { ++passed; std::cout << "PASS: " << name << std::endl; }
        else        { ++failed; std::cout << "FAIL: " << name << std::endl; }
    };

    run("basic_enqueue_execute", test_basic_enqueue_execute());
    run("fifo_ordering", test_fifo_ordering());
    run("wait_blocks_until_empty", test_wait_blocks_until_empty());
    run("shutdown_drains_remaining", test_shutdown_drains_remaining());
    run("drop_newest_policy", test_drop_newest_policy());
    run("drop_oldest_policy", test_drop_oldest_policy());
    run("block_policy", test_block_policy());
    run("block_policy_wakes_on_capacity", test_block_policy_wakes_on_capacity());
    run("exception_in_task", test_exception_in_task());
    run("concurrent_producers", test_concurrent_producers());
    run("post_shutdown_rejection", test_post_shutdown_rejection());

    std::cout << "\n" << passed << " passed, " << failed << " failed" << std::endl;
    return failed > 0 ? 1 : 0;
}
