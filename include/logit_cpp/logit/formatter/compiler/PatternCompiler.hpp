#pragma once
#ifndef _LOGIT_PATTERN_COMPILER_HPP_INCLUDED
#define _LOGIT_PATTERN_COMPILER_HPP_INCLUDED

/// \file PatternCompiler.hpp
/// \brief Header file for the pattern compiler used in log formatting.

#include <time_shield/time_conversions.hpp>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <cstdio>

namespace logit {

    /// \struct FormatInstruction
    /// \brief Structure to store log formatting instructions.
    struct FormatInstruction {

        /// \enum CompileContext
        /// \brief Compilation context for handling special cases, such as when arguments are missing.
        enum class CompileContext {
            Default,            ///< Standard behavior without modifications.
            NoArgsFallback      ///< Handle a special pattern for cases with no arguments.
        };

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
            SourceFileAndLine,      ///< %\@: Source file and line number
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

        CompileContext context;     ///< Compilation context, e.g., default or handling no-argument cases.
        FormatType type;            ///< The type of the format instruction.
        std::string static_text;    ///< Used only if type == StaticText

        // Fields for alignment and width
        int width           = 0;        ///< Width for formatting.
        bool left_align     = false;    ///< Left alignment flag.
        bool center_align   = false;    ///< Center alignment flag.
        bool truncate       = false;    ///< Truncation flag.
        bool strip_ansi     = false;    ///< Removes ANSI escape codes (e.g., colors) if true.

        /// \brief Constructor for static text.
        /// \param context Compilation context for handling special cases.
        /// \param text The static text.
        /// \param strip_ansi If true, removes ANSI escape codes (e.g., colors).
        explicit FormatInstruction(
                CompileContext context,
                const std::string& text,
                bool strip_ansi)
            : context(context), type(FormatType::StaticText), static_text(text), strip_ansi(strip_ansi) {
        };

        /// \brief Constructor for other types.
        /// \param context Compilation context for handling special cases.
        /// \param type The format type.
        /// \param width The width for formatting.
        /// \param left Left alignment flag.
        /// \param center Center alignment flag.
        /// \param trunc Truncation flag.
        /// \param strip_ansi If true, removes ANSI escape codes (e.g., colors).
        explicit FormatInstruction(
                CompileContext context,
                FormatType type,
                int width = 0,
                bool left = false,
                bool center = false,
                bool trunc = false,
                bool strip_ansi = false) :
            context(context), type(type), width(width),
            left_align(left), center_align(center),
            truncate(trunc), strip_ansi(strip_ansi) {
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

            if (context == CompileContext::NoArgsFallback && (
                !record.format.empty() ||
                !record.args_array.empty())) return;

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
                    if (!strip_ansi) {
                        oss << get_log_level_color(record.log_level);
                    }
                    break;
                case FormatType::EndColor:
                    if (!strip_ansi) {
                        oss << to_string(LOGIT_DEFAULT_COLOR);
                    }
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
                            if (!record.print_mode && i) temp_stream << ", ";
                            const auto& arg = record.args_array[i];
                            switch (arg.type) {
                            case ValueType::STRING_VAL:
                            case ValueType::EXCEPTION_VAL:
                            case ValueType::ERROR_CODE_VAL:
                            case ValueType::ENUM_VAL:
                            case ValueType::DURATION_VAL:
                            case ValueType::TIME_POINT_VAL:
                            case ValueType::POINTER_VAL:
                            case ValueType::SMART_POINTER_VAL:
                            case ValueType::VARIANT_VAL:
                            case ValueType::OPTIONAL_VAL:
#ifdef LOGIT_WITH_FMT
                                temp_stream << (record.fmt_mode ? arg.to_string_fmt(record.format.c_str()) : arg.to_string(record.format.c_str()));
#else
                                temp_stream << arg.to_string(record.format.c_str());
#endif
                                break;
                            default:
#ifdef LOGIT_WITH_FMT
                                if (arg.is_literal) {
                                    temp_stream << arg.name << ": " << (record.fmt_mode ? arg.to_string_fmt(record.format.c_str()) : arg.to_string(record.format.c_str()));
                                } else {
                                    temp_stream << (record.fmt_mode ? arg.to_string_fmt(record.format.c_str()) : arg.to_string(record.format.c_str()));
                                }
#else
                                if (arg.is_literal) {
                                    temp_stream << arg.name << ": " << arg.to_string(record.format.c_str());
                                } else {
                                    temp_stream << arg.to_string(record.format.c_str());
                                }
#endif
                                break;
                            };
                        }
                    } else
                    if (!record.args_array.empty()) {
                        using ValueType = VariableValue::ValueType;
                        for (size_t i = 0; i < record.args_array.size(); ++i) {
                            if (!record.print_mode && i) temp_stream << ", ";
                            const auto& arg = record.args_array[i];
                            switch (arg.type) {
                            case ValueType::STRING_VAL:
                            case ValueType::EXCEPTION_VAL:
                            case ValueType::ERROR_CODE_VAL:
                                temp_stream << arg.to_string();
                                break;
                            case ValueType::ENUM_VAL:
                                if (arg.is_literal) {
                                    if (record.print_mode) temp_stream << arg.to_string();
                                    else temp_stream << arg.name << ": " << arg.to_string();
                                    break;
                                }
                                temp_stream << arg.to_string();
                                break;
                            case ValueType::PATH_VAL:
                            case ValueType::DURATION_VAL:
                            case ValueType::TIME_POINT_VAL:
                            case ValueType::POINTER_VAL:
                            case ValueType::SMART_POINTER_VAL:
                            case ValueType::VARIANT_VAL:
                            case ValueType::OPTIONAL_VAL:
                                if (record.print_mode) temp_stream << arg.to_string();
                                else temp_stream << arg.name << ": " << arg.to_string();
                                break;
                            default:
                                if (arg.is_literal) {
                                    if (record.print_mode) temp_stream << arg.to_string();
                                    else temp_stream << arg.name << ": " << arg.to_string();
                                    break;
                                }
                                temp_stream << arg.to_string();
                                break;
                            };
                        }
                    }
                    break;
            };

            // Get the string representation
            std::string result = strip_ansi ? remove_ansi_escape_codes(temp_stream.str()) : temp_stream.str();

            // Truncate if required
            if (truncate && result.size() > static_cast<size_t>(width)) {
                switch (type) {
                // File and Function
                case FormatType::FileName:
                case FormatType::FullFileName:
                case FormatType::SourceFileAndLine:
                case FormatType::FunctionName: {
                    const std::string placeholder = "..."; // Placeholder for omitted sections
                    int placeholder_size = static_cast<int>(placeholder.size());

                    // If the width is less than or equal to the placeholder size, return only the placeholder
                    if (width <= placeholder_size) {
                        result = placeholder.substr(0, width);
                    } else {
                        // Keep portions of the string from the beginning and end
                        size_t keep_size = (width - placeholder_size) / 2; // Portion to keep from each side
                        size_t keep_end = result.size() - keep_size;
                        int line_size = static_cast<int>(2 * keep_size + placeholder.size());

                        while (line_size < width) {
                            if (keep_end > 0) {
                                --keep_end;
                                ++line_size;
                            } else break;
                        }

                         // Construct the result: start + placeholder + end
                        result = result.substr(0, keep_size) + placeholder + result.substr(keep_end);
                    }
                    break;
                }
                default:
                    // Standard truncation for other types
                    result = result.substr(0, width);
                };
            }

            // Apply alignment and width
            if (width > 0 && result.size() < static_cast<size_t>(width)) {
                if (left_align) {
                    // Left alignment
                    oss << std::left << std::setw(width) << result;
                } else 
                if (center_align) {
                    // Center alignment
                    const int padding = static_cast<int>((width - result.size()) / 2);
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

    private:

        /// \brief Removes ANSI escape codes (including color codes and cursor movement) from a string.
        /// \param input The input string containing possible ANSI escape codes.
        /// \return A string with all ANSI escape codes removed.
        std::string remove_ansi_escape_codes(const std::string& input) const {
            std::string result;
            result.reserve(input.size());
            bool in_escape_sequence = false;

            for (size_t i = 0; i < input.size(); ++i) {
                if (in_escape_sequence) {
                    if ((input[i] >= 'a' && input[i] <= 'z') || (input[i] >= 'A' && input[i] <= 'Z')) {
                        in_escape_sequence = false;
                    }
                } else {
                    if (input[i] == '\033' && (i + 1) < input.size() && input[i + 1] == '[') {
                        in_escape_sequence = true;
                        ++i;  // Skip '[' after '\033'
                    } else {
                        result += input[i];  // Append non-escape characters to the result
                    }
                }
            }

            return result;
        }
    }; // FormatInstruction

    /// \class PatternCompiler
    /// \brief Compiler for log formatting patterns.
    class PatternCompiler {
    public:
        using CompileContext = FormatInstruction::CompileContext;

        /// \brief Compiles a pattern string into a list of format instructions.
        /// \param pattern The pattern string to compile.
        /// \param context Compilation context for handling special cases.
        /// \return A vector of format instructions.
        static std::vector<FormatInstruction> compile(
                const std::string& pattern,
                CompileContext context = CompileContext::Default) {
            using FormatType = FormatInstruction::FormatType;
            std::vector<FormatInstruction> instructions;
            std::string buffer;
            buffer.reserve(pattern.size());
            bool strip_ansi = false;

            for (size_t i = 0; i < pattern.size(); ++i) {
                char c = pattern[i];

                if (c == '%') {
                    if (!buffer.empty()) {
                        instructions.push_back(FormatInstruction(context, buffer, strip_ansi));
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
                                instructions.emplace_back(context, FormatType::Year, width, left_align, center_align, truncate, strip_ansi);
                                break;
                            case 'm':
                                if ((i + 1) < pattern.size() && pattern[i + 1] == 's') {
                                    instructions.emplace_back(context, FormatType::MilliSecondTimeStamp, width, left_align, center_align, truncate, strip_ansi);
                                    ++i;  // Skip 's' after 'm'
                                    break;
                                }
                                instructions.emplace_back(context, FormatType::Month, width, left_align, center_align, truncate, strip_ansi);
                                break;
                            case 'd':
                                instructions.emplace_back(context, FormatType::Day, width, left_align, center_align, truncate, strip_ansi);
                                break;
                            case 'H':
                                instructions.emplace_back(context, FormatType::Hour, width, left_align, center_align, truncate, strip_ansi);
                                break;
                            case 'M':
                                instructions.emplace_back(context, FormatType::Minute, width, left_align, center_align, truncate, strip_ansi);
                                break;
                            case 'S':
                                if ((i + 1) < pattern.size() && pattern[i + 1] == 'C') {
                                    strip_ansi = true;
                                    ++i;  // Skip 'C' after 'S'
                                    break;
                                }
                                instructions.emplace_back(context, FormatType::Second, width, left_align, center_align, truncate, strip_ansi);
                                break;
                            case 'e':
                                if ((i + 1) < pattern.size() && pattern[i + 1] == 'c') {
                                    strip_ansi = false;
                                    ++i;  // Skip 'c' after 'e'
                                    break;
                                }
                                instructions.emplace_back(context, FormatType::Millisecond, width, left_align, center_align, truncate, strip_ansi);
                                break;
                            case 'C':
                                instructions.emplace_back(context, FormatType::TwoDigitYear, width, left_align, center_align, truncate, strip_ansi);
                                break;
                            case 'c':
                                instructions.emplace_back(context, FormatType::DateTime, width, left_align, center_align, truncate, strip_ansi);
                                break;
                            case 'D':
                                instructions.emplace_back(context, FormatType::ShortDate, width, left_align, center_align, truncate, strip_ansi);
                                break;
                            case 'T':
                            case 'X':
                                instructions.emplace_back(context, FormatType::TimeISO8601, width, left_align, center_align, truncate, strip_ansi);
                                break;
                            case 'F':
                                instructions.emplace_back(context, FormatType::DateISO8601, width, left_align, center_align, truncate, strip_ansi);
                                break;
                            case 's':
                                if ((i + 1) < pattern.size() && pattern[i + 1] == 'c') {
                                    strip_ansi = true;
                                    ++i;  // Skip 'c' after 's'
                                    break;
                                } else {
                                    instructions.emplace_back(context, FormatType::TimeStamp, width, left_align, center_align, truncate, strip_ansi);
                                }
                                break;
                            case 'E':
                                if ((i + 1) < pattern.size() && pattern[i + 1] == 'C') {
                                    strip_ansi = false;
                                    ++i;  // Skip 'C' after 'E'
                                    break;
                                }
                                instructions.emplace_back(context, FormatType::TimeStamp, width, left_align, center_align, truncate, strip_ansi);
                                break;
                            // Weekday and Month Names
                            case 'b':
                                if ((i + 1) < pattern.size() && pattern[i + 1] == 's') {
                                    instructions.emplace_back(context, FormatType::FileName, width, left_align, center_align, truncate, strip_ansi);
                                    ++i;  // Skip 's' after 'b'
                                    break;
                                }
                                instructions.emplace_back(context, FormatType::AbbreviatedMonthName, width, left_align, center_align, truncate, strip_ansi);
                                break;
                            case 'B':
                                instructions.emplace_back(context, FormatType::FullMonthName, width, left_align, center_align, truncate, strip_ansi);
                                break;
                            case 'a':
                                instructions.emplace_back(context, FormatType::AbbreviatedWeekdayName, width, left_align, center_align, truncate, strip_ansi);
                                break;
                            case 'A':
                                instructions.emplace_back(context, FormatType::FullWeekdayName, width, left_align, center_align, truncate, strip_ansi);
                                break;

                            // Log Level
                            case 'l':
                                instructions.emplace_back(context, FormatType::LogLevel, width, left_align, center_align, truncate, strip_ansi);
                                break;
                            case 'L':
                                instructions.emplace_back(context, FormatType::ShortLogLevel, width, left_align, center_align, truncate, strip_ansi);
                                break;

                            // Thread
                            case 't':
                                instructions.emplace_back(context, FormatType::ThreadId, width, left_align, center_align, truncate, strip_ansi);
                                break;

                            // File and Function
                            case 'f':
                                if ((i + 1) < pattern.size() && pattern[i + 1] == 'f' && (i + 2) < pattern.size() && pattern[i + 2] == 'n') {
                                    instructions.emplace_back(context, FormatType::FullFileName, width, left_align, center_align, truncate, strip_ansi);
                                    i += 2; // Skip 'fn' after 'f'
                                    break;
                                }
                                if ((i + 1) < pattern.size() && pattern[i + 1] == 'n') {
                                    instructions.emplace_back(context, FormatType::FileName, width, left_align, center_align, truncate, strip_ansi);
                                    ++i;  // Skip 'n' after 'f'
                                    break;
                                }
                                instructions.emplace_back(context, FormatType::FileName, width, left_align, center_align, truncate, strip_ansi);
                                break;
                            case 'g':
                                instructions.emplace_back(context, FormatType::FullFileName, width, left_align, center_align, truncate, strip_ansi);
                                break;
                            case '@':
                                instructions.emplace_back(context, FormatType::SourceFileAndLine, width, left_align, center_align, truncate, strip_ansi);
                                break;
                            case '#':
                                instructions.emplace_back(context, FormatType::LineNumber, width, left_align, center_align, truncate, strip_ansi);
                                break;
                            case '!':
                                instructions.emplace_back(context, FormatType::FunctionName, width, left_align, center_align, truncate, strip_ansi);
                                break;

                            // Color
                            case '^':
                                instructions.emplace_back(context, FormatType::StartColor, 0, false, false, false, strip_ansi);
                                break;
                            case '$':
                                instructions.emplace_back(context, FormatType::EndColor, 0, false, false, false, strip_ansi);
                                break;

                            // Message
                            case 'v':
                                instructions.emplace_back(context, FormatType::Message, width, left_align, center_align, truncate, strip_ansi);
                                break;

                            //
                            case 'N':
                                if ((i + 2) < pattern.size() && pattern[i + 1] == '(') {
                                    size_t end = pattern.find(')', i + 2);
                                    if (end == std::string::npos) {
                                        ++i;
                                        break;
                                    }
                                    std::string params = pattern.substr(i + 2, end - i - 2);
                                    auto no_args_instructions = compile(params, CompileContext::NoArgsFallback);
                                    instructions.insert(instructions.end(), no_args_instructions.begin(), no_args_instructions.end());
                                    i = end;
                                }
                                break;

                            // Escape character or unknown
                            case '%':
                            default:
                                buffer += next;  // Unrecognized symbols are recorded as text
                                break;
                        };
                    }
                } else {
                    buffer += c;
                }
            }

            if (!buffer.empty()) {
                instructions.push_back(FormatInstruction(context, buffer, strip_ansi));
            }
            return instructions;
        }
    }; // PatternCompiler

}; // namespace logit

#endif // _LOGIT_PATTERN_COMPILER_HPP_INCLUDED
