#include <logit.hpp>

namespace {
class NullLogger final : public logit::ILogger {
public:
    void log(const logit::LogRecord&, const std::string&) override {}
    std::string get_string_param(const logit::LoggerParam&) const override { return std::string(); }
    int64_t get_int_param(const logit::LoggerParam&) const override { return 0; }
    double get_float_param(const logit::LoggerParam&) const override { return 0.0; }
    void set_log_level(logit::LogLevel level) override { m_level = level; }
    logit::LogLevel get_log_level() const override { return m_level; }
    void wait() override {}

private:
    logit::LogLevel m_level = logit::LogLevel::LOG_LVL_TRACE;
};
} // namespace

int main() {
    LOGIT_ADD_LOGGER(NullLogger, (), logit::SimpleLogFormatter, ("%v"));
    LOGIT_ADD_MEMORY_LOGGER_DEFAULT_SINGLE_MODE();

    LOGIT_INFO("not buffered by single-mode memory logger");
    LOGIT_INFO_TO(1, "remote snapshot");

    const auto empty_invalid = LOGIT_GET_BUFFERED_STRINGS(99);
    if (!empty_invalid.empty()) {
        return 1;
    }

    const auto empty_non_memory = LOGIT_GET_BUFFERED_STRINGS(0);
    if (!empty_non_memory.empty()) {
        return 1;
    }

    const auto strings = LOGIT_GET_BUFFERED_STRINGS(1);
    if (strings.size() != 1 || strings[0] != "remote snapshot") {
        return 1;
    }

    const auto entries = LOGIT_GET_BUFFERED_ENTRIES(1);
    if (entries.size() != 1 || entries[0].message != "remote snapshot") {
        return 1;
    }

    return 0;
}
