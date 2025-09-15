/// \file example_logit_system_logger.cpp
/// \brief Demonstrates usage of the cross-platform SystemLogger.

#include <LogIt.hpp>

int main() {
    LOGIT_ADD_LOGGER(logit::SystemLogger, (), logit::SimpleLogFormatter, (LOGIT_CONSOLE_PATTERN));
    LOGIT_INFO("system log entry");
    LOGIT_WAIT();
    LOGIT_SHUTDOWN();
    return 0;
}
