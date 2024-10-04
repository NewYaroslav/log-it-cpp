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

#define LOGIT_STREAM_TRACE()    LOGIT_STREAM(logit::LogLevel::LOG_LVL_TRACE)
#define LOGIT_STREAM_DEBUG()    LOGIT_STREAM(logit::LogLevel::LOG_LVL_DEBUG)
#define LOGIT_STREAM_INFO()     LOGIT_STREAM(logit::LogLevel::LOG_LVL_INFO)
#define LOGIT_STREAM_WARN()     LOGIT_STREAM(logit::LogLevel::LOG_LVL_WARN)
#define LOGIT_STREAM_ERROR()    LOGIT_STREAM(logit::LogLevel::LOG_LVL_ERROR)
#define LOGIT_STREAM_FATAL()    LOGIT_STREAM(logit::LogLevel::LOG_LVL_FATAL)

#define LOGIT_STREAM_TRACE_TO(index)    LOGIT_STREAM_WITH_INDEX(logit::LogLevel::LOG_LVL_TRACE, index)
#define LOGIT_STREAM_DEBUG_TO(index)    LOGIT_STREAM_WITH_INDEX(logit::LogLevel::LOG_LVL_DEBUG, index)
#define LOGIT_STREAM_INFO_TO(index)     LOGIT_STREAM_WITH_INDEX(logit::LogLevel::LOG_LVL_INFO, index)
#define LOGIT_STREAM_WARN_TO(index)     LOGIT_STREAM_WITH_INDEX(logit::LogLevel::LOG_LVL_WARN, index)
#define LOGIT_STREAM_ERROR_TO(index)    LOGIT_STREAM_WITH_INDEX(logit::LogLevel::LOG_LVL_ERROR, index)
#define LOGIT_STREAM_FATAL_TO(index)    LOGIT_STREAM_WITH_INDEX(logit::LogLevel::LOG_LVL_FATAL, index)

#if defined(LOGIT_SHORT_NAME)
// Shorter versions of the stream-based logging macros when LOGIT_SHORT_NAME is defined

#define LOG_S_TRACE     LOGIT_STREAM_TRACE()
#define LOG_S_DEBUG     LOGIT_STREAM_DEBUG()
#define LOG_S_INFO      LOGIT_STREAM_INFO()
#define LOG_S_WARN      LOGIT_STREAM_WARN()
#define LOG_S_ERROR     LOGIT_STREAM_ERROR()
#define LOG_S_FATAL     LOGIT_STREAM_FATAL()

#define LOG_S_TRACE_TO(index)  LOGIT_STREAM_TRACE_TO(index)
#define LOG_S_DEBUG_TO(index)  LOGIT_STREAM_DEBUG_TO(index)
#define LOG_S_INFO_TO(index)   LOGIT_STREAM_INFO_TO(index)
#define LOG_S_WARN_TO(index)   LOGIT_STREAM_WARN_TO(index)
#define LOG_S_ERROR_TO(index)  LOGIT_STREAM_ERROR_TO(index)
#define LOG_S_FATAL_TO(index)  LOGIT_STREAM_FATAL_TO(index)

#endif // LOGIT_SHORT_NAME

//------------------------------------------------------------------------------
// Macros for logging without arguments

/// \brief Logs a message without arguments.
/// \param level The log level.
/// \param format The log message format.
#define LOGIT_LOG_AND_RETURN_NOARGS(level, format)              \
    logit::Logger::get_instance().log_and_return(               \
        logit::LogRecord{level, LOGIT_CURRENT_TIMESTAMP_MS(),   \
        logit::make_relative(__FILE__, LOGIT_BASE_PATH), __LINE__, LOGIT_FUNCTION, format, {}, -1})

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
        LOGIT_FUNCTION, format, arg_names, -1}, __VA_ARGS__)

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
        LOGIT_FUNCTION, format, arg_names, index}, __VA_ARGS__)

//------------------------------------------------------------------------------
// Macros for each log level

// TRACE level macros
#define LOGIT_TRACE(...)                LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_TRACE, {}, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_TRACE0()                  LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_TRACE, {})
#define LOGIT_NOARGS_TRACE()            LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_TRACE, {})
#define LOGIT_FORMAT_TRACE(fmt, ...)    LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_TRACE, fmt, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINT_TRACE(fmt)          LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_TRACE, fmt)
#define LOGIT_PRINTF_TRACE(fmt, ...)    LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_TRACE, logit::format(fmt, __VA_ARGS__))

// TRACE macros with index
#define LOGIT_TRACE_TO(index, ...)      LOGIT_LOG_AND_RETURN_WITH_INDEX(logit::LogLevel::LOG_LVL_TRACE, index, {}, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_TRACE0_TO(index)          LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_TRACE, index, {})
#define LOGIT_NOARGS_TRACE_TO(index)    LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_TRACE, index, {})
#define LOGIT_FORMAT_TRACE_TO(index, fmt, ...) LOGIT_LOG_AND_RETURN_WITH_INDEX(logit::LogLevel::LOG_LVL_TRACE, index, fmt, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINT_TRACE_TO(index, fmt)       LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_TRACE, index, fmt)
#define LOGIT_PRINTF_TRACE_TO(index, fmt, ...) LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_TRACE, index, logit::format(fmt, __VA_ARGS__))

// INFO level macros
#define LOGIT_INFO(...)                 LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_INFO, {}, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_INFO0()                   LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_INFO, {})
#define LOGIT_NOARGS_INFO()             LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_INFO, {})
#define LOGIT_FORMAT_INFO(fmt, ...)     LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_INFO, fmt, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINT_INFO(fmt)           LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_INFO, fmt)
#define LOGIT_PRINTF_INFO(fmt, ...)     LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_INFO, logit::format(fmt, __VA_ARGS__))

// INFO macros with index
#define LOGIT_INFO_TO(index, ...)       LOGIT_LOG_AND_RETURN_WITH_INDEX(logit::LogLevel::LOG_LVL_INFO, index, {}, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_INFO0_TO(index)           LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_INFO, index, {})
#define LOGIT_NOARGS_INFO_TO(index)     LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_INFO, index, {})
#define LOGIT_FORMAT_INFO_TO(index, fmt, ...) LOGIT_LOG_AND_RETURN_WITH_INDEX(logit::LogLevel::LOG_LVL_INFO, index, fmt, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINT_INFO_TO(index, fmt)       LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_INFO, index, fmt)
#define LOGIT_PRINTF_INFO_TO(index, fmt, ...) LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_INFO, index, logit::format(fmt, __VA_ARGS__))

// DEBUG level macros
#define LOGIT_DEBUG(...)                LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_DEBUG, {}, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_DEBUG0()                  LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_DEBUG, {})
#define LOGIT_NOARGS_DEBUG()            LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_DEBUG, {})
#define LOGIT_FORMAT_DEBUG(fmt, ...)    LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_DEBUG, fmt, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINT_DEBUG(fmt)          LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_DEBUG, fmt)
#define LOGIT_PRINTF_DEBUG(fmt, ...)    LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_DEBUG, logit::format(fmt, __VA_ARGS__))

// DEBUG macros with index
#define LOGIT_DEBUG_TO(index, ...)      LOGIT_LOG_AND_RETURN_WITH_INDEX(logit::LogLevel::LOG_LVL_DEBUG, index, {}, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_DEBUG0_TO(index)          LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_DEBUG, index, {})
#define LOGIT_NOARGS_DEBUG_TO(index)    LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_DEBUG, index, {})
#define LOGIT_FORMAT_DEBUG_TO(index, fmt, ...) LOGIT_LOG_AND_RETURN_WITH_INDEX(logit::LogLevel::LOG_LVL_DEBUG, index, fmt, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINT_DEBUG_TO(index, fmt)       LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_DEBUG, index, fmt)
#define LOGIT_PRINTF_DEBUG_TO(index, fmt, ...) LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_DEBUG, index, logit::format(fmt, __VA_ARGS__))

// WARN level macros
#define LOGIT_WARN(...)                 LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_WARN, {}, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_WARN0()                   LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_WARN, {})
#define LOGIT_NOARGS_WARN()             LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_WARN, {})
#define LOGIT_FORMAT_WARN(fmt, ...)     LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_WARN, fmt, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINT_WARN(fmt)           LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_WARN, fmt)
#define LOGIT_PRINTF_WARN(fmt, ...)     LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_WARN, logit::format(fmt, __VA_ARGS__))

// WARN macros with index
#define LOGIT_WARN_TO(index, ...)       LOGIT_LOG_AND_RETURN_WITH_INDEX(logit::LogLevel::LOG_LVL_WARN, index, {}, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_WARN0_TO(index)           LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_WARN, index, {})
#define LOGIT_NOARGS_WARN_TO(index)     LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_WARN, index, {})
#define LOGIT_FORMAT_WARN_TO(index, fmt, ...) LOGIT_LOG_AND_RETURN_WITH_INDEX(logit::LogLevel::LOG_LVL_WARN, index, fmt, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINT_WARN_TO(index, fmt)       LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_WARN, index, fmt)
#define LOGIT_PRINTF_WARN_TO(index, fmt, ...) LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_WARN, index, logit::format(fmt, __VA_ARGS__))

// ERROR level macros
#define LOGIT_ERROR(...)                LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_ERROR, {}, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_ERROR0()                  LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_ERROR, {})
#define LOGIT_NOARGS_ERROR()            LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_ERROR, {})
#define LOGIT_FORMAT_ERROR(fmt, ...)    LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_ERROR, fmt, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINT_ERROR(fmt)          LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_ERROR, fmt)
#define LOGIT_PRINTF_ERROR(fmt, ...)    LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_ERROR, logit::format(fmt, __VA_ARGS__))

// ERROR macros with index
#define LOGIT_ERROR_TO(index, ...)      LOGIT_LOG_AND_RETURN_WITH_INDEX(logit::LogLevel::LOG_LVL_ERROR, index, {}, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_ERROR0_TO(index)          LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_ERROR, index, {})
#define LOGIT_NOARGS_ERROR_TO(index)    LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_ERROR, index, {})
#define LOGIT_FORMAT_ERROR_TO(index, fmt, ...) LOGIT_LOG_AND_RETURN_WITH_INDEX(logit::LogLevel::LOG_LVL_ERROR, index, fmt, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINT_ERROR_TO(index, fmt)       LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_ERROR, index, fmt)
#define LOGIT_PRINTF_ERROR_TO(index, fmt, ...) LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_ERROR, index, logit::format(fmt, __VA_ARGS__))

// FATAL level macros
#define LOGIT_FATAL(...)                LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_FATAL, {}, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_FATAL0()                  LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_FATAL, {})
#define LOGIT_NOARGS_FATAL()            LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_FATAL, {})
#define LOGIT_FORMAT_FATAL(fmt, ...)    LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_FATAL, fmt, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINT_FATAL(fmt)          LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_FATAL, fmt)
#define LOGIT_PRINTF_FATAL(fmt, ...)    LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_FATAL, logit::format(fmt, __VA_ARGS__))

// FATAL macros with index
#define LOGIT_FATAL_TO(index, ...)      LOGIT_LOG_AND_RETURN_WITH_INDEX(logit::LogLevel::LOG_LVL_FATAL, index, {}, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_FATAL0_TO(index)          LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_FATAL, index, {})
#define LOGIT_NOARGS_FATAL_TO(index)    LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_FATAL, index, {})
#define LOGIT_FORMAT_FATAL_TO(index, fmt, ...) LOGIT_LOG_AND_RETURN_WITH_INDEX(logit::LogLevel::LOG_LVL_FATAL, index, fmt, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINT_FATAL_TO(index, fmt)       LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_FATAL, index, fmt)
#define LOGIT_PRINTF_FATAL_TO(index, fmt, ...) LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_FATAL, index, logit::format(fmt, __VA_ARGS__))

//------------------------------------------------------------------------------
// Conditional logging macros (logging based on a condition)

// TRACE level conditional macros
#define LOGIT_TRACE_IF(condition, ...)        if (condition) LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_TRACE, {}, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_TRACE0_IF(condition)            if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_TRACE, {})
#define LOGIT_NOARGS_TRACE_IF(condition)      if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_TRACE, {})
#define LOGIT_FORMAT_TRACE_IF(condition, fmt, ...) if (condition) LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_TRACE, fmt, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINT_TRACE_IF(condition, fmt)  if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_TRACE, fmt)
#define LOGIT_PRINTF_TRACE_IF(condition, fmt, ...) if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_TRACE, logit::format(fmt, __VA_ARGS__))

// INFO level conditional macros
#define LOGIT_INFO_IF(condition, ...)         if (condition) LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_INFO, {}, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_INFO0_IF(condition)             if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_INFO, {})
#define LOGIT_NOARGS_INFO_IF(condition)       if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_INFO, {})
#define LOGIT_FORMAT_INFO_IF(condition, fmt, ...) if (condition) LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_INFO, fmt, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINT_INFO_IF(condition, fmt)   if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_INFO, fmt)
#define LOGIT_PRINTF_INFO_IF(condition, fmt, ...) if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_INFO, logit::format(fmt, __VA_ARGS__))

// DEBUG level conditional macros
#define LOGIT_DEBUG_IF(condition, ...)        if (condition) LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_DEBUG, {}, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_DEBUG0_IF(condition)            if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_DEBUG, {})
#define LOGIT_NOARGS_DEBUG_IF(condition)      if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_DEBUG, {})
#define LOGIT_FORMAT_DEBUG_IF(condition, fmt, ...) if (condition) LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_DEBUG, fmt, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINT_DEBUG_IF(condition, fmt)  if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_DEBUG, fmt)
#define LOGIT_PRINTF_DEBUG_IF(condition, fmt, ...) if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_DEBUG, logit::format(fmt, __VA_ARGS__))

// WARN level conditional macros
#define LOGIT_WARN_IF(condition, ...)         if (condition) LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_WARN, {}, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_WARN0_IF(condition)             if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_WARN, {})
#define LOGIT_NOARGS_WARN_IF(condition)       if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_WARN, {})
#define LOGIT_FORMAT_WARN_IF(condition, fmt, ...) if (condition) LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_WARN, fmt, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINT_WARN_IF(condition, fmt)   if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_WARN, fmt)
#define LOGIT_PRINTF_WARN_IF(condition, fmt, ...) if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_WARN, logit::format(fmt, __VA_ARGS__))

// ERROR level conditional macros
#define LOGIT_ERROR_IF(condition, ...)        if (condition) LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_ERROR, {}, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_ERROR0_IF(condition)            if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_ERROR, {})
#define LOGIT_NOARGS_ERROR_IF(condition)      if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_ERROR, {})
#define LOGIT_FORMAT_ERROR_IF(condition, fmt, ...) if (condition) LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_ERROR, fmt, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINT_ERROR_IF(condition, fmt)  if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_ERROR, fmt)
#define LOGIT_PRINTF_ERROR_IF(condition, fmt, ...) if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_ERROR, logit::format(fmt, __VA_ARGS__))

// FATAL level conditional macros
#define LOGIT_FATAL_IF(condition, ...)        if (condition) LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_FATAL, {}, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_FATAL0_IF(condition)            if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_FATAL, {})
#define LOGIT_NOARGS_FATAL_IF(condition)      if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_FATAL, {})
#define LOGIT_FORMAT_FATAL_IF(condition, fmt, ...) if (condition) LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_FATAL, fmt, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINT_FATAL_IF(condition, fmt)  if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_FATAL, fmt)
#define LOGIT_PRINTF_FATAL_IF(condition, fmt, ...) if (condition) LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_FATAL, logit::format(fmt, __VA_ARGS__))

//------------------------------------------------------------------------------


#if defined(LOGIT_SHORT_NAME)
// Уровень TRACE
#define LOG_TRACE(...)                 LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_TRACE, std::string(), #__VA_ARGS__, __VA_ARGS__)
#define LOG_TRACE0()                   LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_TRACE, std::string())
#define LOG_TRACE_NOARGS()             LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_TRACE, std::string())
#define LOG_FORMAT_TRACE(format, ...)  LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_TRACE, format, #__VA_ARGS__, __VA_ARGS__)
#define LOG_PRINT_TRACE(format)        LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_TRACE, format)
#define LOG_PRINTF_TRACE(format, ...)  LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_TRACE, logit::format(format, __VA_ARGS__))
#endif

#if defined(LOGIT_SHORT_NAME)
// Shorter versions of the macros when LOGIT_SHORT_NAME is defined

// TRACE level
#define LOG_T(...)                      LOGIT_TRACE(__VA_ARGS__)
#define LOG_T0()                        LOGIT_TRACE0()
#define LOG_T_NOARGS()                  LOGIT_NOARGS_TRACE()
#define LOG_TF(fmt, ...)                LOGIT_FORMAT_TRACE(fmt, __VA_ARGS__)
#define LOG_T_PRINT(fmt)                LOGIT_PRINT_TRACE(fmt)
#define LOG_T_PRINTF(fmt, ...)          LOGIT_PRINTF_TRACE(fmt, __VA_ARGS__)

#define LOG_TRACE(...)                  LOGIT_TRACE(__VA_ARGS__)
#define LOG_TRACE0()                    LOGIT_TRACE0()
#define LOG_TRACE_NOARGS()              LOGIT_NOARGS_TRACE()
#define LOG_TRACEF(fmt, ...)            LOGIT_FORMAT_TRACE(fmt, __VA_ARGS__)
#define LOG_TRACE_PRINT(fmt)            LOGIT_PRINT_TRACE(fmt)
#define LOG_TRACE_PRINTF(fmt, ...)      LOGIT_PRINTF_TRACE(fmt, __VA_ARGS__)

// INFO level
#define LOG_I(...)                      LOGIT_INFO(__VA_ARGS__)
#define LOG_I0()                        LOGIT_INFO0()
#define LOG_I_NOARGS()                  LOGIT_NOARGS_INFO()
#define LOG_IF(fmt, ...)                LOGIT_FORMAT_INFO(fmt, __VA_ARGS__)
#define LOG_I_PRINT(fmt)                LOGIT_PRINT_INFO(fmt)
#define LOG_I_PRINTF(fmt, ...)          LOGIT_PRINTF_INFO(fmt, __VA_ARGS__)

#define LOG_INFO(...)                   LOGIT_INFO(__VA_ARGS__)
#define LOG_INFO0()                     LOGIT_INFO0()
#define LOG_INFO_NOARGS()               LOGIT_NOARGS_INFO()
#define LOG_INFOF(fmt, ...)             LOGIT_FORMAT_INFO(fmt, __VA_ARGS__)
#define LOG_INFO_PRINT(fmt)             LOGIT_PRINT_INFO(fmt)
#define LOG_INFO_PRINTF(fmt, ...)       LOGIT_PRINTF_INFO(fmt, __VA_ARGS__)

// DEBUG level
#define LOG_D(...)                      LOGIT_DEBUG(__VA_ARGS__)
#define LOG_D0()                        LOGIT_DEBUG0()
#define LOG_D_NOARGS()                  LOGIT_NOARGS_DEBUG()
#define LOG_DF(fmt, ...)                LOGIT_FORMAT_DEBUG(fmt, __VA_ARGS__)
#define LOG_D_PRINT(fmt)                LOGIT_PRINT_DEBUG(fmt)
#define LOG_D_PRINTF(fmt, ...)          LOGIT_PRINTF_DEBUG(fmt, __VA_ARGS__)

#define LOG_DEBUG(...)                  LOGIT_DEBUG(__VA_ARGS__)
#define LOG_DEBUG0()                    LOGIT_DEBUG0()
#define LOG_DEBUG_NOARGS()              LOGIT_NOARGS_DEBUG()
#define LOG_DEBUGF(fmt, ...)            LOGIT_FORMAT_DEBUG(fmt, __VA_ARGS__)
#define LOG_DEBUG_PRINT(fmt)            LOGIT_PRINT_DEBUG(fmt)
#define LOG_DEBUG_PRINTF(fmt, ...)      LOGIT_PRINTF_DEBUG(fmt, __VA_ARGS__)

// WARN level
#define LOG_W(...)                      LOGIT_WARN(__VA_ARGS__)
#define LOG_W0()                        LOGIT_WARN0()
#define LOG_W_NOARGS()                  LOGIT_NOARGS_WARN()
#define LOG_WF(fmt, ...)                LOGIT_FORMAT_WARN(fmt, __VA_ARGS__)
#define LOG_W_PRINT(fmt)                LOGIT_PRINT_WARN(fmt)
#define LOG_W_PRINTF(fmt, ...)          LOGIT_PRINTF_WARN(fmt, __VA_ARGS__)

#define LOG_WARN(...)                   LOGIT_WARN(__VA_ARGS__)
#define LOG_WARN0()                     LOGIT_WARN0()
#define LOG_WARN_NOARGS()               LOGIT_NOARGS_WARN()
#define LOG_WARNF(fmt, ...)             LOGIT_FORMAT_WARN(fmt, __VA_ARGS__)
#define LOG_WARN_PRINT(fmt)             LOGIT_PRINT_WARN(fmt)
#define LOG_WARN_PRINTF(fmt, ...)       LOGIT_PRINTF_WARN(fmt, __VA_ARGS__)

// ERROR level
#define LOG_E(...)                      LOGIT_ERROR(__VA_ARGS__)
#define LOG_E0()                        LOGIT_ERROR0()
#define LOG_E_NOARGS()                  LOGIT_NOARGS_ERROR()
#define LOG_EF(fmt, ...)                LOGIT_FORMAT_ERROR(fmt, __VA_ARGS__)
#define LOG_E_PRINT(fmt)                LOGIT_PRINT_ERROR(fmt)
#define LOG_E_PRINTF(fmt, ...)          LOGIT_PRINTF_ERROR(fmt, __VA_ARGS__)

#define LOG_ERROR(...)                  LOGIT_ERROR(__VA_ARGS__)
#define LOG_ERROR0()                    LOGIT_ERROR0()
#define LOG_ERROR_NOARGS()              LOGIT_NOARGS_ERROR()
#define LOG_ERRORF(fmt, ...)            LOGIT_FORMAT_ERROR(fmt, __VA_ARGS__)
#define LOG_ERROR_PRINT(fmt)            LOGIT_PRINT_ERROR(fmt)
#define LOG_ERROR_PRINTF(fmt, ...)      LOGIT_PRINTF_ERROR(fmt, __VA_ARGS__)

// FATAL level
#define LOG_F(...)                      LOGIT_FATAL(__VA_ARGS__)
#define LOG_F0()                        LOGIT_FATAL0()
#define LOG_F_NOARGS()                  LOGIT_NOARGS_FATAL()
#define LOG_FF(fmt, ...)                LOGIT_FORMAT_FATAL(fmt, __VA_ARGS__)
#define LOG_F_PRINT(fmt)                LOGIT_PRINT_FATAL(fmt)
#define LOG_F_PRINTF(fmt, ...)          LOGIT_PRINTF_FATAL(fmt, __VA_ARGS__)

#define LOG_FATAL(...)                  LOGIT_FATAL(__VA_ARGS__)
#define LOG_FATAL0()                    LOGIT_FATAL0()
#define LOG_FATAL_NOARGS()              LOGIT_NOARGS_FATAL()
#define LOG_FATALF(fmt, ...)            LOGIT_FORMAT_FATAL(fmt, __VA_ARGS__)
#define LOG_FATAL_PRINT(fmt)            LOGIT_PRINT_FATAL(fmt)
#define LOG_FATAL_PRINTF(fmt, ...)      LOGIT_PRINTF_FATAL(fmt, __VA_ARGS__)

#endif // LOGIT_SHORT_NAME

//------------------------------------------------------------------------------
// Macros for starting the console logger with optional pattern and mode (async/sync)

#if __cplusplus >= 201703L // C++17 or later

/// \brief Macro for starting the logger with console output and a specific pattern and mode (C++17 or later).
/// \param pattern The format pattern for log messages.
/// \param async Boolean indicating whether logging should be asynchronous (true) or synchronous (false).
///
/// This version uses `std::make_unique`, available in C++17 and later.
#define LOGIT_CONSOLE(pattern, async)                           \
    logit::Logger::get_instance().add_logger(                   \
        std::make_unique<logit::ConsoleLogger>(async),          \
        std::make_unique<logit::SimpleLogFormatter>(pattern));

/// \brief Macro for starting the logger with default console output (C++17 or later).
#define LOGIT_CONSOLE_DEFAULT()                                 \
    logit::Logger::get_instance().add_logger(                   \
        std::make_unique<logit::ConsoleLogger>(true),           \
        std::make_unique<logit::SimpleLogFormatter>("%H:%M:%S.%e | %^%v%$"));

#else // C++11 fallback

/// \brief Macro for starting the logger with console output and a specific pattern and mode (C++11).
/// \param pattern The format pattern for log messages.
/// \param async Boolean indicating whether logging should be asynchronous (true) or synchronous (false).
///
/// This version uses `new` and `std::unique_ptr` for C++11 compatibility.
#define LOGIT_CONSOLE(pattern, async)                           \
    logit::Logger::get_instance().add_logger(                   \
        std::unique_ptr<logit::ConsoleLogger>(new logit::ConsoleLogger(async)),  \
        std::unique_ptr<logit::SimpleLogFormatter>(new logit::SimpleLogFormatter(pattern)));

/// \brief Macro for starting the logger with default console output (C++11).
#define LOGIT_CONSOLE_DEFAULT()                                 \
    logit::Logger::get_instance().add_logger(                   \
        std::unique_ptr<logit::ConsoleLogger>(new logit::ConsoleLogger(true)),   \
        std::unique_ptr<logit::SimpleLogFormatter>(new logit::SimpleLogFormatter("%H:%M:%S.%e | %^%v%$")));

#endif // C++ version check

/// \brief Macro for waiting for all asynchronous loggers to finish processing.
#define LOGIT_WAIT()    logit::Logger::get_instance().wait();

//------------------------------------------------------------------------------

#endif // _LOGIT_LOG_MACROS_HPP_INCLUDED
