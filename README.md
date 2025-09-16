# LogIt++ Library
![LogIt++ Logo](docs/logo-640x320.png)

[![MIT License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS%20%7C%20Emscripten-blue)
![C++ Standard](https://img.shields.io/badge/C++-11--17-orange)
![CI Windows](https://img.shields.io/github/actions/workflow/status/NewYaroslav/log-it-cpp/ci.yml?branch=main&label=Windows&logo=windows)
![CI Linux](https://img.shields.io/github/actions/workflow/status/NewYaroslav/log-it-cpp/ci.yml?branch=main&label=Linux&logo=linux)
![CI macOS](https://img.shields.io/github/actions/workflow/status/NewYaroslav/log-it-cpp/ci.yml?branch=main&label=macOS&logo=apple)

[Читать на русском](README-RU.md)

## Overview

**LogIt++** is a macro-first C++ logging library that supports C++11 and newer toolchains. It pairs lightweight instrumentation macros with configurable backends (console, rotating files, syslog, Windows Event Log, or custom sinks) and routes messages through an asynchronous queue so applications remain responsive while recording detailed diagnostics. The library combines the convenience of macro-driven logging similar to **IceCream-Cpp** with the configurability of engines such as **spdlog**.

Key characteristics:

- **Macro-oriented API.** Consistent macro families (`LOGIT_<LEVEL>`, `LOGIT_PRINTF_<LEVEL>`, `LOGIT_STREAM_<LEVEL>`, etc.) cover immediate messages, printf-style formatting, streaming, throttling, and tagging. Defining `LOGIT_SHORT_NAME` when including `<logit.hpp>` enables compact aliases like `LOG_I`, `LOG_WPF`, and `LOG_S_INFO`.
- **Flexible formatting and routing.** Customize output patterns, mix console/file/system backends, or supply custom logger implementations.
- **Async by default.** Each backend is served by the task executor with configurable queue sizes and overflow policies, plus helpers such as `LOGIT_WARN_ONCE` or `LOGIT_ERROR_THROTTLE` to keep repeated messages under control.

See the macro examples below or browse the `examples/` folder for focused demonstrations, including queue tuning and crash handling.

## Macro Examples

### Long-form macros

```cpp
#include <logit.hpp>

int main() {
    LOGIT_ADD_CONSOLE_DEFAULT();
    LOGIT_SET_MAX_QUEUE(32);
    LOGIT_SET_QUEUE_POLICY(LOGIT_QUEUE_DROP);

    const bool verbose = true;
    int attempt = 1;
    double latency_ms = 12.5;

    LOGIT_TRACE0();
    LOGIT_DEBUG_IF(verbose, "Verbose diagnostics enabled");
    LOGIT_INFO("Starting service", attempt);
    LOGIT_WARN_ONCE("initializing subsystem");
    LOGIT_ERROR_EVERY_N(3, "retrying connection", attempt);
    LOGIT_ERROR_THROTTLE(250, "still failing");
    LOGIT_PRINTF_WARN("Latency %.2f ms", latency_ms);
    LOGIT_FORMAT_INFO("%.2f", 1.23f, 4.56f);
    LOGIT_INFO_TAG(({{"order_id", 123}, {"side", "BUY"}}), "sent order");
    LOGIT_STREAM_INFO() << "Streaming value: " << attempt;

    LOGIT_WAIT();
}
```

### Short aliases

Define `LOGIT_SHORT_NAME` before including `<logit.hpp>` to enable single-letter level prefixes:

```cpp
#define LOGIT_SHORT_NAME
#include <logit.hpp>

void short_names_demo() {
    LOGIT_ADD_CONSOLE_DEFAULT(); // call once during initialization

    int attempt = 2;

    LOG_I("Short alias for info");
    LOG_IPF("Attempt %d finished", attempt);
    LOG_W("Warning alias");
    LOG_WPF("Retry %d/3", attempt);
    LOG_S_INFO() << "Streaming alias " << attempt;
}
```

For a standalone program that brings everything together and intentionally aborts after logging a fatal message, check `examples/example_logit_minimal_crash.cpp`.

### System error helpers

`LOGIT_SYSERR_<LEVEL>` captures the current `errno` (or `GetLastError()` on Windows) and appends the decoded information to the message, so failure details stay attached to the original context. The lower-level `LOGIT_PERROR_<LEVEL>` and `LOGIT_WINERR_<LEVEL>` families are also available if you want to explicitly choose the platform macro.

```cpp
#include <logit.hpp>
#include <fcntl.h>

int main() {
    LOGIT_ADD_CONSOLE_DEFAULT();

    if (::open("missing.cfg", O_RDONLY) == -1) {
        LOGIT_SYSERR_ERROR("Failed to open configuration");
    }

    LOGIT_WAIT();
}
```

The error suffix can be customised at compile time via `config.hpp` macros:

```cpp
#define LOGIT_OS_ERROR_JOIN " <- "
#define LOGIT_POSIX_ERROR_PATTERN "[%s] errno=%d (%s)"
#define LOGIT_WINDOWS_ERROR_PATTERN "[%s] GetLastError=%lu (%s)"
#include <logit.hpp>

// ... later in the code ...
LOGIT_SYSERR_ERROR("Deleting temp directory failed");
```

---

## Features

- **Flexible Log Formatting**: 

Customize log message formats using patterns. You can redefine patterns via macros or specify them directly when adding a logger backend. Both standard format flags (`%H`, `%M`, `%S`, `%v`, etc.) and special ones like `%N([...])` for fallback logs without arguments are supported.

```
#define LOGIT_CONSOLE_PATTERN "%H:%M:%S.%e | %^%N([%!g:%#])%v%$"

try {
    throw std::runtime_error("An example runtime error");
} catch (const std::exception& ex) {
    LOGIT_FATAL(ex);
}

// Output:
> 23:59:59.128 | An example runtime error
```

 - **Macro-Based Logging**:

Easily log variables and messages using macros. Choose the macro that matches the desired formatting style:

* `LOGIT_PRINTF_<LEVEL>` mimics `printf`, where the format string controls each argument.
* `LOGIT_FORMAT_<LEVEL>` applies the same format to every argument in the list.

```
float someFloat = 123.456f;
int someInt = 789;
LOGIT_INFO(someFloat, someInt);

auto now = std::chrono::system_clock::now();
LOGIT_PRINT_INFO("TimePoint example: ", now);
LOGIT_PRINTF_INFO("%.2f %d", someFloat, someInt); // printf-style
LOGIT_FORMAT_INFO("%.2f", someFloat, 654.321f);   // same format for all args
```

- **Log Filters and Throttling**:

Reduce noise from repetitive messages with macros like `LOGIT_WARN_ONCE`,
`LOGIT_INFO_EVERY_N`, and `LOGIT_ERROR_THROTTLE`. Use the `_THROTTLE`
variants (e.g., `LOGIT_INFO_THROTTLE`) to limit output to one message per
time period.

```cpp
for (int i = 0; i < 10; ++i) {
    LOGIT_WARN_ONCE("initializing");                     // prints once
    LOGIT_INFO_EVERY_N(3, "heartbeat", i);               // every 3rd call
    LOGIT_ERROR_THROTTLE(200, "repeated error");         // max once/200ms
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}
```

- **Tagged Logging**:

Attach simple key-value attributes for easier filtering in log aggregators.

```cpp
LOGIT_INFO_TAG(({{"order_id", 123}, {"side", "BUY"}}), "sent order");
// Output: [info] sent order order_id=123 side=BUY
```

- **Rotating File Logs**:

  Automatic file rotation based on size with optional asynchronous compression using gzip or zstd.

- **Support for Multiple Backends**:

Easily configure loggers for console and file output. If necessary, add support for sending messages to servers or databases by creating custom backends.

```
// Adding three backends: console, file, and unique file loggers
LOGIT_ADD_CONSOLE_DEFAULT();
LOGIT_ADD_FILE_LOGGER_DEFAULT();
LOGIT_ADD_UNIQUE_FILE_LOGGER_DEFAULT_SINGLE_MODE();
```

- **System Backends**:

Use the host OS logging facility. `SyslogLogger` works with POSIX `syslog`, while `EventLogLogger` writes to the Windows Event Log.

- **Asynchronous Logging**:

Improve application performance with asynchronous logging. All loggers handle messages in a separate thread by default.

- **Stream-Based Logging**: 

Use stream operators for complex messages.

```
LOGIT_STREAM_INFO() << "Stream-based info logging with short macro. Integer value: " << 123;
```

- **Extensibility**: 

Create custom loggers and formatters to meet your specific requirements.

```
class CustomLogger : public logit::ILogger {
public:
    CustomLogger() = default;

    /// brief Logs a message by formatting the log record and message.
    /// \param record The log record containing event details.
    /// \param message The formatted log message to log.
    void log(const logit::LogRecord& record, const std::string& message) override {
        // Implementation for sending logs...
    }

    ~CustomLogger() override = default;
};

LOGIT_ADD_LOGGER(CustomLogger, (), logit::SimpleLogFormatter, ("%v"));
```

---

## Usage

Here’s a simple example demonstrating how to use LogIt++ in your application. The task queue size and overflow behavior are configurable via `LOGIT_SET_MAX_QUEUE` and `LOGIT_SET_QUEUE_POLICY` (use `LOGIT_QUEUE_DROP` or `LOGIT_QUEUE_BLOCK`):

```cpp
#define LOGIT_SHORT_NAME
#include <logit.hpp>

int main() {
    // Initialize the logger with default console output
    LOGIT_ADD_CONSOLE_DEFAULT();
    LOGIT_SET_MAX_QUEUE(64);
    LOGIT_SET_QUEUE_POLICY(LOGIT_QUEUE_DROP);

    float a = 123.456f;
    int b = 789;
    int c = 899;
    const char* someStr = "Hello, World!";

    // Basic logging using macros
    LOG_INFO("Starting the application");
    LOG_DEBUG("Variable values", a, b);
    LOG_WARN("This is a warning message");

    // Formatted logging
    LOG_PRINTF_INFO("Formatted log: value of a = %.2f", a);
    LOG_FORMAT_WARN("%.4d", b, c);

    // Error and fatal logs
    LOG_ERROR("An error occurred", b);
    LOG_FATAL("Fatal error. Terminating application.");

    // Conditional logging
    LOG_ERROR_IF(b < 0, "Value of b is negative");
    LOG_WARN_IF(a > 100, "Value of a exceeds 100");

    // Stream-based logging with short and long names
    LOG_S_INFO() << "Logging a float: " << a << ", and an int: " << b;
    LOG_S_ERROR() << "Error occurred in the system";
    LOGIT_STREAM_WARN() << "Warning: potential issue detected with value: " << someStr;

    // Using LOGIT_TRACE for tracing function execution
    LOGIT_TRACE0();   // Trace without arguments
    LOG_PRINT_TRACE("Entering main function with variable a =", a);

    // Wait for all asynchronous logs to be processed
    LOGIT_WAIT();

    return 0;
}
```

For more usage examples, please refer to the `examples` folder in the repository, where you can find detailed demonstrations of various logging scenarios and configurations.

---

### Compile-Time Log Level

You can exclude lower-severity logs from the binary by specifying the maximum level to compile. Define the `LOGIT_COMPILED_LEVEL` macro during compilation:

```bash
g++ -DLOGIT_COMPILED_LEVEL=logit::LogLevel::LOG_LVL_WARN ...
```

With the example above, `TRACE`, `DEBUG`, and `INFO` macros are turned into no-ops at compile time.

---

## Log Format Customization

`LogIt++` supports customizable log message formatting using patterns that define how each message should appear. You can specify patterns through macros or provide them when adding logger backends.

### Format Example

Example of setting a custom format for the console logger:

```
LOGIT_ADD_LOGGER(
    logit::ConsoleLogger, (), 
    logit::SimpleLogFormatter, 
    ("%Y-%m-%d %H:%M:%S.%e [%l] %^%N(%g:%#)%v%$")
);
```

Or specify the pattern using macros:

```
#define LOGIT_CONSOLE_PATTERN "%H:%M:%S.%e | %^%N([%!g:%#])%v%$"
LOGIT_ADD_CONSOLE_DEFAULT();
```

The logger will automatically substitute the specified data into the template, for example:

```
23:59:59.128 | path/to/file.cpp:123 A sample log message
```

### Log Message Formatting Flags

`LogIt++` supports customizable log message formatting using flags. You can define how each log message should appear by including placeholders for various data, such as timestamps, log levels, file names, function names, and messages.

Below is a list of supported formatting flags:

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

    - `%f`, `%fn`, `%bs`: Base name of the source file
    - `%g`, `%ffn`: Full file path
    - `%#`: Line number
    - `%!`: Function name
    
- *Thread Flags*:

    - `%t`: Thread identifier
    
- *Color Flags*:

    - `%^`: Start color formatting
    - `%$`: End color formatting
    - `%SC`: Start removing color codes (Strip Color)
    - `%EC`: End removing color codes (End Color)

- *Message Flags*:

    - `%v`: The log message content
    - `%N(...)`: Used as a fallback when no arguments are provided (*e.g., in `LOG_TRACE0()` calls*). The pattern specified in parentheses will be used. Example: `%N(%g:%#)` will add the file name and line number if no message is provided.

### Alignment and Truncation Support

`LogIt++` allows message text formatting with width, alignment, and truncation:

- Alignment:
    - Left: Use the `-` sign before the width number, e.g., `%-10v`.
    - Center: Use the `=` sign before the width number, e.g., `%=10v`.
    - Right (default): `%10v`.

- Truncation:
    - The `!` symbol after the width number specifies that text should be truncated if it exceeds the specified length. Example: `%10!v`.

**Examples**:

- `%10v` – Right-align the message to 10 characters.
- `%-10v` – Left-align the message to 10 characters.
- `%10!v` – Truncate the message to 10 characters with right alignment.
- `%-10!v` – Truncate the message to 10 characters with left alignment.

### Advanced Path Handling

For file-related flags (`%f`, `%g`, `%@`), truncation ensures that the filename and the beginning of the path are preserved, replacing the middle portion with `...` if the width is smaller than the path length.

**Example:**

- Input: `/very/long/path/to/file.cpp`
- Truncated to width=15: `/very...file.cpp`

---

## Shortened Logging Macros

LogIt++ provides shortened versions of logging macros when `LOGIT_SHORT_NAME` is defined. These macros allow for concise logging across different log levels, including both standard and stream-based logging.

### Available TRACE-level macros:

- Basic logging:

	- `LOG_T(...)`: Logs a TRACE-level message.
	- `LOG_T0()`: Logs a TRACE-level message without arguments.
	- `LOG_0T()`: Alias for `LOG_T0()`.
	- `LOG_0_T()`: Alias for `LOG_T0()`.
	- `LOG_T_NOARGS()`: Alias for `LOG_T0()`.
	- `LOG_NOARGS_T()`: Alias for `LOG_T0()`.
	
- Formatted logging:

	- `LOG_TF(fmt, ...)`: Logs a formatted TRACE-level message using format strings.
	- `LOG_FT(fmt, ...)`: Alias for `LOG_TF(fmt, ...)`.
	- `LOG_T_PRINT(...)`: Logs a TRACE-level message by printing each argument.
	- `LOG_PRINT_T(...)`: Alias for `LOG_T_PRINT(...)`.
	- `LOG_T_PRINTF(fmt, ...)`: Logs a formatted TRACE-level message using printf-style formatting.
	- `LOG_PRINTF_T(fmt, ...)`: Alias for `LOG_T_PRINTF(fmt, ...)`.
	- `LOG_TP(...)`: Alias for `LOG_T_PRINT(...)`.
	- `LOG_PT(...)`: Alias for `LOG_T_PRINT(...)`.
	- `LOG_TPF(fmt, ...)`: Alias for `LOG_T_PRINTF(fmt, ...)`.
	- `LOG_PFT(fmt, ...)`: Alias for `LOG_T_PRINTF(fmt, ...)`.
	
- Alternative TRACE-level macros:

	- `LOG_TRACE(...)`: Logs a TRACE-level message (same as `LOG_T(...)`).
	- `LOG_TRACE0()`: Logs a TRACE-level message without arguments (same as `LOG_T0()`).
	- `LOG_0TRACE()`: Alias for `LOG_TRACE0()`.
	- `LOG_0_TRACE()`: Alias for `LOG_TRACE0()`.
	- `LOG_TRACE_NOARGS()`: Logs a TRACE-level message with no arguments (same as `LOG_T_NOARGS()`).
	- `LOG_NOARGS_TRACE()`: Alias for `LOG_TRACE_NOARGS()`.
	- `LOG_TRACEF(fmt, ...)`: Logs a formatted TRACE-level message (same as `LOG_TF(fmt, ...)`).
	- `LOG_FTRACE(fmt, ...)`: Alias for `LOG_TRACEF(fmt, ...)`.
	- `LOG_TRACE_PRINT(...)`: Logs a TRACE-level message by printing each argument (same as `LOG_T_PRINT(...)`).
	- `LOG_PRINT_TRACE(...)`: Alias for `LOG_TRACE_PRINT(...)`.
	- `LOG_TRACE_PRINTF(fmt, ...)`: Logs a formatted TRACE-level message using printf-style formatting (same as `LOG_T_PRINTF(fmt, ...)`).
	- `LOG_PRINTF_TRACE(fmt, ...)`: Alias for `LOG_TRACE_PRINTF(fmt, ...)`.

These macros provide flexibility and convenience when logging messages at the TRACE level. They allow you to choose between different logging styles, such as standard logging, formatted logging, and printing each argument separately.

**Note**: Similar macros are available for other log levels — **INFO** (`LOG_I`, `LOG_INFO`), **DEBUG** (`LOG_D`, `LOG_DEBUG`), **WARN** (`LOG_W`, `LOG_WARN`), **ERROR** (`LOG_E`, `LOG_ERROR`), and **FATAL** (`LOG_F`, `LOG_FATAL`). The naming conventions are consistent across levels, you only need to replace the level letter or word in the macro name.

- Example:

```cpp
LOG_T("Trace message using short macro");
LOG_TF("%.4d", 999);
LOG_T_PRINT("Printing trace message with multiple variables: ", var1, var2);
LOG_TRACE("Trace message (alias for LOG_T)");
LOG_TRACE_PRINTF("Formatted trace: value = %d", value);
```

---

## Configuration Macros

LogIt++ provides several macros that allow for customization and configuration. Below are the available configuration macros:

- **LOGIT_BASE_PATH**: Defines the base path used for log file paths. If `LOGIT_BASE_PATH` is not defined or is empty (`{}`), the full path from `__FILE__` will be used for log file paths. You can override this to specify a custom base path for your log files.

```cpp
#define LOGIT_BASE_PATH "/path/to/your/project"
```

- **LOGIT_DEFAULT_COLOR**: Defines the default color for console output. If `LOGIT_DEFAULT_COLOR` is not defined, it defaults to `TextColor::LightGray`. You can set a custom console text color by overriding this macro.

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

- **LOGIT_FILE_LOGGER_PATTERN**: Defines the default log pattern for file-based loggers. This pattern controls the formatting of log messages written to log files, including timestamp, filename, line number, function, and thread information. If `LOGIT_FILE_LOGGER_PATTERN` is not defined, it defaults to `[%Y-%m-%d %H:%M:%S.%e] [%ffn:%#] [%!] [thread:%t] [%l] %SC%v`.

```cpp
#define LOGIT_FILE_LOGGER_PATTERN "[%Y-%m-%d %H:%M:%S.%e] [%l] %SC%v"
```

- **LOGIT_UNIQUE_FILE_LOGGER_PATH**: Defines the default directory path for unique log files. If `LOGIT_UNIQUE_FILE_LOGGER_PATH` is not defined, it defaults to *"data/logs/unique_logs"*. You can specify a custom path for unique log files.

```cpp
#define LOGIT_UNIQUE_FILE_LOGGER_PATH "/custom/unique/log/directory"
```

- **LOGIT_UNIQUE_FILE_LOGGER_PATTERN**: Defines the default log pattern for unique file-based loggers. If `LOGIT_UNIQUE_FILE_LOGGER_PATTERN` is not defined, it defaults to `"%v"`. You can customize this pattern to control the format of log messages in unique files.

```cpp
#define LOGIT_UNIQUE_FILE_LOGGER_PATTERN "%v"
```

- **LOGIT_UNIQUE_FILE_LOGGER_HASH_LENGTH**: Defines the length of the hash used in unique log file names. If `LOGIT_UNIQUE_FILE_LOGGER_HASH_LENGTH` is not defined, it defaults to `8` characters. This ensures that unique filenames are generated for each log entry.

```cpp
#define LOGIT_UNIQUE_FILE_LOGGER_HASH_LENGTH 12	 // Set hash length to 12 characters
```

- **LOGIT_SHORT_NAME**: Enables short names for logging macros, such as `LOG_T`, `LOG_D`, `LOG_E`, etc., for more concise logging statements.


---

## Custom Logger Backend and Formatter

You can extend LogIt++ by implementing your own loggers and formatters. Here’s how:

### Custom Logger Example

```cpp
#include <fstream>
#include <mutex>
#include <logit.hpp>

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
#include <logit.hpp>
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

## Macro Reference

| Macro pattern | Description |
| ------------- | ----------- |
| `LOGIT_<LEVEL>(...)` | Log a message with the given level (`TRACE`, `DEBUG`, `INFO`, `WARN`, `ERROR`, `FATAL`). |
| `LOGIT_PRINT_<LEVEL>(...)` | Log a pre-formatted string or stream-built message. |
| `LOGIT_PRINTF_<LEVEL>(fmt, ...)` | `printf`-style formatting with placeholders for each argument. |
| `LOGIT_FORMAT_<LEVEL>(fmt, ...)` | Apply the same format string to every argument. |
| `LOGIT_STREAM_<LEVEL>()` | Stream-style logging with `<<` operators; short aliases `LOG_S_<LEVEL>()` when `LOGIT_SHORT_NAME` is defined. |
| `LOGIT_<LEVEL>_IF(condition, ...)` | Log only when `condition` is true. |
| `LOGIT_<LEVEL>_ONCE(...)` | Log only the first time the macro is executed. |
| `LOGIT_<LEVEL>_EVERY_N(n, ...)` | Log on every `n`th invocation. |
| `LOGIT_<LEVEL>_THROTTLE(period_ms, ...)` | Log at most once per `period_ms` milliseconds. |
| `LOGIT_<LEVEL>_TAG(({{"k", "v"}}), msg)` | Attach key-value tags to a message. |

### Configuration Macros

| Macro | Description |
| ----- | ----------- |
| `LOGIT_BASE_PATH` | Trim this prefix from `__FILE__` paths shown in logs. |
| `LOGIT_DEFAULT_COLOR` | Default console color for messages. |
| `LOGIT_COLOR_<LEVEL>` | Color for each log level. |
| `LOGIT_CONSOLE_PATTERN` | Default format pattern for console output. |
| `LOGIT_FILE_LOGGER_PATH` | Directory for rotating file logs. |
| `LOGIT_UNIQUE_FILE_LOGGER_PATH` | Directory for one-message-per-file logs. |
| `LOGIT_TAGS_JOIN` | Separator inserted between the message and tag list. |

### Management Macros

| Macro | Description |
| ----- | ----------- |
| `LOGIT_SET_MAX_QUEUE(size)` | Limit the asynchronous task queue (0 for unlimited). |
| `LOGIT_SET_QUEUE_POLICY(mode)` | Set overflow behavior: `LOGIT_QUEUE_DROP_NEWEST`, `LOGIT_QUEUE_DROP_OLDEST`, or `LOGIT_QUEUE_BLOCK`. |
| `LOGIT_SET_LOG_LEVEL_TO(index, level)` | Set minimum log level for a specific logger. |
| `LOGIT_SET_LOG_LEVEL(level)` | Set minimum log level for all loggers. |
| `LOGIT_SET_LOGGER_ENABLED(index, enabled)` | Enable or disable a logger. |
| `LOGIT_IS_LOGGER_ENABLED(index)` | Check whether a logger is enabled. |
| `LOGIT_SET_SINGLE_MODE(index, single_mode)` | Toggle single-message-per-file mode for a logger. |
| `LOGIT_IS_SINGLE_MODE(index)` | Determine if a logger is in single mode. |
| `LOGIT_SET_TIME_OFFSET(index, offset_ms)` | Adjust timestamp offset for a logger. |
| `LOGIT_GET_STRING_PARAM(index, param)` | Retrieve a string parameter from a logger. |
| `LOGIT_GET_INT_PARAM(index, param)` | Retrieve an integer parameter from a logger. |
| `LOGIT_GET_FLOAT_PARAM(index, param)` | Retrieve a floating-point parameter from a logger. |
| `LOGIT_GET_LAST_FILE_NAME(index)` | Get the last file name written by a logger. |
| `LOGIT_GET_LAST_FILE_PATH(index)` | Get the last file path written by a logger. |
| `LOGIT_GET_LAST_LOG_TIMESTAMP(index)` | Get the timestamp of the last log entry. |
| `LOGIT_GET_TIME_SINCE_LAST_LOG(index)` | Seconds elapsed since the last log entry. |
| `LOGIT_WAIT()` | Wait for all asynchronous loggers to finish. |
| `LOGIT_SHUTDOWN()` | Shut down the logging system. |

---

## Installation

LogIt++ is a header-only library. To integrate it into your project, follow these steps:

1. Clone the repository with its submodules:

```bash
git clone --recurse-submodules https://github.com/NewYaroslav/log-it-cpp.git
```
2. Include the LogIt++ headers in your project:

```cpp
#include <logit.hpp>
```

3. Set up include paths for dependencies like time-shield-cpp.

LogIt++ depends on **time-shield-cpp**, which is located in the `libs` folder as a submodule. Ensure that the path to `libs\time-shield-cpp\include` is added to your project's include directories.
If you are using an IDE like **Visual Studio** or **CLion**, you can add the include path in the project settings.

4. (Optional) Enable fmt-style macros:

LogIt++ includes the *fmt* library for `{}`-based formatting. To use the `LOGIT_FMT_*` and `LOGIT_SCOPE_FMT_*` macros, build the library with the CMake option `-DLOGIT_WITH_FMT=ON`.

## System Backends

LogIt++ can forward messages to system logging facilities.

### Syslog (Unix)

Available when `LOGIT_WITH_SYSLOG=ON` on Unix-like systems. Log levels are mapped as follows: TRACE/DEBUG → `LOG_DEBUG`, INFO → `LOG_INFO`, WARN → `LOG_WARNING`, ERROR/FATAL → `LOG_ERR`/`LOG_CRIT`.

```cpp
LOGIT_ADD_SYSLOG_DEFAULT();
LOGIT_INFO("Syslog is alive");
```

### Windows Event Log

Enabled with `LOGIT_WITH_WIN_EVENT_LOG=ON` on Windows. Levels map TRACE/DEBUG/INFO → `INFORMATION`, WARN → `WARNING`, ERROR/FATAL → `ERROR`.

```cpp
LOGIT_ADD_EVENT_LOG_DEFAULT();
LOGIT_ERROR("Something went wrong");
```

Both loggers compile to no-ops on unsupported platforms.

### Emscripten

When building with Emscripten the library runs without threads. Console logging
works as usual while file-based loggers are replaced by stubs that warn when
used.

---

## Documentation

Detailed documentation for LogIt++, including API reference and usage examples, can be found [here](https://newyaroslav.github.io/log-it-cpp/).

---

## License
This library is licensed under the MIT License. See the [LICENSE](https://github.com/NewYaroslav/log-it-cpp/blob/main/LICENSE) file in the repository for more details.
