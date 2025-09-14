#pragma once
#ifndef _LOGIT_CONSOLE_LOGGER_HPP_INCLUDED
#define _LOGIT_CONSOLE_LOGGER_HPP_INCLUDED

/// \file ConsoleLogger.hpp
/// \brief Console logger implementation that outputs logs to the console with color support.

#include "ILogger.hpp"
#include <iostream>
#if defined(_WIN32)
#include <windows.h>
#endif
#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#if defined(LOGIT_EM_BROWSER_COLORS)
EM_JS(void, log_ansi_js, (int lvl, const char* cmsg, const char* cdefcolor), {
  const msg = UTF8ToString(cmsg);
  const def = UTF8ToString(cdefcolor);
  const isNode = (typeof process !== 'undefined' && process.versions && process.versions.node);
  const fn = lvl >= 5 ? console.error : (lvl == 4 ? console.warn : console.log);
  if (isNode) { fn(msg); return; }
  const map = {30:"black",31:"darkred",32:"darkgreen",33:"olive",34:"darkblue",35:"purple",36:"teal",37:"lightgray",
               90:"gray",91:"red",92:"green",93:"yellow",94:"blue",95:"magenta",96:"cyan",97:"white"};
  const re = /\x1b\[(\d+)m/g;
  let last = 0, m, style = 'color:' + def;
  const fmt = [];
  const styles = [];
  while ((m = re.exec(msg)) !== null) {
    if (m.index > last) { fmt.push('%c' + msg.slice(last, m.index)); styles.push(style); }
    style = 'color:' + (map[m[1]] || def);
    last = re.lastIndex;
  }
  if (last < msg.length) { fmt.push('%c' + msg.slice(last)); styles.push(style); }
  fn(fmt.join(''), ...styles);
});
#else
EM_JS(void, log_level, (int lvl, const char* msg), {
  const s = UTF8ToString(msg);
  const fn = lvl >= 5 ? console.error : (lvl == 4 ? console.warn : console.log);
  fn(s);
});
#endif
#endif
#include <mutex>
#include <atomic>

namespace logit {

    /// \class ConsoleLogger
    /// \ingroup LogBackends
    /// \brief Outputs log messages to the console with optional ANSI color support.
    ///
    /// This logger supports synchronous and asynchronous logging. It also handles
    /// platform-specific differences in handling colored console output.
    ///
    /// **Key Features:**
    /// - Cross-platform color support (ANSI on Linux/macOS, Windows-specific handling).
    /// - Thread-safe logging.
    /// - Synchronous or asynchronous operation.
    class ConsoleLogger : public ILogger {
    public:

        /// \struct Config
        /// \brief Configuration for the console logger.
        struct Config {
            TextColor default_color = LOGIT_DEFAULT_COLOR; ///< Default text color for console output.
#ifdef __EMSCRIPTEN__
            bool async = false; ///< Async logging disabled under Emscripten.
#else
            bool async = true;  ///< Flag indicating whether logging should be asynchronous.
#endif
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
            m_last_log_ts = record.timestamp_ms;
#ifdef __EMSCRIPTEN__
            std::unique_lock<std::mutex> lock(m_mutex);
            const int lvl = static_cast<int>(record.log_level);
            if (!m_config.async) {
#   if defined(LOGIT_EM_BROWSER_COLORS)
                log_ansi_js(lvl, message.c_str(), text_color_to_css(m_config.default_color));
#   else
                log_level(lvl, message.c_str());
#   endif
                return;
            }
            auto msg_copy = std::string(message);
            const auto def_color = m_config.default_color;
            lock.unlock();
            detail::TaskExecutor::get_instance().add_task([this, lvl, msg_copy, def_color]() {
                std::lock_guard<std::mutex> inner_lock(m_mutex);
#   if defined(LOGIT_EM_BROWSER_COLORS)
                log_ansi_js(lvl, msg_copy.c_str(), text_color_to_css(def_color));
#   else
                log_level(lvl, msg_copy.c_str());
#   endif
            });
            return;
#else
            std::unique_lock<std::mutex> lock(m_mutex);
            if (!m_config.async) {
#               if defined(_WIN32)
                // For Windows, parse the message for ANSI color codes and apply them
                handle_ansi_colors_windows(message);
#               else
                // For other systems, output the message as is
                std::cout << message << std::endl;
#               endif
                return;
            }
            lock.unlock();
            detail::TaskExecutor::get_instance().add_task([this, message](){
                std::lock_guard<std::mutex> lock(m_mutex);
#               if defined(_WIN32)
                // For Windows, parse the message for ANSI color codes and apply them
                handle_ansi_colors_windows(message);
#               else
                // For other systems, output the message as is
                std::cout << message << std::endl;
#               endif
            });
#endif
        }

        /// \brief Retrieves a string parameter from the logger.
        ///
        /// This function does not return parameters related to file-based loggers, such as
        /// `LastFileName` and `LastFilePath`.
        ///
        /// \param param The parameter type to retrieve.
        /// \return A string representing the requested parameter, or an empty string if the parameter is unsupported.
        std::string get_string_param(const LoggerParam& param) const override {
            switch (param) {
            case LoggerParam::LastLogTimestamp: return std::to_string(get_last_log_ts());
            case LoggerParam::TimeSinceLastLog: return std::to_string(get_time_since_last_log());
            default:
                break;
            };
            return std::string();
        }

        /// \brief Retrieves an integer parameter from the logger.
        /// \param param The parameter type to retrieve.
        /// \return An integer representing the requested parameter, or 0 if the parameter is unsupported.
        int64_t get_int_param(const LoggerParam& param) const override {
            switch (param) {
            case LoggerParam::LastLogTimestamp: return get_last_log_ts();
            case LoggerParam::TimeSinceLastLog: return get_time_since_last_log();
            default:
                break;
            };
            return 0;
        }

        /// \brief Retrieves a floating-point parameter from the logger.
        /// \param param The parameter type to retrieve.
        /// \return A double representing the requested parameter, or 0.0 if the parameter is unsupported.
        double get_float_param(const LoggerParam& param) const override {
            switch (param) {
            case LoggerParam::LastLogTimestamp: return (double)get_last_log_ts() / 1000.0;
            case LoggerParam::TimeSinceLastLog: return (double)get_time_since_last_log() / 1000.0;
            default:
                break;
            };
            return 0.0;
        }

        /// \brief Sets the minimal log level for this logger.
        void set_log_level(LogLevel level) override {
            m_log_level = static_cast<int>(level);
        }

        /// \brief Gets the minimal log level for this logger.
        LogLevel get_log_level() const override {
            return static_cast<LogLevel>(m_log_level.load());
        }

        /// \brief Waits for all asynchronous tasks to complete.
        /// If asynchronous logging is enabled, waits for all pending log messages to be written.
        void wait() override {
            std::unique_lock<std::mutex> lock(m_mutex);
            if (!m_config.async) return;
            lock.unlock();
            detail::TaskExecutor::get_instance().wait();
        }

    private:
        mutable std::mutex m_mutex;     ///< Mutex to protect console output
        Config             m_config;    ///< Configuration for the console logger.
        std::atomic<int64_t> m_last_log_ts = ATOMIC_VAR_INIT(0);
        std::atomic<int>    m_log_level = ATOMIC_VAR_INIT(static_cast<int>(LogLevel::LOG_LVL_TRACE));

#       ifdef __EMSCRIPTEN__
        /// \brief Convert TextColor to a CSS color name.
        const char* text_color_to_css(TextColor color) const {
            switch (color) {
                case TextColor::Black:       return "black";
                case TextColor::DarkRed:     return "darkred";
                case TextColor::DarkGreen:   return "darkgreen";
                case TextColor::DarkYellow:  return "olive";
                case TextColor::DarkBlue:    return "darkblue";
                case TextColor::DarkMagenta: return "purple";
                case TextColor::DarkCyan:    return "teal";
                case TextColor::LightGray:   return "lightgray";
                case TextColor::DarkGray:    return "gray";
                case TextColor::Red:         return "red";
                case TextColor::Green:       return "green";
                case TextColor::Yellow:      return "yellow";
                case TextColor::Blue:        return "blue";
                case TextColor::Magenta:     return "magenta";
                case TextColor::Cyan:        return "cyan";
                case TextColor::White:       return "white";
                default:                     return "inherit";
            }
        }
#       endif

#       if defined(_WIN32)

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
#           ifdef __EMSCRIPTEN__
            // No persistent console color in browsers
            return;
#           elif defined(_WIN32)
            handle_ansi_colors_windows(std::string());
#           else
            std::cout << to_string(m_config.default_color);
#           endif
        }

        /// \brief Retrieves the timestamp of the last log.
        /// \return The last log timestamp.
        int64_t get_last_log_ts() const {
            return m_last_log_ts;
        }

        /// \brief Retrieves the time since the last log.
        /// \return The time in milliseconds since the last log.
        int64_t get_time_since_last_log() const {
            return LOGIT_CURRENT_TIMESTAMP_MS() - m_last_log_ts;
        }
    }; // ConsoleLogger

}; // namespace logit

#endif // _LOGIT_CONSOLE_LOGGER_HPP_INCLUDED
