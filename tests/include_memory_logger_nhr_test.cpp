#include <logit/loggers.hpp>
#include <logit/loggers/MemoryLogger.hpp>

int main() {
    logit::MemoryLogger logger(logit::MemoryLogger::Config{16, 256, 1000});
    logger.set_log_level(logit::LogLevel::LOG_LVL_INFO);
    return logger.get_log_level() == logit::LogLevel::LOG_LVL_INFO ? 0 : 1;
}
