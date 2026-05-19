#include <logit.hpp>

#include <atomic>
#include <vector>

int main() {
    {
        logit::detail::SingleThreadExecutor executor;
        std::atomic<int> counter{0};

        executor.add_task([&counter]() {
            counter.fetch_add(1, std::memory_order_relaxed);
        });
        executor.wait();

        if (counter.load(std::memory_order_relaxed) != 1) {
            return 1;
        }
    }

    {
        logit::detail::SingleThreadExecutor executor;
        executor.set_max_queue_size(2);
        executor.set_queue_policy(logit::detail::QueuePolicy::DropNewest);
        std::atomic<int> counter{0};

        executor.add_task([&counter]() { counter.fetch_add(1, std::memory_order_relaxed); });
        executor.add_task([&counter]() { counter.fetch_add(1, std::memory_order_relaxed); });
        executor.add_task([&counter]() { counter.fetch_add(1, std::memory_order_relaxed); });
        executor.wait();

        if (counter.load(std::memory_order_relaxed) != 2 || executor.dropped_tasks() != 1) {
            return 2;
        }
    }

    {
        logit::detail::SingleThreadExecutor executor;
        executor.set_max_queue_size(2);
        executor.set_queue_policy(logit::detail::QueuePolicy::DropOldest);
        std::vector<int> order;

        executor.add_task([&order]() { order.push_back(1); });
        executor.add_task([&order]() { order.push_back(2); });
        executor.add_task([&order]() { order.push_back(3); });
        executor.wait();

        if (order.size() != 2 || order[0] != 2 || order[1] != 3 ||
            executor.dropped_tasks() != 1) {
            return 3;
        }
    }

    {
        logit::detail::SingleThreadExecutor executor;
        executor.set_max_queue_size(1);
        executor.set_queue_policy(logit::detail::QueuePolicy::Block);
        std::atomic<int> counter{0};

        executor.add_task([&counter]() { counter.fetch_add(1, std::memory_order_relaxed); });
        executor.add_task([&counter]() { counter.fetch_add(1, std::memory_order_relaxed); });
        executor.wait();

        if (counter.load(std::memory_order_relaxed) != 2 || executor.dropped_tasks() != 0) {
            return 4;
        }
    }

    {
        logit::detail::SingleThreadExecutor executor;
        std::atomic<int> counter{0};

        executor.add_task([&counter]() { counter.fetch_add(1, std::memory_order_relaxed); });
        executor.shutdown();
        executor.add_task([&counter]() { counter.fetch_add(1, std::memory_order_relaxed); });

        if (counter.load(std::memory_order_relaxed) != 1) {
            return 5;
        }
    }

    return 0;
}
