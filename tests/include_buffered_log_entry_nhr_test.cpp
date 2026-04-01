#include <logit/utils.hpp>
#include <logit/utils/BufferedLogEntry.hpp>

int main() {
    logit::BufferedLogEntry entry;
    entry.level = logit::LogLevel::LOG_LVL_INFO;
    entry.timestamp_ms = 42;
    entry.file = __FILE__;
    entry.line = __LINE__;
    entry.function = __func__;
    entry.message = "hello";
    return entry.message == "hello" ? 0 : 1;
}
