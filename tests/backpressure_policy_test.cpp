#include <logit.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <thread>
#include <vector>

namespace {

constexpr std::size_t kSingleProducerBurst = 32;
constexpr std::size_t kSingleProducerQueueCapacity = 4;
constexpr std::size_t kMultiProducerThreads = 4;
constexpr std::size_t kMessagesPerProducer = 32;
constexpr std::size_t kMultiProducerQueueCapacity = 16;
constexpr auto kSlowTaskDelay = std::chrono::milliseconds{5};

struct ScenarioResult {
    std::chrono::steady_clock::duration publish_duration{};
    std::size_t processed{};
    std::size_t dropped{};
};

ScenarioResult run_single_producer_scenario(logit::detail::QueuePolicy policy,
                                           bool hold_consumer) {
    auto &executor = logit::detail::TaskExecutor::get_instance();
    LOGIT_SET_QUEUE_POLICY(policy);
    LOGIT_RESET_DROPPED_TASKS();

    std::atomic<std::size_t> processed{0};
    std::condition_variable gate_cv;
    std::mutex gate_mutex;
    bool gate_open = !hold_consumer;

    const auto start = std::chrono::steady_clock::now();
    for (std::size_t i = 0; i < kSingleProducerBurst; ++i) {
        executor.add_task([&processed, hold_consumer, &gate_cv, &gate_mutex, &gate_open]() {
            if (hold_consumer) {
                std::unique_lock<std::mutex> lock(gate_mutex);
                gate_cv.wait(lock, [&gate_open]() { return gate_open; });
            }
            std::this_thread::sleep_for(kSlowTaskDelay);
            processed.fetch_add(1, std::memory_order_relaxed);
        });
    }
    const auto publish_duration = std::chrono::steady_clock::now() - start;

    if (hold_consumer) {
        {
            std::lock_guard<std::mutex> lock(gate_mutex);
            gate_open = true;
        }
        gate_cv.notify_all();
    }

    executor.wait();

    ScenarioResult result{};
    result.publish_duration = publish_duration;
    result.processed = processed.load(std::memory_order_relaxed);
    result.dropped = LOGIT_GET_DROPPED_TASKS();
    return result;
}

struct MultiProducerResult {
    std::size_t processed{};
    std::size_t dropped{};
};

MultiProducerResult run_multi_producer_scenario(logit::detail::QueuePolicy policy,
                                               bool hold_consumer) {
    auto &executor = logit::detail::TaskExecutor::get_instance();
    LOGIT_SET_QUEUE_POLICY(policy);
    LOGIT_RESET_DROPPED_TASKS();

    std::atomic<std::size_t> processed{0};
    std::atomic<bool> start_flag{false};
    std::condition_variable gate_cv;
    std::mutex gate_mutex;
    bool gate_open = !hold_consumer;

    std::vector<std::thread> producers;
    producers.reserve(kMultiProducerThreads);

    for (std::size_t i = 0; i < kMultiProducerThreads; ++i) {
        producers.emplace_back([&executor, &processed, &start_flag, hold_consumer, &gate_cv, &gate_mutex, &gate_open]() {
            while (!start_flag.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            for (std::size_t j = 0; j < kMessagesPerProducer; ++j) {
                executor.add_task([&processed, hold_consumer, &gate_cv, &gate_mutex, &gate_open]() {
                    if (hold_consumer) {
                        std::unique_lock<std::mutex> lock(gate_mutex);
                        gate_cv.wait(lock, [&gate_open]() { return gate_open; });
                    }
                    std::this_thread::sleep_for(kSlowTaskDelay);
                    processed.fetch_add(1, std::memory_order_relaxed);
                });
            }
        });
    }

    start_flag.store(true, std::memory_order_release);

    for (auto &producer : producers) {
        producer.join();
    }

    if (hold_consumer) {
        {
            std::lock_guard<std::mutex> lock(gate_mutex);
            gate_open = true;
        }
        gate_cv.notify_all();
    }

    executor.wait();

    MultiProducerResult result{};
    result.processed = processed.load(std::memory_order_relaxed);
    result.dropped = LOGIT_GET_DROPPED_TASKS();
    return result;
}

} // namespace

int main() {
    auto &executor = logit::detail::TaskExecutor::get_instance();
    executor.wait();

    LOGIT_SET_MAX_QUEUE(kSingleProducerQueueCapacity);

    const auto block_result = run_single_producer_scenario(logit::detail::QueuePolicy::Block,
                                                          false);
    if (block_result.processed != kSingleProducerBurst) {
        return 1;
    }
    if (block_result.dropped != 0) {
        return 2;
    }

    const auto drop_newest_result = run_single_producer_scenario(logit::detail::QueuePolicy::DropNewest,
                                                                true);
    if (drop_newest_result.dropped == 0) {
        return 3;
    }
    if (drop_newest_result.processed + drop_newest_result.dropped != kSingleProducerBurst) {
        return 4;
    }
    const auto single_min_survivors = std::min(kSingleProducerQueueCapacity, kSingleProducerBurst);
    const auto single_max_survivors = std::min(kSingleProducerQueueCapacity + 1, kSingleProducerBurst);
    if (drop_newest_result.processed < single_min_survivors) {
        return 5;
    }
    if (drop_newest_result.processed > single_max_survivors) {
        return 6;
    }

    const auto drop_oldest_result = run_single_producer_scenario(logit::detail::QueuePolicy::DropOldest,
                                                                true);
    if (drop_oldest_result.dropped == 0) {
        return 7;
    }
    if (drop_oldest_result.processed + drop_oldest_result.dropped != kSingleProducerBurst) {
        return 8;
    }
    if (drop_oldest_result.processed < single_min_survivors) {
        return 9;
    }
    if (drop_oldest_result.processed > single_max_survivors) {
        return 10;
    }

    if (block_result.publish_duration <= drop_newest_result.publish_duration) {
        return 11;
    }
    if (block_result.publish_duration <= drop_oldest_result.publish_duration) {
        return 12;
    }

    LOGIT_RESET_DROPPED_TASKS();

    LOGIT_SET_MAX_QUEUE(kMultiProducerQueueCapacity);

    const auto total_messages = kMultiProducerThreads * kMessagesPerProducer;

    const auto block_multi_result = run_multi_producer_scenario(logit::detail::QueuePolicy::Block,
                                                                false);
    if (block_multi_result.processed != total_messages) {
        return 13;
    }
    if (block_multi_result.dropped != 0) {
        return 14;
    }

    const auto drop_newest_multi = run_multi_producer_scenario(logit::detail::QueuePolicy::DropNewest,
                                                               true);
    if (drop_newest_multi.dropped == 0) {
        return 15;
    }
    if (drop_newest_multi.processed + drop_newest_multi.dropped != total_messages) {
        return 16;
    }
    const auto multi_min_survivors = std::min(kMultiProducerQueueCapacity, total_messages);
    const auto multi_max_survivors = std::min(kMultiProducerQueueCapacity + 1, total_messages);
    if (drop_newest_multi.processed < multi_min_survivors) {
        return 17;
    }
    if (drop_newest_multi.processed > multi_max_survivors) {
        return 18;
    }

    const auto drop_oldest_multi = run_multi_producer_scenario(logit::detail::QueuePolicy::DropOldest,
                                                               true);
    if (drop_oldest_multi.dropped == 0) {
        return 19;
    }
    if (drop_oldest_multi.processed + drop_oldest_multi.dropped != total_messages) {
        return 20;
    }
    if (drop_oldest_multi.processed < multi_min_survivors) {
        return 21;
    }
    if (drop_oldest_multi.processed > multi_max_survivors) {
        return 22;
    }

    LOGIT_RESET_DROPPED_TASKS();
    return 0;
}

