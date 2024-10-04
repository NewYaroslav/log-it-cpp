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
/// If `LOGIT_CONSOLE_PATTERN` is not defined, it defaults to "%H:%M:%S.%e | %^%v%$".
///
/// This pattern controls the formatting of log messages sent to the console, including timestamp, message, and color.
#ifndef LOGIT_CONSOLE_PATTERN
    #define LOGIT_CONSOLE_PATTERN "%H:%M:%S.%e | %^%v%$"
#endif

/// \brief Defines the default directory path for log files.
/// If `LOGIT_FILE_LOGGER_PATH` is not defined, it defaults to "data/logs".
///
/// This macro specifies the directory where log files will be stored.
#ifndef LOGIT_FILE_LOGGER_PATH
    #define LOGIT_FILE_LOGGER_PATH "data/logs"
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
/// "[%Y-%m-%d %H:%M:%S.%e] [%ffn:%#] [%!] [thread:%t] [%l] %v".
///
/// This pattern controls the formatting of log messages written to log files, including timestamp, filename, line number, function, and thread information.
#ifndef LOGIT_FILE_LOGGER_PATTERN
    #define LOGIT_FILE_LOGGER_PATTERN "[%Y-%m-%d %H:%M:%S.%e] [%ffn:%#] [%!] [thread:%t] [%l] %v"
#endif

#endif // _LOGIT_CONFIG_HPP_INCLUDED
