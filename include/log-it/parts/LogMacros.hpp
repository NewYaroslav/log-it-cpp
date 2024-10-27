#pragma once
#ifndef _LOGIT_LOG_MACROS_HPP_INCLUDED
#define _LOGIT_LOG_MACROS_HPP_INCLUDED
/// \file LogMacros.hpp
/// \brief Provides various logging macros for different log levels and options.

//------------------------------------------------------------------------------
// Function name macro for different compilers
#if defined(__GNUC__)
    #define LOGIT_FUNCTION __PRETTY_FUNCTION__
#elif defined(_MSC_VER)
    #define LOGIT_FUNCTION __FUNCSIG__
#else
    #define LOGIT_FUNCTION __func__
#endif

//------------------------------------------------------------------------------
// Stream-based logging macros for various levels

#define LOGIT_STREAM(level) \
    logit::LogStream(level, logit::make_relative(__FILE__, LOGIT_BASE_PATH), __LINE__, LOGIT_FUNCTION, -1)

#define LOGIT_STREAM_WITH_INDEX(level, index) \
    logit::LogStream(level, logit::make_relative(__FILE__, LOGIT_BASE_PATH), __LINE__, LOGIT_FUNCTION, index)

#define LOGIT_STREAM_TRACE()            LOGIT_STREAM(logit::LogLevel::LOG_LVL_TRACE)
#define LOGIT_STREAM_DEBUG()            LOGIT_STREAM(logit::LogLevel::LOG_LVL_DEBUG)
#define LOGIT_STREAM_INFO()             LOGIT_STREAM(logit::LogLevel::LOG_LVL_INFO)
#define LOGIT_STREAM_WARN()             LOGIT_STREAM(logit::LogLevel::LOG_LVL_WARN)
#define LOGIT_STREAM_ERROR()            LOGIT_STREAM(logit::LogLevel::LOG_LVL_ERROR)
#define LOGIT_STREAM_FATAL()            LOGIT_STREAM(logit::LogLevel::LOG_LVL_FATAL)

#define LOGIT_STREAM_TRACE_TO(index)    LOGIT_STREAM_WITH_INDEX(logit::LogLevel::LOG_LVL_TRACE, index)
#define LOGIT_STREAM_DEBUG_TO(index)    LOGIT_STREAM_WITH_INDEX(logit::LogLevel::LOG_LVL_DEBUG, index)
#define LOGIT_STREAM_INFO_TO(index)     LOGIT_STREAM_WITH_INDEX(logit::LogLevel::LOG_LVL_INFO, index)
#define LOGIT_STREAM_WARN_TO(index)     LOGIT_STREAM_WITH_INDEX(logit::LogLevel::LOG_LVL_WARN, index)
#define LOGIT_STREAM_ERROR_TO(index)    LOGIT_STREAM_WITH_INDEX(logit::LogLevel::LOG_LVL_ERROR, index)
#define LOGIT_STREAM_FATAL_TO(index)    LOGIT_STREAM_WITH_INDEX(logit::LogLevel::LOG_LVL_FATAL, index)

#if defined(LOGIT_SHORT_NAME)
// Shorter versions of the stream-based logging macros when LOGIT_SHORT_NAME is defined

#define LOG_S_TRACE()           LOGIT_STREAM_TRACE()
#define LOG_S_DEBUG()           LOGIT_STREAM_DEBUG()
#define LOG_S_INFO()            LOGIT_STREAM_INFO()
#define LOG_S_WARN()            LOGIT_STREAM_WARN()
#define LOG_S_ERROR()           LOGIT_STREAM_ERROR()
#define LOG_S_FATAL()           LOGIT_STREAM_FATAL()

#define LOG_S_TRACE_TO(index)   LOGIT_STREAM_TRACE_TO(index)
#define LOG_S_DEBUG_TO(index)   LOGIT_STREAM_DEBUG_TO(index)
#define LOG_S_INFO_TO(index)    LOGIT_STREAM_INFO_TO(index)
#define LOG_S_WARN_TO(index)    LOGIT_STREAM_WARN_TO(index)
#define LOG_S_ERROR_TO(index)   LOGIT_STREAM_ERROR_TO(index)
#define LOG_S_FATAL_TO(index)   LOGIT_STREAM_FATAL_TO(index)

#endif // LOGIT_SHORT_NAME

//------------------------------------------------------------------------------
// Macros for logging without arguments

/// \brief Logs a message without arguments.
/// \param level The log level.
/// \param format The log message format.
#define LOGIT_LOG_AND_RETURN_NOARGS(level, format)              \
    logit::Logger::get_instance().log_and_return(               \
        logit::LogRecord{level, LOGIT_CURRENT_TIMESTAMP_MS(),   \
        logit::make_relative(__FILE__, LOGIT_BASE_PATH),        \
        __LINE__, LOGIT_FUNCTION, format, {}, -1, false})

/// \brief Logs a message to a specific logger without arguments.
/// \param level The log level.
/// \param index The index of the logger to log to.
/// \param format The log message format.
#define LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(level, index, format) \
    logit::Logger::get_instance().log_and_return(                    \
        logit::LogRecord{level, LOGIT_CURRENT_TIMESTAMP_MS(),        \
        logit::make_relative(__FILE__, LOGIT_BASE_PATH), __LINE__,   \
        LOGIT_FUNCTION, format, {}, index})

//------------------------------------------------------------------------------
// Macros for logging with arguments

/// \brief Logs a message with arguments.
/// \param level The log level.
/// \param format The log message format.
/// \param arg_names The names of the arguments.
/// \param ... The arguments to log.
#define LOGIT_LOG_AND_RETURN(level, format, arg_names, ...)        \
    logit::Logger::get_instance().log_and_return(                  \
        logit::LogRecord{level, LOGIT_CURRENT_TIMESTAMP_MS(),      \
        logit::make_relative(__FILE__, LOGIT_BASE_PATH), __LINE__, \
        LOGIT_FUNCTION, format, arg_names, -1, false}, __VA_ARGS__)

/// \brief Logs a message with arguments, but prints them without using a format string.
/// \param level The log level.
/// \param arg_names The names of the arguments.
/// \param ... The arguments to log.
/// \details This macro logs the raw arguments without applying any formatting to them.
#define LOGIT_LOG_AND_RETURN_PRINT(level, arg_names, ...)        \
    logit::Logger::get_instance().log_and_return(                  \
        logit::LogRecord{level, LOGIT_CURRENT_TIMESTAMP_MS(),      \
        logit::make_relative(__FILE__, LOGIT_BASE_PATH), __LINE__, \
        LOGIT_FUNCTION, {}, arg_names, -1, true}, __VA_ARGS__)

/// \brief Logs a message with arguments to a specific logger.
/// \param level The log level.
/// \param index The index of the logger to log to.
/// \param format The log message format.
/// \param arg_names The names of the arguments.
/// \param ... The arguments to log.
#define LOGIT_LOG_AND_RETURN_WITH_INDEX(level, index, format, arg_names, ...) \
    logit::Logger::get_instance().log_and_return(                             \
        logit::LogRecord{level, LOGIT_CURRENT_TIMESTAMP_MS(),                 \
        logit::make_relative(__FILE__, LOGIT_BASE_PATH), __LINE__,            \
        LOGIT_FUNCTION, format, arg_names, index, false}, __VA_ARGS__)

/// \brief Logs a message with arguments to a specific logger, but prints them without using a format string.
/// \param level The log level.
/// \param index The index of the logger to log to.
/// \param arg_names The names of the arguments.
/// \param ... The arguments to log.
/// \details This macro logs the raw arguments without applying any formatting to them to a specific logger.
#define LOGIT_LOG_AND_RETURN_PRINT_WITH_INDEX(level, index, arg_names, ...)   \
    logit::Logger::get_instance().log_and_return(                             \
        logit::LogRecord{level, LOGIT_CURRENT_TIMESTAMP_MS(),                 \
        logit::make_relative(__FILE__, LOGIT_BASE_PATH), __LINE__,            \
        LOGIT_FUNCTION, {}, arg_names, index, true}, __VA_ARGS__)

//------------------------------------------------------------------------------
// Macros for each log level

// TRACE level macros
#define LOGIT_TRACE(...)                LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_TRACE, {}, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_TRACE0()                  LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_TRACE, {})
#define LOGIT_0TRACE()                  LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_TRACE, {})
#define LOGIT_0_TRACE()                 LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_TRACE, {})
#define LOGIT_NOARGS_TRACE()            LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_TRACE, {})
#define LOGIT_FORMAT_TRACE(fmt, ...)    LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_TRACE, fmt, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINT_TRACE(...)          LOGIT_LOG_AND_RETURN_PRINT(logit::LogLevel::LOG_LVL_TRACE, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINTF_TRACE(fmt, ...)    LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_TRACE, logit::format(fmt, __VA_ARGS__))

// TRACE macros with index
#define LOGIT_TRACE_TO(index, ...)      LOGIT_LOG_AND_RETURN_WITH_INDEX(logit::LogLevel::LOG_LVL_TRACE, index, {}, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_TRACE0_TO(index)          LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_TRACE, index, {})
#define LOGIT_0TRACE_TO(index)          LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_TRACE, index, {})
#define LOGIT_0_TRACE_TO(index)         LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_TRACE, index, {})
#define LOGIT_NOARGS_TRACE_TO(index)    LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_TRACE, index, {})
#define LOGIT_FORMAT_TRACE_TO(index, fmt, ...) LOGIT_LOG_AND_RETURN_WITH_INDEX(logit::LogLevel::LOG_LVL_TRACE, index, fmt, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINT_TRACE_TO(index, ...)       LOGIT_LOG_AND_RETURN_PRINT_WITH_INDEX(logit::LogLevel::LOG_LVL_TRACE, index, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINTF_TRACE_TO(index, fmt, ...) LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_TRACE, index, logit::format(fmt, __VA_ARGS__))

// INFO level macros
#define LOGIT_INFO(...)                 LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_INFO, {}, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_INFO0()                   LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_INFO, {})
#define LOGIT_0INFO()                   LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_INFO, {})
#define LOGIT_0_INFO()                  LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_INFO, {})
#define LOGIT_NOARGS_INFO()             LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_INFO, {})
#define LOGIT_FORMAT_INFO(fmt, ...)     LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_INFO, fmt, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINT_INFO(...)           LOGIT_LOG_AND_RETURN_PRINT(logit::LogLevel::LOG_LVL_INFO, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINTF_INFO(fmt, ...)     LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_INFO, logit::format(fmt, __VA_ARGS__))

// INFO macros with index
#define LOGIT_INFO_TO(index, ...)       LOGIT_LOG_AND_RETURN_WITH_INDEX(logit::LogLevel::LOG_LVL_INFO, index, {}, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_INFO0_TO(index)           LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_INFO, index, {})
#define LOGIT_0INFO_TO(index)           LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_INFO, index, {})
#define LOGIT_0_INFO_TO(index)          LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_INFO, index, {})
#define LOGIT_NOARGS_INFO_TO(index)     LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_INFO, index, {})
#define LOGIT_FORMAT_INFO_TO(index, fmt, ...) LOGIT_LOG_AND_RETURN_WITH_INDEX(logit::LogLevel::LOG_LVL_INFO, index, fmt, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINT_INFO_TO(index, ...)       LOGIT_LOG_AND_RETURN_PRINT_WITH_INDEX(logit::LogLevel::LOG_LVL_INFO, index, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINTF_INFO_TO(index, fmt, ...) LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_INFO, index, logit::format(fmt, __VA_ARGS__))

// DEBUG level macros
#define LOGIT_DEBUG(...)                LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_DEBUG, {}, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_DEBUG0()                  LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_DEBUG, {})
#define LOGIT_0DEBUG()                  LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_DEBUG, {})
#define LOGIT_0_DEBUG()                 LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_DEBUG, {})
#define LOGIT_NOARGS_DEBUG()            LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_DEBUG, {})
#define LOGIT_FORMAT_DEBUG(fmt, ...)    LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_DEBUG, fmt, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINT_DEBUG(...)          LOGIT_LOG_AND_RETURN_PRINT(logit::LogLevel::LOG_LVL_DEBUG, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINTF_DEBUG(fmt, ...)    LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_DEBUG, logit::format(fmt, __VA_ARGS__))

// DEBUG macros with index
#define LOGIT_DEBUG_TO(index, ...)      LOGIT_LOG_AND_RETURN_WITH_INDEX(logit::LogLevel::LOG_LVL_DEBUG, index, {}, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_DEBUG0_TO(index)          LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_DEBUG, index, {})
#define LOGIT_0DEBUG_TO(index)          LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_DEBUG, index, {})
#define LOGIT_0_DEBUG_TO(index)         LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_DEBUG, index, {})
#define LOGIT_NOARGS_DEBUG_TO(index)    LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_DEBUG, index, {})
#define LOGIT_FORMAT_DEBUG_TO(index, fmt, ...) LOGIT_LOG_AND_RETURN_WITH_INDEX(logit::LogLevel::LOG_LVL_DEBUG, index, fmt, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINT_DEBUG_TO(index, ...)       LOGIT_LOG_AND_RETURN_PRINT_WITH_INDEX(logit::LogLevel::LOG_LVL_DEBUG, index, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINTF_DEBUG_TO(index, fmt, ...) LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_DEBUG, index, logit::format(fmt, __VA_ARGS__))

// WARN level macros
#define LOGIT_WARN(...)                 LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_WARN, {}, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_WARN0()                   LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_WARN, {})
#define LOGIT_0WARN()                   LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_WARN, {})
#define LOGIT_0_WARN()                  LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_WARN, {})
#define LOGIT_NOARGS_WARN()             LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_WARN, {})
#define LOGIT_FORMAT_WARN(fmt, ...)     LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_WARN, fmt, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINT_WARN(...)           LOGIT_LOG_AND_RETURN_PRINT(logit::LogLevel::LOG_LVL_WARN, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINTF_WARN(fmt, ...)     LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_WARN, logit::format(fmt, __VA_ARGS__))

// WARN macros with index
#define LOGIT_WARN_TO(index, ...)       LOGIT_LOG_AND_RETURN_WITH_INDEX(logit::LogLevel::LOG_LVL_WARN, index, {}, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_WARN0_TO(index)           LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_WARN, index, {})
#define LOGIT_0WARN_TO(index)           LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_WARN, index, {})
#define LOGIT_0_WARN_TO(index)          LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_WARN, index, {})
#define LOGIT_NOARGS_WARN_TO(index)     LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_WARN, index, {})
#define LOGIT_FORMAT_WARN_TO(index, fmt, ...) LOGIT_LOG_AND_RETURN_WITH_INDEX(logit::LogLevel::LOG_LVL_WARN, index, fmt, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINT_WARN_TO(index, ...)       LOGIT_LOG_AND_RETURN_PRINT_WITH_INDEX(logit::LogLevel::LOG_LVL_WARN, index, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINTF_WARN_TO(index, fmt, ...) LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_WARN, index, logit::format(fmt, __VA_ARGS__))

// ERROR level macros
#define LOGIT_ERROR(...)                LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_ERROR, {}, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_ERROR0()                  LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_ERROR, {})
#define LOGIT_0ERROR()                  LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_ERROR, {})
#define LOGIT_0_ERROR()                 LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_ERROR, {})
#define LOGIT_NOARGS_ERROR()            LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_ERROR, {})
#define LOGIT_FORMAT_ERROR(fmt, ...)    LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_ERROR, fmt, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINT_ERROR(...)          LOGIT_LOG_AND_RETURN_PRINT(logit::LogLevel::LOG_LVL_ERROR, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINTF_ERROR(fmt, ...)    LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_ERROR, logit::format(fmt, __VA_ARGS__))

// ERROR macros with index
#define LOGIT_ERROR_TO(index, ...)      LOGIT_LOG_AND_RETURN_WITH_INDEX(logit::LogLevel::LOG_LVL_ERROR, index, {}, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_ERROR0_TO(index)          LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_ERROR, index, {})
#define LOGIT_0ERROR_TO(index)          LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_ERROR, index, {})
#define LOGIT_0_ERROR_TO(index)         LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_ERROR, index, {})
#define LOGIT_NOARGS_ERROR_TO(index)    LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_ERROR, index, {})
#define LOGIT_FORMAT_ERROR_TO(index, fmt, ...) LOGIT_LOG_AND_RETURN_WITH_INDEX(logit::LogLevel::LOG_LVL_ERROR, index, fmt, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINT_ERROR_TO(index, ...)       LOGIT_LOG_AND_RETURN_PRINT_WITH_INDEX(logit::LogLevel::LOG_LVL_ERROR, index, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINTF_ERROR_TO(index, fmt, ...) LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_ERROR, index, logit::format(fmt, __VA_ARGS__))

// FATAL level macros
#define LOGIT_FATAL(...)                LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_FATAL, {}, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_FATAL0()                  LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_FATAL, {})
#define LOGIT_0FATAL()                  LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_FATAL, {})
#define LOGIT_0_FATAL()                 LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_FATAL, {})
#define LOGIT_NOARGS_FATAL()            LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_FATAL, {})
#define LOGIT_FORMAT_FATAL(fmt, ...)    LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_FATAL, fmt, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINT_FATAL(...)          LOGIT_LOG_AND_RETURN_PRINT(logit::LogLevel::LOG_LVL_FATAL, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINTF_FATAL(fmt, ...)    LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_FATAL, logit::format(fmt, __VA_ARGS__))

// FATAL macros with index
#define LOGIT_FATAL_TO(index, ...)      LOGIT_LOG_AND_RETURN_WITH_INDEX(logit::LogLevel::LOG_LVL_FATAL, index, {}, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_FATAL0_TO(index)          LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_FATAL, index, {})
#define LOGIT_0FATAL_TO(index)          LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_FATAL, index, {})
#define LOGIT_0_FATAL_TO(index)         LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_FATAL, index, {})
#define LOGIT_NOARGS_FATAL_TO(index)    LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_FATAL, index, {})
#define LOGIT_FORMAT_FATAL_TO(index, fmt, ...) LOGIT_LOG_AND_RETURN_WITH_INDEX(logit::LogLevel::LOG_LVL_FATAL, index, fmt, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINT_FATAL_TO(index, ...)       LOGIT_LOG_AND_RETURN_PRINT_WITH_INDEX(logit::LogLevel::LOG_LVL_FATAL, index, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINTF_FATAL_TO(index, fmt, ...) LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_FATAL, index, logit::format(fmt, __VA_ARGS__))

//------------------------------------------------------------------------------
// Conditional logging macros (logging based on a condition)

// TRACE level conditional macros
#define LOGIT_TRACE_IF(condition, ...)        if (condition) LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_TRACE, {}, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_TRACE0_IF(condition)            if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_TRACE, {})
#define LOGIT_0TRACE_IF(condition)            if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_TRACE, {})
#define LOGIT_0_TRACE_IF(condition)           if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_TRACE, {})
#define LOGIT_NOARGS_TRACE_IF(condition)      if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_TRACE, {})
#define LOGIT_FORMAT_TRACE_IF(condition, fmt, ...) if (condition) LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_TRACE, fmt, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINT_TRACE_IF(condition, fmt)  if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_TRACE, fmt)
#define LOGIT_PRINTF_TRACE_IF(condition, fmt, ...) if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_TRACE, logit::format(fmt, __VA_ARGS__))

// INFO level conditional macros
#define LOGIT_INFO_IF(condition, ...)         if (condition) LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_INFO, {}, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_INFO0_IF(condition)             if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_INFO, {})
#define LOGIT_0INFO_IF(condition)             if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_INFO, {})
#define LOGIT_0_INFO_IF(condition)            if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_INFO, {})
#define LOGIT_NOARGS_INFO_IF(condition)       if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_INFO, {})
#define LOGIT_FORMAT_INFO_IF(condition, fmt, ...) if (condition) LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_INFO, fmt, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINT_INFO_IF(condition, fmt)   if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_INFO, fmt)
#define LOGIT_PRINTF_INFO_IF(condition, fmt, ...) if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_INFO, logit::format(fmt, __VA_ARGS__))

// DEBUG level conditional macros
#define LOGIT_DEBUG_IF(condition, ...)        if (condition) LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_DEBUG, {}, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_DEBUG0_IF(condition)            if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_DEBUG, {})
#define LOGIT_0DEBUG_IF(condition)            if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_DEBUG, {})
#define LOGIT_0_DEBUG_IF(condition)           if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_DEBUG, {})
#define LOGIT_NOARGS_DEBUG_IF(condition)      if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_DEBUG, {})
#define LOGIT_FORMAT_DEBUG_IF(condition, fmt, ...) if (condition) LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_DEBUG, fmt, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINT_DEBUG_IF(condition, fmt)  if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_DEBUG, fmt)
#define LOGIT_PRINTF_DEBUG_IF(condition, fmt, ...) if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_DEBUG, logit::format(fmt, __VA_ARGS__))

// WARN level conditional macros
#define LOGIT_WARN_IF(condition, ...)         if (condition) LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_WARN, {}, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_WARN0_IF(condition)             if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_WARN, {})
#define LOGIT_0WARN_IF(condition)             if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_WARN, {})
#define LOGIT_0_WARN_IF(condition)            if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_WARN, {})
#define LOGIT_NOARGS_WARN_IF(condition)       if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_WARN, {})
#define LOGIT_FORMAT_WARN_IF(condition, fmt, ...) if (condition) LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_WARN, fmt, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINT_WARN_IF(condition, fmt)   if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_WARN, fmt)
#define LOGIT_PRINTF_WARN_IF(condition, fmt, ...) if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_WARN, logit::format(fmt, __VA_ARGS__))

// ERROR level conditional macros
#define LOGIT_ERROR_IF(condition, ...)        if (condition) LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_ERROR, {}, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_ERROR0_IF(condition)            if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_ERROR, {})
#define LOGIT_0ERROR_IF(condition)            if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_ERROR, {})
#define LOGIT_0_ERROR_IF(condition)           if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_ERROR, {})
#define LOGIT_NOARGS_ERROR_IF(condition)      if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_ERROR, {})
#define LOGIT_FORMAT_ERROR_IF(condition, fmt, ...) if (condition) LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_ERROR, fmt, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINT_ERROR_IF(condition, fmt)  if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_ERROR, fmt)
#define LOGIT_PRINTF_ERROR_IF(condition, fmt, ...) if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_ERROR, logit::format(fmt, __VA_ARGS__))

// FATAL level conditional macros
#define LOGIT_FATAL_IF(condition, ...)        if (condition) LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_FATAL, {}, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_FATAL0_IF(condition)            if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_FATAL, {})
#define LOGIT_0FATAL_IF(condition)            if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_FATAL, {})
#define LOGIT_0_FATAL_IF(condition)           if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_FATAL, {})
#define LOGIT_NOARGS_FATAL_IF(condition)      if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_FATAL, {})
#define LOGIT_FORMAT_FATAL_IF(condition, fmt, ...) if (condition) LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_FATAL, fmt, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINT_FATAL_IF(condition, fmt)  if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_FATAL, fmt)
#define LOGIT_PRINTF_FATAL_IF(condition, fmt, ...) if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_FATAL, logit::format(fmt, __VA_ARGS__))

//------------------------------------------------------------------------------
// Shorter versions of the macros when LOGIT_SHORT_NAME is defined
#if defined(LOGIT_SHORT_NAME)

// TRACE level
#define LOG_T(...)                      LOGIT_TRACE(__VA_ARGS__)
#define LOG_T0()                        LOGIT_TRACE0()
#define LOG_0T()                        LOGIT_TRACE0()
#define LOG_0_T()                       LOGIT_TRACE0()
#define LOG_T_NOARGS()                  LOGIT_NOARGS_TRACE()
#define LOG_NOARGS_T()                  LOGIT_NOARGS_TRACE()
#define LOG_TF(fmt, ...)                LOGIT_FORMAT_TRACE(fmt, __VA_ARGS__)
#define LOG_FT(fmt, ...)                LOGIT_FORMAT_TRACE(fmt, __VA_ARGS__)
#define LOG_T_PRINT(...)                LOGIT_PRINT_TRACE(__VA_ARGS__)
#define LOG_PRINT_T(...)                LOGIT_PRINT_TRACE(__VA_ARGS__)
#define LOG_T_PRINTF(fmt, ...)          LOGIT_PRINTF_TRACE(fmt, __VA_ARGS__)
#define LOG_PRINTF_T(fmt, ...)          LOGIT_PRINTF_TRACE(fmt, __VA_ARGS__)
#define LOG_TP(...)                     LOGIT_PRINT_TRACE(__VA_ARGS__)
#define LOG_PT(...)                     LOGIT_PRINT_TRACE(__VA_ARGS__)
#define LOG_TPF(fmt, ...)               LOGIT_PRINTF_TRACE(fmt, __VA_ARGS__)
#define LOG_PFT(fmt, ...)               LOGIT_PRINTF_TRACE(fmt, __VA_ARGS__)

#define LOG_TRACE(...)                  LOGIT_TRACE(__VA_ARGS__)
#define LOG_TRACE0()                    LOGIT_TRACE0()
#define LOG_0TRACE()                    LOGIT_TRACE0()
#define LOG_0_TRACE()                   LOGIT_TRACE0()
#define LOG_TRACE_NOARGS()              LOGIT_NOARGS_TRACE()
#define LOG_NOARGS_TRACE()              LOGIT_NOARGS_TRACE()
#define LOG_TRACEF(fmt, ...)            LOGIT_FORMAT_TRACE(fmt, __VA_ARGS__)
#define LOG_FTRACE(fmt, ...)            LOGIT_FORMAT_TRACE(fmt, __VA_ARGS__)
#define LOG_TRACE_PRINT(...)            LOGIT_PRINT_TRACE(__VA_ARGS__)
#define LOG_PRINT_TRACE(...)            LOGIT_PRINT_TRACE(__VA_ARGS__)
#define LOG_TRACE_PRINTF(fmt, ...)      LOGIT_PRINTF_TRACE(fmt, __VA_ARGS__)
#define LOG_PRINTF_TRACE(fmt, ...)      LOGIT_PRINTF_TRACE(fmt, __VA_ARGS__)

// INFO level
#define LOG_I(...)                      LOGIT_INFO(__VA_ARGS__)
#define LOG_I0()                        LOGIT_INFO0()
#define LOG_0I()                        LOGIT_INFO0()
#define LOG_0_I()                       LOGIT_INFO0()
#define LOG_I_NOARGS()                  LOGIT_NOARGS_INFO()
#define LOG_NOARGS_I()                  LOGIT_NOARGS_INFO()
#define LOG_IF(fmt, ...)                LOGIT_FORMAT_INFO(fmt, __VA_ARGS__)
#define LOG_FI(fmt, ...)                LOGIT_FORMAT_INFO(fmt, __VA_ARGS__)
#define LOG_I_PRINT(...)                LOGIT_PRINT_INFO(__VA_ARGS__)
#define LOG_PRINT_I(...)                LOGIT_PRINT_INFO(__VA_ARGS__)
#define LOG_I_PRINTF(fmt, ...)          LOGIT_PRINTF_INFO(fmt, __VA_ARGS__)
#define LOG_PRINTF_I(fmt, ...)          LOGIT_PRINTF_INFO(fmt, __VA_ARGS__)
#define LOG_IP(...)                     LOGIT_PRINT_INFO(__VA_ARGS__)
#define LOG_PI(...)                     LOGIT_PRINT_INFO(__VA_ARGS__)
#define LOG_IPF(fmt, ...)               LOGIT_PRINTF_INFO(fmt, __VA_ARGS__)
#define LOG_PFI(fmt, ...)               LOGIT_PRINTF_INFO(fmt, __VA_ARGS__)

#define LOG_INFO(...)                   LOGIT_INFO(__VA_ARGS__)
#define LOG_INFO0()                     LOGIT_INFO0()
#define LOG_0INFO()                     LOGIT_INFO0()
#define LOG_0_INFO()                    LOGIT_INFO0()
#define LOG_INFO_NOARGS()               LOGIT_NOARGS_INFO()
#define LOG_NOARGS_INFO()               LOGIT_NOARGS_INFO()
#define LOG_INFOF(fmt, ...)             LOGIT_FORMAT_INFO(fmt, __VA_ARGS__)
#define LOG_FINFO(fmt, ...)             LOGIT_FORMAT_INFO(fmt, __VA_ARGS__)
#define LOG_INFO_PRINT(...)             LOGIT_PRINT_INFO(__VA_ARGS__)
#define LOG_PRINT_INFO(...)             LOGIT_PRINT_INFO(__VA_ARGS__)
#define LOG_INFO_PRINTF(fmt, ...)       LOGIT_PRINTF_INFO(fmt, __VA_ARGS__)
#define LOG_PRINTF_INFO(fmt, ...)       LOGIT_PRINTF_INFO(fmt, __VA_ARGS__)

// DEBUG level
#define LOG_D(...)                      LOGIT_DEBUG(__VA_ARGS__)
#define LOG_D0()                        LOGIT_DEBUG0()
#define LOG_0D()                        LOGIT_DEBUG0()
#define LOG_0_D()                       LOGIT_DEBUG0()
#define LOG_D_NOARGS()                  LOGIT_NOARGS_DEBUG()
#define LOG_NOARGS_D()                  LOGIT_NOARGS_DEBUG()
#define LOG_DF(fmt, ...)                LOGIT_FORMAT_DEBUG(fmt, __VA_ARGS__)
#define LOG_FD(fmt, ...)                LOGIT_FORMAT_DEBUG(fmt, __VA_ARGS__)
#define LOG_D_PRINT(...)                LOGIT_PRINT_DEBUG(__VA_ARGS__)
#define LOG_PRINT_D(...)                LOGIT_PRINT_DEBUG(__VA_ARGS__)
#define LOG_D_PRINTF(fmt, ...)          LOGIT_PRINTF_DEBUG(fmt, __VA_ARGS__)
#define LOG_PRINTF_D(fmt, ...)          LOGIT_PRINTF_DEBUG(fmt, __VA_ARGS__)
#define LOG_DP(...)                     LOGIT_PRINT_DEBUG(__VA_ARGS__)
#define LOG_PD(...)                     LOGIT_PRINT_DEBUG(__VA_ARGS__)
#define LOG_DPF(fmt, ...)               LOGIT_PRINTF_DEBUG(fmt, __VA_ARGS__)
#define LOG_PFD(fmt, ...)               LOGIT_PRINTF_DEBUG(fmt, __VA_ARGS__)

#define LOG_DEBUG(...)                  LOGIT_DEBUG(__VA_ARGS__)
#define LOG_DEBUG0()                    LOGIT_DEBUG0()
#define LOG_0DEBUG()                    LOGIT_DEBUG0()
#define LOG_0_DEBUG()                   LOGIT_DEBUG0()
#define LOG_DEBUG_NOARGS()              LOGIT_NOARGS_DEBUG()
#define LOG_NOARGS_DEBUG()              LOGIT_NOARGS_DEBUG()
#define LOG_DEBUGF(fmt, ...)            LOGIT_FORMAT_DEBUG(fmt, __VA_ARGS__)
#define LOG_FDEBUG(fmt, ...)            LOGIT_FORMAT_DEBUG(fmt, __VA_ARGS__)
#define LOG_DEBUG_PRINT(...)            LOGIT_PRINT_DEBUG(__VA_ARGS__)
#define LOG_PRINT_DEBUG(...)            LOGIT_PRINT_DEBUG(__VA_ARGS__)
#define LOG_DEBUG_PRINTF(fmt, ...)      LOGIT_PRINTF_DEBUG(fmt, __VA_ARGS__)
#define LOG_PRINTF_DEBUG(fmt, ...)      LOGIT_PRINTF_DEBUG(fmt, __VA_ARGS__)

// WARN level
#define LOG_W(...)                      LOGIT_WARN(__VA_ARGS__)
#define LOG_W0()                        LOGIT_WARN0()
#define LOG_0W()                        LOGIT_WARN0()
#define LOG_0_W()                       LOGIT_WARN0()
#define LOG_W_NOARGS()                  LOGIT_NOARGS_WARN()
#define LOG_NOARGS_W()                  LOGIT_NOARGS_WARN()
#define LOG_WF(fmt, ...)                LOGIT_FORMAT_WARN(fmt, __VA_ARGS__)
#define LOG_FW(fmt, ...)                LOGIT_FORMAT_WARN(fmt, __VA_ARGS__)
#define LOG_W_PRINT(...)                LOGIT_PRINT_WARN(__VA_ARGS__)
#define LOG_PRINT_W(...)                LOGIT_PRINT_WARN(__VA_ARGS__)
#define LOG_W_PRINTF(fmt, ...)          LOGIT_PRINTF_WARN(fmt, __VA_ARGS__)
#define LOG_PRINTF_W(fmt, ...)          LOGIT_PRINTF_WARN(fmt, __VA_ARGS__)
#define LOG_WP(...)                     LOGIT_PRINT_WARN(__VA_ARGS__)
#define LOG_PW(...)                     LOGIT_PRINT_WARN(__VA_ARGS__)
#define LOG_WPF(fmt, ...)               LOGIT_PRINTF_WARN(fmt, __VA_ARGS__)
#define LOG_PFW(fmt, ...)               LOGIT_PRINTF_WARN(fmt, __VA_ARGS__)

#define LOG_WARN(...)                   LOGIT_WARN(__VA_ARGS__)
#define LOG_WARN0()                     LOGIT_WARN0()
#define LOG_0WARN()                     LOGIT_WARN0()
#define LOG_0_WARN()                    LOGIT_WARN0()
#define LOG_WARN_NOARGS()               LOGIT_NOARGS_WARN()
#define LOG_NOARGS_WARN()               LOGIT_NOARGS_WARN()
#define LOG_WARNF(fmt, ...)             LOGIT_FORMAT_WARN(fmt, __VA_ARGS__)
#define LOG_FWARN(fmt, ...)             LOGIT_FORMAT_WARN(fmt, __VA_ARGS__)
#define LOG_WARN_PRINT(...)             LOGIT_PRINT_WARN(__VA_ARGS__)
#define LOG_PRINT_WARN(...)             LOGIT_PRINT_WARN(__VA_ARGS__)
#define LOG_WARN_PRINTF(fmt, ...)       LOGIT_PRINTF_WARN(fmt, __VA_ARGS__)
#define LOG_PRINTF_WARN(fmt, ...)       LOGIT_PRINTF_WARN(fmt, __VA_ARGS__)

// ERROR level
#define LOG_E(...)                      LOGIT_ERROR(__VA_ARGS__)
#define LOG_E0()                        LOGIT_ERROR0()
#define LOG_0E()                        LOGIT_ERROR0()
#define LOG_0_E()                       LOGIT_ERROR0()
#define LOG_E_NOARGS()                  LOGIT_NOARGS_ERROR()
#define LOG_NOARGS_E()                  LOGIT_NOARGS_ERROR()
#define LOG_EF(fmt, ...)                LOGIT_FORMAT_ERROR(fmt, __VA_ARGS__)
#define LOG_FE(fmt, ...)                LOGIT_FORMAT_ERROR(fmt, __VA_ARGS__)
#define LOG_E_PRINT(...)                LOGIT_PRINT_ERROR(__VA_ARGS__)
#define LOG_PRINT_E(...)                LOGIT_PRINT_ERROR(__VA_ARGS__)
#define LOG_E_PRINTF(fmt, ...)          LOGIT_PRINTF_ERROR(fmt, __VA_ARGS__)
#define LOG_PRINTF_E(fmt, ...)          LOGIT_PRINTF_ERROR(fmt, __VA_ARGS__)
#define LOG_EP(...)                     LOGIT_PRINT_ERROR(__VA_ARGS__)
#define LOG_PE(...)                     LOGIT_PRINT_ERROR(__VA_ARGS__)
#define LOG_EPF(fmt, ...)               LOGIT_PRINTF_ERROR(fmt, __VA_ARGS__)
#define LOG_PFE(fmt, ...)               LOGIT_PRINTF_ERROR(fmt, __VA_ARGS__)

#define LOG_ERROR(...)                  LOGIT_ERROR(__VA_ARGS__)
#define LOG_ERROR0()                    LOGIT_ERROR0()
#define LOG_0ERROR()                    LOGIT_ERROR0()
#define LOG_0_ERROR()                   LOGIT_ERROR0()
#define LOG_ERROR_NOARGS()              LOGIT_NOARGS_ERROR()
#define LOG_NOARGS_ERROR()              LOGIT_NOARGS_ERROR()
#define LOG_ERRORF(fmt, ...)            LOGIT_FORMAT_ERROR(fmt, __VA_ARGS__)
#define LOG_FERROR(fmt, ...)            LOGIT_FORMAT_ERROR(fmt, __VA_ARGS__)
#define LOG_ERROR_PRINT(...)            LOGIT_PRINT_ERROR(__VA_ARGS__)
#define LOG_PRINT_ERROR(...)            LOGIT_PRINT_ERROR(__VA_ARGS__)
#define LOG_ERROR_PRINTF(fmt, ...)      LOGIT_PRINTF_ERROR(fmt, __VA_ARGS__)
#define LOG_PRINTF_ERROR(fmt, ...)      LOGIT_PRINTF_ERROR(fmt, __VA_ARGS__)

// FATAL level
#define LOG_F(...)                      LOGIT_FATAL(__VA_ARGS__)
#define LOG_F0()                        LOGIT_FATAL0()
#define LOG_0F()                        LOGIT_FATAL0()
#define LOG_0_F()                       LOGIT_FATAL0()
#define LOG_F_NOARGS()                  LOGIT_NOARGS_FATAL()
#define LOG_NOARGS_F()                  LOGIT_NOARGS_FATAL()
#define LOG_FF(fmt, ...)                LOGIT_FORMAT_FATAL(fmt, __VA_ARGS__)
#define LOG_F_PRINT(...)                LOGIT_PRINT_FATAL(__VA_ARGS__)
#define LOG_PRINT_F(...)                LOGIT_PRINT_FATAL(__VA_ARGS__)
#define LOG_F_PRINTF(fmt, ...)          LOGIT_PRINTF_FATAL(fmt, __VA_ARGS__)
#define LOG_PRINTF_F(fmt, ...)          LOGIT_PRINTF_FATAL(fmt, __VA_ARGS__)
#define LOG_FP(...)                     LOGIT_PRINT_FATAL(__VA_ARGS__)
#define LOG_PF(...)                     LOGIT_PRINT_FATAL(__VA_ARGS__)
#define LOG_FPF(fmt, ...)               LOGIT_PRINTF_FATAL(fmt, __VA_ARGS__)
#define LOG_PFF(fmt, ...)               LOGIT_PRINTF_FATAL(fmt, __VA_ARGS__)

#define LOG_FATAL(...)                  LOGIT_FATAL(__VA_ARGS__)
#define LOG_FATAL0()                    LOGIT_FATAL0()
#define LOG_0FATAL()                    LOGIT_FATAL0()
#define LOG_0_FATAL()                   LOGIT_FATAL0()
#define LOG_FATAL_NOARGS()              LOGIT_NOARGS_FATAL()
#define LOG_NOARGS_FATAL()              LOGIT_NOARGS_FATAL()
#define LOG_FATALF(fmt, ...)            LOGIT_FORMAT_FATAL(fmt, __VA_ARGS__)
#define LOG_FFATAL(fmt, ...)            LOGIT_FORMAT_FATAL(fmt, __VA_ARGS__)
#define LOG_FATAL_PRINT(...)            LOGIT_PRINT_FATAL(__VA_ARGS__)
#define LOG_PRINT_FATAL(...)            LOGIT_PRINT_FATAL(__VA_ARGS__)
#define LOG_FATAL_PRINTF(fmt, ...)      LOGIT_PRINTF_FATAL(fmt, __VA_ARGS__)
#define LOG_PRINTF_FATAL(fmt, ...)      LOGIT_PRINTF_FATAL(fmt, __VA_ARGS__)

#endif // LOGIT_SHORT_NAME

//------------------------------------------------------------------------------
// Macros for adding and configuring various loggers
// with optional patterns, modes, and single_mode (exclusive) configuration

#if __cplusplus >= 201703L // C++17 or later

/// \brief Macro for adding a logger with a specific formatter.
/// \param logger_type The type of logger (e.g., `ConsoleLogger`).
/// \param logger_args Arguments for the logger constructor, enclosed in parentheses.
/// \param formatter_type The type of formatter (e.g., `SimpleLogFormatter`).
/// \param formatter_args Arguments for the formatter constructor, enclosed in parentheses.
/// This version uses `std::make_unique` for C++17 compatibility.
#define LOGIT_ADD_LOGGER(logger_type, logger_args, formatter_type, formatter_args)  \
    logit::Logger::get_instance().add_logger(                                       \
        std::make_unique<logger_type> logger_args,                                  \
        std::make_unique<formatter_type> formatter_args)

/// \brief Macro for adding a logger with a specific formatter in single_mode.
/// \param logger_type The type of logger (e.g., `ConsoleLogger`).
/// \param logger_args Arguments for the logger constructor, enclosed in parentheses.
/// \param formatter_type The type of formatter (e.g., `SimpleLogFormatter`).
/// \param formatter_args Arguments for the formatter constructor, enclosed in parentheses.
/// In single_mode, loggers can only be invoked explicitly using macros like `LOGIT_TRACE_TO`.
/// This version uses `std::make_unique` for C++17 compatibility.
#define LOGIT_ADD_LOGGER_SINGLE_MODE(logger_type, logger_args, formatter_type, formatter_args)  \
    logit::Logger::get_instance().add_logger(                                                   \
        std::make_unique<logger_type> logger_args,                                              \
        std::make_unique<formatter_type> formatter_args,                                        \
        true)

/// \brief Macro for adding a console logger with a specific pattern and mode (C++17 or later).
/// \param pattern The format pattern for log messages.
/// \param async Boolean indicating whether logging should be asynchronous (true) or synchronous (false).
/// This version uses `std::make_unique`, available in C++17 and later.
#define LOGIT_ADD_CONSOLE(pattern, async)                                   \
    logit::Logger::get_instance().add_logger(                               \
        std::make_unique<logit::ConsoleLogger>(async),                      \
        std::make_unique<logit::SimpleLogFormatter>(pattern));

/// \brief Macro for adding a console logger in single_mode with a specific pattern and mode.
/// \param pattern The format pattern for log messages.
/// \param async Boolean indicating whether logging should be asynchronous (true) or synchronous (false).
/// In single_mode, the console logger can only be invoked using macros like `LOGIT_TRACE_TO`.
/// This version uses `std::make_unique`, available in C++17 and later.
#define LOGIT_ADD_CONSOLE_SINGLE_MODE(pattern, async)                       \
    logit::Logger::get_instance().add_logger(                               \
        std::make_unique<logit::ConsoleLogger>(async),                      \
        std::make_unique<logit::SimpleLogFormatter>(pattern),               \
        true)

/// \brief Macro for adding the default console logger.
/// This logger uses the default format pattern and asynchronous logging.
/// This version uses `std::make_unique`, available in C++17 and later.
#define LOGIT_ADD_CONSOLE_DEFAULT()                                         \
    logit::Logger::get_instance().add_logger(                               \
        std::make_unique<logit::ConsoleLogger>(true),                       \
        std::make_unique<logit::SimpleLogFormatter>(LOGIT_CONSOLE_PATTERN));

/// \brief Macro for adding the default console logger in single_mode.
/// In single_mode, the console logger can only be invoked using macros like `LOGIT_TRACE_TO`.
/// This version uses `std::make_unique`, available in C++17 and later.
#define LOGIT_ADD_CONSOLE_DEFAULT_SINGLE_MODE()                             \
    logit::Logger::get_instance().add_logger(                               \
        std::make_unique<logit::ConsoleLogger>(true),                       \
        std::make_unique<logit::SimpleLogFormatter>(LOGIT_CONSOLE_PATTERN), \
        true)

/// \brief Macro for adding a file logger with specific settings.
/// \param directory The directory where log files will be stored.
/// \param async Boolean indicating whether logging should be asynchronous (true) or synchronous (false).
/// \param auto_delete_days Number of days after which old log files are deleted.
/// \param pattern The format pattern for log messages.
/// This version uses `std::make_unique`, available in C++17 and later.
#define LOGIT_ADD_FILE_LOGGER(directory, async, auto_delete_days, pattern)  \
    logit::Logger::get_instance().add_logger(                               \
        std::make_unique<logit::FileLogger>(                                \
            directory, async, auto_delete_days),                            \
        std::make_unique<logit::SimpleLogFormatter>(pattern));

/// \brief Macro for adding a file logger in single_mode with specific settings.
/// \param directory The directory where log files will be stored.
/// \param async Boolean indicating whether logging should be asynchronous (true) or synchronous (false).
/// \param auto_delete_days Number of days after which old log files are deleted.
/// \param pattern The format pattern for log messages.
/// In single_mode, the file logger can only be invoked using macros like `LOGIT_TRACE_TO`.
/// This version uses `std::make_unique`, available in C++17 and later.
#define LOGIT_ADD_FILE_LOGGER_SINGLE_MODE(directory, async, auto_delete_days, pattern)  \
    logit::Logger::get_instance().add_logger(                                           \
        std::make_unique<logit::FileLogger>(                                            \
            directory, async, auto_delete_days),                                        \
        std::make_unique<logit::SimpleLogFormatter>(pattern),                           \
        true)

/// \brief Macro for adding the default file logger.
/// This logger writes logs to the default file path and deletes logs older than the default number of days.
/// This version uses `std::make_unique`, available in C++17 and later.
#define LOGIT_ADD_FILE_LOGGER_DEFAULT()                                     \
    logit::Logger::get_instance().add_logger(                               \
        std::make_unique<logit::FileLogger>(                                \
            LOGIT_FILE_LOGGER_PATH, true,                                   \
            LOGIT_FILE_LOGGER_AUTO_DELETE_DAYS),                            \
        std::make_unique<logit::SimpleLogFormatter>(LOGIT_FILE_LOGGER_PATTERN));

/// \brief Macro for adding the default file logger in single_mode.
/// In single_mode, the file logger can only be invoked using macros like `LOGIT_TRACE_TO`.
/// This version uses `std::make_unique`, available in C++17 and later.
#define LOGIT_ADD_FILE_LOGGER_DEFAULT_SINGLE_MODE()                         \
    logit::Logger::get_instance().add_logger(                               \
        std::make_unique<logit::FileLogger>(                                \
            LOGIT_FILE_LOGGER_PATH, true,                                   \
            LOGIT_FILE_LOGGER_AUTO_DELETE_DAYS),                            \
        std::make_unique<logit::SimpleLogFormatter>(LOGIT_FILE_LOGGER_PATTERN), \
        true)

/// \brief Macro for adding a unique file logger with custom parameters.
/// \param directory The directory where log files will be stored.
/// \param async Boolean indicating whether logging should be asynchronous (true) or synchronous (false).
/// \param auto_delete_days Number of days after which old log files are deleted.
/// \param hash_length The length of the hash to be appended to the log file name.
/// \param pattern The format pattern for log messages.
/// This version uses `std::make_unique`, available in C++17 and later.
#define LOGIT_ADD_UNIQUE_FILE_LOGGER(directory, async, auto_delete_days, hash_length, pattern)  \
    logit::Logger::get_instance().add_logger(                                                   \
        std::make_unique<logit::UniqueFileLogger>(                                              \
            directory, async, auto_delete_days, hash_length),                                   \
        std::make_unique<logit::SimpleLogFormatter>(pattern));

/// \brief Macro for adding a unique file logger in single_mode with custom parameters.
/// \param directory The directory where log files will be stored.
/// \param async Boolean indicating whether logging should be asynchronous (true) or synchronous (false).
/// \param auto_delete_days Number of days after which old log files are deleted.
/// \param hash_length The length of the hash to be appended to the log file name.
/// \param pattern The format pattern for log messages.
/// In single_mode, the unique file logger can only be invoked using macros like `LOGIT_TRACE_TO`.
/// This version uses `std::make_unique`, available in C++17 and later.
#define LOGIT_ADD_UNIQUE_FILE_LOGGER_SINGLE_MODE(directory, async, auto_delete_days, hash_length, pattern)  \
    logit::Logger::get_instance().add_logger(                                                                \
        std::make_unique<logit::UniqueFileLogger>(                                                           \
            directory, async, auto_delete_days, hash_length),                                                \
        std::make_unique<logit::SimpleLogFormatter>(pattern),                                                \
        true)

/// \brief Macro for adding a unique file logger with default parameters.
/// This macro adds a `UniqueFileLogger` with default settings, which writes each log message to a new file.
/// Log files are stored in the directory specified by `LOGIT_UNIQUE_FILE_LOGGER_PATH`, using asynchronous mode
/// and auto-deleting logs older than the number of days specified by `LOGIT_FILE_LOGGER_AUTO_DELETE_DAYS`.
/// This version uses `std::make_unique`, available in C++17 and later.
#define LOGIT_ADD_UNIQUE_FILE_LOGGER_DEFAULT()      \
    logit::Logger::get_instance().add_logger(       \
        std::make_unique<logit::UniqueFileLogger>(  \
            LOGIT_UNIQUE_FILE_LOGGER_PATH, true,    \
            LOGIT_FILE_LOGGER_AUTO_DELETE_DAYS),    \
        std::make_unique<logit::SimpleLogFormatter>(LOGIT_FILE_LOGGER_PATTERN));

/// \brief Macro for adding the default unique file logger in single_mode.
/// This macro adds a `UniqueFileLogger` with default settings, which writes each log message to a new file.
/// Log files are stored in the directory specified by `LOGIT_UNIQUE_FILE_LOGGER_PATH`, using asynchronous mode
/// and auto-deleting logs older than the number of days specified by `LOGIT_FILE_LOGGER_AUTO_DELETE_DAYS`.
/// In single_mode, the unique file logger can only be invoked using macros like `LOGIT_TRACE_TO`.
/// This version uses `std::make_unique`, available in C++17 and later.
#define LOGIT_ADD_UNIQUE_FILE_LOGGER_DEFAULT_SINGLE_MODE()                       \
    logit::Logger::get_instance().add_logger(                                    \
        std::make_unique<logit::UniqueFileLogger>(                               \
            LOGIT_UNIQUE_FILE_LOGGER_PATH, true,                                 \
            LOGIT_FILE_LOGGER_AUTO_DELETE_DAYS),                                 \
        std::make_unique<logit::SimpleLogFormatter>(LOGIT_FILE_LOGGER_PATTERN),  \
        true)

#else // C++11 fallback

/// \brief Macro for adding a logger with a specific formatter.
/// \param logger_type The type of logger (e.g., `ConsoleLogger`).
/// \param logger_args Arguments for the logger constructor, enclosed in parentheses.
/// \param formatter_type The type of formatter (e.g., `SimpleLogFormatter`).
/// \param formatter_args Arguments for the formatter constructor, enclosed in parentheses.
/// This version uses `new` and `std::unique_ptr` for C++11 compatibility.
#define LOGIT_ADD_LOGGER(logger_type, logger_args, formatter_type, formatter_args)  \
    logit::Logger::get_instance().add_logger(                                       \
        std::unique_ptr<logger_type>(new logger_type logger_args),                  \
        std::unique_ptr<formatter_type>(new formatter_type formatter_args),         \
        false)

/// \brief Macro for adding a logger with a specific formatter in single_mode.
/// \param logger_type The type of logger (e.g., `ConsoleLogger`).
/// \param logger_args Arguments for the logger constructor, enclosed in parentheses.
/// \param formatter_type The type of formatter (e.g., `SimpleLogFormatter`).
/// \param formatter_args Arguments for the formatter constructor, enclosed in parentheses.
/// In single_mode, loggers can only be invoked explicitly using macros like `LOGIT_TRACE_TO`.
/// This version uses `new` and `std::unique_ptr` for C++11 compatibility.
#define LOGIT_ADD_LOGGER_SINGLE_MODE(logger_type, logger_args, formatter_type, formatter_args)  \
    logit::Logger::get_instance().add_logger(                                                   \
        std::unique_ptr<logger_type>(new logger_type logger_args),                              \
        std::unique_ptr<formatter_type>(new formatter_type formatter_args),                     \
        true)

/// \brief Macro for adding a console logger with a specific pattern and mode.
/// \param pattern The format pattern for log messages.
/// \param async Boolean indicating whether logging should be asynchronous (true) or synchronous (false).
/// This version uses `new` and `std::unique_ptr` for C++11 compatibility.
#define LOGIT_ADD_CONSOLE(pattern, async)                                       \
    logit::Logger::get_instance().add_logger(                                   \
        std::unique_ptr<logit::ConsoleLogger>(new logit::ConsoleLogger(async)), \
        std::unique_ptr<logit::SimpleLogFormatter>(new logit::SimpleLogFormatter(pattern)), \
        false)

/// \brief Macro for adding a console logger in single_mode with a specific pattern and mode.
/// \param pattern The format pattern for log messages.
/// \param async Boolean indicating whether logging should be asynchronous (true) or synchronous (false).
/// In single_mode, the console logger can only be invoked using macros like `LOGIT_TRACE_TO`.
/// This version uses `new` and `std::unique_ptr` for C++11 compatibility.
#define LOGIT_ADD_CONSOLE_SINGLE_MODE(pattern, async)                           \
    logit::Logger::get_instance().add_logger(                                   \
        std::unique_ptr<logit::ConsoleLogger>(new logit::ConsoleLogger(async)), \
        std::unique_ptr<logit::SimpleLogFormatter>(new logit::SimpleLogFormatter(pattern)), \
        true)

/// \brief Macro for adding the default console logger.
/// This logger uses the default format pattern and asynchronous logging.
/// This version uses `new` and `std::unique_ptr` for C++11 compatibility.
#define LOGIT_ADD_CONSOLE_DEFAULT()                                             \
    logit::Logger::get_instance().add_logger(                                   \
        std::unique_ptr<logit::ConsoleLogger>(new logit::ConsoleLogger(true)),  \
        std::unique_ptr<logit::SimpleLogFormatter>(                             \
            new logit::SimpleLogFormatter(LOGIT_CONSOLE_PATTERN)),              \
        false)

/// \brief Macro for adding the default console logger in single_mode.
/// In single_mode, the console logger can only be invoked using macros like `LOGIT_TRACE_TO`.
/// This version uses `new` and `std::unique_ptr` for C++11 compatibility.
#define LOGIT_ADD_CONSOLE_DEFAULT_SINGLE_MODE()                                 \
    logit::Logger::get_instance().add_logger(                                   \
        std::unique_ptr<logit::ConsoleLogger>(new logit::ConsoleLogger(true)),  \
        std::unique_ptr<logit::SimpleLogFormatter>(                             \
            new logit::SimpleLogFormatter(LOGIT_CONSOLE_PATTERN)),              \
        true)

/// \brief Macro for adding a file logger with specific settings.
/// \param directory The directory where log files will be stored.
/// \param async Boolean indicating whether logging should be asynchronous (true) or synchronous (false).
/// \param auto_delete_days Number of days after which old log files are deleted.
/// \param pattern The format pattern for log messages.
/// This version uses `new` and `std::unique_ptr` for C++11 compatibility.
#define LOGIT_ADD_FILE_LOGGER(directory, async, auto_delete_days, pattern)  \
    logit::Logger::get_instance().add_logger(                               \
        std::unique_ptr<logit::FileLogger>(new logit::FileLogger(           \
            directory, async, auto_delete_days)),                           \
        std::unique_ptr<logit::SimpleLogFormatter>(new logit::SimpleLogFormatter(pattern)))

/// \brief Macro for adding a file logger in single_mode with specific settings.
/// \param directory The directory where log files will be stored.
/// \param async Boolean indicating whether logging should be asynchronous (true) or synchronous (false).
/// \param auto_delete_days Number of days after which old log files are deleted.
/// \param pattern The format pattern for log messages.
/// In single_mode, the file logger can only be invoked using macros like `LOGIT_TRACE_TO`.
/// This version uses `new` and `std::unique_ptr` for C++11 compatibility.
#define LOGIT_ADD_FILE_LOGGER_SINGLE_MODE(directory, async, auto_delete_days, pattern)  \
    logit::Logger::get_instance().add_logger(                                           \
        std::unique_ptr<logit::FileLogger>(new logit::FileLogger(                       \
            directory, async, auto_delete_days)),                                       \
        std::unique_ptr<logit::SimpleLogFormatter>(new logit::SimpleLogFormatter(pattern)), \
        true)

/// \brief Macro for adding the default file logger.
/// This logger writes logs to the default file path and deletes logs older than the default number of days.
/// This version uses `new` and `std::unique_ptr` for C++11 compatibility.
#define LOGIT_ADD_FILE_LOGGER_DEFAULT()                                     \
    logit::Logger::get_instance().add_logger(                               \
        std::unique_ptr<logit::FileLogger>(new logit::FileLogger(           \
            LOGIT_FILE_LOGGER_PATH, true,                                   \
            LOGIT_FILE_LOGGER_AUTO_DELETE_DAYS)),                           \
        std::unique_ptr<logit::SimpleLogFormatter>(                         \
            new logit::SimpleLogFormatter(LOGIT_FILE_LOGGER_PATTERN)))

/// \brief Macro for adding the default file logger in single_mode.
/// In single_mode, the file logger can only be invoked using macros like `LOGIT_TRACE_TO`.
/// This version uses `new` and `std::unique_ptr` for C++11 compatibility.
#define LOGIT_ADD_FILE_LOGGER_DEFAULT_SINGLE_MODE()                         \
    logit::Logger::get_instance().add_logger(                               \
        std::unique_ptr<logit::FileLogger>(new logit::FileLogger(           \
            LOGIT_FILE_LOGGER_PATH, true,                                   \
            LOGIT_FILE_LOGGER_AUTO_DELETE_DAYS)),                           \
        std::unique_ptr<logit::SimpleLogFormatter>(                         \
            new logit::SimpleLogFormatter(LOGIT_FILE_LOGGER_PATTERN)),      \
        true)

/// \brief Macro for adding a unique file logger with custom parameters.
/// \param directory The directory where log files will be stored.
/// \param async Boolean indicating whether logging should be asynchronous (true) or synchronous (false).
/// \param auto_delete_days Number of days after which old log files are deleted.
/// \param hash_length The length of the hash to be appended to the log file name.
/// \param pattern The format pattern for log messages.
/// This version uses `new` and `std::unique_ptr` for C++11 compatibility.
#define LOGIT_ADD_UNIQUE_FILE_LOGGER(directory, async, auto_delete_days, hash_length, pattern)  \
    logit::Logger::get_instance().add_logger(                                                   \
        std::unique_ptr<logit::UniqueFileLogger>(new logit::UniqueFileLogger(                   \
            directory, async, auto_delete_days, hash_length)),                                  \
        std::unique_ptr<logit::SimpleLogFormatter>(new logit::SimpleLogFormatter(pattern)))

/// \brief Macro for adding a unique file logger in single_mode with custom parameters.
/// \param directory The directory where log files will be stored.
/// \param async Boolean indicating whether logging should be asynchronous (true) or synchronous (false).
/// \param auto_delete_days Number of days after which old log files are deleted.
/// \param hash_length The length of the hash to be appended to the log file name.
/// \param pattern The format pattern for log messages.
/// In single_mode, the unique file logger can only be invoked using macros like `LOGIT_TRACE_TO`.
/// This version uses `new` and `std::unique_ptr` for C++11 compatibility.
#define LOGIT_ADD_UNIQUE_FILE_LOGGER_SINGLE_MODE(directory, async, auto_delete_days, hash_length, pattern)  \
    logit::Logger::get_instance().add_logger(                                                               \
        std::unique_ptr<logit::UniqueFileLogger>(new logit::UniqueFileLogger(                               \
            directory, async, auto_delete_days, hash_length)),                                              \
        std::unique_ptr<logit::SimpleLogFormatter>(new logit::SimpleLogFormatter(pattern)),                 \
        true)

/// \brief Macro for adding the default unique file logger.
/// This macro adds a `UniqueFileLogger` with default settings, which writes each log message to a new file.
/// Log files are stored in the directory specified by `LOGIT_UNIQUE_FILE_LOGGER_PATH`, using asynchronous mode
/// and auto-deleting logs older than the number of days specified by `LOGIT_FILE_LOGGER_AUTO_DELETE_DAYS`.
/// This version uses `new` and `std::unique_ptr` for C++11 compatibility.
#define LOGIT_ADD_UNIQUE_FILE_LOGGER_DEFAULT()      \
    logit::Logger::get_instance().add_logger(       \
        std::unique_ptr<logit::UniqueFileLogger>(new logit::UniqueFileLogger(   \
            LOGIT_UNIQUE_FILE_LOGGER_PATH, true,                                \
            LOGIT_FILE_LOGGER_AUTO_DELETE_DAYS)),                               \
        std::unique_ptr<logit::SimpleLogFormatter>(new logit::SimpleLogFormatter(LOGIT_FILE_LOGGER_PATTERN)))

/// \brief Macro for adding the default unique file logger in single_mode.
/// This macro adds a `UniqueFileLogger` with default settings, which writes each log message to a new file.
/// Log files are stored in the directory specified by `LOGIT_UNIQUE_FILE_LOGGER_PATH`, using asynchronous mode
/// and auto-deleting logs older than the number of days specified by `LOGIT_FILE_LOGGER_AUTO_DELETE_DAYS`.
/// In single_mode, the unique file logger can only be invoked using macros like `LOGIT_TRACE_TO`.
/// This version uses `new` and `std::unique_ptr` for C++11 compatibility.
#define LOGIT_ADD_UNIQUE_FILE_LOGGER_DEFAULT_SINGLE_MODE()                       \
    logit::Logger::get_instance().add_logger(                                    \
        std::unique_ptr<logit::UniqueFileLogger>(new logit::UniqueFileLogger(    \
            LOGIT_UNIQUE_FILE_LOGGER_PATH, true,                                 \
            LOGIT_FILE_LOGGER_AUTO_DELETE_DAYS)),                                \
        std::unique_ptr<logit::SimpleLogFormatter>(new logit::SimpleLogFormatter(LOGIT_FILE_LOGGER_PATTERN)), \
        true)

#endif // C++ version check

/// \brief Macro for retrieving a string parameter from a logger.
/// \param logger_index The index of the logger.
/// \param param The logger parameter to retrieve.
/// \return A string representing the requested parameter.
#define LOGIT_GET_STRING_PARAM(logger_index, param) \
    logit::Logger::get_instance().get_string_param(logger_index, param)

/// \brief Macro for retrieving an integer parameter from a logger.
/// \param logger_index The index of the logger.
/// \param param The logger parameter to retrieve.
/// \return An integer representing the requested parameter.
#define LOGIT_GET_INT_PARAM(logger_index, param) \
    logit::Logger::get_instance().get_int_param(logger_index, param)

/// \brief Macro for retrieving a floating-point parameter from a logger.
/// \param logger_index The index of the logger.
/// \param param The logger parameter to retrieve.
/// \return A double representing the requested parameter.
#define LOGIT_GET_FLOAT_PARAM(logger_index, param) \
    logit::Logger::get_instance().get_float_param(logger_index, param)

/// \brief Macro for retrieving the last log file name from a specific logger.
/// \param logger_index The index of the logger.
/// \return The name of the last file written to.
#define LOGIT_GET_LAST_FILE_NAME(logger_index) \
    logit::Logger::get_instance().get_string_param(logger_index, logit::LoggerParam::LastFileName)

/// \brief Macro for retrieving the last log file path from a specific logger.
/// \param logger_index The index of the logger.
/// \return The full path of the last file written to.
#define LOGIT_GET_LAST_FILE_PATH(logger_index) \
    logit::Logger::get_instance().get_string_param(logger_index, logit::LoggerParam::LastFilePath)

/// \brief Macro for retrieving the timestamp of the last log from a specific logger.
/// \param logger_index The index of the logger.
/// \return The timestamp of the last log.
#define LOGIT_GET_LAST_LOG_TIMESTAMP(logger_index) \
    logit::Logger::get_instance().get_int_param(logger_index, logit::LoggerParam::LastLogTimestamp)

/// \brief Macro for retrieving the time since the last log from a specific logger.
/// \param logger_index The index of the logger.
/// \return The time elapsed since the last log in seconds.
#define LOGIT_GET_TIME_SINCE_LAST_LOG(logger_index) \
    logit::Logger::get_instance().get_float_param(logger_index, logit::LoggerParam::TimeSinceLastLog)

/// \brief Macro for waiting for all asynchronous loggers to finish processing.
#define LOGIT_WAIT()    logit::Logger::get_instance().wait();

//------------------------------------------------------------------------------

#endif // _LOGIT_LOG_MACROS_HPP_INCLUDED
