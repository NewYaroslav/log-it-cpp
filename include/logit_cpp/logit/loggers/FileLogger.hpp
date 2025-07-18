#ifndef _LOGIT_FILE_LOGGER_HPP_INCLUDED
#define _LOGIT_FILE_LOGGER_HPP_INCLUDED

/// \file FileLogger.hpp
/// \brief File logger implementation that outputs logs to files with rotation and deletion of old logs.

#include "ILogger.hpp"
#include <iostream>
#include <fstream>
#include <mutex>
#include <atomic>
#include <regex>
#include <queue>
#include <functional>
#include <time_shield/time_parser.hpp>

namespace logit {

#if defined(__EMSCRIPTEN__)

    class FileLogger : public ILogger {
    public:
        struct Config {
            std::string directory = "logs";
            bool        async     = false;
            int         auto_delete_days = 30;
        };

        FileLogger() { warn(); }
        FileLogger(const Config&) { warn(); }
        FileLogger(const std::string&, const bool& = true, const int& = 30) { warn(); }

        void log(const LogRecord&, const std::string&) override { warn(); }
        std::string get_string_param(const LoggerParam&) const override { return {}; }
        int64_t get_int_param(const LoggerParam&) const override { return 0; }
        double get_float_param(const LoggerParam&) const override { return 0.0; }
        void wait() override {}

    private:
        void warn() const { std::cerr << "FileLogger is not supported under Emscripten" << std::endl; }
    };

#else

    /// \class FileLogger
    /// \ingroup LogBackends
    /// \brief Logs messages to files with date-based rotation and automatic deletion of old logs.
    ///
    /// This logger writes logs to files organized by date. It supports asynchronous logging
    /// and manages old files based on a configurable retention period.
    ///
    /// **Key Features:**
    /// - Date-based file rotation.
    /// - Automatic cleanup of old files.
    /// - Synchronous or asynchronous operation.
    class FileLogger : public ILogger {
    public:

        /// \struct Config
        /// \brief Configuration for the file logger.
        struct Config {
            std::string directory           = "logs"; ///< Directory where log files are stored.
            bool        async               = true; ///< Flag indicating whether logging should be asynchronous.
            int         auto_delete_days    = 30;   ///< Number of days after which old log files are deleted.
        };

        /// \brief Default constructor that uses default configuration.
        FileLogger() {
            start_logging();
        }

        /// \brief Constructor with custom configuration.
        /// \param config The configuration for the logger.
        FileLogger(const Config& config) : m_config(config) {
            start_logging();
        }

        /// \brief Constructor with directory and asynchronous flag.
        /// \param directory The directory where log files will be stored.
        /// \param async Boolean flag for asynchronous logging.
        /// \param auto_delete_days Number of days after which old log files are deleted.
        FileLogger(
                const std::string& directory,
                const bool& async = true,
                const int& auto_delete_days = 30) {
            m_config.directory = directory;
            m_config.async = async;
            m_config.auto_delete_days = auto_delete_days;
            start_logging();
        }

        /// \brief Destructor to stop logging and close file.
        virtual ~FileLogger() {
            stop_logging();
        }

        /// \brief Logs a message to a file with thread safety.
        ///
        /// If asynchronous logging is enabled, the message is added to the task queue;
        /// otherwise, it is logged directly.
        ///
        /// \param record The log record containing log information.
        /// \param message The formatted log message.
        void log(const LogRecord& record, const std::string& message) override {
            m_last_log_ts = record.timestamp_ms;
            if (!m_config.async) {
                std::lock_guard<std::mutex> lock(m_mutex);
                try {
                    write_log(message, record.timestamp_ms);
                } catch (const std::exception& e) {
                    std::cerr << "Log error: " << e.what() << std::endl;
                }
                return;
            }
            auto timestamp_ms = record.timestamp_ms;
            TaskExecutor::get_instance().add_task([this, message, timestamp_ms]() {
                std::lock_guard<std::mutex> lock(m_mutex);
                try {
                    write_log(message, timestamp_ms);
                } catch (const std::exception& e) {
                    std::cerr << "Log async log error: " << e.what() << std::endl;
                }
            });
        }

        /// \brief Retrieves a string parameter from the logger.
        /// \param param The parameter type to retrieve.
        /// \return A string representing the requested parameter.
        std::string get_string_param(const LoggerParam& param) const override {
            switch (param) {
            case LoggerParam::LastFileName: return get_last_log_file_name();
            case LoggerParam::LastFilePath: return get_last_log_file_path();
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

        /// \brief Waits for all asynchronous tasks to complete.
        void wait() override {
            if (!m_config.async) return;
            TaskExecutor::get_instance().wait();
        }

    private:
        mutable std::mutex m_mutex;    ///< Mutex to protect file operations.
        Config             m_config;   ///< Configuration for the file logger.
        std::ofstream      m_file;     ///< Output file stream for logging.
        mutable std::mutex m_file_path_mutex; ///< Mutex to protect file path operations.
        std::string        m_file_path; ///< Path of the currently open log file.
        std::string        m_file_name; ///< Name of the currently open log file.
        int64_t            m_current_date_ts = 0; ///< Timestamp of the current log file's date.
        std::atomic<int64_t> m_last_log_ts = ATOMIC_VAR_INIT(0); ///< Timestamp of the last log.

        /// \brief Starts the logging process by initializing the file and directory.
        void start_logging() {
            // I/O streams (e.g., std::cin, std::cout, std::cerr) may be closed before the program exits.
            // In this case, calls to functions that use I/O streams (for example, the std::regex constructor)
            // can lead to undesirable behavior such as hangs or segmentation faults.
            is_valid_log_filename("2024-01-01.log");
            std::lock_guard<std::mutex> lock(m_mutex);
            try {
                initialize_directory();
                open_log_file(get_current_utc_date_ts());
                remove_old_logs();
            } catch (const std::exception& e) {
                std::cerr << "Initialization error: " << e.what() << std::endl;
            }
        }

        /// \brief Stops the logging process by closing the file and waiting for tasks.
        void stop_logging() {
            wait();
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_file.is_open()) {
                m_file.close();
            }
        }

        /// \brief Initializes the logging directory.
        void initialize_directory() {
            create_directories(get_directory_path());
        }

        /// \brief Gets the full path to the logging directory.
        /// \return The path to the logging directory.
        std::string get_directory_path() const {
#           if defined(_WIN32)
            return get_exec_dir() + "\\" + m_config.directory;
#           else
            return get_exec_dir() + "/" + m_config.directory;
#           endif
        }

        /// \brief Opens a new log file based on the provided date timestamp.
        /// \param date_ts The timestamp representing the date for the log file.
        void open_log_file(const int64_t& date_ts) {
            if (m_file.is_open()) {
                m_file.close();
            }
            m_current_date_ts = date_ts;
            std::unique_lock<std::mutex> lock(m_file_path_mutex);
            m_file_path = create_file_path(date_ts);
            m_file_name = get_file_name(m_file_path);
            lock.unlock();
#           if defined(_WIN32)
            m_file.open(utf8_to_ansi(m_file_path), std::ios_base::app);
#           else
            m_file.open(m_file_path, std::ios_base::app);
#           endif
            if (!m_file.is_open()) {
                throw std::runtime_error("Failed to open log file: " + m_file_path);
            }
        }

        /// \brief Creates a file path for the log file based on the date timestamp.
        /// \param date_ts The timestamp representing the date for the log file.
        /// \return The path to the log file.
        std::string create_file_path(int64_t date_ts) const {
            std::string date_str = time_shield::to_iso8601_date(date_ts);
            return get_directory_path() + "/" + date_str + ".log";
        }

        /// \brief Writes a log message to the file.
        /// \param message The log message to write.
        /// \param timestamp_ms The timestamp of the log message in milliseconds.
        void write_log(const std::string& message, const int64_t& timestamp_ms) {
            const int64_t message_date_ts = time_shield::start_of_day(time_shield::ms_to_sec(timestamp_ms));
            if (message_date_ts != m_current_date_ts) {
                open_log_file(message_date_ts);
            }
            if (m_file.is_open()) {
                m_file << message << std::endl;
            }
            remove_old_logs();
        }

        /// \brief Removes old log files based on the auto-delete days configuration.
        void remove_old_logs() {
            const int64_t threshold_ts = m_current_date_ts - (time_shield::SEC_PER_DAY * m_config.auto_delete_days);
#           if __cplusplus >= 201703L
#           ifdef _WIN32
            fs::path dir_path = fs::u8path(get_directory_path());
#           else
            fs::path dir_path(get_directory_path());
#           endif

            if (!fs::exists(dir_path) ||
                !fs::is_directory(dir_path)) {
                return;
            }

            for (const auto& entry : fs::directory_iterator(dir_path)) {
                if (!fs::is_regular_file(entry.status())) continue;
                std::string filename = entry.path().filename().string();
                if (is_valid_log_filename(filename)) {
                    const int64_t file_date_ts = get_date_ts_from_filename(filename);
                    if (file_date_ts < threshold_ts) {
                        fs::remove(entry.path());
                    }
                }
            }
#           else
            std::vector<std::string> file_list = get_list_files(get_directory_path());
            for (const auto& file_path : file_list) {
                // Extract the file name
                std::string filename = file_path.substr(file_path.find_last_of("/\\") + 1);
                if (is_valid_log_filename(filename)) {
                    const int64_t file_date_ts = get_date_ts_from_filename(filename);
                    if (file_date_ts < threshold_ts) {
#                       if defined(_WIN32)
                        remove(utf8_to_ansi(file_path).c_str());
#                       else
                        remove(file_path.c_str());
#                       endif
                    }
                }
            }
#           endif
        }

        /// \brief Checks if the filename matches the log file naming pattern.
        /// \param filename The filename to check.
        /// \return True if the filename matches the pattern, false otherwise.
        bool is_valid_log_filename(const std::string& filename) const {
            static const std::regex pattern(R"((\d{4}-\d{2}-\d{2})\.log)");
            return std::regex_match(filename, pattern);
        }

        /// \brief Extracts the date timestamp from the log filename.
        /// \param filename The filename to extract the date from.
        /// \return The date timestamp.
        int64_t get_date_ts_from_filename(const std::string& filename) const {
            constexpr size_t EXTENSION_LENGTH = sizeof(".log") - 1;
            return time_shield::ts(filename.substr(0, filename.size() - EXTENSION_LENGTH));
        }

        /// \brief Gets the current UTC date timestamp in seconds.
        /// \return The current UTC date timestamp in seconds.
        int64_t get_current_utc_date_ts() const {
            return time_shield::start_of_day(time_shield::ms_to_sec(current_timestamp_ms()));
        }

        /// \brief Gets the current timestamp in milliseconds.
        /// \return The current timestamp in milliseconds.
        int64_t current_timestamp_ms() const {
            return LOGIT_CURRENT_TIMESTAMP_MS();
        }

        /// \brief Retrieves the last log file path.
        /// \return The last log file path.
        std::string get_last_log_file_path() const {
            std::lock_guard<std::mutex> lock(m_file_path_mutex);
            return m_file_path;
        }

        /// \brief Retrieves the last log file name.
        /// \return The last log file name.
        std::string get_last_log_file_name() const {
            std::lock_guard<std::mutex> lock(m_file_path_mutex);
            return m_file_name;
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
    }; // FileLogger
#endif // defined(__EMSCRIPTEN__)

}; // namespace logit

#endif // _LOGIT_FILE_LOGGER_HPP_INCLUDED
