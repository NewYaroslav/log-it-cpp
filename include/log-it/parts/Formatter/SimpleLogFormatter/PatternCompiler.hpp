#pragma once
#ifndef _LOGIT_PATTERN_COMPILER_HPP_INCLUDED
#define _LOGIT_PATTERN_COMPILER_HPP_INCLUDED
/// \file PatternCompiler.hpp
/// \brief Header file for the pattern compiler used in log formatting.

#include <time_shield_cpp/time_shield.hpp>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>

namespace logit {

    /// \struct FormatInstruction
    /// \brief Structure to store log formatting instructions.
    struct FormatInstruction {

        /// \enum FormatType
        /// \brief Possible types of instructions for log formatting.
        enum class FormatType {
            // Text
            StaticText,             ///< Static text

            // Date and Time
            Year,                   ///< %Y: Year
            Month,                  ///< %m: Month
            Day,                    ///< %d: Day
            Hour,                   ///< %H: Hour
            Minute,                 ///< %M: Minute
            Second,                 ///< %S: Second
            Millisecond,            ///< %e: Millisecond
            TwoDigitYear,           ///< %C: Two-digit year
            DateTime,               ///< %c: Date and time
            ShortDate,              ///< %D: Short date (%m/%d/%y)
            TimeISO8601,            ///< %T or %X: Time in ISO 8601 format (%H:%M:%S)
            DateISO8601,            ///< %F: Date in ISO 8601 format (%Y-%m-%d)
            TimeStamp,              ///< %s or %E: Timestamp in seconds
            MilliSecondTimeStamp,   ///< %ms: Timestamp in milliseconds

            // Weekday and Month Names
            AbbreviatedMonthName,   ///< %b: Abbreviated month name
            FullMonthName,          ///< %B: Full month name
            AbbreviatedWeekdayName, ///< %a: Abbreviated weekday name
            FullWeekdayName,        ///< %A: Full weekday name

            // Log Level
            LogLevel,               ///< %l: Log level
            ShortLogLevel,          ///< %L: Short log level

            // File and Function
            FileName,               ///< %f, %fn, %bs: Basename of the source file
            FullFileName,           ///< %g, %ffn: Full file path
            SourceFileAndLine,      ///< %@: Source file and line number
            LineNumber,             ///< %#: Line number
            FunctionName,           ///< %!: Function name

            // Thread
            ThreadId,               ///< %t: Thread identifier

            // Color
            StartColor,             ///< %^: Start of color range
            EndColor,               ///< %$: End of color range

            // Message
            Message                 ///< %v: Message
        };

        FormatType type; ///< The type of the format instruction.
        std::string static_text; ///< Used only if type == StaticText

        // Fields for alignment and width
        int width = 0; ///< Width for formatting.
        bool left_align = false; ///< Left alignment flag.
        bool center_align = false; ///< Center alignment flag.
        bool truncate = false; ///< Truncation flag.

        /// \brief Constructor for static text.
        /// \param text The static text.
        explicit FormatInstruction(const std::string& text)
            : type(FormatType::StaticText), static_text(text) {
        };

        /// \brief Constructor for other types.
        /// \param type The format type.
        /// \param width The width for formatting.
        /// \param left Left alignment flag.
        /// \param center Center alignment flag.
        /// \param trunc Truncation flag.
        explicit FormatInstruction(
                const FormatType type,
                const int width = 0,
                const bool left = false,
                const bool center = false,
                const bool trunc = false)
            : type(type), width(width), left_align(left),
            center_align(center), truncate(trunc) {
        };

        /// \brief Apply formatting considering alignment and width.
        /// \tparam StreamType The type of the output stream.
        /// \param oss The output stream.
        /// \param record The log record.
        /// \param dt The date and time structure.
        template<class StreamType>
        void apply(
                StreamType& oss,
                const LogRecord& record,
                const time_shield::DateTimeStruct& dt) const {
            std::ostringstream temp_stream;
            switch (type) {
                // Text
                case FormatType::StaticText:
                    temp_stream << static_text;
                    break;

                // Date and Time
                case FormatType::Year:
                    temp_stream << dt.year;
                    break;
                case FormatType::Month:
                    temp_stream << std::setw(2) << std::setfill('0') << dt.mon;
                    break;
                case FormatType::Day:
                    temp_stream << std::setw(2) << std::setfill('0') << dt.day;
                    break;
                case FormatType::Hour:
                    temp_stream << std::setw(2) << std::setfill('0') << dt.hour;
                    break;
                case FormatType::Minute:
                    temp_stream << std::setw(2) << std::setfill('0') << dt.min;
                    break;
                case FormatType::Second:
                    temp_stream << std::setw(2) << std::setfill('0') << dt.sec;
                    break;
                case FormatType::Millisecond:
                    temp_stream << std::setw(3) << std::setfill('0') << dt.ms;
                    break;
                case FormatType::TwoDigitYear:
                    temp_stream << std::setw(2) << std::setfill('0') << (dt.year % 100);
                    break;
                case FormatType::DateTime: {
                    // Format equivalent to 'ctime'
                    char buffer[16];
                    temp_stream << time_shield::to_str(time_shield::day_of_week(dt.year, dt.mon, dt.day), time_shield::FormatType::SHORT_NAME);
                    temp_stream << " ";
                    temp_stream << time_shield::to_str(static_cast<time_shield::Month>(dt.mon), time_shield::FormatType::SHORT_NAME);
                    temp_stream << " ";
                    // day
                    snprintf(buffer, sizeof(buffer), "%2d ", dt.day);
                    temp_stream << buffer;
                    // time
                    snprintf(buffer, sizeof(buffer), "%.2d:%.2d:%.2d ", dt.hour, dt.min, dt.sec);
                    temp_stream << buffer;
                    // year
                    temp_stream << dt.year;
                    break;
                }
                case FormatType::ShortDate:
                    temp_stream << std::setw(2) << std::setfill('0') << dt.mon << "/"
                        << std::setw(2) << std::setfill('0') << dt.day << "/"
                        << std::setw(2) << std::setfill('0') << (dt.year % 100);
                    break;
                case FormatType::TimeISO8601:
                    temp_stream
                        << std::setw(2) << std::setfill('0') << dt.hour << ":"
                        << std::setw(2) << std::setfill('0') << dt.min << ":"
                        << std::setw(2) << std::setfill('0') << dt.sec;
                    break;
                case FormatType::DateISO8601:
                    temp_stream
                        << dt.year << "-"
                        << std::setw(2) << std::setfill('0') << dt.mon << "-"
                        << std::setw(2) << std::setfill('0') << dt.day;
                    break;
                case FormatType::TimeStamp:
                    temp_stream << time_shield::ms_to_sec(record.timestamp_ms);
                    break;
                case FormatType::MilliSecondTimeStamp:
                    temp_stream << record.timestamp_ms;
                    break;

                // Weekday and Month Names
                case FormatType::AbbreviatedMonthName:
                    temp_stream << time_shield::to_str(static_cast<time_shield::Month>(dt.mon), time_shield::FormatType::SHORT_NAME);
                    break;
                case FormatType::FullMonthName:
                    temp_stream << time_shield::to_str(static_cast<time_shield::Month>(dt.mon), time_shield::FormatType::FULL_NAME);
                    break;
                case FormatType::AbbreviatedWeekdayName:
                    temp_stream << time_shield::to_str(time_shield::day_of_week(dt.year, dt.mon, dt.day), time_shield::FormatType::SHORT_NAME);
                    break;
                case FormatType::FullWeekdayName:
                    temp_stream << time_shield::to_str(time_shield::day_of_week(dt.year, dt.mon, dt.day), time_shield::FormatType::FULL_NAME);
                    break;

                // Log Level
                case FormatType::LogLevel:
                    temp_stream << to_string(record.log_level);
                    break;
                case FormatType::ShortLogLevel:
                    temp_stream << to_string(record.log_level, 1);
                    break;

                // File and Function
                case FormatType::FileName: {
                    std::string full_path = record.file;
                    size_t pos = full_path.find_last_of("/\\");
                    if (pos != std::string::npos) {
                        temp_stream << full_path.substr(pos + 1);
                    } else {
                        temp_stream << full_path;
                    }
                    break;
                }
                case FormatType::FullFileName:
                    temp_stream << record.file;
                    break;
                case FormatType::SourceFileAndLine:
                    temp_stream << record.file << ":" << record.line;
                    break;
                case FormatType::LineNumber:
                    temp_stream << record.line;
                    break;
                case FormatType::FunctionName:
                    temp_stream << record.function;
                    break;

                // Thread
                case FormatType::ThreadId:
                    temp_stream << record.thread_id;
                    break;

                // Color
                case FormatType::StartColor:
                    oss << get_log_level_color(record.log_level);
                    break;
                case FormatType::EndColor:
                    oss << to_string(LOGIT_DEFAULT_COLOR);
                    break;

                // Message
                case FormatType::Message:
                    if (!record.format.empty()) {
                        if (record.args_array.empty()) {
                            temp_stream << record.format;
                            break;
                        }
                        using ValueType = VariableValue::ValueType;
                        for (size_t i = 0; i < record.args_array.size(); ++i) {
                            if (i) temp_stream << ", ";
                            const auto& arg = record.args_array[i];
                            switch (arg.type) {
                            case ValueType::STRING_VAL:
                            case ValueType::EXCEPTION_VAL:
                                temp_stream << arg.to_string(record.format.c_str());
                                break;
                            default:
                                if (arg.is_literal) {
                                    temp_stream << arg.name << ": " << arg.to_string(record.format.c_str());
                                } else {
                                    temp_stream << arg.to_string(record.format.c_str());
                                }
                                break;
                            };
                        }
                    } else if (!record.args_array.empty()) {
                        using ValueType = VariableValue::ValueType;
                        for (size_t i = 0; i < record.args_array.size(); ++i) {
                            if (i) temp_stream << ", ";
                            const auto& arg = record.args_array[i];
                            switch (arg.type) {
                            case ValueType::STRING_VAL:
                            case ValueType::EXCEPTION_VAL:
                                temp_stream << arg.to_string();
                                break;
                            default:
                                if (arg.is_literal) {
                                    temp_stream << arg.name << ": " << arg.to_string();
                                } else {
                                    temp_stream << arg.to_string();
                                }
                                break;
                            };
                        }
                    }
                    break;
            };

            // Get the string representation
            std::string result = temp_stream.str();

            // Truncate if required
            if (truncate && result.size() > static_cast<size_t>(width)) {
                result = result.substr(0, width);
            }

            // Apply alignment and width
            if (width > 0 && result.size() < static_cast<size_t>(width)) {
                if (left_align) {
                    // Left alignment
                    oss << std::left << std::setw(width) << result;
                } else if (center_align) {
                    // Center alignment
                    const int padding = (width - result.size()) / 2;
                    oss << std::string(padding, ' ') << result << std::string(width - padding - result.size(), ' ');
                } else {
                    // Right alignment (default)
                    oss << std::right << std::setw(width) << result;
                }
            } else {
                // If width not specified, simply append the result
                oss << result;
            }
        }
    };

    /// \class PatternCompiler
    /// \brief Compiler for log formatting patterns.
    class PatternCompiler {
    public:

        /// \brief Compiles a pattern string into a list of format instructions.
        /// \param pattern The pattern string to compile.
        /// \return A vector of format instructions.
        static std::vector<FormatInstruction> compile(const std::string& pattern) {
            using FormatType = FormatInstruction::FormatType;
            std::vector<FormatInstruction> instructions;
            std::string buffer;

            for (size_t i = 0; i < pattern.size(); ++i) {
                char c = pattern[i];

                if (c == '%') {
                    if (!buffer.empty()) {
                        instructions.push_back(FormatInstruction(buffer));
                        buffer.clear();
                    }

                    // Handling alignment, width, and truncation
                    bool left_align = false;
                    bool center_align = false;
                    bool truncate = false;
                    int width = 0;

                    // Check for alignment and width
                    while ((i + 1) < pattern.size() && (
                            std::isdigit(pattern[i + 1]) ||
                            pattern[i + 1] == '-' ||
                            pattern[i + 1] == '=')) {
                        char next = pattern[++i];
                        if (next == '-') {
                            left_align = true;
                        } else if (next == '=') {
                            center_align = true;
                        } else if (std::isdigit(next)) {
                            width = width * 10 + (next - '0');
                        }

                        // Check for truncation '!'
                        if ((i + 1) < pattern.size() && pattern[i + 1] == '!') {
                            truncate = true;
                            ++i;
                            break;
                        }
                    }

                    if ((i + 1) < pattern.size()) {
                        char next = pattern[++i];
                        switch (next) {
                            // Date and Time
                            case 'Y':
                                instructions.emplace_back(FormatType::Year, width, left_align, center_align, truncate);
                                break;
                            case 'm':
                                if ((i + 1) < pattern.size() && pattern[i + 1] == 's') {
                                    instructions.emplace_back(FormatType::MilliSecondTimeStamp, width, left_align, center_align, truncate);
                                    ++i;  // Skip 's' after 'm'
                                    break;
                                }
                                instructions.emplace_back(FormatType::Month, width, left_align, center_align, truncate);
                                break;
                            case 'd':
                                instructions.emplace_back(FormatType::Day, width, left_align, center_align, truncate);
                                break;
                            case 'H':
                                instructions.emplace_back(FormatType::Hour, width, left_align, center_align, truncate);
                                break;
                            case 'M':
                                instructions.emplace_back(FormatType::Minute, width, left_align, center_align, truncate);
                                break;
                            case 'S':
                                instructions.emplace_back(FormatType::Second, width, left_align, center_align, truncate);
                                break;
                            case 'e':
                                instructions.emplace_back(FormatType::Millisecond, width, left_align, center_align, truncate);
                                break;
                            case 'C':
                                instructions.emplace_back(FormatType::TwoDigitYear, width, left_align, center_align, truncate);
                                break;
                            case 'c':
                                instructions.emplace_back(FormatType::DateTime, width, left_align, center_align, truncate);
                                break;
                            case 'D':
                                instructions.emplace_back(FormatType::ShortDate, width, left_align, center_align, truncate);
                                break;
                            case 'T':
                            case 'X':
                                instructions.emplace_back(FormatType::TimeISO8601, width, left_align, center_align, truncate);
                                break;
                            case 'F':
                                instructions.emplace_back(FormatType::DateISO8601, width, left_align, center_align, truncate);
                                break;
                            case 's':
                            case 'E':
                                instructions.emplace_back(FormatType::TimeStamp, width, left_align, center_align, truncate);
                                break;

                            // Weekday and Month Names
                            case 'b':
                                if ((i + 1) < pattern.size() && pattern[i + 1] == 's') {
                                    instructions.emplace_back(FormatType::FileName, width, left_align, center_align, truncate);
                                    ++i;  // Skip 's' after 'b'
                                    break;
                                }
                                instructions.emplace_back(FormatType::AbbreviatedMonthName, width, left_align, center_align, truncate);
                                break;
                            case 'B':
                                instructions.emplace_back(FormatType::FullMonthName, width, left_align, center_align, truncate);
                                break;
                            case 'a':
                                instructions.emplace_back(FormatType::AbbreviatedWeekdayName, width, left_align, center_align, truncate);
                                break;
                            case 'A':
                                instructions.emplace_back(FormatType::FullWeekdayName, width, left_align, center_align, truncate);
                                break;

                            // Log Level
                            case 'l':
                                instructions.emplace_back(FormatType::LogLevel, width, left_align, center_align, truncate);
                                break;
                            case 'L':
                                instructions.emplace_back(FormatType::ShortLogLevel, width, left_align, center_align, truncate);
                                break;

                            // Thread
                            case 't':
                                instructions.emplace_back(FormatType::ThreadId, width, left_align, center_align, truncate);
                                break;

                            // File and Function
                            case 'f':
                                if ((i + 1) < pattern.size() && pattern[i + 1] == 'f' && (i + 2) < pattern.size() && pattern[i + 2] == 'n') {
                                    instructions.emplace_back(FormatType::FullFileName, width, left_align, center_align, truncate);
                                    i += 2; // Skip 'fn' after 'f'
                                    break;
                                }
                                if ((i + 1) < pattern.size() && pattern[i + 1] == 'n') {
                                    instructions.emplace_back(FormatType::FileName, width, left_align, center_align, truncate);
                                    ++i;  // Skip 'n' after 'f'
                                    break;
                                }
                                instructions.emplace_back(FormatType::FileName, width, left_align, center_align, truncate);
                                break;
                            case 'g':
                                instructions.emplace_back(FormatType::FullFileName, width, left_align, center_align, truncate);
                                break;
                            case '@':
                                instructions.emplace_back(FormatType::SourceFileAndLine, width, left_align, center_align, truncate);
                                break;
                            case '#':
                                instructions.emplace_back(FormatType::LineNumber, width, left_align, center_align, truncate);
                                break;
                            case '!':
                                instructions.emplace_back(FormatType::FunctionName, width, left_align, center_align, truncate);
                                break;

                            // Color
                            case '^':
                                instructions.emplace_back(FormatType::StartColor);
                                break;
                            case '$':
                                instructions.emplace_back(FormatType::EndColor);
                                break;

                            // Message
                            case 'v':
                                instructions.emplace_back(FormatType::Message, width, left_align, center_align, truncate);
                                break;

                            // Escape character or unknown
                            case '%':
                            default:
                                buffer += next;  // Unrecognized symbols are recorded as text
                                break;
                        }
                    }
                } else {
                    buffer += c;
                }
            }

            if (!buffer.empty()) {
                instructions.push_back(FormatInstruction(buffer));
            }

            return instructions;
        }
    };

}; // namespace logit

#endif // _LOGIT_PATTERN_COMPILER_HPP_INCLUDED
