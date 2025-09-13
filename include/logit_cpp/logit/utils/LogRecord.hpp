#pragma once
#ifndef _LOGIT_LOG_RECORD_HPP_INCLUDED
#define _LOGIT_LOG_RECORD_HPP_INCLUDED

/// \file LogRecord.hpp
/// \brief Contains the definition of the LogRecord structure for storing log data.

#include <string>
#include <vector>
#include <cstdint>
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
        const bool          print_mode;     ///< Flag to determine whether arguments are printed in a raw format without special symbols.

        /// \brief Constructor with argument names.
        /// \param log_level Log severity level.
        /// \param timestamp_ms Timestamp in milliseconds.
        /// \param file Source file name.
        /// \param line Line number.
        /// \param function Function name.
        /// \param format Format string for the log.
        /// \param arg_names Names of the log arguments.
        /// \param logger_index Logger index (-1 for all loggers).
        /// \param print_mode Flag indicating if the log should print arguments in a raw format (true) or use formatted output (false).
        LogRecord(
            LogLevel log_level,
            int64_t timestamp_ms,
            const std::string& file,
            int line,
            const std::string& function,
            const std::string& format,
            const std::string& arg_names,
            int logger_index,
            bool print_mode) :
                log_level(log_level),
                timestamp_ms(timestamp_ms),
                file(file),
                line(line),
                function(function),
                format(format),
                arg_names(arg_names),
                thread_id(std::this_thread::get_id()),
                logger_index(logger_index),
                print_mode(print_mode) {
        };
    };

}; // namespace logit

#endif // _LOGIT_LOG_RECORD_HPP_INCLUDED
