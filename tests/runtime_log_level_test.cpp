#include <logit.hpp>

int main() {
    LOGIT_ADD_MEMORY_LOGGER_DEFAULT();
    LOGIT_ADD_MEMORY_LOGGER_DEFAULT();

    if (LOGIT_GET_LOG_LEVEL(0) != logit::LogLevel::LOG_LVL_TRACE ||
        LOGIT_GET_LOG_LEVEL(1) != logit::LogLevel::LOG_LVL_TRACE) {
        return 1;
    }

    LOGIT_SET_LOG_LEVEL_TO(0, logit::LogLevel::LOG_LVL_WARN);
    if (LOGIT_GET_LOG_LEVEL(0) != logit::LogLevel::LOG_LVL_WARN ||
        LOGIT_GET_LOG_LEVEL(1) != logit::LogLevel::LOG_LVL_TRACE) {
        return 1;
    }

    LOGIT_INFO("info-before-global");
    LOGIT_WARN("warn-before-global");

    auto first_logger = LOGIT_GET_BUFFERED_STRINGS(0);
    auto second_logger = LOGIT_GET_BUFFERED_STRINGS(1);
    if (first_logger.size() != 1 || first_logger[0] != "warn-before-global") {
        return 1;
    }
    if (second_logger.size() != 2 ||
        second_logger[0] != "info-before-global" ||
        second_logger[1] != "warn-before-global") {
        return 1;
    }

    LOGIT_SET_LOG_LEVEL(logit::LogLevel::LOG_LVL_ERROR);
    if (LOGIT_GET_LOG_LEVEL(0) != logit::LogLevel::LOG_LVL_ERROR ||
        LOGIT_GET_LOG_LEVEL(1) != logit::LogLevel::LOG_LVL_ERROR) {
        return 1;
    }

    LOGIT_WARN("warn-after-global");
    LOGIT_ERROR("error-after-global");

    first_logger = LOGIT_GET_BUFFERED_STRINGS(0);
    second_logger = LOGIT_GET_BUFFERED_STRINGS(1);

    if (first_logger.size() != 2 ||
        first_logger[0] != "warn-before-global" ||
        first_logger[1] != "error-after-global") {
        return 1;
    }

    if (second_logger.size() != 3 ||
        second_logger[0] != "info-before-global" ||
        second_logger[1] != "warn-before-global" ||
        second_logger[2] != "error-after-global") {
        return 1;
    }

    return 0;
}
