#pragma once
#ifndef _LOGIT_CONFIG_HPP_INCLUDED
#define _LOGIT_CONFIG_HPP_INCLUDED
/// \file LogItConfig.hpp
/// \brief Configuration macros for the LogIt logging system.

/// \brief Defines the base path used for log file paths.
/// If `LOGIT_BASE_PATH` is not defined or is empty (`{}`), the full path from `__FILE__` will be used for log file paths.
#ifndef LOGIT_BASE_PATH
    #define LOGIT_BASE_PATH {}
#endif

/// \brief Defines the default color for console output.
/// If LOGIT_DEFAULT_COLOR is not defined, defaults to `TextColor::LightGray`.
///
/// This macro allows setting a default console text color for log messages.
#ifndef LOGIT_DEFAULT_COLOR
    #define LOGIT_DEFAULT_COLOR TextColor::LightGray
#endif

#ifndef LOGIT_COLOR_TRACE
    #define LOGIT_COLOR_TRACE   TextColor::DarkGray
#endif

#ifndef LOGIT_COLOR_DEBUG
    #define LOGIT_COLOR_DEBUG   TextColor::Blue
#endif

#ifndef LOGIT_COLOR_INFO
    #define LOGIT_COLOR_INFO    TextColor::Green
#endif

#ifndef LOGIT_COLOR_WARN
    #define LOGIT_COLOR_WARN    TextColor::Yellow
#endif

#ifndef LOGIT_COLOR_ERROR
    #define LOGIT_COLOR_ERROR   TextColor::Red
#endif

#ifndef LOGIT_COLOR_FATAL
    #define LOGIT_COLOR_FATAL   TextColor::Magenta
#endif

#ifndef LOGIT_COLOR_DEFAULT
    #define LOGIT_COLOR_DEFAULT TextColor::White
#endif

/// \brief Macro to get the current timestamp in milliseconds.
/// If LOGIT_CURRENT_TIMESTAMP_MS is not defined, it uses `std::chrono` to return the current time in milliseconds.
///
/// This macro can be overridden to provide a custom method for generating timestamps if needed.
#ifndef LOGIT_CURRENT_TIMESTAMP_MS
    #define LOGIT_CURRENT_TIMESTAMP_MS() \
        (std::chrono::duration_cast<std::chrono::milliseconds>( \
        std::chrono::system_clock::now().time_since_epoch()).count())
#endif

/// \brief Defines the default log pattern for the console logger.
/// If `LOGIT_CONSOLE_PATTERN` is not defined, it defaults to "%H:%M:%S.%e | %^%N([%50!g:%#])%v%$".
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
/// If `LOGIT_FILE_LOGGER_PATTERN` is not defined, it defaults to
/// "[%Y-%m-%d %H:%M:%S.%e] [%ffn:%#] [%!] [thread:%t] [%l] %SC%v".
///
/// This pattern controls the formatting of log messages written to log files, including timestamp, filename, line number, function, and thread information.
#ifndef LOGIT_FILE_LOGGER_PATTERN
    #define LOGIT_FILE_LOGGER_PATTERN "[%Y-%m-%d %H:%M:%S.%e] [%ffn:%#] [%!] [thread:%t] [%l] %SC%v"
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

#endif // _LOGIT_CONFIG_HPP_INCLUDED
