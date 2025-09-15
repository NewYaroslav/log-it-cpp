/// \file example_logit_basic.cpp
/// \brief Demonstrates the usage of the LogIt library with various data types and scenarios.

// #define LOGIT_BASE_PATH "E:\\_repoz\\log-it-cpp"  <- set via CMake

#include <iostream>
#include <stdexcept>
#include <LogIt.hpp>
#include <logit/test/loggers/file/file/file/test_log_depth_3.hpp>

// Example enumeration
enum class COLORS {
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

std::ostream& operator<<(std::ostream& os, COLORS c) {
    switch (c) {
        case COLORS::RED: os << "RED"; break;
        case COLORS::GREEN: os << "GREEN"; break;
        case COLORS::BLUE: os << "BLUE"; break;
        default: os << "UNKNOWN_COLOR"; break;
    }
    return os;
}

namespace logit {

    template<>
    std::string enum_to_string(COLORS value) {
        switch (value) {
            LOGIT_ENUM_TO_STR_CASE(COLORS::RED)
            LOGIT_ENUM_TO_STR_CASE(COLORS::GREEN)
            LOGIT_ENUM_TO_STR_CASE(COLORS::BLUE)
            default: return "UNKNOWN_COLOR";
        };
    }

};

// Custom error category for demonstration
class CustomErrorCategory : public std::error_category {
public:
    const char* name() const noexcept override {
        return u8"CustomErrorCategory";
    }

    std::string message(int ev) const override {
        switch (ev) {
            case 1: return u8"Custom error: Invalid operation";
            case 2: return u8"Custom error: Resource not found";
            default: return u8"Custom error: Unknown error";
        }
    }
};

// Singleton instance of custom error category
const std::error_category& custom_error_category() {
    static CustomErrorCategory instance;
    return instance;
}

int main() {
    std::cout << "Starting logging example..." << std::endl;

    // Add three logging backends: console, default file logger, and unique file logger
    LOGIT_ADD_CONSOLE_DEFAULT();
    LOGIT_ADD_FILE_LOGGER_DEFAULT();
    LOGIT_ADD_UNIQUE_FILE_LOGGER_DEFAULT_SINGLE_MODE();
    LOGIT_SET_MAX_QUEUE(64);
    LOGIT_SET_QUEUE_POLICY(LOGIT_QUEUE_DROP);

    logit::test_log_depth_3();

    LOGIT_TRACE0();
    
    bool auth_success = true;
    std::string email = "user@example.com";
    LOGIT_TRACE(auth_success, email);

    {
        LOGIT_SCOPE_INFO("load_config");
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    {
        LOGIT_SCOPE_INFO_T(10, "heavy_step");
        std::this_thread::sleep_for(std::chrono::milliseconds(11));
    }
    {
        int step_id = 7;
        LOGIT_SCOPE_FMT_INFO("step_{}", step_id);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // Demonstrate macros for controlling log frequency and tagging
    for (int i = 0; i < 5; ++i) {
        LOGIT_TRACE_ONCE("Trace once example");
        LOGIT_TRACE_EVERY_N(2, "Trace every 2 iterations", i);
        LOGIT_TRACE_THROTTLE(100, "Throttled trace", i);
        LOGIT_TRACE_TAG("Tagged trace", LOGIT_TAG("iteration", i));
        int id = i * 1000;
        LOGIT_INFO_TAG("sent order", LOGIT_TAG("order_id", id), LOGIT_TAG("side", "BUY"));
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }

    // Log various levels of messages
    float someFloat = 123.456f;
    int someInt = 789;
    COLORS color = COLORS::RED;

    LOGIT_INFO(color);
    LOGIT_FORMAT_INFO("%s", color);
    LOGIT_INFO("This is an informational message", someFloat, someInt);
    LOGIT_DEBUG_IF(true, "This debug message is conditionally logged.");
    LOGIT_WARN("Warning: Something might go wrong here!");
    LOGIT_PRINT_ERROR("An error has occurred during processing with color: ", color);
    LOGIT_FATAL("Fatal error! Immediate attention required!");

    // Demonstrating formatted logging for homogeneous variables
    LOGIT_FORMAT_INFO("%.2f", someFloat, 654.321f);     // Two float values
    LOGIT_FORMAT_INFO("%.4d", someInt, 999);            // Two int values
    LOGIT_FORMAT_INFO("%.2f", 747.000L);                // Long double value

    // Stream-based logging
    LOGIT_STREAM_INFO() << "Stream logging: float=" << someFloat << ", int=" << someInt << ", color=" << color;

    // Writing to the unique file logger using the second logger (index 2)
    LOGIT_STREAM_TRACE_TO(2) << "Logging to unique file logger with a trace message. Color: " << color;
    LOGIT_PRINT_INFO("Unique log was written to file: ", LOGIT_GET_LAST_FILE_NAME(2));

    // Wait for all logs to flush
    LOGIT_WAIT();

    // Logging exceptions
    try {
        // Simulate an exception
        throw std::runtime_error("An example runtime error");
    } catch (const std::exception& ex) {
        // Log the exception using the logger
        LOGIT_FATAL(ex);
    }

    // Logging std::error_code
    std::error_code ec(1, custom_error_category());
    LOGIT_ERROR(ec);
    LOGIT_FORMAT_ERROR("error_code: (%s, %d)", ec);

#   if __cplusplus >= 201703L
    // Logging std::filesystem::path
    std::filesystem::path log_path = "/var/log/example.log";
    LOGIT_INFO(log_path);
    LOGIT_PRINT_INFO("The log file path is: ", log_path);
    LOGIT_FORMAT_INFO("%s", log_path);
#   endif

    // Logging std::chrono::duration
    std::chrono::seconds        seconds(120);
    std::chrono::milliseconds   milliseconds(500);
    std::chrono::microseconds   microseconds(123456);
    std::chrono::minutes        minutes(5);
    std::chrono::hours          hours(2);

    LOGIT_PRINT_TRACE("Seconds example: ", seconds);
    LOGIT_TRACE(milliseconds, minutes, hours);
    LOGIT_FORMAT_TRACE("%s", microseconds);

    // Logging std::chrono::time_point
    auto now = std::chrono::system_clock::now();
    LOGIT_PRINT_INFO("TimePoint example: ", now);
    LOGIT_INFO(now);

    // Logging void*
    void* ptr = &now;
    LOGIT_PRINT_TRACE("Pointer example: ", ptr);
    LOGIT_INFO(ptr);

    // Logging smart pointers
    auto shared = std::make_shared<int>(42);
    LOGIT_PRINT_TRACE("Shared pointer example: ", shared);

#   if __cplusplus >= 201402L
    auto unique = std::make_unique<int>(84);
#   else
    std::unique_ptr<int> unique(new int(84));
#   endif
    LOGIT_PRINT_TRACE("Unique pointer example: ", unique);

#   if __cplusplus >= 201703L
    // Logging std::variant
    std::variant<int, std::string> variant = "Hello, variant!";
    LOGIT_PRINT_INFO("Variant example: ", variant);
    variant = 123;
    LOGIT_TRACE(variant);

    // Logging std::optional
    std::optional<std::string> optional = "Hello, optional!";
    LOGIT_PRINT_INFO("Optional example: ", optional);
    std::optional<std::string> optional_null;
    LOGIT_ERROR(optional_null);
#   endif

    // Ensure all loggers are flushed and cleaned up before exiting
    LOGIT_SHUTDOWN();

    std::cout << "Logging example completed." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    return 0;
}
