#include <logit.hpp>

#include <cstddef>
#include <vector>

namespace {

constexpr std::size_t kMessages = 128;
constexpr std::size_t kQueueCapacity = 64;

} // namespace

int main() {
    auto &executor = logit::detail::TaskExecutor::get_instance();
    executor.wait();

    LOGIT_SET_QUEUE_POLICY(logit::detail::QueuePolicy::Block);
    LOGIT_SET_MAX_QUEUE(kQueueCapacity);
    LOGIT_RESET_DROPPED_TASKS();

    std::vector<std::size_t> order;
    order.reserve(kMessages);

    for (std::size_t i = 0; i < kMessages; ++i) {
        executor.add_task([i, &order]() {
            order.push_back(i);
        });
    }

    executor.wait();

    if (order.size() != kMessages) {
        return 1;
    }

    for (std::size_t index = 0; index < order.size(); ++index) {
        if (order[index] != index) {
            return 2;
        }
    }

    if (LOGIT_GET_DROPPED_TASKS() != 0) {
        return 3;
    }

    LOGIT_RESET_DROPPED_TASKS();
    return 0;
}

