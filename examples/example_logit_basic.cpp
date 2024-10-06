#define LOGIT_BASE_PATH "E:\\_repoz\\log-it-cpp"

#include <iostream>
#include <stdexcept>
#include <log-it/LogIt.hpp>

// Example enumeration
enum COLORS {
    NC = -1,
    BLACK,
    RED,
    GREEN,
    YELLOW,
    BLUE,
    MAGENTA,
    CYAN,
    WHITE,
};

int main() {
    std::cout << "Starting logging example..." << std::endl;

    // Add three logging backends: console, default file logger, and unique file logger
    LOGIT_ADD_CONSOLE_DEFAULT();
    LOGIT_ADD_FILE_LOGGER_DEFAULT();
    LOGIT_ADD_UNIQUE_FILE_LOGGER_DEFAULT_SINGLE_MODE();

    // Log various levels of messages
    float someFloat = 123.456f;
    int someInt = 789;
    COLORS color = RED;

    LOGIT_INFO("This is an informational message", someFloat, someInt);
    LOGIT_DEBUG_IF(true, "This debug message is conditionally logged.");
    LOGIT_WARN("Warning: Something might go wrong here!");
    LOGIT_ERROR("An error has occurred during processing with color", color);
    LOGIT_FATAL("Fatal error! Immediate attention required!");

    // Demonstrating log formatting and streaming
    LOGIT_FORMAT_INFO("Formatted log: %.2f, %d", someFloat, someInt);
    LOGIT_STREAM_INFO() << "Stream logging: float=" << someFloat << ", int=" << someInt << ", color=" << color;

    // Writing to the unique file logger using the second logger (index 2)
    LOGIT_STREAM_TRACE_TO(2) << "Logging to unique file logger with a trace message. Color: " << color;
    LOGIT_PRINT_INFO("Unique log was written to file: ", LOGIT_GET_LAST_FILE_NAME(2));

    try {
        // Simulate an exception
        throw std::runtime_error("An example runtime error");
    } catch (const std::exception& ex) {
        // Log the exception using the logger
        LOGIT_FATAL(ex);

        // Optionally, use the VariableValue for detailed exception logging
        logit::VariableValue exceptionVar("ExceptionCaught", ex);
        LOGIT_STREAM_ERROR() << "Detailed exception logging: " << exceptionVar.to_string();
    }

    // Ensure all loggers are flushed and cleaned up before exiting
    LOGIT_WAIT();

    std::cout << "Logging example completed." << std::endl;
    return 0;
}
