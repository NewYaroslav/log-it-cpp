// #define LOGIT_BASE_PATH "E:\\_repoz\\log-it-cpp" <- set via CMake

// Customizing the logging settings
#define LOGIT_CONSOLE_PATTERN "%Y-%m-%d %H:%M:%S [%L] %v"       // Custom pattern for console logs
#define LOGIT_FILE_LOGGER_PATH "E:\\logs\\default_logs"         // Custom path for default file logger
#define LOGIT_UNIQUE_FILE_LOGGER_PATH "E:\\logs\\unique_logs"   // Custom path for unique file logger
#define LOGIT_FILE_LOGGER_AUTO_DELETE_DAYS 10                   // Auto-delete logs older than 10 days

#include <LogIt.hpp>
#include <iostream>
#include <stdexcept>

// Example enumeration
enum LogLevel {
    L_DEBUG,
    L_INFO,
    L_WARN,
    L_ERROR,
    L_FATAL
};

int main() {
    std::cout << "Starting customized logging example..." << std::endl;

    // Add logging backends with modified settings
    LOGIT_ADD_CONSOLE_DEFAULT();
    LOGIT_ADD_FILE_LOGGER_DEFAULT();
    LOGIT_ADD_UNIQUE_FILE_LOGGER_DEFAULT_SINGLE_MODE();

    // Log various levels of messages
    float someFloat = 42.42f;
    int someInt = 100;
    LogLevel level = L_INFO;

    LOGIT_INFO("Logging an informational message with customized settings", someFloat, someInt, level);
    LOGIT_WARN("Warning: Potential issue with customized log settings!");
    LOGIT_ERROR("Error: Something went wrong!", level);

    // Demonstrate logging to unique file logger
    LOGIT_STREAM_TRACE_TO(2) << "Logging to unique file with trace message. LogLevel: " << level;
    LOGIT_PRINT_INFO("Unique log file: ", LOGIT_GET_LAST_FILE_NAME(2));

    try {
        // Simulate an exception
        throw std::runtime_error("A runtime error occurred");
    } catch (const std::exception& ex) {
        // Log the exception
        LOGIT_FATAL(ex);
    }

    // Wait for all logs to flush
    LOGIT_SHUTDOWN();

    std::cout << "Customized logging example completed." << std::endl;
    return 0;
}
