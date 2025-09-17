#include <logit/formatter.hpp>
#include <logit/formatter/SimpleLogFormatter.hpp>

int main() {
    logit::SimpleLogFormatter formatter;
    logit::LogRecord record(
        logit::LogLevel::LOG_LVL_INFO,
        123,
        __FILE__,
        __LINE__,
        __func__,
        "format",
        "",
        -1,
        false);
    const std::string formatted = formatter.format(record);
    return formatted.empty() ? 1 : 0;
}
