#pragma once
#ifndef _LOGIT_ILOGGER_HPP_INCLUDED
#define _LOGIT_ILOGGER_HPP_INCLUDED

/// \file ILogger.hpp
/// \brief Defines the interface for loggers used in the logging system.

/// \ingroup LogBackends Logging Backends
/// \{

#include <string>
#include <vector>

namespace logit {

    /// \brief Optional buffered snapshot access returns empty results by default.
    /// Custom loggers may override these methods when they keep recent history.

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
        virtual void log(const LogRecord& record, const std::string& message) = 0;

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

        /// \brief Sets the minimal log level for this logger.
        /// \param level Minimum log level.
        virtual void set_log_level(LogLevel level) = 0;

        /// \brief Gets the minimal log level for this logger.
        /// \return Current minimal log level.
        virtual LogLevel get_log_level() const = 0;

        /// \brief Returns buffered formatted messages in chronological order.
        /// \details Override only if the logger keeps recent history and can
        /// safely serve snapshots while `log()` may run concurrently.
        /// \return Buffered messages, or an empty vector if unsupported.
        virtual std::vector<std::string> get_buffered_strings() const {
            return std::vector<std::string>();
        }

        /// \brief Returns buffered structured entries in chronological order.
        /// \details Override only if the logger keeps recent history and can
        /// safely serve snapshots while `log()` may run concurrently.
        /// \return Buffered entries, or an empty vector if unsupported.
        virtual std::vector<BufferedLogEntry> get_buffered_entries() const {
            return std::vector<BufferedLogEntry>();
        }

        /// \brief Lists persisted log files owned by this backend.
        /// \details Override only for file-based backends that can safely
        /// enumerate their own persisted files while `log()` may run
        /// concurrently.
        /// \return File metadata list, or an empty vector if unsupported.
        virtual std::vector<LogFileInfo> list_log_files() const {
            return std::vector<LogFileInfo>();
        }

        /// \brief Reads one persisted log file owned by this backend.
        /// \details This API is intended for persisted-file access only. It
        /// must not drain async queues or wait for pending writes to finish.
        /// \param path Full path returned by `list_log_files()`.
        /// \return Read result. `ok` is false when unsupported or unavailable.
        virtual LogFileReadResult read_log_file(const std::string& path) const {
            LogFileReadResult result;
            result.file.path = path;
            result.ok = false;
            return result;
        }

        /// \brief Reads several persisted log files owned by this backend.
        /// \details Default implementation preserves request order by calling
        /// `read_log_file()` for each requested path.
        /// \param paths Full paths returned by `list_log_files()`.
        /// \return Per-file results in the same order as `paths`.
        virtual std::vector<LogFileReadResult> read_log_files(const std::vector<std::string>& paths) const {
            std::vector<LogFileReadResult> results;
            results.reserve(paths.size());
            for (size_t i = 0; i < paths.size(); ++i) {
                results.push_back(read_log_file(paths[i]));
            }
            return results;
        }

        /// \brief Waits for all asynchronous logging operations to complete.
        ///
        /// This pure virtual function must be implemented by derived logger classes.
        /// It ensures that any pending log messages are fully processed, especially when logging asynchronously.
        virtual void wait() = 0;
    }; // ILogger

}; // namespace logit

/// \}

#endif // _LOGIT_ILOGGER_HPP_INCLUDED
