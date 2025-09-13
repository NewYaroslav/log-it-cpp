#pragma once
#ifndef _LOGIT_LOG_MACROS_HPP_INCLUDED
#define _LOGIT_LOG_MACROS_HPP_INCLUDED

/// \file LogMacros.hpp
/// \brief Provides various logging macros for different log levels and options.

/// \ingroup LoggingMacros
/// \{

//------------------------------------------------------------------------------
// Function name macro for different compilers
#if defined(__GNUC__)
    #define LOGIT_FUNCTION __PRETTY_FUNCTION__
#elif defined(_MSC_VER)
    #define LOGIT_FUNCTION __FUNCSIG__
#else
    #define LOGIT_FUNCTION __func__
#endif

/// \brief Expands to a `case` statement returning the stringified enum value.
/// \param value The enum value.
#define LOGIT_ENUM_TO_STR_CASE(value) case value: return #value;

//------------------------------------------------------------------------------
// Stream-based logging macros for various levels

/// \name Stream-Based Logging
/// Macros for logging using a stream-like syntax.
/// \{

/// \brief Begin a log stream for the specified log level.
#define LOGIT_STREAM(level) \
    logit::LogStream(level, logit::make_relative(__FILE__, LOGIT_BASE_PATH), __LINE__, LOGIT_FUNCTION, -1)

/// \brief Begin a log stream for the specified log level, targeting a specific logger.
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

/// \}

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

/// \cond INTERNAL
/// \brief Helper macro defining the logging macros for a specific level.
/// \param NAME    Macro base name (e.g. TRACE).
/// \param LEVEL   Enumeration value of ::logit::LogLevel.
#define LOGIT_DEFINE_LOG_LEVEL_MACROS(NAME, LEVEL)                                             \
    /// \brief Logs a message.                                                                 \
    /// \param ... The arguments to log.                                                       \
    #define LOGIT_##NAME(...)                LOGIT_LOG_AND_RETURN(LEVEL, {}, #__VA_ARGS__, __VA_ARGS__)                 \
    /// \brief Logs a message without arguments.                                               \
    #define LOGIT_##NAME##0()                LOGIT_LOG_AND_RETURN_NOARGS(LEVEL, {})                               \
    #define LOGIT_0##NAME()                  LOGIT_LOG_AND_RETURN_NOARGS(LEVEL, {})                               \
    #define LOGIT_0_##NAME()                 LOGIT_LOG_AND_RETURN_NOARGS(LEVEL, {})                               \
    #define LOGIT_NOARGS_##NAME()            LOGIT_LOG_AND_RETURN_NOARGS(LEVEL, {})                               \
    /// \brief Logs a formatted message.                                                      \
    /// \param fmt Format string.                                                             \
    /// \param ... The arguments to log.                                                      \
    #define LOGIT_FORMAT_##NAME(fmt, ...)    LOGIT_LOG_AND_RETURN(LEVEL, fmt, #__VA_ARGS__, __VA_ARGS__)             \
    /// \brief Logs arguments without formatting.                                             \
    /// \param ... The arguments to log.                                                      \
    #define LOGIT_PRINT_##NAME(...)          LOGIT_LOG_AND_RETURN_PRINT(LEVEL, #__VA_ARGS__, __VA_ARGS__)            \
    /// \brief Logs a printf-style formatted message.                                         \
    /// \param fmt Format string.                                                             \
    /// \param ... The arguments to log.                                                      \
    #define LOGIT_PRINTF_##NAME(fmt, ...)    LOGIT_LOG_AND_RETURN_NOARGS(LEVEL, logit::format(fmt, __VA_ARGS__))     \
    /// \brief Logs a message to a specific logger.                                           \
    /// \param index Logger index.                                                            \
    /// \param ... The arguments to log.                                                      \
    #define LOGIT_##NAME##_TO(index, ...)    LOGIT_LOG_AND_RETURN_WITH_INDEX(LEVEL, index, {}, #__VA_ARGS__, __VA_ARGS__) \
    /// \brief Logs a message without arguments to a specific logger.                         \
    /// \param index Logger index.                                                            \
    #define LOGIT_##NAME##0_TO(index)        LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(LEVEL, index, {})               \
    #define LOGIT_0##NAME##_TO(index)        LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(LEVEL, index, {})               \
    #define LOGIT_0_##NAME##_TO(index)       LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(LEVEL, index, {})               \
    #define LOGIT_NOARGS_##NAME##_TO(index)  LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(LEVEL, index, {})               \
    /// \brief Logs a formatted message to a specific logger.                                 \
    /// \param index Logger index.                                                            \
    /// \param fmt Format string.                                                             \
    /// \param ... The arguments to log.                                                      \
    #define LOGIT_FORMAT_##NAME##_TO(index, fmt, ...) LOGIT_LOG_AND_RETURN_WITH_INDEX(LEVEL, index, fmt, #__VA_ARGS__, __VA_ARGS__) \
    /// \brief Logs arguments without formatting to a specific logger.                        \
    /// \param index Logger index.                                                            \
    /// \param ... The arguments to log.                                                      \
    #define LOGIT_PRINT_##NAME##_TO(index, ...)       LOGIT_LOG_AND_RETURN_PRINT_WITH_INDEX(LEVEL, index, #__VA_ARGS__, __VA_ARGS__) \
    /// \brief Logs a printf-style formatted message to a specific logger.                    \
    /// \param index Logger index.                                                            \
    /// \param fmt Format string.                                                             \
    /// \param ... The arguments to log.                                                      \
    #define LOGIT_PRINTF_##NAME##_TO(index, fmt, ...) LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(LEVEL, index, logit::format(fmt, __VA_ARGS__))

/// \brief Helper macro defining the conditional logging macros for a specific level.
/// \param NAME    Macro base name (e.g. TRACE).
/// \param LEVEL   Enumeration value of ::logit::LogLevel.
#define LOGIT_DEFINE_LOG_LEVEL_IF_MACROS(NAME, LEVEL)                                         \
    /// \brief Logs a message if a condition is true.                                         \
    /// \param condition Condition to check.                                                 \
    /// \param ... The arguments to log.                                                     \
    #define LOGIT_##NAME##_IF(condition, ...)        if (condition) LOGIT_LOG_AND_RETURN(LEVEL, {}, #__VA_ARGS__, __VA_ARGS__) \
    /// \brief Logs a message without arguments if a condition is true.                      \
    /// \param condition Condition to check.                                                 \
    #define LOGIT_##NAME##0_IF(condition)            if (condition) LOGIT_LOG_AND_RETURN_NOARGS(LEVEL, {})          \
    #define LOGIT_0##NAME##_IF(condition)            if (condition) LOGIT_LOG_AND_RETURN_NOARGS(LEVEL, {})          \
    #define LOGIT_0_##NAME##_IF(condition)           if (condition) LOGIT_LOG_AND_RETURN_NOARGS(LEVEL, {})          \
    #define LOGIT_NOARGS_##NAME##_IF(condition)      if (condition) LOGIT_LOG_AND_RETURN_NOARGS(LEVEL, {})          \
    /// \brief Logs a formatted message if a condition is true.                              \
    /// \param condition Condition to check.                                                 \
    /// \param fmt Format string.                                                            \
    /// \param ... The arguments to log.                                                     \
    #define LOGIT_FORMAT_##NAME##_IF(condition, fmt, ...) if (condition) LOGIT_LOG_AND_RETURN(LEVEL, fmt, #__VA_ARGS__, __VA_ARGS__) \
    /// \brief Logs a raw message if a condition is true.                                    \
    /// \param condition Condition to check.                                                 \
    /// \param fmt Message or format string.                                                \
    #define LOGIT_PRINT_##NAME##_IF(condition, fmt)  if (condition) LOGIT_LOG_AND_RETURN_NOARGS(LEVEL, fmt)         \
    /// \brief Logs a printf-style formatted message if a condition is true.                 \
    /// \param condition Condition to check.                                                 \
    /// \param fmt Format string.                                                            \
    /// \param ... The arguments to log.                                                     \
    #define LOGIT_PRINTF_##NAME##_IF(condition, fmt, ...) if (condition) LOGIT_LOG_AND_RETURN_NOARGS(LEVEL, logit::format(fmt, __VA_ARGS__))
/// \endcond

//------------------------------------------------------------------------------
// Macros for each log level

// TRACE level macros
LOGIT_DEFINE_LOG_LEVEL_MACROS(TRACE, logit::LogLevel::LOG_LVL_TRACE)

// INFO level macros
LOGIT_DEFINE_LOG_LEVEL_MACROS(INFO,  logit::LogLevel::LOG_LVL_INFO)

// DEBUG level macros
LOGIT_DEFINE_LOG_LEVEL_MACROS(DEBUG, logit::LogLevel::LOG_LVL_DEBUG)

// WARN level macros
LOGIT_DEFINE_LOG_LEVEL_MACROS(WARN,  logit::LogLevel::LOG_LVL_WARN)

// ERROR level macros
LOGIT_DEFINE_LOG_LEVEL_MACROS(ERROR, logit::LogLevel::LOG_LVL_ERROR)

// FATAL level macros
LOGIT_DEFINE_LOG_LEVEL_MACROS(FATAL, logit::LogLevel::LOG_LVL_FATAL)

//------------------------------------------------------------------------------
// Conditional logging macros (logging based on a condition)

/// \name Conditional Logging
/// Macros for logging based on conditions.
/// \{

// TRACE level conditional macros
LOGIT_DEFINE_LOG_LEVEL_IF_MACROS(TRACE, logit::LogLevel::LOG_LVL_TRACE)

// INFO level conditional macros
LOGIT_DEFINE_LOG_LEVEL_IF_MACROS(INFO,  logit::LogLevel::LOG_LVL_INFO)

// DEBUG level conditional macros
LOGIT_DEFINE_LOG_LEVEL_IF_MACROS(DEBUG, logit::LogLevel::LOG_LVL_DEBUG)

// WARN level conditional macros
LOGIT_DEFINE_LOG_LEVEL_IF_MACROS(WARN,  logit::LogLevel::LOG_LVL_WARN)

// ERROR level conditional macros
LOGIT_DEFINE_LOG_LEVEL_IF_MACROS(ERROR, logit::LogLevel::LOG_LVL_ERROR)

// FATAL level conditional macros
LOGIT_DEFINE_LOG_LEVEL_IF_MACROS(FATAL, logit::LogLevel::LOG_LVL_FATAL)

//------------------------------------------------------------------------------
// Shorter versions of the macros when LOGIT_SHORT_NAME is defined
#undef LOGIT_DEFINE_LOG_LEVEL_MACROS
#undef LOGIT_DEFINE_LOG_LEVEL_IF_MACROS
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

/// \}

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
        std::make_unique<logit::SimpleLogFormatter>(LOGIT_UNIQUE_FILE_LOGGER_PATTERN));

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
        std::make_unique<logit::SimpleLogFormatter>(LOGIT_UNIQUE_FILE_LOGGER_PATTERN),  \
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
        std::unique_ptr<logit::SimpleLogFormatter>(new logit::SimpleLogFormatter(LOGIT_UNIQUE_FILE_LOGGER_PATTERN)))

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
        std::unique_ptr<logit::SimpleLogFormatter>(new logit::SimpleLogFormatter(LOGIT_UNIQUE_FILE_LOGGER_PATTERN)), \
        true)

#endif // C++ version check

/// \name Logger Management
/// Macros for managing loggers.
/// \{

/// \brief Retrieves string parameter from a logger.
/// \param logger_index Index of logger.
/// \param param Parameter to retrieve.
/// \return Requested parameter as a string.
#define LOGIT_GET_STRING_PARAM(logger_index, param) \
    logit::Logger::get_instance().get_string_param(logger_index, param)

/// \brief Retrieves integer parameter from a logger.
/// \param logger_index Index of logger.
/// \param param Parameter to retrieve.
/// \return Requested parameter as an integer.
#define LOGIT_GET_INT_PARAM(logger_index, param) \
    logit::Logger::get_instance().get_int_param(logger_index, param)

/// \brief Retrieves floating-point parameter from a logger.
/// \param logger_index Index of logger.
/// \param param Parameter to retrieve.
/// \return Requested parameter as a double.
#define LOGIT_GET_FLOAT_PARAM(logger_index, param) \
    logit::Logger::get_instance().get_float_param(logger_index, param)

/// \brief Retrieves last log file name from a specific logger.
/// \param logger_index Index of logger.
/// \return Last file name written to.
#define LOGIT_GET_LAST_FILE_NAME(logger_index) \
    logit::Logger::get_instance().get_string_param(logger_index, logit::LoggerParam::LastFileName)

/// \brief Retrieves last log file path from a specific logger.
/// \param logger_index Index of logger.
/// \return Full path of last file written to.
#define LOGIT_GET_LAST_FILE_PATH(logger_index) \
    logit::Logger::get_instance().get_string_param(logger_index, logit::LoggerParam::LastFilePath)

/// \brief Retrieves timestamp of last log from a specific logger.
/// \param logger_index Index of logger.
/// \return Timestamp of last log.
#define LOGIT_GET_LAST_LOG_TIMESTAMP(logger_index) \
    logit::Logger::get_instance().get_int_param(logger_index, logit::LoggerParam::LastLogTimestamp)

/// \brief Retrieves time since last log from a specific logger.
/// \param logger_index Index of logger.
/// \return Time elapsed since last log in seconds.
#define LOGIT_GET_TIME_SINCE_LAST_LOG(logger_index) \
    logit::Logger::get_instance().get_float_param(logger_index, logit::LoggerParam::TimeSinceLastLog)

/// \brief Enables or disables a logger.
/// \param logger_index Index of logger.
/// \param enabled True to enable, false to disable.
#define LOGIT_SET_LOGGER_ENABLED(logger_index, enabled) \
    logit::Logger::get_instance().set_logger_enabled(logger_index, enabled)

/// \brief Checks if a logger is enabled.
/// \param logger_index Index of logger.
/// \return True if enabled, false otherwise.
#define LOGIT_IS_LOGGER_ENABLED(logger_index) \
    logit::Logger::get_instance().is_logger_enabled(logger_index)

/// \brief Sets single mode for a specific logger.
/// \param logger_index Index of logger.
/// \param single_mode True to enable single mode, false otherwise.
#define LOGIT_SET_SINGLE_MODE(logger_index, single_mode) \
    logit::Logger::get_instance().set_logger_single_mode(logger_index, single_mode)

/// \brief Sets timestamp offset for a specific logger.
/// \param logger_index Index of logger.
/// \param offset_ms Offset in milliseconds.
#define LOGIT_SET_TIME_OFFSET(logger_index, offset_ms) \
    logit::Logger::get_instance().set_timestamp_offset(logger_index, offset_ms)

/// \brief Sets minimal log level for a specific logger.
/// \param logger_index Index of logger.
/// \param level Minimum log level.
#define LOGIT_SET_LOG_LEVEL_TO(logger_index, level) \
    logit::Logger::get_instance().set_log_level(logger_index, level)

/// \brief Sets minimal log level for all loggers.
/// \param level Minimum log level.
#define LOGIT_SET_LOG_LEVEL(level) \
    logit::Logger::get_instance().set_log_level(level)

/// \brief Checks if a logger is in single mode.
/// \param logger_index Index of logger.
/// \return True if in single mode, false otherwise.
#define LOGIT_IS_SINGLE_MODE(logger_index) \
    logit::Logger::get_instance().is_logger_single_mode(logger_index)

/// \}

/// \brief Macro for waiting for all asynchronous loggers to finish processing.
#define LOGIT_WAIT() logit::Logger::get_instance().wait();

/// \brief Macro for shutting down logger system.
#define LOGIT_SHUTDOWN() logit::Logger::get_instance().shutdown();

//------------------------------------------------------------------------------

/// \}

#endif // _LOGIT_LOG_MACROS_HPP_INCLUDED
