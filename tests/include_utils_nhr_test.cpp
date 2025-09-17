#include <logit/utils.hpp>
#include <logit/utils/LogRecord.hpp>

int main() {
    logit::LogRecord record(
        logit::LogLevel::LOG_LVL_DEBUG,
        0,
        __FILE__,
        __LINE__,
        __func__,
        "%s",
        "",
        -1,
        true);
    record.args_array.emplace_back("answer", 42);
    return record.logger_index == -1 ? 0 : 1;
}
