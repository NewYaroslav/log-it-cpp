#pragma once
#ifndef _LOGIT_ILOGGER_HPP_INCLUDED
#define _LOGIT_ILOGGER_HPP_INCLUDED
/// \file ILogger.hpp
/// \brief Defines the interface for loggers used in the logging system.

/// \defgroup LogBackends Logging Backends
/// \brief A collection of backends for the LogIt logging system.
///
/// This group includes various logger implementations that define how
/// log messages are processed and stored, such as console output, file logging,
/// and unique file logging.
///
/// ### Backends Included:
/// - ConsoleLogger: Outputs logs to the console with optional color coding.
/// - FileLogger: Logs messages to files with date-based rotation and old file deletion.
/// - UniqueFileLogger: Writes each log message to a unique file with automatic cleanup.
///
/// \{

namespace logit {

    /// \interface ILogger
    /// \brief Interface for loggers that handle log message output.
    class ILogger {
    public:
        virtual ~ILogger() = default;

        /// \brief Logs a message.
        ///
        /// This pure virtual function must be implemented by derived logger classes.
        /// It handles the logging of messages, which could be output to a console, file, or other destinations.
        ///
        /// \param record The log record containing details about the log event.
        /// \param message The formatted log message.
        virtual void log(const LogRecord&, const std::string&) = 0;

        /// \brief Retrieves a string parameter from the logger.
        /// Derived classes should implement this to return specific string-based parameters.
        /// \param param The parameter type to retrieve.
        /// \return A string representing the requested parameter.
        virtual std::string get_string_param(const LoggerParam& param) const = 0;

        /// \brief Retrieves an integer parameter from the logger.
        /// Derived classes should implement this to return specific integer-based parameters.
        /// \param param The parameter type to retrieve.
        /// \return An integer representing the requested parameter.
        virtual int64_t get_int_param(const LoggerParam& param) const = 0;

        /// \brief Retrieves a floating-point parameter from the logger.
        /// Derived classes should implement this to return specific floating-point-based parameters.
        /// \param param The parameter type to retrieve.
        /// \return A double representing the requested parameter.
        virtual double get_float_param(const LoggerParam& param) const = 0;

        /// \brief Waits for all asynchronous logging operations to complete.
        ///
        /// This pure virtual function must be implemented by derived logger classes.
        /// It ensures that any pending log messages are fully processed, especially when logging asynchronously.
        virtual void wait() = 0;
    }; // ILogger

}; // namespace logit

/// \}

#endif // _LOGIT_ILOGGER_HPP_INCLUDED
