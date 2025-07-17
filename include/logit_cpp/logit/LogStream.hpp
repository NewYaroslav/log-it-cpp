#pragma once
#ifndef _LOGIT_LOG_STREAM_HPP_INCLUDED
#define _LOGIT_LOG_STREAM_HPP_INCLUDED

/// \file LogStream.hpp
/// \brief Defines the LogStream class for stream-like logging functionality.

#include <sstream>
#include "Logger.hpp"
#include "utils/path_utils.hpp"

namespace logit {

    /// \class LogStream
    /// \brief A stream-based logger that collects log messages using the `<<` operator.
    ///
    /// The `LogStream` class allows for logging messages in a stream-like manner. It collects log
    /// data through the `<<` operator and automatically sends the log message when the object is destroyed.
    class LogStream {
    public:

        /// \brief Constructor.
        /// \param level The log level.
        /// \param file Source file name.
        /// \param line Line number in the source file.
        /// \param function Function name.
        /// \param logger_index Logger index to use.
        LogStream(
            LogLevel level,
            const std::string& file,
            int line,
            const std::string& function,
            int logger_index)
            : m_level(level), m_file(file), m_line(line), m_function(function),
            m_logger_index(logger_index) {
        }

        /// \brief Destructor that logs the collected message when the object goes out of scope.
        ///
        /// When the `LogStream` object is destroyed, the log message is automatically sent using
        /// the `Logger` instance. The message is collected through the `<<` operator and stored in
        /// an internal string stream until the object is destructed.
        ~LogStream() {
            // Automatically log when the LogStream object is destroyed (end of line).
            Logger::get_instance().log_and_return(LogRecord{
                m_level, LOGIT_CURRENT_TIMESTAMP_MS(),
                m_file, m_line, m_function,
                m_stream.str(), std::string(),  // No argument names for stream-based logs.
                m_logger_index,
                false
            });
        }

        /// \brief Overloaded `<<` operator to accumulate log content.
        /// \tparam T The type of value to log.
        /// \param value The value to add to the log message.
        /// \return A reference to the `LogStream` for chaining.
        template <typename T>
        LogStream& operator<<(const T& value) {
            m_stream << value;
            return *this;
        }

        // Overload of operator<< for manipulators (e.g., std::endl)
        LogStream& operator<<(std::ostream& (*manip)(std::ostream&)) {
            m_stream << manip; // Apply the manipulator to the internal stream
            return *this;
        }

    private:
        LogLevel            m_level;        ///< Log level.
        std::ostringstream  m_stream;       ///< Stream for accumulating log content.
        std::string         m_file;         ///< Source file name.
        int                 m_line;         ///< Line number.
        std::string         m_function;     ///< Function name.
        int                 m_logger_index; ///< Logger index.
    };

} // namespace logit

#endif // _LOGIT_LOG_STREAM_HPP_INCLUDED
