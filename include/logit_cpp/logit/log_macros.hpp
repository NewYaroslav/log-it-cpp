#pragma once
#ifndef LOGIT_LOG_MACROS_HPP_INCLUDED
#define LOGIT_LOG_MACROS_HPP_INCLUDED

#ifdef LOGIT_WITH_FMT
#include <fmt/format.h>
#endif

#include "config.hpp"
#include "utils.hpp"
#include "Logger.hpp"

#include "detail/LogStream.hpp"
#include "detail/ScopeTimer.hpp"
#include "detail/system_error_macros.hpp"

/// \file log_macros.hpp
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

#define LOGIT_LEVEL_TRACE 0
#define LOGIT_LEVEL_DEBUG 1
#define LOGIT_LEVEL_INFO  2
#define LOGIT_LEVEL_WARN  3
#define LOGIT_LEVEL_ERROR 4
#define LOGIT_LEVEL_FATAL 5
/// \brief Concatenate two tokens without macro expansion.
/// \param x First token.
/// \param y Second token.
#define LOGIT_CONCAT_IMPL(x, y) x##y

/// \brief Concatenate two tokens with macro expansion.
/// \param x First token.
/// \param y Second token.
#define LOGIT_CONCAT(x, y) LOGIT_CONCAT_IMPL(x, y)

#ifdef _LOGIT_ENUMS_HPP_INCLUDED
static_assert(LOGIT_LEVEL_TRACE == static_cast<int>(logit::LogLevel::LOG_LVL_TRACE),
              "LOGIT_LEVEL_TRACE mismatch");
static_assert(LOGIT_LEVEL_DEBUG == static_cast<int>(logit::LogLevel::LOG_LVL_DEBUG),
              "LOGIT_LEVEL_DEBUG mismatch");
static_assert(LOGIT_LEVEL_INFO  == static_cast<int>(logit::LogLevel::LOG_LVL_INFO),
              "LOGIT_LEVEL_INFO mismatch");
static_assert(LOGIT_LEVEL_WARN  == static_cast<int>(logit::LogLevel::LOG_LVL_WARN),
              "LOGIT_LEVEL_WARN mismatch");
static_assert(LOGIT_LEVEL_ERROR == static_cast<int>(logit::LogLevel::LOG_LVL_ERROR),
              "LOGIT_LEVEL_ERROR mismatch");
static_assert(LOGIT_LEVEL_FATAL == static_cast<int>(logit::LogLevel::LOG_LVL_FATAL),
              "LOGIT_LEVEL_FATAL mismatch");
#endif // _LOGIT_ENUMS_HPP_INCLUDED

#ifndef LOGIT_COMPILED_LEVEL
#    define LOGIT_COMPILED_LEVEL LOGIT_LEVEL_TRACE
#endif

#if __cplusplus >= 201703L
#    define LOGIT_IF_COMPILED_LEVEL(level) if constexpr (LOGIT_COMPILED_LEVEL <= static_cast<int>(level))
#endif

//------------------------------------------------------------------------------
// System error logging helpers

//------------------------------------------------------------------------------
// Platform-specific error logging macros

#if LOGIT_COMPILED_LEVEL <= LOGIT_LEVEL_TRACE
#define LOGIT_PERROR_TRACE(message) LOGIT_DETAIL_PERROR(TRACE, message)
#else
#define LOGIT_PERROR_TRACE(message) do { } while (0)
#endif

#if LOGIT_COMPILED_LEVEL <= LOGIT_LEVEL_DEBUG
#define LOGIT_PERROR_DEBUG(message) LOGIT_DETAIL_PERROR(DEBUG, message)
#else
#define LOGIT_PERROR_DEBUG(message) do { } while (0)
#endif

#if LOGIT_COMPILED_LEVEL <= LOGIT_LEVEL_INFO
#define LOGIT_PERROR_INFO(message)  LOGIT_DETAIL_PERROR(INFO, message)
#else
#define LOGIT_PERROR_INFO(message)  do { } while (0)
#endif

#if LOGIT_COMPILED_LEVEL <= LOGIT_LEVEL_WARN
#define LOGIT_PERROR_WARN(message)  LOGIT_DETAIL_PERROR(WARN, message)
#else
#define LOGIT_PERROR_WARN(message)  do { } while (0)
#endif

#if LOGIT_COMPILED_LEVEL <= LOGIT_LEVEL_ERROR
#define LOGIT_PERROR_ERROR(message) LOGIT_DETAIL_PERROR(ERROR, message)
#else
#define LOGIT_PERROR_ERROR(message) do { } while (0)
#endif

#if LOGIT_COMPILED_LEVEL <= LOGIT_LEVEL_FATAL
#define LOGIT_PERROR_FATAL(message) LOGIT_DETAIL_PERROR(FATAL, message)
#else
#define LOGIT_PERROR_FATAL(message) do { } while (0)
#endif

#if defined(_WIN32)

#if LOGIT_COMPILED_LEVEL <= LOGIT_LEVEL_TRACE
#define LOGIT_WINERR_TRACE(message) LOGIT_DETAIL_WINERR(TRACE, message)
#else
#define LOGIT_WINERR_TRACE(message) do { } while (0)
#endif

#if LOGIT_COMPILED_LEVEL <= LOGIT_LEVEL_DEBUG
#define LOGIT_WINERR_DEBUG(message) LOGIT_DETAIL_WINERR(DEBUG, message)
#else
#define LOGIT_WINERR_DEBUG(message) do { } while (0)
#endif

#if LOGIT_COMPILED_LEVEL <= LOGIT_LEVEL_INFO
#define LOGIT_WINERR_INFO(message)  LOGIT_DETAIL_WINERR(INFO, message)
#else
#define LOGIT_WINERR_INFO(message)  do { } while (0)
#endif

#if LOGIT_COMPILED_LEVEL <= LOGIT_LEVEL_WARN
#define LOGIT_WINERR_WARN(message)  LOGIT_DETAIL_WINERR(WARN, message)
#else
#define LOGIT_WINERR_WARN(message)  do { } while (0)
#endif

#if LOGIT_COMPILED_LEVEL <= LOGIT_LEVEL_ERROR
#define LOGIT_WINERR_ERROR(message) LOGIT_DETAIL_WINERR(ERROR, message)
#else
#define LOGIT_WINERR_ERROR(message) do { } while (0)
#endif

#if LOGIT_COMPILED_LEVEL <= LOGIT_LEVEL_FATAL
#define LOGIT_WINERR_FATAL(message) LOGIT_DETAIL_WINERR(FATAL, message)
#else
#define LOGIT_WINERR_FATAL(message) do { } while (0)
#endif

#else // defined(_WIN32)

#define LOGIT_WINERR_TRACE(message) do { } while (0)
#define LOGIT_WINERR_DEBUG(message) do { } while (0)
#define LOGIT_WINERR_INFO(message)  do { } while (0)
#define LOGIT_WINERR_WARN(message)  do { } while (0)
#define LOGIT_WINERR_ERROR(message) do { } while (0)
#define LOGIT_WINERR_FATAL(message) do { } while (0)

#endif // defined(_WIN32)

#if defined(_WIN32)
#define LOGIT_SYSERR_TRACE(message) LOGIT_WINERR_TRACE(message)
#define LOGIT_SYSERR_DEBUG(message) LOGIT_WINERR_DEBUG(message)
#define LOGIT_SYSERR_INFO(message)  LOGIT_WINERR_INFO(message)
#define LOGIT_SYSERR_WARN(message)  LOGIT_WINERR_WARN(message)
#define LOGIT_SYSERR_ERROR(message) LOGIT_WINERR_ERROR(message)
#define LOGIT_SYSERR_FATAL(message) LOGIT_WINERR_FATAL(message)
#else
#define LOGIT_SYSERR_TRACE(message) LOGIT_PERROR_TRACE(message)
#define LOGIT_SYSERR_DEBUG(message) LOGIT_PERROR_DEBUG(message)
#define LOGIT_SYSERR_INFO(message)  LOGIT_PERROR_INFO(message)
#define LOGIT_SYSERR_WARN(message)  LOGIT_PERROR_WARN(message)
#define LOGIT_SYSERR_ERROR(message) LOGIT_PERROR_ERROR(message)
#define LOGIT_SYSERR_FATAL(message) LOGIT_PERROR_FATAL(message)
#endif

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
// Scope-based timing macros

/// \name Scope-Based Timing
/// RAII timers that log the duration of a scope.
/// \{

#define LOGIT_DETAIL_SCOPE(level, phase) \
    ::logit::detail::ScopeTimer LOGIT_CONCAT(_logit_scope_, __COUNTER__)(level, (phase), __FILE__, __LINE__, LOGIT_FUNCTION, -1, 0)

#define LOGIT_DETAIL_SCOPE_T(level, threshold_ms, phase) \
    ::logit::detail::ScopeTimer LOGIT_CONCAT(_logit_scope_, __COUNTER__)(level, (phase), __FILE__, __LINE__, LOGIT_FUNCTION, -1, (threshold_ms))

#define LOGIT_DETAIL_SCOPE_PRINTF(level, fmt_str, ...) \
    ::logit::detail::ScopeTimer LOGIT_CONCAT(_logit_scope_, __COUNTER__)(level, logit::format(fmt_str, __VA_ARGS__), __FILE__, __LINE__, LOGIT_FUNCTION, -1, 0)
#define LOGIT_DETAIL_SCOPE_PRINTF_T(level, threshold_ms, fmt_str, ...) \
    ::logit::detail::ScopeTimer LOGIT_CONCAT(_logit_scope_, __COUNTER__)(level, logit::format(fmt_str, __VA_ARGS__), __FILE__, __LINE__, LOGIT_FUNCTION, -1, (threshold_ms))

#ifdef LOGIT_WITH_FMT
#define LOGIT_DETAIL_SCOPE_FMT(level, fmt_str, ...) \
    ::logit::detail::ScopeTimer LOGIT_CONCAT(_logit_scope_, __COUNTER__)(level, fmt::format(fmt_str, __VA_ARGS__), __FILE__, __LINE__, LOGIT_FUNCTION, -1, 0)
#define LOGIT_DETAIL_SCOPE_FMT_T(level, threshold_ms, fmt_str, ...) \
    ::logit::detail::ScopeTimer LOGIT_CONCAT(_logit_scope_, __COUNTER__)(level, fmt::format(fmt_str, __VA_ARGS__), __FILE__, __LINE__, LOGIT_FUNCTION, -1, (threshold_ms))
#else
#define LOGIT_DETAIL_SCOPE_FMT(level, fmt_str, ...) do { } while (0)
#define LOGIT_DETAIL_SCOPE_FMT_T(level, threshold_ms, fmt_str, ...) do { } while (0)
#endif

#if LOGIT_COMPILED_LEVEL <= LOGIT_LEVEL_TRACE
#define LOGIT_SCOPE_TRACE(phase)            LOGIT_DETAIL_SCOPE(::logit::LogLevel::LOG_LVL_TRACE, phase)
#define LOGIT_SCOPE_TRACE_T(threshold_ms, phase) \
    LOGIT_DETAIL_SCOPE_T(::logit::LogLevel::LOG_LVL_TRACE, threshold_ms, phase)
#else
#define LOGIT_SCOPE_TRACE(phase)            do { } while (0)
#define LOGIT_SCOPE_TRACE_T(threshold_ms, phase) do { } while (0)
#endif

#if LOGIT_COMPILED_LEVEL <= LOGIT_LEVEL_DEBUG
#define LOGIT_SCOPE_DEBUG(phase)            LOGIT_DETAIL_SCOPE(::logit::LogLevel::LOG_LVL_DEBUG, phase)
#define LOGIT_SCOPE_DEBUG_T(threshold_ms, phase) \
    LOGIT_DETAIL_SCOPE_T(::logit::LogLevel::LOG_LVL_DEBUG, threshold_ms, phase)
#else
#define LOGIT_SCOPE_DEBUG(phase)            do { } while (0)
#define LOGIT_SCOPE_DEBUG_T(threshold_ms, phase) do { } while (0)
#endif

#if LOGIT_COMPILED_LEVEL <= LOGIT_LEVEL_INFO
#define LOGIT_SCOPE_INFO(phase)             LOGIT_DETAIL_SCOPE(::logit::LogLevel::LOG_LVL_INFO, phase)
#define LOGIT_SCOPE_INFO_T(threshold_ms, phase) \
    LOGIT_DETAIL_SCOPE_T(::logit::LogLevel::LOG_LVL_INFO, threshold_ms, phase)
#else
#define LOGIT_SCOPE_INFO(phase)             do { } while (0)
#define LOGIT_SCOPE_INFO_T(threshold_ms, phase) do { } while (0)
#endif

#if LOGIT_COMPILED_LEVEL <= LOGIT_LEVEL_WARN
#define LOGIT_SCOPE_WARN(phase)             LOGIT_DETAIL_SCOPE(::logit::LogLevel::LOG_LVL_WARN, phase)
#define LOGIT_SCOPE_WARN_T(threshold_ms, phase) \
    LOGIT_DETAIL_SCOPE_T(::logit::LogLevel::LOG_LVL_WARN, threshold_ms, phase)
#else
#define LOGIT_SCOPE_WARN(phase)             do { } while (0)
#define LOGIT_SCOPE_WARN_T(threshold_ms, phase) do { } while (0)
#endif

#if LOGIT_COMPILED_LEVEL <= LOGIT_LEVEL_ERROR
#define LOGIT_SCOPE_ERROR(phase)            LOGIT_DETAIL_SCOPE(::logit::LogLevel::LOG_LVL_ERROR, phase)
#define LOGIT_SCOPE_ERROR_T(threshold_ms, phase) \
    LOGIT_DETAIL_SCOPE_T(::logit::LogLevel::LOG_LVL_ERROR, threshold_ms, phase)
#else
#define LOGIT_SCOPE_ERROR(phase)            do { } while (0)
#define LOGIT_SCOPE_ERROR_T(threshold_ms, phase) do { } while (0)
#endif

#if LOGIT_COMPILED_LEVEL <= LOGIT_LEVEL_FATAL
#define LOGIT_SCOPE_FATAL(phase)            LOGIT_DETAIL_SCOPE(::logit::LogLevel::LOG_LVL_FATAL, phase)
#define LOGIT_SCOPE_FATAL_T(threshold_ms, phase) \
    LOGIT_DETAIL_SCOPE_T(::logit::LogLevel::LOG_LVL_FATAL, threshold_ms, phase)
#else
#define LOGIT_SCOPE_FATAL(phase)            do { } while (0)
#define LOGIT_SCOPE_FATAL_T(threshold_ms, phase) do { } while (0)
#endif

#if LOGIT_COMPILED_LEVEL <= LOGIT_LEVEL_TRACE
#define LOGIT_SCOPE_PRINTF_TRACE(fmt_str, ...) \
    LOGIT_DETAIL_SCOPE_PRINTF(::logit::LogLevel::LOG_LVL_TRACE, fmt_str, __VA_ARGS__)
#define LOGIT_SCOPE_PRINTF_TRACE_T(threshold_ms, fmt_str, ...) \
    LOGIT_DETAIL_SCOPE_PRINTF_T(::logit::LogLevel::LOG_LVL_TRACE, threshold_ms, fmt_str, __VA_ARGS__)
#else
#define LOGIT_SCOPE_PRINTF_TRACE(fmt_str, ...) do { } while (0)
#define LOGIT_SCOPE_PRINTF_TRACE_T(threshold_ms, fmt_str, ...) do { } while (0)
#endif

#if LOGIT_COMPILED_LEVEL <= LOGIT_LEVEL_DEBUG
#define LOGIT_SCOPE_PRINTF_DEBUG(fmt_str, ...) \
    LOGIT_DETAIL_SCOPE_PRINTF(::logit::LogLevel::LOG_LVL_DEBUG, fmt_str, __VA_ARGS__)
#define LOGIT_SCOPE_PRINTF_DEBUG_T(threshold_ms, fmt_str, ...) \
    LOGIT_DETAIL_SCOPE_PRINTF_T(::logit::LogLevel::LOG_LVL_DEBUG, threshold_ms, fmt_str, __VA_ARGS__)
#else
#define LOGIT_SCOPE_PRINTF_DEBUG(fmt_str, ...) do { } while (0)
#define LOGIT_SCOPE_PRINTF_DEBUG_T(threshold_ms, fmt_str, ...) do { } while (0)
#endif

#if LOGIT_COMPILED_LEVEL <= LOGIT_LEVEL_INFO
#define LOGIT_SCOPE_PRINTF_INFO(fmt_str, ...) \
    LOGIT_DETAIL_SCOPE_PRINTF(::logit::LogLevel::LOG_LVL_INFO, fmt_str, __VA_ARGS__)
#define LOGIT_SCOPE_PRINTF_INFO_T(threshold_ms, fmt_str, ...) \
    LOGIT_DETAIL_SCOPE_PRINTF_T(::logit::LogLevel::LOG_LVL_INFO, threshold_ms, fmt_str, __VA_ARGS__)
#else
#define LOGIT_SCOPE_PRINTF_INFO(fmt_str, ...) do { } while (0)
#define LOGIT_SCOPE_PRINTF_INFO_T(threshold_ms, fmt_str, ...) do { } while (0)
#endif

#if LOGIT_COMPILED_LEVEL <= LOGIT_LEVEL_WARN
#define LOGIT_SCOPE_PRINTF_WARN(fmt_str, ...) \
    LOGIT_DETAIL_SCOPE_PRINTF(::logit::LogLevel::LOG_LVL_WARN, fmt_str, __VA_ARGS__)
#define LOGIT_SCOPE_PRINTF_WARN_T(threshold_ms, fmt_str, ...) \
    LOGIT_DETAIL_SCOPE_PRINTF_T(::logit::LogLevel::LOG_LVL_WARN, threshold_ms, fmt_str, __VA_ARGS__)
#else
#define LOGIT_SCOPE_PRINTF_WARN(fmt_str, ...) do { } while (0)
#define LOGIT_SCOPE_PRINTF_WARN_T(threshold_ms, fmt_str, ...) do { } while (0)
#endif

#if LOGIT_COMPILED_LEVEL <= LOGIT_LEVEL_ERROR
#define LOGIT_SCOPE_PRINTF_ERROR(fmt_str, ...) \
    LOGIT_DETAIL_SCOPE_PRINTF(::logit::LogLevel::LOG_LVL_ERROR, fmt_str, __VA_ARGS__)
#define LOGIT_SCOPE_PRINTF_ERROR_T(threshold_ms, fmt_str, ...) \
    LOGIT_DETAIL_SCOPE_PRINTF_T(::logit::LogLevel::LOG_LVL_ERROR, threshold_ms, fmt_str, __VA_ARGS__)
#else
#define LOGIT_SCOPE_PRINTF_ERROR(fmt_str, ...) do { } while (0)
#define LOGIT_SCOPE_PRINTF_ERROR_T(threshold_ms, fmt_str, ...) do { } while (0)
#endif

#if LOGIT_COMPILED_LEVEL <= LOGIT_LEVEL_FATAL
#define LOGIT_SCOPE_PRINTF_FATAL(fmt_str, ...) \
    LOGIT_DETAIL_SCOPE_PRINTF(::logit::LogLevel::LOG_LVL_FATAL, fmt_str, __VA_ARGS__)
#define LOGIT_SCOPE_PRINTF_FATAL_T(threshold_ms, fmt_str, ...) \
    LOGIT_DETAIL_SCOPE_PRINTF_T(::logit::LogLevel::LOG_LVL_FATAL, threshold_ms, fmt_str, __VA_ARGS__)
#else
#define LOGIT_SCOPE_PRINTF_FATAL(fmt_str, ...) do { } while (0)
#define LOGIT_SCOPE_PRINTF_FATAL_T(threshold_ms, fmt_str, ...) do { } while (0)
#endif

#if LOGIT_COMPILED_LEVEL <= LOGIT_LEVEL_TRACE
#define LOGIT_SCOPE_FMT_TRACE(fmt_str, ...) \
    LOGIT_DETAIL_SCOPE_FMT(::logit::LogLevel::LOG_LVL_TRACE, fmt_str, __VA_ARGS__)
#define LOGIT_SCOPE_FMT_TRACE_T(threshold_ms, fmt_str, ...) \
    LOGIT_DETAIL_SCOPE_FMT_T(::logit::LogLevel::LOG_LVL_TRACE, threshold_ms, fmt_str, __VA_ARGS__)
#else
#define LOGIT_SCOPE_FMT_TRACE(fmt_str, ...) do { } while (0)
#define LOGIT_SCOPE_FMT_TRACE_T(threshold_ms, fmt_str, ...) do { } while (0)
#endif

#if LOGIT_COMPILED_LEVEL <= LOGIT_LEVEL_DEBUG
#define LOGIT_SCOPE_FMT_DEBUG(fmt_str, ...) \
    LOGIT_DETAIL_SCOPE_FMT(::logit::LogLevel::LOG_LVL_DEBUG, fmt_str, __VA_ARGS__)
#define LOGIT_SCOPE_FMT_DEBUG_T(threshold_ms, fmt_str, ...) \
    LOGIT_DETAIL_SCOPE_FMT_T(::logit::LogLevel::LOG_LVL_DEBUG, threshold_ms, fmt_str, __VA_ARGS__)
#else
#define LOGIT_SCOPE_FMT_DEBUG(fmt_str, ...) do { } while (0)
#define LOGIT_SCOPE_FMT_DEBUG_T(threshold_ms, fmt_str, ...) do { } while (0)
#endif

#if LOGIT_COMPILED_LEVEL <= LOGIT_LEVEL_INFO
#define LOGIT_SCOPE_FMT_INFO(fmt_str, ...) \
    LOGIT_DETAIL_SCOPE_FMT(::logit::LogLevel::LOG_LVL_INFO, fmt_str, __VA_ARGS__)
#define LOGIT_SCOPE_FMT_INFO_T(threshold_ms, fmt_str, ...) \
    LOGIT_DETAIL_SCOPE_FMT_T(::logit::LogLevel::LOG_LVL_INFO, threshold_ms, fmt_str, __VA_ARGS__)
#else
#define LOGIT_SCOPE_FMT_INFO(fmt_str, ...) do { } while (0)
#define LOGIT_SCOPE_FMT_INFO_T(threshold_ms, fmt_str, ...) do { } while (0)
#endif

#if LOGIT_COMPILED_LEVEL <= LOGIT_LEVEL_WARN
#define LOGIT_SCOPE_FMT_WARN(fmt_str, ...) \
    LOGIT_DETAIL_SCOPE_FMT(::logit::LogLevel::LOG_LVL_WARN, fmt_str, __VA_ARGS__)
#define LOGIT_SCOPE_FMT_WARN_T(threshold_ms, fmt_str, ...) \
    LOGIT_DETAIL_SCOPE_FMT_T(::logit::LogLevel::LOG_LVL_WARN, threshold_ms, fmt_str, __VA_ARGS__)
#else
#define LOGIT_SCOPE_FMT_WARN(fmt_str, ...) do { } while (0)
#define LOGIT_SCOPE_FMT_WARN_T(threshold_ms, fmt_str, ...) do { } while (0)
#endif

#if LOGIT_COMPILED_LEVEL <= LOGIT_LEVEL_ERROR
#define LOGIT_SCOPE_FMT_ERROR(fmt_str, ...) \
    LOGIT_DETAIL_SCOPE_FMT(::logit::LogLevel::LOG_LVL_ERROR, fmt_str, __VA_ARGS__)
#define LOGIT_SCOPE_FMT_ERROR_T(threshold_ms, fmt_str, ...) \
    LOGIT_DETAIL_SCOPE_FMT_T(::logit::LogLevel::LOG_LVL_ERROR, threshold_ms, fmt_str, __VA_ARGS__)
#else
#define LOGIT_SCOPE_FMT_ERROR(fmt_str, ...) do { } while (0)
#define LOGIT_SCOPE_FMT_ERROR_T(threshold_ms, fmt_str, ...) do { } while (0)
#endif

#if LOGIT_COMPILED_LEVEL <= LOGIT_LEVEL_FATAL
#define LOGIT_SCOPE_FMT_FATAL(fmt_str, ...) \
    LOGIT_DETAIL_SCOPE_FMT(::logit::LogLevel::LOG_LVL_FATAL, fmt_str, __VA_ARGS__)
#define LOGIT_SCOPE_FMT_FATAL_T(threshold_ms, fmt_str, ...) \
    LOGIT_DETAIL_SCOPE_FMT_T(::logit::LogLevel::LOG_LVL_FATAL, threshold_ms, fmt_str, __VA_ARGS__)
#else
#define LOGIT_SCOPE_FMT_FATAL(fmt_str, ...) do { } while (0)
#define LOGIT_SCOPE_FMT_FATAL_T(threshold_ms, fmt_str, ...) do { } while (0)
#endif

/// \}

//------------------------------------------------------------------------------
// Macros for logging without arguments

/// \brief Logs a message without arguments.
/// \param level The log level.
/// \param format The log message format.
#if __cplusplus >= 201703L
#define LOGIT_LOG_AND_RETURN_NOARGS(level, format)                                          \
    do {                                                                                    \
        LOGIT_IF_COMPILED_LEVEL(level)                                                      \
            logit::Logger::get_instance().log_and_return(                                   \
                logit::LogRecord{level, LOGIT_CURRENT_TIMESTAMP_MS(),                       \
                logit::make_relative(__FILE__, LOGIT_BASE_PATH), __LINE__,                  \
                LOGIT_FUNCTION, format, {}, -1, false});                                    \
    } while (0)
#else
#define LOGIT_LOG_AND_RETURN_NOARGS(level, format)                                          \
    do {                                                                                    \
        logit::Logger::get_instance().log_and_return(                                       \
            logit::LogRecord{level, LOGIT_CURRENT_TIMESTAMP_MS(),                           \
            logit::make_relative(__FILE__, LOGIT_BASE_PATH), __LINE__,                      \
            LOGIT_FUNCTION, format, {}, -1, false});                                        \
    } while (0)
#endif

/// \brief Logs a message to a specific logger without arguments.
/// \param level The log level.
/// \param index The index of the logger to log to.
/// \param format The log message format.
#if __cplusplus >= 201703L
#define LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(level, index, format)                      \
    do {                                                                                  \
        LOGIT_IF_COMPILED_LEVEL(level)                                                    \
            logit::Logger::get_instance().log_and_return(                                 \
                logit::LogRecord{level, LOGIT_CURRENT_TIMESTAMP_MS(),                     \
                logit::make_relative(__FILE__, LOGIT_BASE_PATH), __LINE__,                \
                LOGIT_FUNCTION, format, {}, index});                                      \
    } while (0)
#else
#define LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(level, index, format)                      \
    do {                                                                                  \
        logit::Logger::get_instance().log_and_return(                                     \
            logit::LogRecord{level, LOGIT_CURRENT_TIMESTAMP_MS(),                         \
            logit::make_relative(__FILE__, LOGIT_BASE_PATH), __LINE__,                    \
            LOGIT_FUNCTION, format, {}, index});                                          \
    } while (0)
#endif

//------------------------------------------------------------------------------
// Macros for logging with arguments

/// \brief Logs a message with arguments.
/// \param level The log level.
/// \param format The log message format.
/// \param arg_names The names of the arguments.
/// \param ... The arguments to log.
#if __cplusplus >= 201703L
#define LOGIT_LOG_AND_RETURN(level, format, arg_names, ...)                               \
    do {                                                                                  \
        LOGIT_IF_COMPILED_LEVEL(level)                                                    \
            logit::Logger::get_instance().log_and_return(                                 \
                logit::LogRecord{level, LOGIT_CURRENT_TIMESTAMP_MS(),                     \
                logit::make_relative(__FILE__, LOGIT_BASE_PATH), __LINE__,                \
                LOGIT_FUNCTION, format, arg_names, -1, false}, __VA_ARGS__);               \
    } while (0)
#else
#define LOGIT_LOG_AND_RETURN(level, format, arg_names, ...)                               \
    do {                                                                                  \
        logit::Logger::get_instance().log_and_return(                                     \
            logit::LogRecord{level, LOGIT_CURRENT_TIMESTAMP_MS(),                         \
            logit::make_relative(__FILE__, LOGIT_BASE_PATH), __LINE__,                    \
            LOGIT_FUNCTION, format, arg_names, -1, false}, __VA_ARGS__);                  \
    } while (0)
#endif

/// \brief Logs a message with arguments, but prints them without using a format string.
/// \param level The log level.
/// \param arg_names The names of the arguments.
/// \param ... The arguments to log.
/// \details This macro logs the raw arguments without applying any formatting to them.
#if __cplusplus >= 201703L
#define LOGIT_LOG_AND_RETURN_PRINT(level, arg_names, ...)                                 \
    do {                                                                                  \
        LOGIT_IF_COMPILED_LEVEL(level)                                                    \
            logit::Logger::get_instance().log_and_return(                                 \
                logit::LogRecord{level, LOGIT_CURRENT_TIMESTAMP_MS(),                     \
                logit::make_relative(__FILE__, LOGIT_BASE_PATH), __LINE__,                \
                LOGIT_FUNCTION, {}, arg_names, -1, true}, __VA_ARGS__);                   \
    } while (0)
#else
#define LOGIT_LOG_AND_RETURN_PRINT(level, arg_names, ...)                                 \
    do {                                                                                  \
        logit::Logger::get_instance().log_and_return(                                     \
            logit::LogRecord{level, LOGIT_CURRENT_TIMESTAMP_MS(),                         \
            logit::make_relative(__FILE__, LOGIT_BASE_PATH), __LINE__,                    \
            LOGIT_FUNCTION, {}, arg_names, -1, true}, __VA_ARGS__);                       \
    } while (0)
#endif

/// \brief Logs a message with arguments to a specific logger.
/// \param level The log level.
/// \param index The index of the logger to log to.
/// \param format The log message format.
/// \param arg_names The names of the arguments.
/// \param ... The arguments to log.
#if __cplusplus >= 201703L
#define LOGIT_LOG_AND_RETURN_WITH_INDEX(level, index, format, arg_names, ...)             \
    do {                                                                                  \
        LOGIT_IF_COMPILED_LEVEL(level)                                                    \
            logit::Logger::get_instance().log_and_return(                                 \
                logit::LogRecord{level, LOGIT_CURRENT_TIMESTAMP_MS(),                     \
                logit::make_relative(__FILE__, LOGIT_BASE_PATH), __LINE__,                \
                LOGIT_FUNCTION, format, arg_names, index, false}, __VA_ARGS__);           \
    } while (0)
#else
#define LOGIT_LOG_AND_RETURN_WITH_INDEX(level, index, format, arg_names, ...)             \
    do {                                                                                  \
        logit::Logger::get_instance().log_and_return(                                     \
            logit::LogRecord{level, LOGIT_CURRENT_TIMESTAMP_MS(),                         \
            logit::make_relative(__FILE__, LOGIT_BASE_PATH), __LINE__,                    \
            LOGIT_FUNCTION, format, arg_names, index, false}, __VA_ARGS__);               \
    } while (0)
#endif

/// \brief Logs a message with arguments to a specific logger, but prints them without using a format string.
/// \param level The log level.
/// \param index The index of the logger to log to.
/// \param arg_names The names of the arguments.
/// \param ... The arguments to log.
/// \details This macro logs the raw arguments without applying any formatting to them to a specific logger.
#if __cplusplus >= 201703L
#define LOGIT_LOG_AND_RETURN_PRINT_WITH_INDEX(level, index, arg_names, ...)               \
    do {                                                                                  \
        LOGIT_IF_COMPILED_LEVEL(level)                                                    \
            logit::Logger::get_instance().log_and_return(                                 \
                logit::LogRecord{level, LOGIT_CURRENT_TIMESTAMP_MS(),                     \
                logit::make_relative(__FILE__, LOGIT_BASE_PATH), __LINE__,                \
                LOGIT_FUNCTION, {}, arg_names, index, true}, __VA_ARGS__);                \
    } while (0)
#else
#define LOGIT_LOG_AND_RETURN_PRINT_WITH_INDEX(level, index, arg_names, ...)               \
    do {                                                                                  \
        logit::Logger::get_instance().log_and_return(                                     \
            logit::LogRecord{level, LOGIT_CURRENT_TIMESTAMP_MS(),                         \
            logit::make_relative(__FILE__, LOGIT_BASE_PATH), __LINE__,                    \
            LOGIT_FUNCTION, {}, arg_names, index, true}, __VA_ARGS__);                    \
    } while (0)
#endif

//------------------------------------------------------------------------------
// Macros for logging with fmt formatting

#if __cplusplus >= 201703L
#define LOGIT_LOG_AND_RETURN_FMT(level, format, arg_names, ...)                           \
    do {                                                                                  \
        LOGIT_IF_COMPILED_LEVEL(level)                                                    \
            logit::Logger::get_instance().log_and_return(                                 \
                logit::LogRecord{level, LOGIT_CURRENT_TIMESTAMP_MS(),                     \
                logit::make_relative(__FILE__, LOGIT_BASE_PATH), __LINE__,                \
                LOGIT_FUNCTION, format, arg_names, -1, false, true}, __VA_ARGS__);        \
    } while (0)
#else
#define LOGIT_LOG_AND_RETURN_FMT(level, format, arg_names, ...)                           \
    do {                                                                                  \
        logit::Logger::get_instance().log_and_return(                                     \
            logit::LogRecord{level, LOGIT_CURRENT_TIMESTAMP_MS(),                         \
            logit::make_relative(__FILE__, LOGIT_BASE_PATH), __LINE__,                    \
            LOGIT_FUNCTION, format, arg_names, -1, false, true}, __VA_ARGS__);            \
    } while (0)
#endif

#if __cplusplus >= 201703L
#define LOGIT_LOG_AND_RETURN_FMT_WITH_INDEX(level, index, format, arg_names, ...)         \
    do {                                                                                  \
        LOGIT_IF_COMPILED_LEVEL(level)                                                    \
            logit::Logger::get_instance().log_and_return(                                 \
                logit::LogRecord{level, LOGIT_CURRENT_TIMESTAMP_MS(),                     \
                logit::make_relative(__FILE__, LOGIT_BASE_PATH), __LINE__,                \
                LOGIT_FUNCTION, format, arg_names, index, false, true}, __VA_ARGS__);     \
    } while (0)
#else
#define LOGIT_LOG_AND_RETURN_FMT_WITH_INDEX(level, index, format, arg_names, ...)         \
    do {                                                                                  \
        logit::Logger::get_instance().log_and_return(                                     \
            logit::LogRecord{level, LOGIT_CURRENT_TIMESTAMP_MS(),                         \
            logit::make_relative(__FILE__, LOGIT_BASE_PATH), __LINE__,                    \
            LOGIT_FUNCTION, format, arg_names, index, false, true}, __VA_ARGS__);         \
    } while (0)
#endif

//------------------------------------------------------------------------------
// Macros for each log level

#if LOGIT_COMPILED_LEVEL <= LOGIT_LEVEL_TRACE
// TRACE level macros
#define LOGIT_TRACE(...)                LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_TRACE, {}, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_TRACE0()                  LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_TRACE, {})
#define LOGIT_0TRACE()                  LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_TRACE, {})
#define LOGIT_0_TRACE()                 LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_TRACE, {})
#define LOGIT_NOARGS_TRACE()            LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_TRACE, {})
#define LOGIT_FORMAT_TRACE(fmt, ...)    LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_TRACE, fmt, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINT_TRACE(...)          LOGIT_LOG_AND_RETURN_PRINT(logit::LogLevel::LOG_LVL_TRACE, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINTF_TRACE(fmt, ...)    LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_TRACE, logit::format(fmt, __VA_ARGS__))
#ifdef LOGIT_WITH_FMT
#define LOGITF_TRACE(fmt_str, ...)      LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_TRACE, fmt::format(fmt_str, __VA_ARGS__))
#define LOGIT_FMT_TRACE(fmt_str, ...)   LOGIT_LOG_AND_RETURN_FMT(logit::LogLevel::LOG_LVL_TRACE, fmt_str, #__VA_ARGS__, __VA_ARGS__)
#else
#define LOGITF_TRACE(fmt_str, ...)      do { } while (0)
#define LOGIT_FMT_TRACE(fmt_str, ...)   do { } while (0)
#endif

// TRACE macros with index
#define LOGIT_TRACE_TO(index, ...)      LOGIT_LOG_AND_RETURN_WITH_INDEX(logit::LogLevel::LOG_LVL_TRACE, index, {}, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_TRACE0_TO(index)          LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_TRACE, index, {})
#define LOGIT_0TRACE_TO(index)          LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_TRACE, index, {})
#define LOGIT_0_TRACE_TO(index)         LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_TRACE, index, {})
#define LOGIT_NOARGS_TRACE_TO(index)    LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_TRACE, index, {})
#define LOGIT_FORMAT_TRACE_TO(index, fmt, ...) LOGIT_LOG_AND_RETURN_WITH_INDEX(logit::LogLevel::LOG_LVL_TRACE, index, fmt, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINT_TRACE_TO(index, ...)       LOGIT_LOG_AND_RETURN_PRINT_WITH_INDEX(logit::LogLevel::LOG_LVL_TRACE, index, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINTF_TRACE_TO(index, fmt, ...) LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_TRACE, index, logit::format(fmt, __VA_ARGS__))
#ifdef LOGIT_WITH_FMT
#define LOGITF_TRACE_TO(index, fmt_str, ...) LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_TRACE, index, fmt::format(fmt_str, __VA_ARGS__))
#define LOGIT_FMT_TRACE_TO(index, fmt_str, ...) LOGIT_LOG_AND_RETURN_FMT_WITH_INDEX(logit::LogLevel::LOG_LVL_TRACE, index, fmt_str, #__VA_ARGS__, __VA_ARGS__)
#else
#define LOGITF_TRACE_TO(index, fmt_str, ...) do { } while (0)
#define LOGIT_FMT_TRACE_TO(index, fmt_str, ...) do { } while (0)
#endif
#else
#define LOGIT_TRACE(...)                do { } while (0)
#define LOGIT_TRACE0()                  do { } while (0)
#define LOGIT_0TRACE()                  do { } while (0)
#define LOGIT_0_TRACE()                 do { } while (0)
#define LOGIT_NOARGS_TRACE()            do { } while (0)
#define LOGIT_FORMAT_TRACE(fmt, ...)    do { } while (0)
#define LOGIT_PRINT_TRACE(...)          do { } while (0)
#define LOGIT_PRINTF_TRACE(fmt, ...)    do { } while (0)
#define LOGIT_TRACE_TO(index, ...)      do { } while (0)
#define LOGIT_TRACE0_TO(index)          do { } while (0)
#define LOGIT_0TRACE_TO(index)          do { } while (0)
#define LOGIT_0_TRACE_TO(index)         do { } while (0)
#define LOGIT_NOARGS_TRACE_TO(index)    do { } while (0)
#define LOGIT_FORMAT_TRACE_TO(index, fmt, ...) do { } while (0)
#define LOGIT_PRINT_TRACE_TO(index, ...)       do { } while (0)
#define LOGIT_PRINTF_TRACE_TO(index, fmt, ...) do { } while (0)
#endif

#if LOGIT_COMPILED_LEVEL <= LOGIT_LEVEL_INFO
// INFO level macros
#define LOGIT_INFO(...)                 LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_INFO, {}, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_INFO0()                   LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_INFO, {})
#define LOGIT_0INFO()                   LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_INFO, {})
#define LOGIT_0_INFO()                  LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_INFO, {})
#define LOGIT_NOARGS_INFO()             LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_INFO, {})
#define LOGIT_FORMAT_INFO(fmt, ...)     LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_INFO, fmt, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINT_INFO(...)           LOGIT_LOG_AND_RETURN_PRINT(logit::LogLevel::LOG_LVL_INFO, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINTF_INFO(fmt, ...)     LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_INFO, logit::format(fmt, __VA_ARGS__))
#ifdef LOGIT_WITH_FMT
#define LOGITF_INFO(fmt_str, ...)       LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_INFO, fmt::format(fmt_str, __VA_ARGS__))
#define LOGIT_FMT_INFO(fmt_str, ...)    LOGIT_LOG_AND_RETURN_FMT(logit::LogLevel::LOG_LVL_INFO, fmt_str, #__VA_ARGS__, __VA_ARGS__)
#else
#define LOGITF_INFO(fmt_str, ...)       do { } while (0)
#define LOGIT_FMT_INFO(fmt_str, ...)    do { } while (0)
#endif

// INFO macros with index
#define LOGIT_INFO_TO(index, ...)       LOGIT_LOG_AND_RETURN_WITH_INDEX(logit::LogLevel::LOG_LVL_INFO, index, {}, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_INFO0_TO(index)           LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_INFO, index, {})
#define LOGIT_0INFO_TO(index)           LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_INFO, index, {})
#define LOGIT_0_INFO_TO(index)          LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_INFO, index, {})
#define LOGIT_NOARGS_INFO_TO(index)     LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_INFO, index, {})
#define LOGIT_FORMAT_INFO_TO(index, fmt, ...) LOGIT_LOG_AND_RETURN_WITH_INDEX(logit::LogLevel::LOG_LVL_INFO, index, fmt, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINT_INFO_TO(index, ...)       LOGIT_LOG_AND_RETURN_PRINT_WITH_INDEX(logit::LogLevel::LOG_LVL_INFO, index, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINTF_INFO_TO(index, fmt, ...) LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_INFO, index, logit::format(fmt, __VA_ARGS__))
#ifdef LOGIT_WITH_FMT
#define LOGITF_INFO_TO(index, fmt_str, ...) LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_INFO, index, fmt::format(fmt_str, __VA_ARGS__))
#define LOGIT_FMT_INFO_TO(index, fmt_str, ...) LOGIT_LOG_AND_RETURN_FMT_WITH_INDEX(logit::LogLevel::LOG_LVL_INFO, index, fmt_str, #__VA_ARGS__, __VA_ARGS__)
#else
#define LOGITF_INFO_TO(index, fmt_str, ...) do { } while (0)
#define LOGIT_FMT_INFO_TO(index, fmt_str, ...) do { } while (0)
#endif
#else
#define LOGIT_INFO(...)                 do { } while (0)
#define LOGIT_INFO0()                   do { } while (0)
#define LOGIT_0INFO()                   do { } while (0)
#define LOGIT_0_INFO()                  do { } while (0)
#define LOGIT_NOARGS_INFO()             do { } while (0)
#define LOGIT_FORMAT_INFO(fmt, ...)     do { } while (0)
#define LOGIT_PRINT_INFO(...)           do { } while (0)
#define LOGIT_PRINTF_INFO(fmt, ...)     do { } while (0)
#define LOGIT_INFO_TO(index, ...)       do { } while (0)
#define LOGIT_INFO0_TO(index)           do { } while (0)
#define LOGIT_0INFO_TO(index)           do { } while (0)
#define LOGIT_0_INFO_TO(index)          do { } while (0)
#define LOGIT_NOARGS_INFO_TO(index)     do { } while (0)
#define LOGIT_FORMAT_INFO_TO(index, fmt, ...) do { } while (0)
#define LOGIT_PRINT_INFO_TO(index, ...)       do { } while (0)
#define LOGIT_PRINTF_INFO_TO(index, fmt, ...) do { } while (0)
#endif

#if LOGIT_COMPILED_LEVEL <= LOGIT_LEVEL_DEBUG
// DEBUG level macros
#define LOGIT_DEBUG(...)                LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_DEBUG, {}, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_DEBUG0()                  LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_DEBUG, {})
#define LOGIT_0DEBUG()                  LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_DEBUG, {})
#define LOGIT_0_DEBUG()                 LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_DEBUG, {})
#define LOGIT_NOARGS_DEBUG()            LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_DEBUG, {})
#define LOGIT_FORMAT_DEBUG(fmt, ...)    LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_DEBUG, fmt, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINT_DEBUG(...)          LOGIT_LOG_AND_RETURN_PRINT(logit::LogLevel::LOG_LVL_DEBUG, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINTF_DEBUG(fmt, ...)    LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_DEBUG, logit::format(fmt, __VA_ARGS__))
#ifdef LOGIT_WITH_FMT
#define LOGITF_DEBUG(fmt_str, ...)      LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_DEBUG, fmt::format(fmt_str, __VA_ARGS__))
#define LOGIT_FMT_DEBUG(fmt_str, ...)   LOGIT_LOG_AND_RETURN_FMT(logit::LogLevel::LOG_LVL_DEBUG, fmt_str, #__VA_ARGS__, __VA_ARGS__)
#else
#define LOGITF_DEBUG(fmt_str, ...)      do { } while (0)
#define LOGIT_FMT_DEBUG(fmt_str, ...)   do { } while (0)
#endif

// DEBUG macros with index
#define LOGIT_DEBUG_TO(index, ...)      LOGIT_LOG_AND_RETURN_WITH_INDEX(logit::LogLevel::LOG_LVL_DEBUG, index, {}, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_DEBUG0_TO(index)          LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_DEBUG, index, {})
#define LOGIT_0DEBUG_TO(index)          LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_DEBUG, index, {})
#define LOGIT_0_DEBUG_TO(index)         LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_DEBUG, index, {})
#define LOGIT_NOARGS_DEBUG_TO(index)    LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_DEBUG, index, {})
#define LOGIT_FORMAT_DEBUG_TO(index, fmt, ...) LOGIT_LOG_AND_RETURN_WITH_INDEX(logit::LogLevel::LOG_LVL_DEBUG, index, fmt, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINT_DEBUG_TO(index, ...)       LOGIT_LOG_AND_RETURN_PRINT_WITH_INDEX(logit::LogLevel::LOG_LVL_DEBUG, index, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINTF_DEBUG_TO(index, fmt, ...) LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_DEBUG, index, logit::format(fmt, __VA_ARGS__))
#ifdef LOGIT_WITH_FMT
#define LOGITF_DEBUG_TO(index, fmt_str, ...) LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_DEBUG, index, fmt::format(fmt_str, __VA_ARGS__))
#define LOGIT_FMT_DEBUG_TO(index, fmt_str, ...) LOGIT_LOG_AND_RETURN_FMT_WITH_INDEX(logit::LogLevel::LOG_LVL_DEBUG, index, fmt_str, #__VA_ARGS__, __VA_ARGS__)
#else
#define LOGITF_DEBUG_TO(index, fmt_str, ...) do { } while (0)
#define LOGIT_FMT_DEBUG_TO(index, fmt_str, ...) do { } while (0)
#endif
#else
#define LOGIT_DEBUG(...)                do { } while (0)
#define LOGIT_DEBUG0()                  do { } while (0)
#define LOGIT_0DEBUG()                  do { } while (0)
#define LOGIT_0_DEBUG()                 do { } while (0)
#define LOGIT_NOARGS_DEBUG()            do { } while (0)
#define LOGIT_FORMAT_DEBUG(fmt, ...)    do { } while (0)
#define LOGIT_PRINT_DEBUG(...)          do { } while (0)
#define LOGIT_PRINTF_DEBUG(fmt, ...)    do { } while (0)
#define LOGIT_DEBUG_TO(index, ...)      do { } while (0)
#define LOGIT_DEBUG0_TO(index)          do { } while (0)
#define LOGIT_0DEBUG_TO(index)          do { } while (0)
#define LOGIT_0_DEBUG_TO(index)         do { } while (0)
#define LOGIT_NOARGS_DEBUG_TO(index)    do { } while (0)
#define LOGIT_FORMAT_DEBUG_TO(index, fmt, ...) do { } while (0)
#define LOGIT_PRINT_DEBUG_TO(index, ...)       do { } while (0)
#define LOGIT_PRINTF_DEBUG_TO(index, fmt, ...) do { } while (0)
#endif

#if LOGIT_COMPILED_LEVEL <= LOGIT_LEVEL_WARN
// WARN level macros
#define LOGIT_WARN(...)                 LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_WARN, {}, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_WARN0()                   LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_WARN, {})
#define LOGIT_0WARN()                   LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_WARN, {})
#define LOGIT_0_WARN()                  LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_WARN, {})
#define LOGIT_NOARGS_WARN()             LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_WARN, {})
#define LOGIT_FORMAT_WARN(fmt, ...)     LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_WARN, fmt, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINT_WARN(...)           LOGIT_LOG_AND_RETURN_PRINT(logit::LogLevel::LOG_LVL_WARN, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINTF_WARN(fmt, ...)     LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_WARN, logit::format(fmt, __VA_ARGS__))
#ifdef LOGIT_WITH_FMT
#define LOGITF_WARN(fmt_str, ...)       LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_WARN, fmt::format(fmt_str, __VA_ARGS__))
#define LOGIT_FMT_WARN(fmt_str, ...)    LOGIT_LOG_AND_RETURN_FMT(logit::LogLevel::LOG_LVL_WARN, fmt_str, #__VA_ARGS__, __VA_ARGS__)
#else
#define LOGITF_WARN(fmt_str, ...)       do { } while (0)
#define LOGIT_FMT_WARN(fmt_str, ...)    do { } while (0)
#endif

// WARN macros with index
#define LOGIT_WARN_TO(index, ...)       LOGIT_LOG_AND_RETURN_WITH_INDEX(logit::LogLevel::LOG_LVL_WARN, index, {}, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_WARN0_TO(index)           LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_WARN, index, {})
#define LOGIT_0WARN_TO(index)           LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_WARN, index, {})
#define LOGIT_0_WARN_TO(index)          LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_WARN, index, {})
#define LOGIT_NOARGS_WARN_TO(index)     LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_WARN, index, {})
#define LOGIT_FORMAT_WARN_TO(index, fmt, ...) LOGIT_LOG_AND_RETURN_WITH_INDEX(logit::LogLevel::LOG_LVL_WARN, index, fmt, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINT_WARN_TO(index, ...)       LOGIT_LOG_AND_RETURN_PRINT_WITH_INDEX(logit::LogLevel::LOG_LVL_WARN, index, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINTF_WARN_TO(index, fmt, ...) LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_WARN, index, logit::format(fmt, __VA_ARGS__))
#ifdef LOGIT_WITH_FMT
#define LOGITF_WARN_TO(index, fmt_str, ...) LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_WARN, index, fmt::format(fmt_str, __VA_ARGS__))
#define LOGIT_FMT_WARN_TO(index, fmt_str, ...) LOGIT_LOG_AND_RETURN_FMT_WITH_INDEX(logit::LogLevel::LOG_LVL_WARN, index, fmt_str, #__VA_ARGS__, __VA_ARGS__)
#else
#define LOGITF_WARN_TO(index, fmt_str, ...) do { } while (0)
#define LOGIT_FMT_WARN_TO(index, fmt_str, ...) do { } while (0)
#endif
#else
#define LOGIT_WARN(...)                 do { } while (0)
#define LOGIT_WARN0()                   do { } while (0)
#define LOGIT_0WARN()                   do { } while (0)
#define LOGIT_0_WARN()                  do { } while (0)
#define LOGIT_NOARGS_WARN()             do { } while (0)
#define LOGIT_FORMAT_WARN(fmt, ...)     do { } while (0)
#define LOGIT_PRINT_WARN(...)           do { } while (0)
#define LOGIT_PRINTF_WARN(fmt, ...)     do { } while (0)
#define LOGIT_WARN_TO(index, ...)       do { } while (0)
#define LOGIT_WARN0_TO(index)           do { } while (0)
#define LOGIT_0WARN_TO(index)           do { } while (0)
#define LOGIT_0_WARN_TO(index)          do { } while (0)
#define LOGIT_NOARGS_WARN_TO(index)     do { } while (0)
#define LOGIT_FORMAT_WARN_TO(index, fmt, ...) do { } while (0)
#define LOGIT_PRINT_WARN_TO(index, ...)       do { } while (0)
#define LOGIT_PRINTF_WARN_TO(index, fmt, ...) do { } while (0)
#endif

#if LOGIT_COMPILED_LEVEL <= LOGIT_LEVEL_ERROR
// ERROR level macros
#define LOGIT_ERROR(...)                LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_ERROR, {}, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_ERROR0()                  LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_ERROR, {})
#define LOGIT_0ERROR()                  LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_ERROR, {})
#define LOGIT_0_ERROR()                 LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_ERROR, {})
#define LOGIT_NOARGS_ERROR()            LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_ERROR, {})
#define LOGIT_FORMAT_ERROR(fmt, ...)    LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_ERROR, fmt, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINT_ERROR(...)          LOGIT_LOG_AND_RETURN_PRINT(logit::LogLevel::LOG_LVL_ERROR, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINTF_ERROR(fmt, ...)    LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_ERROR, logit::format(fmt, __VA_ARGS__))
#ifdef LOGIT_WITH_FMT
#define LOGITF_ERROR(fmt_str, ...)      LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_ERROR, fmt::format(fmt_str, __VA_ARGS__))
#define LOGIT_FMT_ERROR(fmt_str, ...)   LOGIT_LOG_AND_RETURN_FMT(logit::LogLevel::LOG_LVL_ERROR, fmt_str, #__VA_ARGS__, __VA_ARGS__)
#else
#define LOGITF_ERROR(fmt_str, ...)      do { } while (0)
#define LOGIT_FMT_ERROR(fmt_str, ...)   do { } while (0)
#endif

// ERROR macros with index
#define LOGIT_ERROR_TO(index, ...)      LOGIT_LOG_AND_RETURN_WITH_INDEX(logit::LogLevel::LOG_LVL_ERROR, index, {}, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_ERROR0_TO(index)          LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_ERROR, index, {})
#define LOGIT_0ERROR_TO(index)          LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_ERROR, index, {})
#define LOGIT_0_ERROR_TO(index)         LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_ERROR, index, {})
#define LOGIT_NOARGS_ERROR_TO(index)    LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_ERROR, index, {})
#define LOGIT_FORMAT_ERROR_TO(index, fmt, ...) LOGIT_LOG_AND_RETURN_WITH_INDEX(logit::LogLevel::LOG_LVL_ERROR, index, fmt, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINT_ERROR_TO(index, ...)       LOGIT_LOG_AND_RETURN_PRINT_WITH_INDEX(logit::LogLevel::LOG_LVL_ERROR, index, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINTF_ERROR_TO(index, fmt, ...) LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_ERROR, index, logit::format(fmt, __VA_ARGS__))
#ifdef LOGIT_WITH_FMT
#define LOGITF_ERROR_TO(index, fmt_str, ...) LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_ERROR, index, fmt::format(fmt_str, __VA_ARGS__))
#define LOGIT_FMT_ERROR_TO(index, fmt_str, ...) LOGIT_LOG_AND_RETURN_FMT_WITH_INDEX(logit::LogLevel::LOG_LVL_ERROR, index, fmt_str, #__VA_ARGS__, __VA_ARGS__)
#else
#define LOGITF_ERROR_TO(index, fmt_str, ...) do { } while (0)
#define LOGIT_FMT_ERROR_TO(index, fmt_str, ...) do { } while (0)
#endif
#else
#define LOGIT_ERROR(...)                do { } while (0)
#define LOGIT_ERROR0()                  do { } while (0)
#define LOGIT_0ERROR()                  do { } while (0)
#define LOGIT_0_ERROR()                 do { } while (0)
#define LOGIT_NOARGS_ERROR()            do { } while (0)
#define LOGIT_FORMAT_ERROR(fmt, ...)    do { } while (0)
#define LOGIT_PRINT_ERROR(...)          do { } while (0)
#define LOGIT_PRINTF_ERROR(fmt, ...)    do { } while (0)
#define LOGIT_ERROR_TO(index, ...)      do { } while (0)
#define LOGIT_ERROR0_TO(index)          do { } while (0)
#define LOGIT_0ERROR_TO(index)          do { } while (0)
#define LOGIT_0_ERROR_TO(index)         do { } while (0)
#define LOGIT_NOARGS_ERROR_TO(index)    do { } while (0)
#define LOGIT_FORMAT_ERROR_TO(index, fmt, ...) do { } while (0)
#define LOGIT_PRINT_ERROR_TO(index, ...)       do { } while (0)
#define LOGIT_PRINTF_ERROR_TO(index, fmt, ...) do { } while (0)
#endif

#if LOGIT_COMPILED_LEVEL <= LOGIT_LEVEL_FATAL
// FATAL level macros
#define LOGIT_FATAL(...)                LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_FATAL, {}, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_FATAL0()                  LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_FATAL, {})
#define LOGIT_0FATAL()                  LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_FATAL, {})
#define LOGIT_0_FATAL()                 LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_FATAL, {})
#define LOGIT_NOARGS_FATAL()            LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_FATAL, {})
#define LOGIT_FORMAT_FATAL(fmt, ...)    LOGIT_LOG_AND_RETURN(logit::LogLevel::LOG_LVL_FATAL, fmt, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINT_FATAL(...)          LOGIT_LOG_AND_RETURN_PRINT(logit::LogLevel::LOG_LVL_FATAL, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINTF_FATAL(fmt, ...)    LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_FATAL, logit::format(fmt, __VA_ARGS__))
#ifdef LOGIT_WITH_FMT
#define LOGITF_FATAL(fmt_str, ...)      LOGIT_LOG_AND_RETURN_NOARGS(logit::LogLevel::LOG_LVL_FATAL, fmt::format(fmt_str, __VA_ARGS__))
#define LOGIT_FMT_FATAL(fmt_str, ...)   LOGIT_LOG_AND_RETURN_FMT(logit::LogLevel::LOG_LVL_FATAL, fmt_str, #__VA_ARGS__, __VA_ARGS__)
#else
#define LOGITF_FATAL(fmt_str, ...)      do { } while (0)
#define LOGIT_FMT_FATAL(fmt_str, ...)   do { } while (0)
#endif

// FATAL macros with index
#define LOGIT_FATAL_TO(index, ...)      LOGIT_LOG_AND_RETURN_WITH_INDEX(logit::LogLevel::LOG_LVL_FATAL, index, {}, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_FATAL0_TO(index)          LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_FATAL, index, {})
#define LOGIT_0FATAL_TO(index)          LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_FATAL, index, {})
#define LOGIT_0_FATAL_TO(index)         LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_FATAL, index, {})
#define LOGIT_NOARGS_FATAL_TO(index)    LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_FATAL, index, {})
#define LOGIT_FORMAT_FATAL_TO(index, fmt, ...) LOGIT_LOG_AND_RETURN_WITH_INDEX(logit::LogLevel::LOG_LVL_FATAL, index, fmt, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINT_FATAL_TO(index, ...)       LOGIT_LOG_AND_RETURN_PRINT_WITH_INDEX(logit::LogLevel::LOG_LVL_FATAL, index, #__VA_ARGS__, __VA_ARGS__)
#define LOGIT_PRINTF_FATAL_TO(index, fmt, ...) LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_FATAL, index, logit::format(fmt, __VA_ARGS__))
#ifdef LOGIT_WITH_FMT
#define LOGITF_FATAL_TO(index, fmt_str, ...) LOGIT_LOG_AND_RETURN_NOARGS_WITH_INDEX(logit::LogLevel::LOG_LVL_FATAL, index, fmt::format(fmt_str, __VA_ARGS__))
#define LOGIT_FMT_FATAL_TO(index, fmt_str, ...) LOGIT_LOG_AND_RETURN_FMT_WITH_INDEX(logit::LogLevel::LOG_LVL_FATAL, index, fmt_str, #__VA_ARGS__, __VA_ARGS__)
#else
#define LOGITF_FATAL_TO(index, fmt_str, ...) do { } while (0)
#define LOGIT_FMT_FATAL_TO(index, fmt_str, ...) do { } while (0)
#endif
#else
#define LOGIT_FATAL(...)                do { } while (0)
#define LOGIT_FATAL0()                  do { } while (0)
#define LOGIT_0FATAL()                  do { } while (0)
#define LOGIT_0_FATAL()                 do { } while (0)
#define LOGIT_NOARGS_FATAL()            do { } while (0)
#define LOGIT_FORMAT_FATAL(fmt, ...)    do { } while (0)
#define LOGIT_PRINT_FATAL(...)          do { } while (0)
#define LOGIT_PRINTF_FATAL(fmt, ...)    do { } while (0)
#define LOGIT_FATAL_TO(index, ...)      do { } while (0)
#define LOGIT_FATAL0_TO(index)          do { } while (0)
#define LOGIT_0FATAL_TO(index)          do { } while (0)
#define LOGIT_0_FATAL_TO(index)         do { } while (0)
#define LOGIT_NOARGS_FATAL_TO(index)    do { } while (0)
#define LOGIT_FORMAT_FATAL_TO(index, fmt, ...) do { } while (0)
#define LOGIT_PRINT_FATAL_TO(index, ...)       do { } while (0)
#define LOGIT_PRINTF_FATAL_TO(index, fmt, ...) do { } while (0)
#endif

//------------------------------------------------------------------------------
// Conditional logging macros (logging based on a condition)

/// \name Conditional Logging
/// Macros for logging based on conditions.
/// \{

#define LOGIT_DETAIL_CONDITIONAL_CALL(condition, expression)                                  \
    do {                                                                                      \
        if (condition)                                                                        \
            expression;                                                                       \
    } while (0)

#define LOGIT_DETAIL_CONDITIONAL(level, condition, ...)                                       \
    LOGIT_DETAIL_CONDITIONAL_CALL(                                                            \
        condition,                                                                            \
        LOGIT_LOG_AND_RETURN(level, {}, #__VA_ARGS__, __VA_ARGS__))

#define LOGIT_DETAIL_CONDITIONAL_NOARGS(level, condition)                                     \
    LOGIT_DETAIL_CONDITIONAL_CALL(condition, LOGIT_LOG_AND_RETURN_NOARGS(level, {}))

#define LOGIT_DETAIL_CONDITIONAL_FORMAT(level, condition, fmt, ...)                           \
    LOGIT_DETAIL_CONDITIONAL_CALL(                                                            \
        condition,                                                                            \
        LOGIT_LOG_AND_RETURN(level, fmt, #__VA_ARGS__, __VA_ARGS__))

#define LOGIT_DETAIL_CONDITIONAL_PRINT(level, condition, ...)                                 \
    LOGIT_DETAIL_CONDITIONAL_CALL(                                                            \
        condition,                                                                            \
        LOGIT_LOG_AND_RETURN_PRINT(level, #__VA_ARGS__, __VA_ARGS__))

#define LOGIT_DETAIL_CONDITIONAL_PRINTF(level, condition, fmt, ...)                           \
    LOGIT_DETAIL_CONDITIONAL_CALL(                                                            \
        condition,                                                                            \
        LOGIT_LOG_AND_RETURN_NOARGS(level, logit::format(fmt, __VA_ARGS__)))

#ifdef LOGIT_WITH_FMT
#define LOGIT_DETAIL_CONDITIONAL_FMT_STRING(level, condition, fmt_str, ...)                   \
    LOGIT_DETAIL_CONDITIONAL_CALL(                                                            \
        condition,                                                                            \
        LOGIT_LOG_AND_RETURN_NOARGS(level, fmt::format(fmt_str, __VA_ARGS__)))

#define LOGIT_DETAIL_CONDITIONAL_FMT(level, condition, fmt_str, ...)                          \
    LOGIT_DETAIL_CONDITIONAL_CALL(                                                            \
        condition,                                                                            \
        LOGIT_LOG_AND_RETURN_FMT(level, fmt_str, #__VA_ARGS__, __VA_ARGS__))
#endif

#if LOGIT_COMPILED_LEVEL <= LOGIT_LEVEL_TRACE
// TRACE level conditional macros
#define LOGIT_TRACE_IF(condition, ...)                                           \
    LOGIT_DETAIL_CONDITIONAL(logit::LogLevel::LOG_LVL_TRACE, condition, __VA_ARGS__)
#define LOGIT_TRACE0_IF(condition)                                               \
    LOGIT_DETAIL_CONDITIONAL_NOARGS(logit::LogLevel::LOG_LVL_TRACE, condition)
#define LOGIT_0TRACE_IF(condition)                                               \
    LOGIT_DETAIL_CONDITIONAL_NOARGS(logit::LogLevel::LOG_LVL_TRACE, condition)
#define LOGIT_0_TRACE_IF(condition)                                              \
    LOGIT_DETAIL_CONDITIONAL_NOARGS(logit::LogLevel::LOG_LVL_TRACE, condition)
#define LOGIT_NOARGS_TRACE_IF(condition)                                         \
    LOGIT_DETAIL_CONDITIONAL_NOARGS(logit::LogLevel::LOG_LVL_TRACE, condition)
#define LOGIT_FORMAT_TRACE_IF(condition, fmt, ...)                               \
    LOGIT_DETAIL_CONDITIONAL_FORMAT(                                             \
        logit::LogLevel::LOG_LVL_TRACE, condition, fmt, __VA_ARGS__)
#define LOGIT_PRINT_TRACE_IF(condition, ...)                                     \
    LOGIT_DETAIL_CONDITIONAL_PRINT(                                              \
        logit::LogLevel::LOG_LVL_TRACE, condition, __VA_ARGS__)
#define LOGIT_PRINTF_TRACE_IF(condition, fmt, ...)                               \
    LOGIT_DETAIL_CONDITIONAL_PRINTF(                                             \
        logit::LogLevel::LOG_LVL_TRACE, condition, fmt, __VA_ARGS__)
#ifdef LOGIT_WITH_FMT
#define LOGITF_TRACE_IF(condition, fmt_str, ...)                                 \
    LOGIT_DETAIL_CONDITIONAL_FMT_STRING(                                         \
        logit::LogLevel::LOG_LVL_TRACE, condition, fmt_str, __VA_ARGS__)
#define LOGIT_FMT_TRACE_IF(condition, fmt_str, ...)                              \
    LOGIT_DETAIL_CONDITIONAL_FMT(                                                \
        logit::LogLevel::LOG_LVL_TRACE, condition, fmt_str, __VA_ARGS__)
#else
#define LOGITF_TRACE_IF(condition, fmt_str, ...) do { } while (0)
#define LOGIT_FMT_TRACE_IF(condition, fmt_str, ...) do { } while (0)
#endif
#else
#define LOGIT_TRACE_IF(condition, ...)        do { } while (0)
#define LOGIT_TRACE0_IF(condition)            do { } while (0)
#define LOGIT_0TRACE_IF(condition)            do { } while (0)
#define LOGIT_0_TRACE_IF(condition)           do { } while (0)
#define LOGIT_NOARGS_TRACE_IF(condition)      do { } while (0)
#define LOGIT_FORMAT_TRACE_IF(condition, fmt, ...) do { } while (0)
#define LOGIT_PRINT_TRACE_IF(condition, ...)  do { } while (0)
#define LOGIT_PRINTF_TRACE_IF(condition, fmt, ...) do { } while (0)
#define LOGITF_TRACE(fmt_str, ...)      do { } while (0)
#define LOGIT_FMT_TRACE(fmt_str, ...)   do { } while (0)
#define LOGITF_TRACE_TO(index, fmt_str, ...) do { } while (0)
#define LOGIT_FMT_TRACE_TO(index, fmt_str, ...) do { } while (0)
#define LOGITF_TRACE_IF(condition, fmt_str, ...) do { } while (0)
#define LOGIT_FMT_TRACE_IF(condition, fmt_str, ...) do { } while (0)
#endif

#if LOGIT_COMPILED_LEVEL <= LOGIT_LEVEL_INFO
// INFO level conditional macros
#define LOGIT_INFO_IF(condition, ...)                                           \
    LOGIT_DETAIL_CONDITIONAL(logit::LogLevel::LOG_LVL_INFO, condition, __VA_ARGS__)
#define LOGIT_INFO0_IF(condition)                                               \
    LOGIT_DETAIL_CONDITIONAL_NOARGS(logit::LogLevel::LOG_LVL_INFO, condition)
#define LOGIT_0INFO_IF(condition)                                               \
    LOGIT_DETAIL_CONDITIONAL_NOARGS(logit::LogLevel::LOG_LVL_INFO, condition)
#define LOGIT_0_INFO_IF(condition)                                              \
    LOGIT_DETAIL_CONDITIONAL_NOARGS(logit::LogLevel::LOG_LVL_INFO, condition)
#define LOGIT_NOARGS_INFO_IF(condition)                                         \
    LOGIT_DETAIL_CONDITIONAL_NOARGS(logit::LogLevel::LOG_LVL_INFO, condition)
#define LOGIT_FORMAT_INFO_IF(condition, fmt, ...)                               \
    LOGIT_DETAIL_CONDITIONAL_FORMAT(                                             \
        logit::LogLevel::LOG_LVL_INFO, condition, fmt, __VA_ARGS__)
#define LOGIT_PRINT_INFO_IF(condition, ...)                                     \
    LOGIT_DETAIL_CONDITIONAL_PRINT(                                              \
        logit::LogLevel::LOG_LVL_INFO, condition, __VA_ARGS__)
#define LOGIT_PRINTF_INFO_IF(condition, fmt, ...)                               \
    LOGIT_DETAIL_CONDITIONAL_PRINTF(                                             \
        logit::LogLevel::LOG_LVL_INFO, condition, fmt, __VA_ARGS__)
#ifdef LOGIT_WITH_FMT
#define LOGITF_INFO_IF(condition, fmt_str, ...)                                 \
    LOGIT_DETAIL_CONDITIONAL_FMT_STRING(                                         \
        logit::LogLevel::LOG_LVL_INFO, condition, fmt_str, __VA_ARGS__)
#define LOGIT_FMT_INFO_IF(condition, fmt_str, ...)                              \
    LOGIT_DETAIL_CONDITIONAL_FMT(                                                \
        logit::LogLevel::LOG_LVL_INFO, condition, fmt_str, __VA_ARGS__)
#else
#define LOGITF_INFO_IF(condition, fmt_str, ...) do { } while (0)
#define LOGIT_FMT_INFO_IF(condition, fmt_str, ...) do { } while (0)
#endif
#else
#define LOGIT_INFO_IF(condition, ...)        do { } while (0)
#define LOGIT_INFO0_IF(condition)            do { } while (0)
#define LOGIT_0INFO_IF(condition)            do { } while (0)
#define LOGIT_0_INFO_IF(condition)           do { } while (0)
#define LOGIT_NOARGS_INFO_IF(condition)      do { } while (0)
#define LOGIT_FORMAT_INFO_IF(condition, fmt, ...) do { } while (0)
#define LOGIT_PRINT_INFO_IF(condition, ...)  do { } while (0)
#define LOGIT_PRINTF_INFO_IF(condition, fmt, ...) do { } while (0)
#define LOGITF_INFO(fmt_str, ...)      do { } while (0)
#define LOGIT_FMT_INFO(fmt_str, ...)   do { } while (0)
#define LOGITF_INFO_TO(index, fmt_str, ...) do { } while (0)
#define LOGIT_FMT_INFO_TO(index, fmt_str, ...) do { } while (0)
#define LOGITF_INFO_IF(condition, fmt_str, ...) do { } while (0)
#define LOGIT_FMT_INFO_IF(condition, fmt_str, ...) do { } while (0)
#endif

#if LOGIT_COMPILED_LEVEL <= LOGIT_LEVEL_DEBUG
// DEBUG level conditional macros
#define LOGIT_DEBUG_IF(condition, ...)                                           \
    LOGIT_DETAIL_CONDITIONAL(logit::LogLevel::LOG_LVL_DEBUG, condition, __VA_ARGS__)
#define LOGIT_DEBUG0_IF(condition)                                               \
    LOGIT_DETAIL_CONDITIONAL_NOARGS(logit::LogLevel::LOG_LVL_DEBUG, condition)
#define LOGIT_0DEBUG_IF(condition)                                               \
    LOGIT_DETAIL_CONDITIONAL_NOARGS(logit::LogLevel::LOG_LVL_DEBUG, condition)
#define LOGIT_0_DEBUG_IF(condition)                                              \
    LOGIT_DETAIL_CONDITIONAL_NOARGS(logit::LogLevel::LOG_LVL_DEBUG, condition)
#define LOGIT_NOARGS_DEBUG_IF(condition)                                         \
    LOGIT_DETAIL_CONDITIONAL_NOARGS(logit::LogLevel::LOG_LVL_DEBUG, condition)
#define LOGIT_FORMAT_DEBUG_IF(condition, fmt, ...)                               \
    LOGIT_DETAIL_CONDITIONAL_FORMAT(                                             \
        logit::LogLevel::LOG_LVL_DEBUG, condition, fmt, __VA_ARGS__)
#define LOGIT_PRINT_DEBUG_IF(condition, ...)                                     \
    LOGIT_DETAIL_CONDITIONAL_PRINT(                                              \
        logit::LogLevel::LOG_LVL_DEBUG, condition, __VA_ARGS__)
#define LOGIT_PRINTF_DEBUG_IF(condition, fmt, ...)                               \
    LOGIT_DETAIL_CONDITIONAL_PRINTF(                                             \
        logit::LogLevel::LOG_LVL_DEBUG, condition, fmt, __VA_ARGS__)
#ifdef LOGIT_WITH_FMT
#define LOGITF_DEBUG_IF(condition, fmt_str, ...)                                 \
    LOGIT_DETAIL_CONDITIONAL_FMT_STRING(                                         \
        logit::LogLevel::LOG_LVL_DEBUG, condition, fmt_str, __VA_ARGS__)
#define LOGIT_FMT_DEBUG_IF(condition, fmt_str, ...)                              \
    LOGIT_DETAIL_CONDITIONAL_FMT(                                                \
        logit::LogLevel::LOG_LVL_DEBUG, condition, fmt_str, __VA_ARGS__)
#else
#define LOGITF_DEBUG_IF(condition, fmt_str, ...) do { } while (0)
#define LOGIT_FMT_DEBUG_IF(condition, fmt_str, ...) do { } while (0)
#endif
#else
#define LOGIT_DEBUG_IF(condition, ...)        do { } while (0)
#define LOGIT_DEBUG0_IF(condition)            do { } while (0)
#define LOGIT_0DEBUG_IF(condition)            do { } while (0)
#define LOGIT_0_DEBUG_IF(condition)           do { } while (0)
#define LOGIT_NOARGS_DEBUG_IF(condition)      do { } while (0)
#define LOGIT_FORMAT_DEBUG_IF(condition, fmt, ...) do { } while (0)
#define LOGIT_PRINT_DEBUG_IF(condition, ...)  do { } while (0)
#define LOGIT_PRINTF_DEBUG_IF(condition, fmt, ...) do { } while (0)
#define LOGITF_DEBUG(fmt_str, ...)      do { } while (0)
#define LOGIT_FMT_DEBUG(fmt_str, ...)   do { } while (0)
#define LOGITF_DEBUG_TO(index, fmt_str, ...) do { } while (0)
#define LOGIT_FMT_DEBUG_TO(index, fmt_str, ...) do { } while (0)
#define LOGITF_DEBUG_IF(condition, fmt_str, ...) do { } while (0)
#define LOGIT_FMT_DEBUG_IF(condition, fmt_str, ...) do { } while (0)
#endif

#if LOGIT_COMPILED_LEVEL <= LOGIT_LEVEL_WARN
// WARN level conditional macros
#define LOGIT_WARN_IF(condition, ...)                                           \
    LOGIT_DETAIL_CONDITIONAL(logit::LogLevel::LOG_LVL_WARN, condition, __VA_ARGS__)
#define LOGIT_WARN0_IF(condition)                                               \
    LOGIT_DETAIL_CONDITIONAL_NOARGS(logit::LogLevel::LOG_LVL_WARN, condition)
#define LOGIT_0WARN_IF(condition)                                               \
    LOGIT_DETAIL_CONDITIONAL_NOARGS(logit::LogLevel::LOG_LVL_WARN, condition)
#define LOGIT_0_WARN_IF(condition)                                              \
    LOGIT_DETAIL_CONDITIONAL_NOARGS(logit::LogLevel::LOG_LVL_WARN, condition)
#define LOGIT_NOARGS_WARN_IF(condition)                                         \
    LOGIT_DETAIL_CONDITIONAL_NOARGS(logit::LogLevel::LOG_LVL_WARN, condition)
#define LOGIT_FORMAT_WARN_IF(condition, fmt, ...)                               \
    LOGIT_DETAIL_CONDITIONAL_FORMAT(                                             \
        logit::LogLevel::LOG_LVL_WARN, condition, fmt, __VA_ARGS__)
#define LOGIT_PRINT_WARN_IF(condition, ...)                                     \
    LOGIT_DETAIL_CONDITIONAL_PRINT(                                              \
        logit::LogLevel::LOG_LVL_WARN, condition, __VA_ARGS__)
#define LOGIT_PRINTF_WARN_IF(condition, fmt, ...)                               \
    LOGIT_DETAIL_CONDITIONAL_PRINTF(                                             \
        logit::LogLevel::LOG_LVL_WARN, condition, fmt, __VA_ARGS__)
#ifdef LOGIT_WITH_FMT
#define LOGITF_WARN_IF(condition, fmt_str, ...)                                 \
    LOGIT_DETAIL_CONDITIONAL_FMT_STRING(                                         \
        logit::LogLevel::LOG_LVL_WARN, condition, fmt_str, __VA_ARGS__)
#define LOGIT_FMT_WARN_IF(condition, fmt_str, ...)                              \
    LOGIT_DETAIL_CONDITIONAL_FMT(                                                \
        logit::LogLevel::LOG_LVL_WARN, condition, fmt_str, __VA_ARGS__)
#else
#define LOGITF_WARN_IF(condition, fmt_str, ...) do { } while (0)
#define LOGIT_FMT_WARN_IF(condition, fmt_str, ...) do { } while (0)
#endif
#else
#define LOGIT_WARN_IF(condition, ...)        do { } while (0)
#define LOGIT_WARN0_IF(condition)            do { } while (0)
#define LOGIT_0WARN_IF(condition)            do { } while (0)
#define LOGIT_0_WARN_IF(condition)           do { } while (0)
#define LOGIT_NOARGS_WARN_IF(condition)      do { } while (0)
#define LOGIT_FORMAT_WARN_IF(condition, fmt, ...) do { } while (0)
#define LOGIT_PRINT_WARN_IF(condition, ...)  do { } while (0)
#define LOGIT_PRINTF_WARN_IF(condition, fmt, ...) do { } while (0)
#define LOGITF_WARN(fmt_str, ...)      do { } while (0)
#define LOGIT_FMT_WARN(fmt_str, ...)   do { } while (0)
#define LOGITF_WARN_TO(index, fmt_str, ...) do { } while (0)
#define LOGIT_FMT_WARN_TO(index, fmt_str, ...) do { } while (0)
#define LOGITF_WARN_IF(condition, fmt_str, ...) do { } while (0)
#define LOGIT_FMT_WARN_IF(condition, fmt_str, ...) do { } while (0)
#endif

#if LOGIT_COMPILED_LEVEL <= LOGIT_LEVEL_ERROR
// ERROR level conditional macros
#define LOGIT_ERROR_IF(condition, ...)                                           \
    LOGIT_DETAIL_CONDITIONAL(logit::LogLevel::LOG_LVL_ERROR, condition, __VA_ARGS__)
#define LOGIT_ERROR0_IF(condition)                                               \
    LOGIT_DETAIL_CONDITIONAL_NOARGS(logit::LogLevel::LOG_LVL_ERROR, condition)
#define LOGIT_0ERROR_IF(condition)                                               \
    LOGIT_DETAIL_CONDITIONAL_NOARGS(logit::LogLevel::LOG_LVL_ERROR, condition)
#define LOGIT_0_ERROR_IF(condition)                                              \
    LOGIT_DETAIL_CONDITIONAL_NOARGS(logit::LogLevel::LOG_LVL_ERROR, condition)
#define LOGIT_NOARGS_ERROR_IF(condition)                                         \
    LOGIT_DETAIL_CONDITIONAL_NOARGS(logit::LogLevel::LOG_LVL_ERROR, condition)
#define LOGIT_FORMAT_ERROR_IF(condition, fmt, ...)                               \
    LOGIT_DETAIL_CONDITIONAL_FORMAT(                                             \
        logit::LogLevel::LOG_LVL_ERROR, condition, fmt, __VA_ARGS__)
#define LOGIT_PRINT_ERROR_IF(condition, ...)                                     \
    LOGIT_DETAIL_CONDITIONAL_PRINT(                                              \
        logit::LogLevel::LOG_LVL_ERROR, condition, __VA_ARGS__)
#define LOGIT_PRINTF_ERROR_IF(condition, fmt, ...)                               \
    LOGIT_DETAIL_CONDITIONAL_PRINTF(                                             \
        logit::LogLevel::LOG_LVL_ERROR, condition, fmt, __VA_ARGS__)
#ifdef LOGIT_WITH_FMT
#define LOGITF_ERROR_IF(condition, fmt_str, ...)                                 \
    LOGIT_DETAIL_CONDITIONAL_FMT_STRING(                                         \
        logit::LogLevel::LOG_LVL_ERROR, condition, fmt_str, __VA_ARGS__)
#define LOGIT_FMT_ERROR_IF(condition, fmt_str, ...)                              \
    LOGIT_DETAIL_CONDITIONAL_FMT(                                                \
        logit::LogLevel::LOG_LVL_ERROR, condition, fmt_str, __VA_ARGS__)
#else
#define LOGITF_ERROR_IF(condition, fmt_str, ...) do { } while (0)
#define LOGIT_FMT_ERROR_IF(condition, fmt_str, ...) do { } while (0)
#endif
#else
#define LOGIT_ERROR_IF(condition, ...)        do { } while (0)
#define LOGIT_ERROR0_IF(condition)            do { } while (0)
#define LOGIT_0ERROR_IF(condition)            do { } while (0)
#define LOGIT_0_ERROR_IF(condition)           do { } while (0)
#define LOGIT_NOARGS_ERROR_IF(condition)      do { } while (0)
#define LOGIT_FORMAT_ERROR_IF(condition, fmt, ...) do { } while (0)
#define LOGIT_PRINT_ERROR_IF(condition, ...)  do { } while (0)
#define LOGIT_PRINTF_ERROR_IF(condition, fmt, ...) do { } while (0)
#define LOGITF_ERROR(fmt_str, ...)      do { } while (0)
#define LOGIT_FMT_ERROR(fmt_str, ...)   do { } while (0)
#define LOGITF_ERROR_TO(index, fmt_str, ...) do { } while (0)
#define LOGIT_FMT_ERROR_TO(index, fmt_str, ...) do { } while (0)
#define LOGITF_ERROR_IF(condition, fmt_str, ...) do { } while (0)
#define LOGIT_FMT_ERROR_IF(condition, fmt_str, ...) do { } while (0)
#endif

#if LOGIT_COMPILED_LEVEL <= LOGIT_LEVEL_FATAL
// FATAL level conditional macros
#define LOGIT_FATAL_IF(condition, ...)                                           \
    LOGIT_DETAIL_CONDITIONAL(logit::LogLevel::LOG_LVL_FATAL, condition, __VA_ARGS__)
#define LOGIT_FATAL0_IF(condition)                                               \
    LOGIT_DETAIL_CONDITIONAL_NOARGS(logit::LogLevel::LOG_LVL_FATAL, condition)
#define LOGIT_0FATAL_IF(condition)                                               \
    LOGIT_DETAIL_CONDITIONAL_NOARGS(logit::LogLevel::LOG_LVL_FATAL, condition)
#define LOGIT_0_FATAL_IF(condition)                                              \
    LOGIT_DETAIL_CONDITIONAL_NOARGS(logit::LogLevel::LOG_LVL_FATAL, condition)
#define LOGIT_NOARGS_FATAL_IF(condition)                                         \
    LOGIT_DETAIL_CONDITIONAL_NOARGS(logit::LogLevel::LOG_LVL_FATAL, condition)
#define LOGIT_FORMAT_FATAL_IF(condition, fmt, ...)                               \
    LOGIT_DETAIL_CONDITIONAL_FORMAT(                                             \
        logit::LogLevel::LOG_LVL_FATAL, condition, fmt, __VA_ARGS__)
#define LOGIT_PRINT_FATAL_IF(condition, ...)                                     \
    LOGIT_DETAIL_CONDITIONAL_PRINT(                                              \
        logit::LogLevel::LOG_LVL_FATAL, condition, __VA_ARGS__)
#define LOGIT_PRINTF_FATAL_IF(condition, fmt, ...)                               \
    LOGIT_DETAIL_CONDITIONAL_PRINTF(                                             \
        logit::LogLevel::LOG_LVL_FATAL, condition, fmt, __VA_ARGS__)
#ifdef LOGIT_WITH_FMT
#define LOGITF_FATAL_IF(condition, fmt_str, ...)                                 \
    LOGIT_DETAIL_CONDITIONAL_FMT_STRING(                                         \
        logit::LogLevel::LOG_LVL_FATAL, condition, fmt_str, __VA_ARGS__)
#define LOGIT_FMT_FATAL_IF(condition, fmt_str, ...)                              \
    LOGIT_DETAIL_CONDITIONAL_FMT(                                                \
        logit::LogLevel::LOG_LVL_FATAL, condition, fmt_str, __VA_ARGS__)
#else
#define LOGITF_FATAL_IF(condition, fmt_str, ...) do { } while (0)
#define LOGIT_FMT_FATAL_IF(condition, fmt_str, ...) do { } while (0)
#endif
#else
#define LOGIT_FATAL_IF(condition, ...)        do { } while (0)
#define LOGIT_FATAL0_IF(condition)            do { } while (0)
#define LOGIT_0FATAL_IF(condition)            do { } while (0)
#define LOGIT_0_FATAL_IF(condition)           do { } while (0)
#define LOGIT_NOARGS_FATAL_IF(condition)      do { } while (0)
#define LOGIT_FORMAT_FATAL_IF(condition, fmt, ...) do { } while (0)
#define LOGIT_PRINT_FATAL_IF(condition, ...)  do { } while (0)
#define LOGIT_PRINTF_FATAL_IF(condition, fmt, ...) do { } while (0)
#define LOGITF_FATAL(fmt_str, ...)      do { } while (0)
#define LOGIT_FMT_FATAL(fmt_str, ...)   do { } while (0)
#define LOGITF_FATAL_TO(index, fmt_str, ...) do { } while (0)
#define LOGIT_FMT_FATAL_TO(index, fmt_str, ...) do { } while (0)
#define LOGITF_FATAL_IF(condition, fmt_str, ...) do { } while (0)
#define LOGIT_FMT_FATAL_IF(condition, fmt_str, ...) do { } while (0)
#endif

#define LOGIT_ONCE(level, ...)                                                     \
    do {                                                                          \
        static bool LOGIT_CONCAT(_logit_once_, __LINE__) = false;                 \
        if (!LOGIT_CONCAT(_logit_once_, __LINE__)) {                              \
            LOGIT_CONCAT(_logit_once_, __LINE__) = true;                          \
            LOGIT_LOG_AND_RETURN(level, {}, #__VA_ARGS__, __VA_ARGS__);           \
        }                                                                         \
    } while (0)

#define LOGIT_EVERY_N(level, n, ...)                                              \
    do {                                                                          \
        static unsigned LOGIT_CONCAT(_logit_count_, __LINE__) = 0;                \
        if (++LOGIT_CONCAT(_logit_count_, __LINE__) % (n) == 0) {                 \
            LOGIT_LOG_AND_RETURN(level, {}, #__VA_ARGS__, __VA_ARGS__);           \
        }                                                                         \
    } while (0)

#define LOGIT_THROTTLE(level, period_ms, ...)                                     \
    do {                                                                          \
        static int64_t LOGIT_CONCAT(_logit_last_, __LINE__) = 0;                  \
        int64_t _logit_now = LOGIT_MONOTONIC_MS();                        \
        if (_logit_now - LOGIT_CONCAT(_logit_last_, __LINE__) >= (period_ms)) {   \
            LOGIT_CONCAT(_logit_last_, __LINE__) = _logit_now;                    \
            LOGIT_LOG_AND_RETURN(level, {}, #__VA_ARGS__, __VA_ARGS__);           \
        }                                                                         \
    } while (0)

#define LOGIT_TRACE_ONCE(...)          LOGIT_ONCE(logit::LogLevel::LOG_LVL_TRACE, __VA_ARGS__)
#define LOGIT_DEBUG_ONCE(...)          LOGIT_ONCE(logit::LogLevel::LOG_LVL_DEBUG, __VA_ARGS__)
#define LOGIT_INFO_ONCE(...)           LOGIT_ONCE(logit::LogLevel::LOG_LVL_INFO, __VA_ARGS__)
#define LOGIT_WARN_ONCE(...)           LOGIT_ONCE(logit::LogLevel::LOG_LVL_WARN, __VA_ARGS__)
#define LOGIT_ERROR_ONCE(...)          LOGIT_ONCE(logit::LogLevel::LOG_LVL_ERROR, __VA_ARGS__)
#define LOGIT_FATAL_ONCE(...)          LOGIT_ONCE(logit::LogLevel::LOG_LVL_FATAL, __VA_ARGS__)

#define LOGIT_TRACE_EVERY_N(n, ...)    LOGIT_EVERY_N(logit::LogLevel::LOG_LVL_TRACE, n, __VA_ARGS__)
#define LOGIT_DEBUG_EVERY_N(n, ...)    LOGIT_EVERY_N(logit::LogLevel::LOG_LVL_DEBUG, n, __VA_ARGS__)
#define LOGIT_INFO_EVERY_N(n, ...)     LOGIT_EVERY_N(logit::LogLevel::LOG_LVL_INFO, n, __VA_ARGS__)
#define LOGIT_WARN_EVERY_N(n, ...)     LOGIT_EVERY_N(logit::LogLevel::LOG_LVL_WARN, n, __VA_ARGS__)
#define LOGIT_ERROR_EVERY_N(n, ...)    LOGIT_EVERY_N(logit::LogLevel::LOG_LVL_ERROR, n, __VA_ARGS__)
#define LOGIT_FATAL_EVERY_N(n, ...)    LOGIT_EVERY_N(logit::LogLevel::LOG_LVL_FATAL, n, __VA_ARGS__)

#define LOGIT_TRACE_THROTTLE(p, ...)   LOGIT_THROTTLE(logit::LogLevel::LOG_LVL_TRACE, p, __VA_ARGS__)
#define LOGIT_DEBUG_THROTTLE(p, ...)   LOGIT_THROTTLE(logit::LogLevel::LOG_LVL_DEBUG, p, __VA_ARGS__)
#define LOGIT_INFO_THROTTLE(p, ...)    LOGIT_THROTTLE(logit::LogLevel::LOG_LVL_INFO, p, __VA_ARGS__)
#define LOGIT_WARN_THROTTLE(p, ...)    LOGIT_THROTTLE(logit::LogLevel::LOG_LVL_WARN, p, __VA_ARGS__)
#define LOGIT_ERROR_THROTTLE(p, ...)   LOGIT_THROTTLE(logit::LogLevel::LOG_LVL_ERROR, p, __VA_ARGS__)
#define LOGIT_FATAL_THROTTLE(p, ...)   LOGIT_THROTTLE(logit::LogLevel::LOG_LVL_FATAL, p, __VA_ARGS__)

#define LOGIT_TAG(k, v) ::logit::detail::make_tag((k), (v))

#define LOGIT_TRACE_TAG(msg, ...)      LOGIT_PRINT_TRACE(msg, ::logit::detail::format_tags({ __VA_ARGS__ }))
#define LOGIT_DEBUG_TAG(msg, ...)      LOGIT_PRINT_DEBUG(msg, ::logit::detail::format_tags({ __VA_ARGS__ }))
#define LOGIT_INFO_TAG(msg, ...)       LOGIT_PRINT_INFO (msg, ::logit::detail::format_tags({ __VA_ARGS__ }))
#define LOGIT_WARN_TAG(msg, ...)       LOGIT_PRINT_WARN (msg, ::logit::detail::format_tags({ __VA_ARGS__ }))
#define LOGIT_ERROR_TAG(msg, ...)      LOGIT_PRINT_ERROR(msg, ::logit::detail::format_tags({ __VA_ARGS__ }))
#define LOGIT_FATAL_TAG(msg, ...)      LOGIT_PRINT_FATAL(msg, ::logit::detail::format_tags({ __VA_ARGS__ }))

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

#define LOGIT_ADD_FILE_LOGGER_WITH_ROTATION(dir, async, days, pattern, max_bytes, max_files) \
    logit::Logger::get_instance().add_logger( \
        std::make_unique<logit::FileLogger>(dir, async, days, max_bytes, max_files), \
        std::make_unique<logit::SimpleLogFormatter>(pattern))

#define LOGIT_ADD_FILE_LOGGER_WITH_ROTATION_SINGLE_MODE(dir, async, days, pattern, max_bytes, max_files) \
    logit::Logger::get_instance().add_logger( \
        std::make_unique<logit::FileLogger>(dir, async, days, max_bytes, max_files), \
        std::make_unique<logit::SimpleLogFormatter>(pattern), \
        true)

#define LOGIT_ADD_FILE_LOGGER_DEFAULT_WITH_ROTATION() \
    logit::Logger::get_instance().add_logger( \
        std::make_unique<logit::FileLogger>( \
            LOGIT_FILE_LOGGER_PATH, true, LOGIT_FILE_LOGGER_AUTO_DELETE_DAYS, \
            LOGIT_FILE_LOGGER_MAX_FILE_SIZE_BYTES, LOGIT_FILE_LOGGER_MAX_ROTATED_FILES), \
        std::make_unique<logit::SimpleLogFormatter>(LOGIT_FILE_LOGGER_PATTERN))

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

/// \brief Macro for adding a crash logger with custom configuration.
/// \param path Path where the crash log should be written.
/// \param buffer_size Size of the in-memory buffer preserving recent messages.
/// This version uses `std::make_unique`, available in C++17 and later.
#define LOGIT_ADD_CRASH_LOGGER(path, buffer_size)  \
    logit::Logger::get_instance().add_logger(      \
        std::make_unique<logit::CrashLogger>(      \
            logit::CrashLogger::Config{path, buffer_size}), \
        std::make_unique<logit::SimpleLogFormatter>(LOGIT_FILE_LOGGER_PATTERN))

/// \brief Macro for adding a crash logger with custom configuration in single_mode.
/// In single_mode, the crash logger can only be invoked using macros like `LOGIT_TRACE_TO`.
/// This version uses `std::make_unique`, available in C++17 and later.
#define LOGIT_ADD_CRASH_LOGGER_SINGLE_MODE(path, buffer_size)  \
    logit::Logger::get_instance().add_logger(                  \
        std::make_unique<logit::CrashLogger>(                  \
            logit::CrashLogger::Config{path, buffer_size}),    \
        std::make_unique<logit::SimpleLogFormatter>(LOGIT_FILE_LOGGER_PATTERN), \
        true)

/// \brief Macro for adding a crash logger with default configuration.
/// This version uses `std::make_unique`, available in C++17 and later.
#define LOGIT_ADD_CRASH_LOGGER_DEFAULT()                             \
    logit::Logger::get_instance().add_logger(                        \
        std::make_unique<logit::CrashLogger>(),                      \
        std::make_unique<logit::SimpleLogFormatter>(LOGIT_FILE_LOGGER_PATTERN))

/// \brief Macro for adding a syslog logger with custom configuration.
/// \param ident Syslog identifier used to tag the log entries.
/// \param facility Syslog facility value (e.g., `LOG_USER`).
/// \param async Boolean indicating whether logging should be asynchronous (`true`) or synchronous (`false`).
/// This version uses `std::make_unique`, available in C++17 and later.
#define LOGIT_ADD_SYSLOG(ident, facility, async) \
    logit::Logger::get_instance().add_logger( \
        std::make_unique<logit::SyslogLogger>(ident, facility, async), \
        std::make_unique<logit::SimpleLogFormatter>(LOGIT_CONSOLE_PATTERN), false)

/// \brief Macro for adding a syslog logger with custom configuration in single_mode.
/// \param ident Syslog identifier used to tag the log entries.
/// \param facility Syslog facility value (e.g., `LOG_USER`).
/// \param async Boolean indicating whether logging should be asynchronous (`true`) or synchronous (`false`).
/// In single_mode, the syslog logger can only be invoked using macros like `LOGIT_TRACE_TO`.
/// This version uses `std::make_unique`, available in C++17 and later.
#define LOGIT_ADD_SYSLOG_SINGLE_MODE(ident, facility, async) \
    logit::Logger::get_instance().add_logger( \
        std::make_unique<logit::SyslogLogger>(ident, facility, async), \
        std::make_unique<logit::SimpleLogFormatter>(LOGIT_CONSOLE_PATTERN), true)

/// \brief Macro for adding a syslog logger with default configuration.
/// This version uses `std::make_unique`, available in C++17 and later.
#define LOGIT_ADD_SYSLOG_DEFAULT() LOGIT_ADD_SYSLOG("log-it", LOG_USER, true)

/// \brief Macro for adding a Windows Event Log logger with custom configuration.
/// \param source_wide Wide-character name of the event source.
/// \param async Boolean indicating whether logging should be asynchronous (`true`) or synchronous (`false`).
/// This version uses `std::make_unique`, available in C++17 and later.
#define LOGIT_ADD_EVENT_LOG(source_wide, async) \
    logit::Logger::get_instance().add_logger( \
        std::make_unique<logit::EventLogLogger>(source_wide, async), \
        std::make_unique<logit::SimpleLogFormatter>(LOGIT_CONSOLE_PATTERN), false)

/// \brief Macro for adding a Windows Event Log logger with custom configuration in single_mode.
/// \param source_wide Wide-character name of the event source.
/// \param async Boolean indicating whether logging should be asynchronous (`true`) or synchronous (`false`).
/// In single_mode, the event log logger can only be invoked using macros like `LOGIT_TRACE_TO`.
/// This version uses `std::make_unique`, available in C++17 and later.
#define LOGIT_ADD_EVENT_LOG_SINGLE_MODE(source_wide, async) \
    logit::Logger::get_instance().add_logger( \
        std::make_unique<logit::EventLogLogger>(source_wide, async), \
        std::make_unique<logit::SimpleLogFormatter>(LOGIT_CONSOLE_PATTERN), true)

/// \brief Macro for adding a Windows Event Log logger with default configuration.
/// This version uses `std::make_unique`, available in C++17 and later.
#define LOGIT_ADD_EVENT_LOG_DEFAULT() LOGIT_ADD_EVENT_LOG(L"LogIt", true)

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

#define LOGIT_ADD_FILE_LOGGER_WITH_ROTATION(dir, async, days, pattern, max_bytes, max_files) \
    logit::Logger::get_instance().add_logger( \
        std::unique_ptr<logit::FileLogger>(new logit::FileLogger( \
            dir, async, days, max_bytes, max_files)), \
        std::unique_ptr<logit::SimpleLogFormatter>(new logit::SimpleLogFormatter(pattern)))

#define LOGIT_ADD_FILE_LOGGER_WITH_ROTATION_SINGLE_MODE(dir, async, days, pattern, max_bytes, max_files) \
    logit::Logger::get_instance().add_logger( \
        std::unique_ptr<logit::FileLogger>(new logit::FileLogger( \
            dir, async, days, max_bytes, max_files)), \
        std::unique_ptr<logit::SimpleLogFormatter>(new logit::SimpleLogFormatter(pattern)), \
        true)

#define LOGIT_ADD_FILE_LOGGER_DEFAULT_WITH_ROTATION() \
    logit::Logger::get_instance().add_logger( \
        std::unique_ptr<logit::FileLogger>(new logit::FileLogger( \
            LOGIT_FILE_LOGGER_PATH, true, LOGIT_FILE_LOGGER_AUTO_DELETE_DAYS, \
            LOGIT_FILE_LOGGER_MAX_FILE_SIZE_BYTES, LOGIT_FILE_LOGGER_MAX_ROTATED_FILES)), \
        std::unique_ptr<logit::SimpleLogFormatter>(new logit::SimpleLogFormatter(LOGIT_FILE_LOGGER_PATTERN)))

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

/// \brief Macro for adding a crash logger with custom configuration.
/// \param path Path where the crash log should be written.
/// \param buffer_size Size of the in-memory buffer preserving recent messages.
/// This version uses `new` and `std::unique_ptr` for C++11 compatibility.
#define LOGIT_ADD_CRASH_LOGGER(path, buffer_size)  \
    logit::Logger::get_instance().add_logger(      \
        std::unique_ptr<logit::CrashLogger>(       \
            new logit::CrashLogger(                \
                logit::CrashLogger::Config{path, buffer_size})), \
        std::unique_ptr<logit::SimpleLogFormatter>(new logit::SimpleLogFormatter(LOGIT_FILE_LOGGER_PATTERN)))

/// \brief Macro for adding a crash logger with custom configuration in single_mode.
/// In single_mode, the crash logger can only be invoked using macros like `LOGIT_TRACE_TO`.
/// This version uses `new` and `std::unique_ptr` for C++11 compatibility.
#define LOGIT_ADD_CRASH_LOGGER_SINGLE_MODE(path, buffer_size)  \
    logit::Logger::get_instance().add_logger(                  \
        std::unique_ptr<logit::CrashLogger>(                   \
            new logit::CrashLogger(                            \
                logit::CrashLogger::Config{path, buffer_size})), \
        std::unique_ptr<logit::SimpleLogFormatter>(new logit::SimpleLogFormatter(LOGIT_FILE_LOGGER_PATTERN)), \
        true)

/// \brief Macro for adding a crash logger with default configuration.
/// This version uses `new` and `std::unique_ptr` for C++11 compatibility.
#define LOGIT_ADD_CRASH_LOGGER_DEFAULT()                             \
    logit::Logger::get_instance().add_logger(                        \
        std::unique_ptr<logit::CrashLogger>(new logit::CrashLogger()), \
        std::unique_ptr<logit::SimpleLogFormatter>(new logit::SimpleLogFormatter(LOGIT_FILE_LOGGER_PATTERN)))

/// \brief Macro for adding a syslog logger with custom configuration.
/// \param ident Syslog identifier used to tag the log entries.
/// \param facility Syslog facility value (e.g., `LOG_USER`).
/// \param async Boolean indicating whether logging should be asynchronous (`true`) or synchronous (`false`).
/// This version uses `new` and `std::unique_ptr` for C++11 compatibility.
#define LOGIT_ADD_SYSLOG(ident, facility, async) \
    logit::Logger::get_instance().add_logger( \
        std::unique_ptr<logit::SyslogLogger>(new logit::SyslogLogger(ident, facility, async)), \
        std::unique_ptr<logit::SimpleLogFormatter>(new logit::SimpleLogFormatter(LOGIT_CONSOLE_PATTERN)), \
        false)

/// \brief Macro for adding a syslog logger with custom configuration in single_mode.
/// \param ident Syslog identifier used to tag the log entries.
/// \param facility Syslog facility value (e.g., `LOG_USER`).
/// \param async Boolean indicating whether logging should be asynchronous (`true`) or synchronous (`false`).
/// In single_mode, the syslog logger can only be invoked using macros like `LOGIT_TRACE_TO`.
/// This version uses `new` and `std::unique_ptr` for C++11 compatibility.
#define LOGIT_ADD_SYSLOG_SINGLE_MODE(ident, facility, async) \
    logit::Logger::get_instance().add_logger( \
        std::unique_ptr<logit::SyslogLogger>(new logit::SyslogLogger(ident, facility, async)), \
        std::unique_ptr<logit::SimpleLogFormatter>(new logit::SimpleLogFormatter(LOGIT_CONSOLE_PATTERN)), \
        true)

/// \brief Macro for adding a syslog logger with default configuration.
/// This version uses `new` and `std::unique_ptr` for C++11 compatibility.
#define LOGIT_ADD_SYSLOG_DEFAULT() LOGIT_ADD_SYSLOG("log-it", LOG_USER, true)

/// \brief Macro for adding a Windows Event Log logger with custom configuration.
/// \param source_wide Wide-character name of the event source.
/// \param async Boolean indicating whether logging should be asynchronous (`true`) or synchronous (`false`).
/// This version uses `new` and `std::unique_ptr` for C++11 compatibility.
#define LOGIT_ADD_EVENT_LOG(source_wide, async) \
    logit::Logger::get_instance().add_logger( \
        std::unique_ptr<logit::EventLogLogger>(new logit::EventLogLogger(source_wide, async)), \
        std::unique_ptr<logit::SimpleLogFormatter>(new logit::SimpleLogFormatter(LOGIT_CONSOLE_PATTERN)), \
        false)

/// \brief Macro for adding a Windows Event Log logger with custom configuration in single_mode.
/// \param source_wide Wide-character name of the event source.
/// \param async Boolean indicating whether logging should be asynchronous (`true`) or synchronous (`false`).
/// In single_mode, the event log logger can only be invoked using macros like `LOGIT_TRACE_TO`.
/// This version uses `new` and `std::unique_ptr` for C++11 compatibility.
#define LOGIT_ADD_EVENT_LOG_SINGLE_MODE(source_wide, async) \
    logit::Logger::get_instance().add_logger( \
        std::unique_ptr<logit::EventLogLogger>(new logit::EventLogLogger(source_wide, async)), \
        std::unique_ptr<logit::SimpleLogFormatter>(new logit::SimpleLogFormatter(LOGIT_CONSOLE_PATTERN)), \
        true)

/// \brief Macro for adding a Windows Event Log logger with default configuration.
/// This version uses `new` and `std::unique_ptr` for C++11 compatibility.
#define LOGIT_ADD_EVENT_LOG_DEFAULT() LOGIT_ADD_EVENT_LOG(L"LogIt", true)

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

/// \name Task Executor Configuration
/// Macros for configuring the TaskExecutor.
/// \{

/// \brief Sets the maximum number of queued tasks.
/// \param size Maximum queue size (0 for unlimited).
#define LOGIT_SET_MAX_QUEUE(size) \
    logit::detail::TaskExecutor::get_instance().set_max_queue_size(size)

/// \brief Queue policy for dropping the newest task when the queue is full.
#define LOGIT_QUEUE_DROP_NEWEST logit::detail::QueuePolicy::DropNewest

/// \brief Queue policy for dropping the oldest task when the queue is full.
#define LOGIT_QUEUE_DROP_OLDEST logit::detail::QueuePolicy::DropOldest

/// \brief Backward-compatible alias for dropping the newest task.
#define LOGIT_QUEUE_DROP LOGIT_QUEUE_DROP_NEWEST

/// \brief Queue policy for blocking when the queue is full.
#define LOGIT_QUEUE_BLOCK logit::detail::QueuePolicy::Block

/// \brief Sets the behavior when the queue is full.
/// \param mode LOGIT_QUEUE_DROP_NEWEST, LOGIT_QUEUE_DROP_OLDEST or LOGIT_QUEUE_BLOCK.
#define LOGIT_SET_QUEUE_POLICY(mode) \
    logit::detail::TaskExecutor::get_instance().set_queue_policy(mode)

/// \}

/// \brief Macro for waiting for all asynchronous loggers to finish processing.
#define LOGIT_WAIT() logit::Logger::get_instance().wait()

/// \brief Macro for shutting down logger system.
#define LOGIT_SHUTDOWN() logit::Logger::get_instance().shutdown()

//------------------------------------------------------------------------------

/// \}

#endif // LOGIT_LOG_MACROS_HPP_INCLUDED
