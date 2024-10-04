#pragma once
#ifndef _LOGIT_LOG_RECORD_HPP_INCLUDED
#define _LOGIT_LOG_RECORD_HPP_INCLUDED
/// \file LogRecord.hpp
/// \brief Contains the definition of the LogRecord structure for storing log data.

#include "argument_utils.hpp"
#include "../Enums.hpp"
#include <thread>

namespace logit {

    /// \struct LogRecord
    /// \brief Stores log metadata and content.
    struct LogRecord {
        const LogLevel      log_level;      ///< Log level (severity).
        const int64_t       timestamp_ms;   ///< Timestamp in milliseconds.
        const std::string   file;           ///< Source file name.
        const int           line;           ///< Line number in the source file.
        const std::string   function;       ///< Function name.
        const std::string   format;         ///< Format string for the message.
        const std::string   arg_names;      ///< Argument names for the log.
        std::vector<VariableValue> args_array;  ///< Argument values for the log.
        std::thread::id     thread_id;      ///< ID of the logging thread.
        const int           logger_index;   ///< Logger index (-1 to log to all).

        /// \brief Constructor with argument names.
        /// \param log_level Log severity level.
        /// \param timestamp_ms Timestamp in milliseconds.
        /// \param file Source file name.
        /// \param line Line number.
        /// \param function Function name.
        /// \param format Format string for the log.
        /// \param arg_names Names of the log arguments.
        /// \param logger_index Logger index (-1 for all loggers).
        LogRecord(
            const LogLevel& log_level,
            const int64_t& timestamp_ms,
            const std::string& file,
            const int& line,
            const std::string& function,
            const std::string& format,
            const std::string& arg_names,
            const int& logger_index) :
                log_level(log_level),
                timestamp_ms(timestamp_ms),
                file(file),
                line(line),
                function(function),
                format(format),
                arg_names(arg_names),
                logger_index(logger_index) {
            thread_id = std::this_thread::get_id();
        };
    };

}; // namespace logit

#endif // _LOGIT_LOG_RECORD_HPP_INCLUDED
