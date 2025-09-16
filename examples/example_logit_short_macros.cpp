// #define LOGIT_BASE_PATH "E:\\_repoz\\log-it-cpp"  <- set via CMake
#define LOGIT_SHORT_NAME // Enable short macros

#include <iostream>
#include <logit.hpp>

#define UNIQUE_LOGGER_ID 2

// Example enumeration
enum COLORS {
    BLACK,
    RED,
    GREEN,
    BLUE
};

int main() {
    std::cout << "Starting logging example with short macros..." << std::endl;

    // Add logging backends
    LOGIT_ADD_CONSOLE_DEFAULT();                        // index 0
    LOGIT_ADD_FILE_LOGGER_DEFAULT();                    // index 1
    LOGIT_ADD_UNIQUE_FILE_LOGGER_DEFAULT_SINGLE_MODE(); // index 2 (UNIQUE_LOGGER_ID)

    // Using short macros for logging various levels of messages
    LOG_I("Info message using short macro");

    // Logging with direct print (various variations)
    LOG_W_PRINT("Warning message with color: ", COLORS::RED); // Using print macro
    LOG_WP("Warning message with color: ", COLORS::RED);      // Abbreviated version

    // Formatted logging with printf-style macros
    LOG_W_PRINTF("Warning message with color: %d", static_cast<int>(COLORS::RED)); // Using printf
    LOG_WPF("Warning message with color: %d", static_cast<int>(COLORS::RED));      // Abbreviated printf

    // Formatted logging with specific format
    const int temp = 42;
    LOG_WF("%.4d", static_cast<int>(COLORS::RED), temp); // Warning log with fixed-width format

    // Error log with float value using print and printf
    LOG_E_PRINT("Error message with float value: ", 42.42f);  // Using print macro
    LOG_EP("Error message with float value: ", 42.42f);       // Abbreviated version

    const int error_code = 404;
    LOG_F("Fatal error message", error_code); // Fatal log with additional data

    // Stream-based short logging macros
    LOG_S_INFO() << "Stream-based info logging with short macro. Integer value: " << 123;
    LOG_S_WARN() << "Stream-based warning with color: " << COLORS::GREEN;

    // Logging to a specific logger (unique file logger with index 2)
    LOG_S_TRACE_TO(UNIQUE_LOGGER_ID) << "Trace logging to unique file with short macro";

    // Formatted info and error logs
    LOG_IPF("Formatted info log: float=%.2f, int=%d", 3.14, 42); // Info formatted log
    LOG_E_PRINTF("Formatted error log: value=%d", 100);          // Error formatted log

    // Ensure all logs are flushed before exiting
    LOGIT_SHUTDOWN();

    std::cout << "Logging example with short macros completed." << std::endl;
    return 0;
}
