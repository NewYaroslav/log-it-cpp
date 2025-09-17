#include <logit/loggers.hpp>
#include <logit/loggers/ConsoleLogger.hpp>

int main() {
    logit::ConsoleLogger logger(false);
    logger.set_log_level(logit::LogLevel::LOG_LVL_WARN);
    logger.wait();
    return 0;
}
