#include <logit/loggers.hpp>
#include <logit/loggers/ILogger.hpp>

namespace {
class CountingLogger final : public logit::ILogger {
public:
    void log(const logit::LogRecord& record, const std::string& message) override {
        ++m_count;
        m_last_message = message;
        m_last_timestamp = record.timestamp_ms;
        m_last_file = record.file;
    }

    std::string get_string_param(const logit::LoggerParam& param) const override {
        switch (param) {
        case logit::LoggerParam::LastFileName:
            return m_last_file;
        case logit::LoggerParam::LastFilePath:
            return m_last_file;
        case logit::LoggerParam::LastLogTimestamp:
        case logit::LoggerParam::TimeSinceLastLog:
            return {};
        }
        return {};
    }

    int64_t get_int_param(const logit::LoggerParam& param) const override {
        switch (param) {
        case logit::LoggerParam::LastLogTimestamp:
            return m_last_timestamp;
        case logit::LoggerParam::TimeSinceLastLog:
            return 0;
        case logit::LoggerParam::LastFileName:
        case logit::LoggerParam::LastFilePath:
            return static_cast<int64_t>(m_count);
        }
        return 0;
    }

    double get_float_param(const logit::LoggerParam&) const override { return 0.0; }

    void set_log_level(logit::LogLevel level) override { m_level = level; }

    logit::LogLevel get_log_level() const override { return m_level; }

    void wait() override {}

    int count() const { return m_count; }
    const std::string& last_message() const { return m_last_message; }

private:
    logit::LogLevel m_level = logit::LogLevel::LOG_LVL_TRACE;
    int m_count = 0;
    std::string m_last_message;
    std::string m_last_file;
    int64_t m_last_timestamp = 0;
};
} // namespace

int main() {
    CountingLogger logger;
    logger.set_log_level(logit::LogLevel::LOG_LVL_INFO);

    logit::LogRecord record(
        logit::LogLevel::LOG_LVL_INFO,
        42,
        __FILE__,
        __LINE__,
        __func__,
        "message",
        "",
        -1,
        false);

    logger.log(record, "from test");
    logger.wait();

    return (logger.count() > 0 && !logger.last_message().empty()) ? 0 : 1;
}
