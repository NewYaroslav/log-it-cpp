#include <logit.hpp>

#include <atomic>
#include <cstddef>
#include <iostream>
#include <thread>
#include <vector>

int main() {
    auto& executor = logit::detail::TaskExecutor::get_instance();
    executor.wait();

    LOGIT_SET_QUEUE_POLICY(logit::detail::QueuePolicy::Block);
    LOGIT_SET_MAX_QUEUE(32);
    LOGIT_RESET_DROPPED_TASKS();

    std::atomic<bool> start(false);
    std::atomic<std::size_t> submitted(0);
    std::atomic<std::size_t> processed(0);

    std::vector<std::thread> producers;
    for (int t = 0; t < 4; ++t) {
        producers.emplace_back([&]() {
            while (!start.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            for (int i = 0; i < 250; ++i) {
                submitted.fetch_add(1, std::memory_order_relaxed);
                executor.add_task([&processed]() {
                    processed.fetch_add(1, std::memory_order_relaxed);
                });
            }
        });
    }

    std::thread resizer([&]() {
        while (!start.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        for (int i = 0; i < 100; ++i) {
            LOGIT_SET_MAX_QUEUE(8 + static_cast<std::size_t>((i % 8) * 8));
            std::this_thread::yield();
        }
    });

    start.store(true, std::memory_order_release);

    for (std::size_t i = 0; i < producers.size(); ++i) {
        producers[i].join();
    }
    resizer.join();

    executor.wait();

    const bool ok = processed.load(std::memory_order_relaxed) ==
                    submitted.load(std::memory_order_relaxed) &&
                    LOGIT_GET_DROPPED_TASKS() == 0;

    LOGIT_SET_MAX_QUEUE(0);
    LOGIT_RESET_DROPPED_TASKS();

    std::cout << (ok ? "PASS" : "FAIL") << ": task_executor_resize_race" << std::endl;
    return ok ? 0 : 1;
}
