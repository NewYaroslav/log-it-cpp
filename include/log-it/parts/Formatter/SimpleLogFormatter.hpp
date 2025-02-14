#pragma once
#ifndef _LOGIT_SIMPLE_LOG_FORMATTER_HPP_INCLUDED
#define _LOGIT_SIMPLE_LOG_FORMATTER_HPP_INCLUDED
/// \file SimpleLogFormatter.hpp
/// \brief Defines the SimpleLogFormatter class for formatting log messages according to a specified pattern or JSON format.

#include "ILogFormatter.hpp"
#include "SimpleLogFormatter/PatternCompiler.hpp"

namespace logit {

    /// \class SimpleLogFormatter
    /// \brief A simple log formatter that formats log messages based on a user-defined pattern.
    ///
    /// The `SimpleLogFormatter` class allows users to define a pattern for formatting log messages.
    /// The pattern supports various placeholders for timestamps, log levels, and other log-related information.
    class SimpleLogFormatter : public ILogFormatter {
    public:
        /// \struct Config
        /// \brief Configuration for the log formatter.
        struct Config {
            std::string pattern = "[%Y-%m-%d %H:%M:%S.%e] [%ffn:%#] [%!] [thread:%t] [%l] %^%v%$"; ///< Default log format string.
            bool json_format = false; ///< Flag to enable JSON formatting.
        };

        /// \brief Default constructor that uses the default log pattern.
        SimpleLogFormatter() {
            compile_pattern();
        }

        /// \brief Constructor that accepts a custom log pattern.
        /// \param pattern The custom format pattern for log messages.
        /// \param json_format Flag to enable JSON formatting.
        SimpleLogFormatter(const std::string& pattern, const bool json_format = false) {
            set_pattern(pattern, json_format);
        }

        /// \brief Sets a custom pattern for log formatting or switches to JSON formatting.
        ///
        /// This method allows changing the format of log messages at runtime. It updates the internal
        /// configuration and recompiles the pattern or switches to JSON formatting.
        ///
        /// \param pattern A string containing the custom format pattern.
        /// \param json_format Flag to enable JSON formatting.
        void set_pattern(const std::string& pattern, bool json_format = false) {
            m_config.pattern = pattern;
            m_config.json_format = json_format;
            compile_pattern();
        }

        /// \brief Sets the timestamp offset for log formatting.
        ///
        /// This function allows setting a timezone offset in milliseconds, which will be used
        /// for adjusting timestamps in formatted log messages.
        ///
        /// \param offset_ms Timezone offset in milliseconds.
        void set_timestamp_offset(int64_t offset_ms) override {
            m_offset_ms = offset_ms;
        }

        /// \brief Formats a log record according to the current pattern or as a JSON string.
        ///
        /// This method formats the log message either by applying the compiled pattern instructions or
        /// by creating a JSON string if the `json_format` flag is enabled.
        ///
        /// \param record The log record containing log information.
        /// \return A formatted string representing the log message.
        std::string format(const LogRecord& record) const override {
            if (m_config.json_format) {
                return format_as_json(record);
            } else {
                return format_as_pattern(record);
            }
        }

    private:
        Config m_config;                                        ///< Formatter configuration holding the log format pattern.
        std::vector<FormatInstruction> m_compiled_instructions; ///< Compiled instructions from the format pattern.
        std::atomic<int64_t> m_offset_ms = ATOMIC_VAR_INIT(0);  ///< Timestamp offset in milliseconds.

        /// \brief Compiles the log format pattern into instructions.
        ///
        /// This method compiles the format string into a series of instructions that are applied
        /// when formatting log messages.
        void compile_pattern() {
            m_compiled_instructions = PatternCompiler::compile(m_config.pattern);
        }

        /// \brief Formats a log record according to the compiled pattern.
        ///
        /// This method formats the log message by applying the compiled pattern instructions to the
        /// log record.
        ///
        /// \param record The log record containing log information.
        /// \return A formatted string representing the log message.
        std::string format_as_pattern(const LogRecord& record) const {
            auto dt = time_shield::to_date_time_ms<time_shield::DateTimeStruct>(record.timestamp_ms + m_offset_ms);
            std::ostringstream oss;
            for (const auto& instruction : m_compiled_instructions) {
                instruction.apply(oss, record, dt);
            }
            return oss.str();
        }

        /// \brief Formats a log record as a JSON string without external JSON libraries.
        ///
        /// This method manually constructs the JSON string using string streams and simple concatenation.
        ///
        /// \param record The log record containing log information.
        /// \return A JSON string representing the log message.
        std::string format_as_json(const LogRecord& record) const {
            std::ostringstream oss;
            oss << "{"
                << "\"log_level\": " << static_cast<int>(record.log_level) << ", "
                << "\"timestamp_ms\": " << record.timestamp_ms << ", "
                << "\"file\": \"" << escape_json_string(record.file) << "\", "
                << "\"line\": " << record.line << ", "
                << "\"function\": \"" << escape_json_string(record.function) << "\", "
                << "\"format\": \"" << escape_json_string(record.format) << "\", "
                << "\"arg_names\": \"" << escape_json_string(record.arg_names) << "\", "
                << "\"args_array\": [";

            for (size_t i = 0; i < record.args_array.size(); ++i) {
                if (i > 0) oss << ", ";
                oss << "\"" << escape_json_string(record.args_array[i].to_string()) << "\"";  // Assuming to_string()
            }

            oss << "], "
                << "\"thread_id\": \"" << escape_json_string(thread_id_to_string(record.thread_id)) << "\""
                << "}";

            return oss.str();
        }

        /// \brief Helper function to escape special characters in JSON strings.
        ///
        /// This function replaces special characters like quotes, backslashes, and control characters with their
        /// escaped versions for proper JSON formatting.
        ///
        /// \param input The input string that may contain special characters.
        /// \return A properly escaped JSON string.
        std::string escape_json_string(const std::string& input) const {
            std::ostringstream oss;
            for (char c : input) {
                switch (c) {
                    case '"': oss << "\\\""; break;
                    case '\\': oss << "\\\\"; break;
                    case '\b': oss << "\\b"; break;
                    case '\f': oss << "\\f"; break;
                    case '\n': oss << "\\n"; break;
                    case '\r': oss << "\\r"; break;
                    case '\t': oss << "\\t"; break;
                    default:
                        if ('\x00' <= c && c <= '\x1f') {
                            oss << "\\u"
                                << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
                        } else {
                            oss << c;
                        }
                }
            }
            return oss.str();
        }

        /// \brief Helper function to convert a thread ID to a string.
        ///
        /// Converts the thread ID to a string format that can be used in the JSON output.
        ///
        /// \param thread_id The thread ID to be converted.
        /// \return A string representation of the thread ID.
        std::string thread_id_to_string(const std::thread::id& thread_id) const {
            std::ostringstream oss;
            oss << thread_id;
            return oss.str();
        }
    }; // class SimpleLogFormatter

}; // namespace logit

#endif // _LOGIT_SIMPLE_LOG_FORMATTER_HPP_INCLUDED
