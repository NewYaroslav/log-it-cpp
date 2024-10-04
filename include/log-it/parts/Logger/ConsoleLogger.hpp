#pragma once
#ifndef _LOGIT_CONSOLE_LOGGER_HPP_INCLUDED
#define _LOGIT_CONSOLE_LOGGER_HPP_INCLUDED
/// \file ConsoleLogger.hpp
/// \brief Console logger implementation that outputs logs to the console with color support.

#include "ILogger.hpp"
#include "../TaskExecutor.hpp"
#include "../LogMacros.hpp"
#include <iostream>
#if defined(__MINGW32__) || defined(_WIN32)
#include <windows.h>
#endif
#include <mutex>

namespace logit {

    /// \class ConsoleLogger
    /// \brief Logger that outputs log messages to the console with optional color coding.
    ///
    /// The `ConsoleLogger` provides synchronous or asynchronous logging to the console,
    /// with optional support for colored output based on ANSI codes. For Windows, it
    /// includes logic to handle ANSI color codes.
    class ConsoleLogger : public ILogger {
    public:

        /// \struct Config
        /// \brief Configuration for the console logger.
        struct Config {
            TextColor default_color = LOGIT_DEFAULT_COLOR; ///< Default text color for console output.
            bool async = true; ///< Flag indicating whether logging should be asynchronous.
        };

        /// \brief Default constructor that uses default configuration.
        ConsoleLogger() {
            reset_color();
        }

        /// \brief Constructor with custom configuration.
        /// \param config The configuration for the logger.
        ConsoleLogger(const Config& config) : m_config(config) {
            reset_color();
        }

        /// \brief Constructor with asynchronous flag.
        /// \param async Boolean flag for asynchronous logging.
        ConsoleLogger(const bool async) {
            m_config.async = async;
            reset_color();
        }

        virtual ~ConsoleLogger() = default;

        /// \brief Sets the logger configuration.
        /// This method sets the logger's configuration and ensures thread safety with a mutex lock.
        /// \param config The new configuration.
        void set_config(const Config& config) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_config = config;
        }

        /// \brief Gets the current logger configuration.
        /// Returns the logger's configuration with thread safety ensured.
        /// \return The current configuration.
        Config get_config() {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_config;
        }

        /// \brief Logs a message to the console with thread safety.
        ///
        /// If asynchronous logging is enabled, the message is added to the task queue;
        /// otherwise, it is logged directly.
        ///
        /// \param record The log record containing log information.
        /// \param message The formatted log message.
        void log(const LogRecord& record, const std::string& message) override {
            std::unique_lock<std::mutex> lock(m_mutex);
            if (!m_config.async) {
#               if defined(__MINGW32__) || defined(_WIN32)
                // For Windows, parse the message for ANSI color codes and apply them
                handle_ansi_colors_windows(message);
#               else
                // For other systems, output the message as is
                std::cout << message << std::endl;
#               endif
                return;
            }
            lock.unlock();
            TaskExecutor::get_instance().add_task([this, message](){
                std::lock_guard<std::mutex> lock(m_mutex);
#               if defined(__MINGW32__) || defined(_WIN32)
                // For Windows, parse the message for ANSI color codes and apply them
                handle_ansi_colors_windows(message);
#               else
                // For other systems, output the message as is
                std::cout << message << std::endl;
#               endif
            });
        }

        /// \brief Waits for all asynchronous tasks to complete.
        /// If asynchronous logging is enabled, waits for all pending log messages to be written.
        void wait() override {
            std::unique_lock<std::mutex> lock(m_mutex);
            if (!m_config.async) return;
            lock.unlock();
            TaskExecutor::get_instance().wait();
        }

    private:
        mutable std::mutex m_mutex;     ///< Mutex to protect console output
        Config             m_config;    ///< Configuration for the console logger.

#       if defined(__MINGW32__) || defined(_WIN32)

        // Windows console colors
        enum class WinColor {
            Black = 0,
            DarkBlue = 1,
            DarkGreen = 2,
            DarkCyan = 3,
            DarkRed = 4,
            DarkMagenta = 5,
            DarkYellow = 6,
            Gray = 7,
            DarkGray = 8,
            Blue = 9,
            Green = 10,
            Cyan = 11,
            Red = 12,
            Magenta = 13,
            Yellow = 14,
            White = 15,
        };

        /// \brief Handle ANSI color codes in the message for Windows console.
        /// \param message The message containing ANSI color codes.
        void handle_ansi_colors_windows(const std::string& message) const {
            std::string::size_type start = 0;
            std::string::size_type pos = 0;

            HANDLE handle_stdout = GetStdHandle(STD_OUTPUT_HANDLE);

            while ((pos = message.find("\033[", start)) != std::string::npos) {
                // Output the part of the string before the ANSI code
                if (pos > start) {
                    std::cout << message.substr(start, pos - start);
                }

                // Find the end of the ANSI code
                std::string::size_type end_pos = message.find('m', pos);
                if (end_pos != std::string::npos) {
                    // Extract the ANSI code
                    std::string ansi_code = message.substr(pos + 2, end_pos - pos - 2);
                    apply_color_from_ansi_code(ansi_code, handle_stdout);

                    // Update position
                    start = end_pos + 1;
                } else {
                    break;
                }
            }

            // Output any remaining part of the message
            if (start < message.size()) {
                std::cout << message.substr(start);
            }
            if (!message.empty()) std::cout << std::endl;

            // Reset the console color to default
            SetConsoleTextAttribute(handle_stdout, static_cast<WORD>(text_color_to_win_color(m_config.default_color)));
        }

        /// \brief Apply color based on ANSI code for Windows console.
        /// \param ansi_code The ANSI code string.
        /// \param handle_stdout The console handle.
        void apply_color_from_ansi_code(const std::string& ansi_code, HANDLE handle_stdout) const {
            WORD color_value = static_cast<WORD>(text_color_to_win_color(m_config.default_color)); // Default color
            const int code = std::stoi(ansi_code);
            switch (code) {
                case 30: color_value = static_cast<WORD>(WinColor::Black); break;
                case 31: color_value = static_cast<WORD>(WinColor::DarkRed); break;
                case 32: color_value = static_cast<WORD>(WinColor::DarkGreen); break;
                case 33: color_value = static_cast<WORD>(WinColor::DarkYellow); break;
                case 34: color_value = static_cast<WORD>(WinColor::DarkBlue); break;
                case 35: color_value = static_cast<WORD>(WinColor::DarkMagenta); break;
                case 36: color_value = static_cast<WORD>(WinColor::DarkCyan); break;
                case 37: color_value = static_cast<WORD>(WinColor::Gray); break;
                case 90: color_value = static_cast<WORD>(WinColor::DarkGray); break;
                case 91: color_value = static_cast<WORD>(WinColor::Red); break;
                case 92: color_value = static_cast<WORD>(WinColor::Green); break;
                case 93: color_value = static_cast<WORD>(WinColor::Yellow); break;
                case 94: color_value = static_cast<WORD>(WinColor::Blue); break;
                case 95: color_value = static_cast<WORD>(WinColor::Magenta); break;
                case 96: color_value = static_cast<WORD>(WinColor::Cyan); break;
                case 97: color_value = static_cast<WORD>(WinColor::White); break;
                default:
                    // Unknown code, use default color
                    break;
            };
            // Set the console text attribute to the desired color
            SetConsoleTextAttribute(handle_stdout, color_value);
        }

        /// \brief Convert TextColor to WinColor.
        /// \param color The TextColor value.
        /// \return Corresponding WinColor value.
        WinColor text_color_to_win_color(const TextColor& color) const {
            switch (color) {
                case TextColor::Black:       return WinColor::Black;
                case TextColor::DarkRed:     return WinColor::DarkRed;
                case TextColor::DarkGreen:   return WinColor::DarkGreen;
                case TextColor::DarkYellow:  return WinColor::DarkYellow;
                case TextColor::DarkBlue:    return WinColor::DarkBlue;
                case TextColor::DarkMagenta: return WinColor::DarkMagenta;
                case TextColor::DarkCyan:    return WinColor::DarkCyan;
                case TextColor::LightGray:   return WinColor::Gray;
                case TextColor::DarkGray:    return WinColor::DarkGray;
                case TextColor::Red:         return WinColor::Red;
                case TextColor::Green:       return WinColor::Green;
                case TextColor::Yellow:      return WinColor::Yellow;
                case TextColor::Blue:        return WinColor::Blue;
                case TextColor::Magenta:     return WinColor::Magenta;
                case TextColor::Cyan:        return WinColor::Cyan;
                case TextColor::White:       return WinColor::White;
                default:                     return WinColor::White;
            }
        }
#       endif

        /// \brief Resets the console text color to the default.
        void reset_color() {
#           if defined(__MINGW32__) || defined(_WIN32)
            handle_ansi_colors_windows(std::string());
#           else
            std::cout << to_string(m_config.default_color);
#           endif
        }
    }; // ConsoleLogger

}; // namespace logit

#endif // _LOGIT_CONSOLE_LOGGER_HPP_INCLUDED
