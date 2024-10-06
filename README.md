# LogIt++ Library
![LogIt++ Logo](docs/logo-640x320.png)

## Introduction

**LogIt++** is a flexible and versatile C++ logging library that supports various backends and stream-based output. It provides an easy-to-use interface for logging messages with different severity levels and allows customization of log formats and destinations.

The library combines the simplicity of macro-based logging similar to **IceCream-Cpp** and the configurability of logging backends and formats like **spdlog**. 
LogIt++ is fully compatible with C++11, ensuring support for a wide range of compilers and systems.

## Features

- **Flexible Log Formatting**: Customize log message formats using patterns.
- **Macro-Based Logging**: Easily log variables and messages using macros.
- **Multiple Backends Support**: Configure loggers to output to console, files, servers, or databases.
- **Asynchronous Logging**: Improve performance by logging in asynchronous mode.
- **Stream-Based Logging**: Use stream operators for logging complex messages.
- **Thread-Safe**: Designed to be used in multi-threaded applications.
- **Extensible**: Create custom loggers and formatters to suit your needs.

## Usage

Here’s a simple example demonstrating how to use LogIt++ in your application:

```cpp
#define LOGIT_SHORT_NAME
#include <log-it/LogIt.hpp>

int main() {
    // Initialize the logger with default console output
    LOGIT_ADD_CONSOLE_DEFAULT();

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
```

## Log Message Formatting Flags
LogIt++ supports customizable log message formatting using format flags. You can define how each log message should appear by including placeholders for different pieces of information such as the timestamp, log level, file name, function name, and message.

Below is a list of supported format flags:

- *Date and Time Flags*:

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
	- `%b`: Abbreviated month name (e.g., Jan)
	- `%B`: Full month name (e.g., January)
	- `%a`: Abbreviated weekday name (e.g., Mon)
	- `%A`: Full weekday name (e.g., Monday)
		
- *Log Level Flags*:

	- `%l`: Full log level (e.g., INFO, ERROR)
	- `%L`: Short log level (e.g., I for INFO, E for ERROR)
	
- *File and Function Flags*:

	- `%f`: Base name of the source file
	- `%g`: Full file path
	- `%#`: Line number
	- `%!`: Function name
	
- *Thread Flags*:

	- `%t`: Thread identifier
	
- *Color Flags*:

	- `%^`: Start color formatting
	- `%$`: End color formatting
	
- *Message Flags*:

	- `%v`: The log message content
	
## Shortened Logging Macros

LogIt++ provides shortened versions of logging macros when LOGIT_SHORT_NAME is defined. These macros allow for concise logging across different log levels, including both standard and stream-based logging.

### Available TRACE-level macros:

- Basic logging:

	- `LOG_T(...)`: Logs a TRACE-level message.
	- `LOG_T0()`: Logs a TRACE-level message without arguments.
	- `LOG_T_NOARGS()`: Logs a TRACE-level message with no arguments.
	
- Formatted logging:

	- `LOG_TF(fmt, ...)`: Logs a formatted TRACE-level message using format strings.
	- `LOG_T_PRINT(...)`: Logs a TRACE-level message by printing each argument.
	- `LOG_T_PRINTF(fmt, ...)`: Logs a formatted TRACE-level message using printf-style formatting.
	- `LOG_TP(...)`: Alias for `LOG_T_PRINT(...)`.
	- `LOG_TPF(fmt, ...)`: Alias for `LOG_T_PRINTF(fmt, ...)`.
	
- Alternative TRACE-level macros:

	- `LOG_TRACE(...)`: Logs a TRACE-level message (same as `LOG_T(...)`).
	- `LOG_TRACE0()`: Logs a TRACE-level message without arguments (same as `LOG_T0()`).
	- `LOG_TRACE_NOARGS()`: Logs a TRACE-level message with no arguments (same as `LOG_T_NOARGS()`).
	- `LOG_TRACEF(fmt, ...)`: Logs a formatted TRACE-level message (same as `LOG_TF(fmt, ...)`).
	- `LOG_TRACE_PRINT(...)`: Logs a TRACE-level message by printing each argument (same as `LOG_T_PRINT(...)`).
	- `LOG_TRACE_PRINTF(fmt, ...)`: Logs a formatted TRACE-level message using printf-style formatting (same as `LOG_T_PRINTF(fmt, ...)`).

- Example:

```cpp
LOG_T("Trace message using short macro");
LOG_TF("%.4d", 999);
LOG_T_PRINT("Printing trace message with multiple variables: ", var1, var2);
LOG_TRACE("Trace message (alias for LOG_T)");
LOG_TRACE_PRINTF("Formatted trace: value = %d", value);
```

## Configuration Macros
LogIt++ provides several macros that allow for customization and configuration. Below are the available configuration macros:

- **LOGIT_BASE_PATH**: Defines the base path used for log file paths. If `LOGIT_BASE_PATH` is not defined or is empty ({}), the full path from `__FILE__` will be used for log file paths. You can override this to specify a custom base path for your log files.

```cpp
#define LOGIT_BASE_PATH "/path/to/your/project"
```

- **LOGIT_DEFAULT_COLOR**: Defines the default color for console output. If `LOGIT_DEFAULT_COLOR` is not defined, it defaults to TextColor::LightGray. You can set a custom console text color by overriding this macro.

```cpp
#define LOGIT_DEFAULT_COLOR TextColor::Green
```

- **LOGIT_CURRENT_TIMESTAMP_MS**: Macro to get the current timestamp in milliseconds. By default, it uses *std::chrono* to get the timestamp. You can override this to provide a custom timestamp function if needed.

```cpp
#define LOGIT_CURRENT_TIMESTAMP_MS() my_custom_timestamp_function()
```

- **LOGIT_CONSOLE_PATTERN**: Defines the default log pattern for the console logger. This pattern controls the formatting of log messages sent to the console, including timestamp, message, and color. If `LOGIT_CONSOLE_PATTERN` is not defined, it defaults to `%H:%M:%S.%e | %^%v%$`.

```cpp
#define LOGIT_CONSOLE_PATTERN "%H:%M:%S.%e | %^%v%$"
```

- **LOGIT_FILE_LOGGER_PATH**: Defines the default directory path for log files. If `LOGIT_FILE_LOGGER_PATH` is not defined, it defaults to "data/logs". You can set this to a custom path to control where the log files are stored.

```cpp
#define LOGIT_FILE_LOGGER_PATH "/custom/log/directory"
```

- **LOGIT_FILE_LOGGER_AUTO_DELETE_DAYS**: Defines the number of days after which old log files are deleted. If `LOGIT_FILE_LOGGER_AUTO_DELETE_DAYS` is not defined, it defaults to `30` days. You can set this to a custom value to control the log file retention policy.

```cpp
#define LOGIT_FILE_LOGGER_AUTO_DELETE_DAYS 60  // Keep logs for 60 days
```

- **LOGIT_FILE_LOGGER_PATTERN**: Defines the default log pattern for file-based loggers. This pattern controls the formatting of log messages written to log files, including timestamp, filename, line number, function, and thread information. If `LOGIT_FILE_LOGGER_PATTERN` is not defined, it defaults to `[%Y-%m-%d %H:%M:%S.%e] [%ffn:%#] [%!] [thread:%t] [%l] %v`.

```cpp
#define LOGIT_FILE_LOGGER_PATTERN "[%Y-%m-%d %H:%M:%S.%e] [%l] %v"
```

- **LOGIT_UNIQUE_FILE_LOGGER_PATH**: Defines the default directory path for unique log files. If LOGIT_UNIQUE_FILE_LOGGER_PATH is not defined, it defaults to "data/logs/unique_logs". You can specify a custom path for unique log files.

```cpp
#define LOGIT_UNIQUE_FILE_LOGGER_PATH "/custom/unique/log/directory"
```

- **LOGIT_UNIQUE_FILE_LOGGER_PATTERN**: Defines the default log pattern for unique file-based loggers. If LOGIT_UNIQUE_FILE_LOGGER_PATTERN is not defined, it defaults to "%v". You can customize this pattern to control the format of log messages in unique files.

```cpp
#define LOGIT_UNIQUE_FILE_LOGGER_PATTERN "[%Y-%m-%d %H:%M:%S.%e] [%l] %v"
```

- **LOGIT_UNIQUE_FILE_LOGGER_HASH_LENGTH**: Defines the length of the hash used in unique log file names. If LOGIT_UNIQUE_FILE_LOGGER_HASH_LENGTH is not defined, it defaults to 8 characters. This ensures that unique filenames are generated for each log entry.

```cpp
#define LOGIT_UNIQUE_FILE_LOGGER_HASH_LENGTH 12  // Set hash length to 12 characters
```

- **LOGIT_SHORT_NAME**: Enables short names for logging macros, such as `LOG_T`, `LOG_D`, `LOG_E`, etc., for more concise logging statements.

- **LOGIT_USE_FMT_LIB**: Enables the use of the fmt library for string formatting.

## Example Format
To define a custom format for your log messages, you can use the following method:

```cpp
LOGIT_ADD_LOGGER(
	logit::ConsoleLogger, (), 
	logit::SimpleLogFormatter, 
	("[%Y-%m-%d %H:%M:%S.%e] [%ffn:%#] [%!] [thread:%t] [%l] %^%v%$"));

// or...

logit::Logger::get_instance().add_logger(
    std::make_unique<logit::ConsoleLogger>(),
    std::make_unique<logit::SimpleLogFormatter>("[%Y-%m-%d %H:%M:%S.%e] [%ffn:%#] [%!] [thread:%t] [%l] %^%v%$"));
```

## Custom Logger Backend and Formatter
You can extend LogIt++ by implementing your own loggers and formatters. Here’s how:

### Custom Logger Example

```cpp
#include <fstream>
#include <mutex>
#include <log-it/LogIt.hpp>

class FileLogger : public logit::ILogger {
public:
    FileLogger(const std::string& file_name) : m_file_name(file_name) {
        m_log_file.open(file_name, std::ios::out | std::ios::app);
    }

    ~FileLogger() {
        if (m_log_file.is_open()) {
            m_log_file.close();
        }
    }

    void log(const logit::LogRecord& record, const std::string& message) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_log_file.is_open()) {
            m_log_file << message << std::endl;
        }
    }

    void wait() override {}

private:
    std::string m_file_name;
    std::ofstream m_log_file;
    std::mutex m_mutex;
};
```

### Custom Formatter Example

```cpp
#include <log-it/LogIt.hpp>
#include <json/json.h>

class JsonLogFormatter : public logit::ILogFormatter {
public:
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
```

## Installation
LogIt++ is a header-only library. To integrate it into your project, follow these steps:

1. Clone the repository with its submodules:

```bash
git clone --recurse-submodules https://github.com/NewYaroslav/log-it-cpp.git
```
2. Include the LogIt++ headers in your project:

```cpp
#include <log-it/LogIt.hpp>
```

3. Set up include paths for dependencies like time-shield-cpp.

LogIt++ depends on **time-shield-cpp**, which is located in the `libs` folder as a submodule. Ensure that the path to `libs\time-shield-cpp\include` is added to your project's include directories.
If you are using an IDE like **Visual Studio** or **CLion**, you can add the include path in the project settings.

4. (Optional) Enable fmt support:

LogIt++ supports the **fmt** library for advanced string formatting, which is also included as a submodule. To enable `fmt` in LogIt++, define the macro `LOGIT_USE_FMT_LIB` in your project:

```cpp
#define LOGIT_USE_FMT_LIB
```

## Documentation

Detailed documentation for LogIt++, including API reference and usage examples, can be found [here](https://newyaroslav.github.io/log-it-cpp/).


## License
This library is licensed under the MIT License. See the [LICENSE](https://github.com/NewYaroslav/log-it-cpp/blob/main/LICENSE) file in the repository for more details.