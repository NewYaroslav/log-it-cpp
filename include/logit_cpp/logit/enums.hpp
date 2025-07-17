#pragma once
#ifndef _LOGIT_ENUMS_HPP_INCLUDED
#define _LOGIT_ENUMS_HPP_INCLUDED
/// \file Enums.hpp
/// \brief Enumerations and utility functions for logging levels and text colors.

#include <array>
#include <string>

namespace logit {

    /// \enum LogLevel
    /// \brief Logging levels.
    enum class LogLevel {
        LOG_LVL_TRACE,      ///< Trace level logging.
        LOG_LVL_DEBUG,      ///< Debug level logging.
        LOG_LVL_INFO,       ///< Information level logging.
        LOG_LVL_WARN,       ///< Warning level logging.
        LOG_LVL_ERROR,      ///< Error level logging.
        LOG_LVL_FATAL       ///< Fatal level logging.
    };

    /// \enum TextColor
    /// \brief Text colors for console output.
    enum class TextColor {
        Black,
        DarkRed,
        DarkGreen,
        DarkYellow,
        DarkBlue,
        DarkMagenta,
        DarkCyan,
        LightGray,
        DarkGray,
        Red,
        Green,
        Yellow,
        Blue,
        Magenta,
        Cyan,
        White,
    };

    /// \enum LoggerParam
    /// \brief Enumeration for different logger parameters that can be retrieved.
    enum class LoggerParam {
        LastFileName,          ///< The name of the last file written to.
        LastFilePath,          ///< The full path of the last file written to.
        LastLogTimestamp,      ///< The timestamp of the last log.
        TimeSinceLastLog       ///< The time elapsed since the last log in seconds.
    };

    /// \brief Convert LogLevel to a C-style string representation.
    /// \param level The log level.
    /// \param mode The output mode (0 for full name, 1 for abbreviation).
    /// \return C-style string representing the log level.
    inline const char* to_c_str(LogLevel level, int mode = 0) {
        static const std::array<const char*, 6> data_str_0 = {
            "TRACE",
            "DEBUG",
            "INFO",
            "WARN",
            "ERROR",
            "FATAL"
        };
        static const std::array<const char*, 6> data_str_1 = {
            "T",
            "D",
            "I",
            "W",
            "E",
            "F"
        };
        switch (mode) {
        case 0:
            return data_str_0[static_cast<size_t>(level)];
        case 1:
            return data_str_1[static_cast<size_t>(level)];
        default:
            break;
        };
        return data_str_0[static_cast<size_t>(level)];
    }

    /// \brief Convert LogLevel to a std::string representation.
    /// \param level The log level.
    /// \param mode The output mode (0 for full name, 1 for abbreviation).
    /// \return std::string representing the log level.
    inline std::string to_string(LogLevel level, int mode = 0) {
        return std::string(to_c_str(level, mode));
    }

    /// \brief Convert TextColor to a C-style string (ANSI escape codes).
    /// \param color The text color.
    /// \return C-style string representing the ANSI escape code for the color.
    inline const char* to_c_str(TextColor color) {
        static const std::array<const char*, 16> ansi_codes = {
            "\033[30m",   // Black
            "\033[31m",   // DarkRed
            "\033[32m",   // DarkGreen
            "\033[33m",   // DarkYellow
            "\033[34m",   // DarkBlue
            "\033[35m",   // DarkMagenta
            "\033[36m",   // DarkCyan
            "\033[37m",   // LightGray
            "\033[90m",   // DarkGray
            "\033[91m",   // Red
            "\033[92m",   // Green
            "\033[93m",   // Yellow
            "\033[94m",   // Blue
            "\033[95m",   // Magenta
            "\033[96m",   // Cyan
            "\033[97m"    // White
        };

        // Convert TextColor to a string with an ANSI code
        return ansi_codes[static_cast<int>(color)];
    }

    /// \brief Convert TextColor to a std::string (ANSI escape codes).
    /// \param color The text color.
    /// \return std::string representing the ANSI escape code for the color.
    inline std::string to_string(TextColor color) {
        return std::string(to_c_str(color));
    }

    /// \brief Get the ANSI color code associated with a log level.
    /// \param log_level The log level.
    /// \return ANSI escape code string representing the color for the log level.
    inline std::string get_log_level_color(LogLevel log_level) {
        switch (log_level) {
            case LogLevel::LOG_LVL_TRACE:
                return to_string(LOGIT_COLOR_TRACE);
            case LogLevel::LOG_LVL_DEBUG:
                return to_string(LOGIT_COLOR_DEBUG);
            case LogLevel::LOG_LVL_INFO:
                return to_string(LOGIT_COLOR_INFO);
            case LogLevel::LOG_LVL_WARN:
                return to_string(LOGIT_COLOR_WARN);
            case LogLevel::LOG_LVL_ERROR:
                return to_string(LOGIT_COLOR_ERROR);
            case LogLevel::LOG_LVL_FATAL:
                return to_string(LOGIT_COLOR_FATAL);
            default:
                break;
        }
        return to_string(LOGIT_COLOR_DEFAULT);
    }

}; // namespace logit

#endif // _LOGIT_ENUMS_HPP_INCLUDED
