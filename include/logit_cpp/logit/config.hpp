#pragma once
#ifndef _LOGIT_CONFIG_HPP_INCLUDED
#define _LOGIT_CONFIG_HPP_INCLUDED

/// \file config.hpp
/// \brief Configuration macros for the LogIt logging system.

/// \ingroup ConfigMacros Configuration Macros
/// \{

#ifndef LOGIT_BASE_PATH
    /// \brief Defines the base path used for log file paths.
    /// If `LOGIT_BASE_PATH` is not defined or is empty (`{}`), the full path from `__FILE__` will be used for log file paths.
    #define LOGIT_BASE_PATH {}
#endif

#ifndef LOGIT_DEFAULT_COLOR
    /// \brief Defines the default color for console output.
    /// If LOGIT_DEFAULT_COLOR is not defined, defaults to `logit::TextColor::LightGray`.
    ///
    /// This macro allows setting a default console text color for log messages.
    #define LOGIT_DEFAULT_COLOR logit::TextColor::LightGray
#endif

/// \brief Defines the sleep duration (in microseconds) used by TaskExecutor when blocking producers.
///
/// When `QueuePolicy::Block` is active, the TaskExecutor periodically waits for
/// capacity to become available. Override this value to tweak the polling
/// cadence in builds where the default wait is not appropriate.
#ifndef LOGIT_TASK_EXECUTOR_BLOCK_WAIT_USEC
    #define LOGIT_TASK_EXECUTOR_BLOCK_WAIT_USEC 200
#endif

/// \name Log Level Colors
/// Default colors for each log level.
/// \{
    
#ifndef LOGIT_COLOR_TRACE
    #define LOGIT_COLOR_TRACE   logit::TextColor::DarkGray
#endif

#ifndef LOGIT_COLOR_DEBUG
    #define LOGIT_COLOR_DEBUG   logit::TextColor::Blue
#endif

#ifndef LOGIT_COLOR_INFO
    #define LOGIT_COLOR_INFO    logit::TextColor::Green
#endif

#ifndef LOGIT_COLOR_WARN
    #define LOGIT_COLOR_WARN    logit::TextColor::Yellow
#endif

#ifndef LOGIT_COLOR_ERROR
    #define LOGIT_COLOR_ERROR   logit::TextColor::Red
#endif

#ifndef LOGIT_COLOR_FATAL
    #define LOGIT_COLOR_FATAL   logit::TextColor::Magenta
#endif

#ifndef LOGIT_COLOR_DEFAULT
    #define LOGIT_COLOR_DEFAULT logit::TextColor::White
#endif
/// \}

#ifndef LOGIT_WALLCLOCK_MS
#define LOGIT_WALLCLOCK_MS() \
  (std::chrono::duration_cast<std::chrono::milliseconds>( \
     std::chrono::system_clock::now().time_since_epoch()).count())
#endif

#ifndef LOGIT_MONOTONIC_MS
#define LOGIT_MONOTONIC_MS() \
  (std::chrono::duration_cast<std::chrono::milliseconds>( \
     std::chrono::steady_clock::now().time_since_epoch()).count())
#endif

/// \brief Macro to get the current timestamp in milliseconds.
/// If LOGIT_CURRENT_TIMESTAMP_MS is not defined, it uses `std::chrono` to return the current time in milliseconds.
///
/// This macro can be overridden to provide a custom method for generating timestamps if needed.
#ifndef LOGIT_CURRENT_TIMESTAMP_MS
#define LOGIT_CURRENT_TIMESTAMP_MS() LOGIT_WALLCLOCK_MS()
#endif

/// \name File Logger Settings
/// Configuration macros for file-based loggers.
/// \{

/// \brief Defines the default log pattern for the console logger.
/// If `LOGIT_CONSOLE_PATTERN` is not defined, it defaults to `%%H:%%M:%%S.%%e | %^%N([%50!g:%#])%%v%$`.
///
/// This pattern controls the formatting of log messages sent to the console, including timestamp, message, and color.
#ifndef LOGIT_CONSOLE_PATTERN
    #define LOGIT_CONSOLE_PATTERN "%H:%M:%S.%e | %^%N([%50!g:%#])%v%$"
#endif

/// \brief Defines the default directory path for log files.
/// If `LOGIT_FILE_LOGGER_PATH` is not defined, it defaults to "data/logs".
///
/// This macro specifies the directory where regular log files will be stored.
/// The default path is relative to the application's execution directory.
#ifndef LOGIT_FILE_LOGGER_PATH
    #define LOGIT_FILE_LOGGER_PATH "data/logs"
#endif

/// \brief Defines the default directory path for unique log files.
/// If `LOGIT_UNIQUE_FILE_LOGGER_PATH` is not defined, it defaults to "data/logs/unique_logs".
///
/// This macro specifies the directory where unique log files, created by `UniqueFileLogger`,
/// will be stored. Each log message will generate a new file in this directory.
#ifndef LOGIT_UNIQUE_FILE_LOGGER_PATH
    #define LOGIT_UNIQUE_FILE_LOGGER_PATH "data/logs/unique_logs"
#endif

/// \brief Defines the number of days after which old log files are deleted.
/// If `LOGIT_FILE_LOGGER_AUTO_DELETE_DAYS` is not defined, it defaults to 30 days.
///
/// This macro controls the log file retention policy by specifying the maximum age of log files.
#ifndef LOGIT_FILE_LOGGER_AUTO_DELETE_DAYS
    #define LOGIT_FILE_LOGGER_AUTO_DELETE_DAYS 30
#endif

/// \brief Defines the default log pattern for file-based loggers.
/// If `LOGIT_FILE_LOGGER_PATTERN` is not defined, it defaults to `[%%Y-%%m-%%d %%H:%%M:%%S.%%e] [%-5l] [%60!@] [thread:%%t] %%SC%%v`.
///
/// This pattern controls the formatting of log messages written to log files, including timestamp, filename, line number, function, and thread information.
#ifndef LOGIT_FILE_LOGGER_PATTERN
    #define LOGIT_FILE_LOGGER_PATTERN "[%Y-%m-%d %H:%M:%S.%e] [%-5l] [%60!@] [thread:%t] %SC%v"
#endif

#ifndef LOGIT_FILE_LOGGER_MAX_FILE_SIZE_BYTES
    #define LOGIT_FILE_LOGGER_MAX_FILE_SIZE_BYTES 0
#endif
#ifndef LOGIT_FILE_LOGGER_MAX_ROTATED_FILES
    #define LOGIT_FILE_LOGGER_MAX_ROTATED_FILES 0
#endif

/// \brief Defines the default log pattern for unique file-based loggers.
/// If `LOGIT_UNIQUE_FILE_LOGGER_PATTERN` is not defined, it defaults to "%v".
#ifndef LOGIT_UNIQUE_FILE_LOGGER_PATTERN
    #define LOGIT_UNIQUE_FILE_LOGGER_PATTERN "%v"
#endif

/// \brief Defines the default hash length for unique file names.
/// If `LOGIT_UNIQUE_FILE_LOGGER_HASH_LENGTH` is not defined, it defaults to 8.
#ifndef LOGIT_UNIQUE_FILE_LOGGER_HASH_LENGTH
    #define LOGIT_UNIQUE_FILE_LOGGER_HASH_LENGTH 8
#endif

/// \}

/// \name Tag formatting
/// Configuration of how tags are rendered after the message.
/// \{

/// \brief Separator inserted between the main message and the tag list (e.g., " | ").
#ifndef LOGIT_TAGS_JOIN
#define LOGIT_TAGS_JOIN " | "
#endif

/// \brief Separator appended before platform error metadata when formatting system error messages.
/// Override to customize how human-readable messages and error codes are joined in log output.
#ifndef LOGIT_OS_ERROR_JOIN
#define LOGIT_OS_ERROR_JOIN " | "
#endif

/// \brief Format string used when appending POSIX `errno` information to a log message.
/// This macro can be overridden to change how the user message, errno value, and description are rendered.
#ifndef LOGIT_POSIX_ERROR_PATTERN
#define LOGIT_POSIX_ERROR_PATTERN "%s" LOGIT_OS_ERROR_JOIN "errno=%d (%s)"
#endif

/// \brief Format string used when appending Windows `GetLastError` details to a log message.
/// This macro can be overridden to adjust how the original text, numeric code, and decoded message are displayed.
#ifndef LOGIT_WINDOWS_ERROR_PATTERN
#define LOGIT_WINDOWS_ERROR_PATTERN "%s" LOGIT_OS_ERROR_JOIN "GetLastError=%lu (%s)"
#endif

/// \brief Selects the default system error formatting macro for the current platform.
/// Users may redefine this alias to integrate custom error formatting logic.
#if defined(_WIN32)
#ifndef LOGIT_SYSTEM_ERROR_PATTERN
#define LOGIT_SYSTEM_ERROR_PATTERN LOGIT_WINDOWS_ERROR_PATTERN
#endif
#else
#ifndef LOGIT_SYSTEM_ERROR_PATTERN
#define LOGIT_SYSTEM_ERROR_PATTERN LOGIT_POSIX_ERROR_PATTERN
#endif
#endif

/// \brief Separator between individual tag pairs (e.g., " " or "; ").
#ifndef LOGIT_TAG_PAIR_SEP
#define LOGIT_TAG_PAIR_SEP " "
#endif

/// \brief Separator between a tag key and its value.
#ifndef LOGIT_TAG_KV_SEP
#define LOGIT_TAG_KV_SEP "="
#endif

/// \brief When 1, quote values that contain spaces or special characters; 0 disables quoting.
#ifndef LOGIT_TAG_QUOTE_VALUES
#define LOGIT_TAG_QUOTE_VALUES 1
#endif

/// \}

/// \name Task executor settings
/// Configuration options for the task executor implementation.
/// \{

/// \brief Maximum number of tasks drained per worker iteration in ring-buffer builds.
/// If `LOGIT_TASK_EXECUTOR_DRAIN_BUDGET` is not defined, the worker drains up to 2048
/// tasks before yielding. Increase the value to process larger bursts before sleeping,
/// or reduce it to prioritise lower per-iteration latency.
#ifndef LOGIT_TASK_EXECUTOR_DRAIN_BUDGET
#define LOGIT_TASK_EXECUTOR_DRAIN_BUDGET 2048
#endif

/// \brief Default capacity for the task executor ring buffer when unlimited is requested.
#ifndef LOGIT_TASK_EXECUTOR_DEFAULT_RING_CAPACITY
#define LOGIT_TASK_EXECUTOR_DEFAULT_RING_CAPACITY 1024
#endif

/// \}


/// \}

#endif // _LOGIT_CONFIG_HPP_INCLUDED
