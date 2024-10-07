#define LOGIT_BASE_PATH "E:\\_repoz\\log-it-cpp"

#include <iostream>
#include <sstream>
#include <log-it/LogIt.hpp>


/// \cond
// Custom Logger class inheriting from ILogger
class CustomLogger : public logit::ILogger {
public:

    CustomLogger() = default;

    /// \brief Logs a message by formatting the log record and message.
    /// \param record The log record containing log event details.
    /// \param message The formatted log message to log.
    void log(const logit::LogRecord& record, const std::string& message) override {
        std::ostringstream oss;

        // Example format: "[LEVEL] timestamp: message"
        oss << "[" << logit::to_string(record.log_level) << "] "
            << record.timestamp_ms << ": "
            << message << std::endl;

        // Output the log message (in this case, to the console)
        std::cout << oss.str();
    }

    // Implementing other pure virtual methods
    std::string get_string_param(const logit::LoggerParam& param) const override {
        // Returning an empty string for this example, can be customized
        return "";
    }

    int64_t get_int_param(const logit::LoggerParam& param) const override {
        // Returning a default value, can be customized
        return 0;
    }

    double get_float_param(const logit::LoggerParam& param) const override {
        // Returning a default value, can be customized
        return 0.0;
    }

    void wait() override {
        // No asynchronous logging here, but method is required
    }

    ~CustomLogger() override = default;
};
/// \endcond

int main() {
    std::cout << "Starting logging example with custom backend..." << std::endl;

    // Adding the custom logger using LOGIT_ADD_LOGGER macro
    LOGIT_ADD_LOGGER(CustomLogger, (), logit::SimpleLogFormatter, ("%v"));

    // Logging some test messages with the custom logger
    float someFloat = 99.99f;
    int someInt = 42;

    LOGIT_INFO("Custom backend info log", someFloat, someInt);
    LOGIT_WARN("This is a warning message from the custom logger");
    LOGIT_ERROR("Custom logger error message occurred");

    // Simulate exception logging with custom backend
    try {
        throw std::runtime_error("Example runtime error");
    } catch (const std::exception& ex) {
        LOGIT_FATAL(ex);
    }

    // Ensure all logs are flushed before exiting
    LOGIT_WAIT();

    std::cout << "Logging example with custom backend completed." << std::endl;
    return 0;
}
