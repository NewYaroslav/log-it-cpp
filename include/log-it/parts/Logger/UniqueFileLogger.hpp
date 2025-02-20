#ifndef _LOGIT_UNIQUE_FILE_LOGGER_HPP_INCLUDED
#define _LOGIT_UNIQUE_FILE_LOGGER_HPP_INCLUDED
/// \file UniqueFileLogger.hpp
/// \brief Logger that writes each log message to a unique file with auto-deletion of old logs.

#include "ILogger.hpp"
#include <iostream>
#include <fstream>
#include <mutex>
#include <atomic>
#include <regex>
#include <queue>
#include <functional>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <random>
#include <algorithm>
#include <unordered_map>

namespace logit {

    /// \class UniqueFileLogger
    /// \ingroup LogBackends
    /// \brief Writes each log message to a unique file with automatic cleanup.
    ///
    /// This logger generates a unique file for each log message. The filename includes
    /// a timestamp and a hash to ensure uniqueness. It also deletes old log files
    /// after a specified number of days.
    ///
    /// **Key Features:**
    /// - Unique file generation for each log message.
    /// - Automatic deletion of old files.
    /// - Synchronous or asynchronous operation.
    class UniqueFileLogger : public ILogger {
    public:

        /// \struct Config
        /// \brief Configuration for the unique file logger.
        struct Config {
            std::string directory           = "unique_logs"; ///< Directory where log files are stored.
            bool        async               = true; ///< Flag indicating whether logging should be asynchronous.
            int         auto_delete_days    = 30;   ///< Number of days after which old log files are deleted.
            size_t      hash_length         = 8;    ///< Length of the hash used in filenames.
        };

        /// \brief Default constructor that uses default configuration.
        UniqueFileLogger() {
            start_logging();
        }

        /// \brief Constructor with custom configuration.
        /// \param config The configuration for the logger.
        UniqueFileLogger(const Config& config) : m_config(config) {
            start_logging();
        }

        /// \brief Constructor with directory and asynchronous flag.
        /// \param directory The directory where log files will be stored.
        /// \param async Boolean flag for asynchronous logging.
        /// \param auto_delete_days Number of days after which old log files are deleted.
        /// \param hash_length Length of the hash used in filenames.
        UniqueFileLogger(
            const std::string& directory,
            bool async = true,
            int auto_delete_days = 30,
            size_t hash_length = 8) {
            m_config.directory = directory;
            m_config.async = async;
            m_config.auto_delete_days = auto_delete_days;
            m_config.hash_length = hash_length;
            start_logging();
        }

        virtual ~UniqueFileLogger() {
            stop_logging();
        }

        /// \brief Logs a message to a unique file with thread safety.
        ///
        /// If asynchronous logging is enabled, the message is added to the task queue;
        /// otherwise, it is logged directly.
        ///
        /// \param record The log record containing log information.
        /// \param message The log message to write.
        void log(const LogRecord& record, const std::string& message) override {
            auto thread_id = record.thread_id;
            m_last_log_ts = record.timestamp_ms;
            if (!m_config.async) {
                std::lock_guard<std::mutex> lock(m_mutex);
                std::string file_path;
                try {
                    file_path = write_log(message, record.timestamp_ms);
                } catch (const std::exception& e) {
                    file_path.clear();
                    std::cerr << "Log error: " << e.what() << std::endl;
                }

                std::unique_lock<std::mutex> info_lock(m_thread_log_info_mutex);
                auto it = m_thread_log_info.find(thread_id);
                if (it == m_thread_log_info.end()) {
                    if (!file_path.empty()) {
                        m_thread_log_info[thread_id] = {0, file_path, get_file_name(file_path)};
                    } else {
                        m_thread_log_info[thread_id] = {0, "Not available", "Not available"};
                    }
                    return;
                }

                if (!file_path.empty()) {
                    it->second.last_file_path = file_path;
                    it->second.last_file_name = get_file_name(file_path);
                } else {
                    it->second.last_file_path = "Not available";
                    it->second.last_file_name = "Not available";
                }

                m_pending_logs_cv.notify_all();
                info_lock.unlock();

                try {
                    remove_old_logs();
                } catch (const std::exception& e) {
                    std::cerr << "Log error: " << e.what() << std::endl;
                }
                return;
            }

            std::unique_lock<std::mutex> info_lock(m_thread_log_info_mutex);
            m_thread_log_info[thread_id].pending_logs++;
            info_lock.unlock();

            auto timestamp_ms = record.timestamp_ms;
            TaskExecutor::get_instance().add_task([this, message, timestamp_ms, thread_id]() {
                std::lock_guard<std::mutex> lock(m_mutex);
                std::string file_path;
                try {
                    file_path = write_log(message, timestamp_ms);
                } catch (const std::exception& e) {
                    file_path.clear();
                    std::cerr << "Async log error: " << e.what() << std::endl;
                }

                std::unique_lock<std::mutex> info_lock(m_thread_log_info_mutex);
                auto it = m_thread_log_info.find(thread_id);
                if (it == m_thread_log_info.end()) return;

                if (!file_path.empty()) {
                    it->second.last_file_path = file_path;
                    it->second.last_file_name = get_file_name(file_path);
                } else {
                    it->second.last_file_path = "Not available";
                    it->second.last_file_name = "Not available";
                }
                it->second.pending_logs--;

                if (it->second.pending_logs == 0) {
                    m_pending_logs_cv.notify_all();
                }
                info_lock.unlock();

                try {
                    remove_old_logs();
                } catch (const std::exception& e) {
                    std::cerr << "Async log error: " << e.what() << std::endl;
                }
            });
        }

        /// \brief Retrieves a string parameter from the logger.
        /// \param param The logger parameter to retrieve.
        /// \return A string representing the requested parameter, or an empty string if the parameter is unsupported.
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
        /// \param param The logger parameter to retrieve.
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
        /// \param param The logger parameter to retrieve.
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
        Config             m_config;   ///< Configuration for the unique file logger.

        struct ThreadLogInfo {
            int pending_logs;
            std::string last_file_path;
            std::string last_file_name;

            ThreadLogInfo() : pending_logs(0) {}

            ThreadLogInfo(
                    const int pending_logs,
                    const std::string last_file_path,
                    const std::string last_file_name) :
                pending_logs(pending_logs),
                last_file_path(last_file_path),
                last_file_name(last_file_name) {
            }
        };

        mutable std::mutex m_thread_log_info_mutex; ///< Mutex to protect access to thread log information.
        mutable std::condition_variable m_pending_logs_cv; ///< Condition variable to wait for pending logs to finish.
        std::unordered_map<std::thread::id, ThreadLogInfo> m_thread_log_info; ///< Map to store log information per thread.

        std::atomic<int64_t> m_last_log_ts = ATOMIC_VAR_INIT(0); ///< Timestamp of the last log.


        /// \brief Starts the logging process by initializing the directory and removing old logs.
        void start_logging() {
            std::lock_guard<std::mutex> lock(m_mutex);
            try {
                initialize_directory();
                remove_old_logs();
            } catch (const std::exception& e) {
                std::cerr << "Initialization error: " << e.what() << std::endl;
            }
        }

        /// \brief Stops the logging process by waiting for tasks.
        void stop_logging() {
            wait();
        }

        /// \brief Initializes the logging directory.
        void initialize_directory() {
            create_directories(get_directory_path());
        }

        /// \brief Gets the full path to the logging directory.
        /// \return The path to the logging directory.
        std::string get_directory_path() const {
            return get_exec_dir() + "/" + m_config.directory;
        }

        /// \brief Writes a log message to a unique file.
        /// \param message The log message to write.
        /// \param timestamp_ms The timestamp of the log message in milliseconds.
        /// \return The name of the file the message was written to.
        std::string write_log(const std::string& message, const int64_t& timestamp_ms) {
            std::string file_path = create_unique_file_path(timestamp_ms);
#           if defined(_WIN32) || defined(_WIN64)
            std::ofstream file(utf8_to_ansi(file_path), std::ios_base::binary);
#           else
            std::ofstream file(file_path, std::ios_base::binary);
#           endif
            if (!file.is_open()) {
                throw std::runtime_error("Failed to open log file: " + file_path);
            }
            file.write(message.data(), message.size());
            file.close();
            return file_path;
        }

        /// \brief Creates a unique file path based on the timestamp and a hash.
        /// \param timestamp_ms The timestamp in milliseconds.
        /// \return The unique file path.
        std::string create_unique_file_path(const int64_t& timestamp_ms) const {
            const std::string timestamp_str = format_timestamp(timestamp_ms);
            const std::string hash_str = generate_fixed_length_hash(m_config.hash_length);
            return get_directory_path() + "/" + timestamp_str + "-" + hash_str + ".log";
        }

        /// \brief Formats the timestamp into a string with date and time.
        /// \param timestamp_ms The timestamp in milliseconds.
        /// \return The formatted timestamp string.
        std::string format_timestamp(const int64_t& timestamp_ms) const {
            const auto dt = time_shield::to_date_time_ms<time_shield::DateTimeStruct>(timestamp_ms);
            char buffer[32] = {0};
            snprintf(buffer, sizeof(buffer), "%lld-%.2d-%.2d_%.2d-%.2d-%.2d-%.3d", dt.year, dt.mon, dt.day, dt.hour, dt.min, dt.sec, dt.ms);
            return std::string(buffer);
        }

        /// \brief Generates a fixed-length hash string.
        /// \param length The desired length of the hash.
        /// \return The hash string.
        std::string generate_fixed_length_hash(size_t length) const {
            static const char charset[] =
                "0123456789"
                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                "abcdefghijklmnopqrstuvwxyz";
            static thread_local std::mt19937 generator(std::random_device{}());
            static thread_local std::uniform_int_distribution<size_t> distribution(0, sizeof(charset) - 2);

            std::string hash;
            hash.reserve(length);
            for (size_t i = 0; i < length; ++i) {
                hash += charset[distribution(generator)];
            }
            return hash;
        }

        /// \brief Removes old log files based on the auto-delete days configuration.
        void remove_old_logs() {
            const int64_t threshold_ts = time_shield::ms_to_sec(current_timestamp_ms()) - (m_config.auto_delete_days * time_shield::SEC_PER_DAY);
#           if __cplusplus >= 201703L
            fs::path dir_path(get_directory_path());
            if (!fs::exists(dir_path) || !fs::is_directory(dir_path)) {
                return;
            }
            for (const auto& entry : fs::directory_iterator(dir_path)) {
                if (!fs::is_regular_file(entry.status())) continue;
                std::string filename = entry.path().filename().string();
                if (is_valid_log_filename(filename)) {
                    const int64_t file_ts = get_timestamp_from_filename(filename);
                    if (file_ts < threshold_ts) {
#                       if defined(_WIN32) || defined(_WIN64)
                        fs::remove(fs::path(utf8_to_ansi(entry.path().string())));
#                       else
                        fs::remove(entry.path());
#                       endif
                    }
                }
            }
#           else
            const std::vector<std::string> file_list = get_list_files(get_directory_path());
            for (const auto& file_path : file_list) {
                std::string filename = get_file_name(file_path);
                if (is_valid_log_filename(filename)) {
                    const int64_t file_ts = get_timestamp_from_filename(filename);
                    if (file_ts < threshold_ts) {
#                       if defined(_WIN32) || defined(_WIN64)
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
            static const std::regex pattern(R"((\d{4}-\d{2}-\d{2}_\d{2}-\d{2}-\d{2}-\d{3})-[a-zA-Z0-9]{1,}\.log)");
            return std::regex_match(filename, pattern);
        }

        /// \brief Extracts the timestamp from the filename.
        /// \param filename The filename to extract the timestamp from.
        /// \return The timestamp in milliseconds.
        int64_t get_timestamp_from_filename(const std::string& filename) const {
            std::string datetime_str = filename.substr(0, 10); // "YYYY-MM-DD"
            return time_shield::ts(datetime_str);
        }

        /// \brief Gets the current timestamp in milliseconds.
        /// \return The current timestamp in milliseconds.
        int64_t current_timestamp_ms() const {
            return LOGIT_CURRENT_TIMESTAMP_MS();
        }

        /// \brief Retrieves the last log file name for the calling thread.
        ///
        /// This method waits until all pending log messages for the calling thread have been processed.
        /// It then returns the name of the last file that the calling thread wrote a log message to.
        ///
        /// \return The last log file name for the calling thread, or an empty string if none exists.
        std::string get_last_log_file_name() const {
            auto thread_id = std::this_thread::get_id();
            std::unique_lock<std::mutex> lock(m_thread_log_info_mutex);
            m_pending_logs_cv.wait(lock, [this, thread_id]() {
                auto it = m_thread_log_info.find(thread_id);
                return it == m_thread_log_info.end() || it->second.pending_logs == 0;
            });
            auto it = m_thread_log_info.find(thread_id);
            if (it != m_thread_log_info.end()) {
                return it->second.last_file_name;
            }
            return std::string();
        }

        /// \brief Retrieves the last log file path for the calling thread.
        ///
        /// This method waits until all pending log messages for the calling thread have been processed.
        /// It then returns the path of the last file that the calling thread wrote a log message to.
        ///
        /// \return The last log file path for the calling thread, or an empty string if none exists.
        std::string get_last_log_file_path() const {
            auto thread_id = std::this_thread::get_id();
            std::unique_lock<std::mutex> lock(m_thread_log_info_mutex);
            m_pending_logs_cv.wait(lock, [this, thread_id]() {
                auto it = m_thread_log_info.find(thread_id);
                return it == m_thread_log_info.end() || it->second.pending_logs == 0;
            });
            auto it = m_thread_log_info.find(thread_id);
            if (it != m_thread_log_info.end()) {
                return it->second.last_file_path;
            }
            return std::string();
        }

        /// \brief Retrieves the timestamp of the last log.
        /// \return The timestamp of the last log in milliseconds.
        int64_t get_last_log_ts() const {
            return m_last_log_ts;
        }

        /// \brief Retrieves the time elapsed since the last log.
        /// \return The time in milliseconds since the last log was written.
        int64_t get_time_since_last_log() const {
            return LOGIT_CURRENT_TIMESTAMP_MS() - m_last_log_ts;
        }

    }; // UniqueFileLogger

}; // namespace logit

#endif // _LOGIT_UNIQUE_FILE_LOGGER_HPP_INCLUDED
