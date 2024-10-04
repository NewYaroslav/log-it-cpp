#pragma once
#ifndef _LOGIT_ILOGGER_HPP_INCLUDED
#define _LOGIT_ILOGGER_HPP_INCLUDED
/// \file ILogger.hpp
/// \brief Defines the interface for loggers used in the logging system.

#include "../Utils/LogRecord.hpp"

namespace logit {

    /// \class ILogger
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

        /// \brief Waits for all asynchronous logging operations to complete.
        ///
        /// This pure virtual function must be implemented by derived logger classes.
        /// It ensures that any pending log messages are fully processed, especially when logging asynchronously.
        virtual void wait() = 0;
    }; // ILogger

}; // namespace logit

#endif // _LOGIT_ILOGGER_HPP_INCLUDED
