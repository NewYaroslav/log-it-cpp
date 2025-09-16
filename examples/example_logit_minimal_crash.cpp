/// \file example_logit_minimal_crash.cpp
/// \brief Minimal LogIt++ setup that intentionally aborts after logging a fatal message.

#include <cstdlib>

#define LOGIT_SHORT_NAME
#include <LogIt.hpp>

namespace {

[[noreturn]] void crash_demo(int attempt) {
    LOG_WPF("Attempt %d exceeded retry budget", attempt);
    LOG_F("Triggering abort to demonstrate fatal logging");
    LOGIT_WAIT();
    std::abort();
}

} // namespace

int main() {
    LOGIT_ADD_CONSOLE_DEFAULT();
    LOGIT_SET_MAX_QUEUE(16);
    LOGIT_SET_QUEUE_POLICY(LOGIT_QUEUE_DROP);

    double progress = 0.42;
    LOG_I("Minimal setup ready");
    LOG_IF("%.0f%% ready", progress * 100.0);
    LOG_S_INFO() << "Streaming alias demo";

    LOGIT_WARN_ONCE("initializing subsystem"); // duplicates suppressed automatically

    // Rapid retries trigger the throttle macro; only the first message is printed.
    for (int retry = 1; retry <= 3; ++retry) {
        LOGIT_ERROR_THROTTLE(200, "Retrying connection to upstream service");
    }

    crash_demo(3);
}
