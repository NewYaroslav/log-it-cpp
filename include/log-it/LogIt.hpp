#pragma once
#ifndef _LOGIT_HPP_INCLUDED
#define _LOGIT_HPP_INCLUDED
/// \file LogIt.hpp
/// \brief Main header file for the LogIt++ library.

#include "LogItConfig.hpp"
#include "parts/Logger.hpp"
#include "parts/LogStream.hpp"
#include "parts/LogMacros.hpp"
#include "parts/Formatter/SimpleLogFormatter.hpp"
#include "parts/Logger/ConsoleLogger.hpp"

/// \namespace logit
/// \brief The primary namespace for the LogIt++ library.
namespace logit {};

/*!
\mainpage LogIt++ Library

\section intro_sec Introduction

**LogIt++** is a flexible and versatile C++ logging library that supports various backends and stream-based output. It provides an easy-to-use interface for logging messages with different severity levels and allows customization of log formats and destinations.

The library combines the simplicity of macro-based logging similar to **IceCream-Cpp** and the configurability of logging backends and formats like **spdlog**.

\section features_sec Features

- **Flexible Log Formatting**: Customize log message formats using patterns.
- **Macro-Based Logging**: Easily log variables and messages using macros.
- **Multiple Backends Support**: Configure loggers to output to console, files, servers, or databases.
- **Asynchronous Logging**: Improve performance by logging in asynchronous mode.
- **Stream-Based Logging**: Use stream operators for logging complex messages.
- **Thread-Safe**: Designed to be used in multi-threaded applications.
- **Extensible**: Create custom loggers and formatters to suit your needs.

\section usage_sec Usage

Here's a simple example demonstrating how to use LogIt++ in your application:

\code{.cpp}
#define LOGIT_SHORT_NAME //
#include <log-it/LogIt.hpp>

int main() {
    // Initialize the logger with default console output
    LOGIT_CONSOLE_DEFAULT();

    float a = 123.456f;
    int b = 789;
    const char* someStr = "Hello, World!";

    // Basic logging using macros
    LOG_INFO("Starting the application");
    LOG_DEBUG("Variable values", a, b);
    LOG_WARN("This is a warning message");

    // Formatted logging
    LOG_PRINTF_INFO("Formatted log: value of a = %.2f", a);
    LOG_FORMAT_WARN("Warning! Values: a = %.2f, b = %d", a, b);

    // Error and fatal logs
    LOG_ERROR("An error occurred with value b =", b);
    LOG_FATAL("Fatal error. Terminating application.");

    // Conditional logging
    LOG_ERROR_IF(b < 0, "Value of b is negative");
    LOG_WARN_IF(a > 100, "Value of a exceeds 100");

    // Stream-based logging with short and long names
    LOG_S_INFO << "Logging a float: " << a << ", and an int: " << b;
    LOG_S_ERROR << "Error occurred in the system";
    LOGIT_STREAM_WARN() << "Warning: potential issue detected with value: " << someStr;

    // Using LOGIT_TRACE for tracing function execution
    LOGIT_TRACE0();  // Trace without arguments
    LOG_TRACE("Entering main function with variable a =", a);

    // Wait for all asynchronous logs to be processed
    LOGIT_WAIT();

    return 0;
}
\endcode

\subsection config_macros Configuration Macros

LogIt++ provides several macros that allow for customization and configuration. Below are the available configuration macros:

- **LOGIT_BASE_PATH**:
    \brief Defines the base path used for log file paths. Defaults to an empty string if not defined.

    \code{.cpp}
    // Defines the base path for project folder.
    #define LOGIT_BASE_PATH "/path/to/your/project"
    \endcode

- **LOGIT_DEFAULT_COLOR**:
    \brief Sets the default color for console output. Defaults to `TextColor::LightGray` if not defined.

    \code{.cpp}
    // Sets the default log message color to green.
    #define LOGIT_DEFAULT_COLOR TextColor::Green
    \endcode

- **LOGIT_CURRENT_TIMESTAMP_MS**:
    \brief Macro to get the current timestamp in milliseconds. By default, it uses `std::chrono` for time calculation.

    \code{.cpp}
    // Customize timestamp calculation if needed.
    #define LOGIT_CURRENT_TIMESTAMP_MS() my_custom_timestamp_function()
    \endcode

\section format_flags_sec Log Message Formatting Flags

LogIt++ supports customizable log message formatting using format flags. You can define how each log message should appear by including placeholders for different pieces of information such as the timestamp, log level, file name, function name, and message.

Below is a list of all supported format flags and their meanings:

\subsection datetime_flags Date and Time Flags
- `%Y`: Year (e.g., 2024)
- `%m`: Month (01-12)
- `%d`: Day of the month (01-31)
- `%H`: Hour (00-23)
- `%M`: Minute (00-59)
- `%S`: Second (00-59)
- `%e`: Millisecond (000-999)
- `%C`: Two-digit year (e.g., 24 for 2024)
- `%c`: Full date and time (e.g., Mon Oct 4 12:45:30 2024)
- `%D`: Short date (e.g., 10/04/24)
- `%T`, `%X`: Time in ISO 8601 format (e.g., 12:45:30)
- `%F`: Date in ISO 8601 format (e.g., 2024-10-04)
- `%s`, `%E`: Unix timestamp in seconds
- `%ms`: Unix timestamp in milliseconds

\subsection weekday_month_flags Weekday and Month Names
- `%b`: Abbreviated month name (e.g., Jan)
- `%B`: Full month name (e.g., January)
- `%a`: Abbreviated weekday name (e.g., Mon)
- `%A`: Full weekday name (e.g., Monday)

\subsection log_level_flags Log Level
- `%l`: Full log level (e.g., INFO, ERROR)
- `%L`: Short log level (e.g., I for INFO, E for ERROR)

\subsection file_function_flags File and Function Information
- `%f`, `%fn`, `%bs`: Base name of the source file (e.g., main.cpp)
- `%g`, `%ffn`: Full file path (e.g., /home/user/project/src/main.cpp)
- `%@`: Source file and line number (e.g., main.cpp:45)
- `%#`: Line number (e.g., 45)
- `%!`: Function name (e.g., main)

\subsection thread_flags Thread Information
- `%t`: Thread identifier

\subsection color_flags Color Formatting
- `%^`: Start color formatting
- `%$`: End color formatting

\subsection message_flags Message Content
- `%v`: The log message content

\subsection example_format Example Format

To define a custom format for your log messages, you can use the following method:

\code{.cpp}
logit::Logger::get_instance().add_logger(
    std::make_unique<logit::ConsoleLogger>(),
    std::make_unique<logit::SimpleLogFormatter>("[%Y-%m-%d %H:%M:%S.%e] [%ffn:%#] [%!] [thread:%t] [%l] %^%v%$"));
\endcode

Alternatively, you can use the `LOGIT_CONSOLE` macro for an even simpler setup:

\code{.cpp}
LOGIT_CONSOLE("[%Y-%m-%d %H:%M:%S.%e] [%ffn:%#] [%!] [thread:%t] [%l] %^%v%$", true);
\endcode

This example format will produce log messages like:

\verbatim
[2024-10-04 12:45:30.123] [main.cpp:45] [main] [thread:1400] [INFO] This is a log message
\endverbatim

You can adjust the format string by changing or rearranging the format flags according to your needs. For example, if you prefer short log levels and don't need thread information, you could use:

\code{.cpp}
LOGIT_CONSOLE("[%Y-%m-%d %H:%M:%S.%e] [%L] %^%v%$", true);
\endcode

This will produce more compact log messages like:

\verbatim
[2024-10-04 12:45:30.123] [I] This is a log message
\endverbatim

You can also include static text and other custom formatting options by mixing text with the flags.

\section custom_backend_sec Custom Logger Backend and Formatter

LogIt++ allows you to extend the logging system by creating your own loggers and formatters. This section explains how to implement a custom backend for logging and a custom log formatter.

\subsection custom_logger Custom Logger Example

To create a custom logger backend, you need to implement the `ILogger` interface, which requires defining the `log()` and `wait()` methods. Here's an example of a simple custom logger that logs messages to a text file:

\code{.cpp}
#include <fstream>
#include <mutex>
#include <log-it/LogIt.hpp>

class FileLogger : public logit::ILogger {
public:
    // Constructor to initialize the file logger with a file name
    FileLogger(const std::string& file_name) : m_file_name(file_name) {
        m_log_file.open(file_name, std::ios::out | std::ios::app);
    }

    ~FileLogger() {
        if (m_log_file.is_open()) {
            m_log_file.close();
        }
    }

    // Logs the message to the file
    void log(const logit::LogRecord& record, const std::string& message) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_log_file.is_open()) {
            m_log_file << message << std::endl;
        }
    }

    // Waits for any asynchronous log operations (none in this case)
    void wait() override {
        // No async processing, so no need to implement
    }

private:
    std::string m_file_name;     ///< The name of the log file
    std::ofstream m_log_file;    ///< The file stream for logging
    std::mutex m_mutex;          ///< Mutex to ensure thread-safe logging
};
\endcode

This `FileLogger` class writes log messages to a specified file. You can add this logger to your logging system like this:

\code{.cpp}
logit::Logger::get_instance().add_logger(
    std::make_unique<FileLogger>("logfile.txt"),
    std::make_unique<logit::SimpleLogFormatter>());
\endcode

\subsection custom_formatter Custom Formatter Example

In addition to creating custom loggers, you can also create custom formatters by implementing the `ILogFormatter` interface. Here's an example of a simple formatter that outputs log messages in JSON format:

\code{.cpp}
#include <log-it/LogIt.hpp>
#include <json/json.h>  // Or your preferred JSON library

class JsonLogFormatter : public logit::ILogFormatter {
public:
    // Formats the log record into a JSON string
    std::string format(const logit::LogRecord& record) const override {
        Json::Value log_entry;
        log_entry["level"] = static_cast<int>(record.log_level);
        log_entry["timestamp_ms"] = record.timestamp_ms;
        log_entry["file"] = record.file;
        log_entry["line"] = record.line;
        log_entry["function"] = record.function;
        log_entry["message"] = record.format;

        Json::StreamWriterBuilder writer;
        return Json::writeString(writer, log_entry);
    }
};
\endcode

This `JsonLogFormatter` formats log messages as JSON objects. You can combine it with any logger, including the `FileLogger`, as shown below:

\code{.cpp}
logit::Logger::get_instance().add_logger(
    std::make_unique<FileLogger>("logfile.json"),
    std::make_unique<JsonLogFormatter>());
\endcode

\subsection summary_custom_backend Summary

By implementing your own `ILogger` and `ILogFormatter`, you can extend the functionality of LogIt++ to log messages to various destinations or in different formats. You can combine custom loggers and formatters to create powerful logging solutions tailored to your application's needs.

Here's a quick summary:

- **ILogger**: Defines where the logs should be sent (e.g., file, console, database, network).
- **ILogFormatter**: Defines how the logs should be formatted (e.g., plain text, JSON, XML).
- **Adding to Logger**: Use `logit::Logger::get_instance().add_logger()` to add your custom logger and formatter to the logging system.

\section install_sec Installation

LogIt++ is a header-only library, which means it can be easily included in your project without the need for compilation or linking. Below are the steps to integrate it into your project.

\subsection step1 Step 1: Clone the Repository

First, clone the LogIt++ repository from GitHub along with its submodules. The library has dependencies on other header-only libraries, such as **time-shield-cpp** (for time utilities) and **fmt** (for string formatting, if `LOGIT_USE_FMT_LIB` is enabled).

To clone the repository with submodules, use the following command:

\code{bash}
git clone --recurse-submodules https://github.com/NewYaroslav/log-it-cpp.git
\endcode

If you have already cloned the repository without submodules, you can initialize and update the submodules by running the following commands:

\code{bash}
git submodule init
git submodule update
\endcode

\subsection step2 Step 2: Include the LogIt++ Headers in Your Project

Since LogIt++ is a header-only library, you can simply include the main header in your project:

\code{cpp}
#include <log-it/LogIt.hpp>
\endcode

This will give you access to the entire logging system.

\subsection step3 Step 3: Configure the Path for Dependencies

LogIt++ depends on **time-shield-cpp**, which is located in the `libs` folder as a submodule. Ensure that the path to `libs\time-shield-cpp\include` is added to your project's include directories.
If you are using an IDE like **Visual Studio** or **CLion**, you can add the include path in the project settings.

\subsection step4 Step 4: Using fmt (Optional)

LogIt++ supports the **fmt** library for advanced string formatting, which is also included as a submodule. To enable `fmt` in LogIt++, define the macro `LOGIT_USE_FMT_LIB` in your project:

\code{cpp}
#define LOGIT_USE_FMT_LIB
\endcode

This will allow you to use `fmt`-style formatting within your log messages. The `fmt` library is also located in the `libs` folder of the repository.

\subsection step5 Step 5: Build and Run Your Project

After adding the necessary include paths, you can proceed to build and run your project. LogIt++ is designed to be easy to integrate and requires no linking since it's header-only.

\section repo_sec Repository

The LogIt++ library is open-source and hosted on GitHub:
[LogIt++ GitHub Repository](https://github.com/NewYaroslav/log-it-cpp).

\section license_sec License

This library is licensed under the **MIT License**. See the [LICENSE](https://github.com/NewYaroslav/log-it-cpp/blob/main/LICENSE) file in the repository for more details.
*/

#endif // _LOGIT_HPP_INCLUDED