#include <logit.hpp>

#include <array>
#include <atomic>
#include <cstddef>
#include <mutex>
#include <thread>
#include <vector>

namespace {

constexpr std::size_t kProducers = 4;
constexpr std::size_t kMessagesPerProducer = 64;
constexpr std::size_t kQueueCapacity = 32;

} // namespace

int main() {
    auto &executor = logit::detail::TaskExecutor::get_instance();
    executor.wait();

    LOGIT_SET_QUEUE_POLICY(logit::detail::QueuePolicy::Block);
    LOGIT_SET_MAX_QUEUE(kQueueCapacity);
    LOGIT_RESET_DROPPED_TASKS();

    // Векторы лежат в стеке main-потока, но их изменяют воркеры.
    // Пишем/читаем их ТОЛЬКО под одним и тем же per-producer мьютексом.
    std::array<std::vector<std::size_t>, kProducers> sequences;
    for (auto &sequence : sequences) {
        sequence.clear();
        sequence.reserve(kMessagesPerProducer);
    }

    std::array<std::mutex, kProducers> sequence_guards{};
    std::atomic<bool> start{false};

    std::vector<std::thread> producers;
    producers.reserve(kProducers);

    for (std::size_t producer_id = 0; producer_id < kProducers; ++producer_id) {
        producers.emplace_back([producer_id, &executor, &start, &sequence_guards, &sequences]() {
            // Барьер запуска
            while (!start.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }

            for (std::size_t seq = 0; seq < kMessagesPerProducer; ++seq) {
                executor.add_task([producer_id, seq, &sequence_guards, &sequences]() {
                    std::lock_guard<std::mutex> lock(sequence_guards[producer_id]);
                    sequences[producer_id].push_back(seq);
                });
            }
        });
    }

    start.store(true, std::memory_order_release);

    for (auto &producer : producers) {
        producer.join();
    }

    executor.wait(); // гарантируем завершение всех задач

    if (LOGIT_GET_DROPPED_TASKS() != 0) {
        return 1;
    }

    // Читаем под тем же мьютексом — это устраняет data race в TSAN
    for (std::size_t producer_id = 0; producer_id < kProducers; ++producer_id) {
        std::lock_guard<std::mutex> lock(sequence_guards[producer_id]);

        const auto &sequence = sequences[producer_id];
        if (sequence.size() != kMessagesPerProducer) {
            return 2;
        }

        for (std::size_t expected = 0; expected < sequence.size(); ++expected) {
            if (sequence[expected] != expected) {
                return 3;
            }
        }
    }

    LOGIT_RESET_DROPPED_TASKS();
    return 0;
}

