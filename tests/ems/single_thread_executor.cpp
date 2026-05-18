#include <logit.hpp>

#include <atomic>

int main() {
    logit::detail::SingleThreadExecutor executor;
    std::atomic<int> counter{0};

    executor.add_task([&counter]() {
        counter.fetch_add(1, std::memory_order_relaxed);
    });
    executor.wait();

    return counter.load(std::memory_order_relaxed) == 1 ? 0 : 1;
}
