#define LOGIT_COMPILED_LEVEL LOGIT_LEVEL_TRACE
#include <logit.hpp>
#include <thread>
#include <chrono>

// Basic smoke test for frequency control macros.

int main() {
    int once_counter = 0;
    for (int i = 0; i < 10; ++i) {
        LOGIT_WARN_ONCE(once_counter++);
    }
    if (once_counter != 1) return 1;

    int every_n_counter = 0;
    for (int i = 0; i < 10; ++i) {
        LOGIT_INFO_EVERY_N(3, every_n_counter++);
    }
    if (every_n_counter != 3) return 1;

    int throttle_counter = 0;
    auto throttle_call = [&]() { LOGIT_ERROR_THROTTLE(50, throttle_counter++); };
    throttle_call();
    throttle_call();
    std::this_thread::sleep_for(std::chrono::milliseconds(60));
    throttle_call();
    if (throttle_counter != 2) return 1;

    return 0;
}
