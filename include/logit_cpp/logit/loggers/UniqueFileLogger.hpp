#pragma once
#ifndef _LOGIT_UNIQUE_FILE_LOGGER_HPP_INCLUDED
#define _LOGIT_UNIQUE_FILE_LOGGER_HPP_INCLUDED

/// \file UniqueFileLogger.hpp
/// \brief Logger that writes each log message to a unique file with auto-deletion of old logs.

#include "ILogger.hpp"
#include <iostream>
#include <fstream>
#include <mutex>
#include <atomic>
#include <queue>
#include <functional>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <random>
#include <algorithm>
#include <unordered_map>
#include <regex>

namespace logit {

#if defined(__EMSCRIPTEN__)

    class UniqueFileLogger : public ILogger {
    public:
        struct Config {
            std::string directory = "unique_logs";
            bool        async     = false;
            int         auto_delete_days = 30;
            size_t      hash_length = 8;
        };

        UniqueFileLogger() { warn(); }
        UniqueFileLogger(const Config&) { warn(); }
        UniqueFileLogger(const std::string&, bool = true, int = 30, size_t = 8) { warn(); }

        void log(const LogRecord&, const std::string&) override { warn(); }
        std::string get_string_param(const LoggerParam&) const override { return {}; }
        int64_t get_int_param(const LoggerParam&) const override { return 0; }
        double get_float_param(const LoggerParam&) const override { return 0.0; }
        void set_log_level(LogLevel) override {}
        LogLevel get_log_level() const override { return LogLevel::LOG_LVL_TRACE; }
        void wait() override {}

    private:
        void warn() const {
            static bool warned = false;
            if (!warned) {
                warned = true;
                std::cerr << "UniqueFileLogger is not supported under Emscripten" << std::endl;
            }
        }
        std::atomic<int> m_log_level = ATOMIC_VAR_INIT(static_cast<int>(LogLevel::LOG_LVL_TRACE));
    };

#else

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
            m_last_log_mono_ts = LOGIT_MONOTONIC_MS();
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
            detail::TaskExecutor::get_instance().add_task([this, message, timestamp_ms, thread_id]() {
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
            case LoggerParam::LastLogTimestamp:
                return static_cast<double>(get_last_log_ts()) / 1000.0;
            case LoggerParam::TimeSinceLastLog:
                return static_cast<double>(get_time_since_last_log()) / 1000.0;
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
        void wait() override {
            if (!m_config.async) return;
            detail::TaskExecutor::get_instance().wait();
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
        std::atomic<int64_t> m_last_log_mono_ts = ATOMIC_VAR_INIT(0); ///< Timestamp of the last log.
        std::atomic<int>    m_log_level = ATOMIC_VAR_INIT(static_cast<int>(LogLevel::LOG_LVL_TRACE));


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
#           if defined(_WIN32)
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

        bool try_make_log_file_info(const std::string& file_path, LogFileInfo& info) const {
            if (!is_direct_child_path(file_path, get_directory_path())) {
                return false;
            }

            const std::string filename = get_file_name(file_path);
            if (!is_valid_log_filename(filename)) {
                return false;
            }

            info.path = file_path;
            info.name = filename;
            info.day_start_ms = time_shield::sec_to_ms(get_date_ts_from_filename(filename));
            info.is_current = false;
            info.is_compressed = is_compressed_log_filename(filename);
            return true;
        }

        LogFileReadResult read_log_file_from_info(const LogFileInfo& info) const {
            LogFileReadResult result;
            result.file = info;
            if (info.is_compressed) {
                result.ok = false;
                return result;
            }

            result.ok = read_plain_file(info.path, result.content);
            return result;
        }

        bool read_plain_file(const std::string& file_path, std::string& out) const {
#           if defined(_WIN32)
            std::ifstream in(utf8_to_ansi(file_path).c_str(), std::ios_base::binary);
#           else
            std::ifstream in(file_path.c_str(), std::ios_base::binary);
#           endif
            if (!in.is_open()) {
                out.clear();
                return false;
            }

            std::ostringstream stream;
            stream << in.rdbuf();
            out = stream.str();
            return !in.bad();
        }

        bool is_direct_child_path(const std::string& file_path, const std::string& directory_path) const {
#           if __cplusplus >= 201703L
#               if defined(_WIN32)
            return fs::u8path(file_path).parent_path().lexically_normal() ==
                   fs::u8path(directory_path).lexically_normal();
#               else
            return fs::path(file_path).parent_path().lexically_normal() ==
                   fs::path(directory_path).lexically_normal();
#               endif
#           else
            const size_t pos = file_path.find_last_of("/\\");
            if (pos == std::string::npos) return false;
            std::string parent = file_path.substr(0, pos);
            return parent == directory_path;
#           endif
        }

        bool same_path(const std::string& lhs, const std::string& rhs) const {
#           if __cplusplus >= 201703L
#               if defined(_WIN32)
            return fs::u8path(lhs).lexically_normal() == fs::u8path(rhs).lexically_normal();
#               else
            return fs::path(lhs).lexically_normal() == fs::path(rhs).lexically_normal();
#               endif
#           else
            return normalize_path_separators(lhs) == normalize_path_separators(rhs);
#           endif
        }

        std::string normalize_path_separators(const std::string& path) const {
            std::string normalized = path;
            for (size_t i = 0; i < normalized.size(); ++i) {
                if (normalized[i] == '\\') normalized[i] = '/';
            }
            return normalized;
        }

        struct UniqueFileSortKey {
            std::string timestamp_prefix;
            std::string artifact_name;
        };

        static UniqueFileSortKey make_file_sort_key(const LogFileInfo& info) {
            UniqueFileSortKey key;
            key.artifact_name = strip_compression_suffix_impl(info.name);
            key.timestamp_prefix =
                key.artifact_name.size() >= 23 ? key.artifact_name.substr(0, 23) : key.artifact_name;
            return key;
        }

        bool is_compressed_log_filename(const std::string& filename) const {
            return ends_with(filename, ".gz") || ends_with(filename, ".zst");
        }

        bool ends_with(const std::string& value, const std::string& suffix) const {
            return ends_with_impl(value, suffix);
        }

        static bool ends_with_impl(const std::string& value, const std::string& suffix) {
            return value.size() >= suffix.size() &&
                   value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
        }

        static std::string strip_compression_suffix_impl(const std::string& filename) {
            if (ends_with_impl(filename, ".gz")) {
                return filename.substr(0, filename.size() - 3);
            }
            if (ends_with_impl(filename, ".zst")) {
                return filename.substr(0, filename.size() - 4);
            }
            return filename;
        }

        /// \brief Removes old log files based on the auto-delete days configuration.
        void remove_old_logs() {
            const int64_t threshold_ts =
                time_shield::ms_to_sec(current_timestamp_ms()) -
                (m_config.auto_delete_days * time_shield::SEC_PER_DAY);
#           if __cplusplus >= 201703L

#           if defined(_WIN32)
            fs::path dir_path = fs::u8path(get_directory_path());
#           else
            fs::path dir_path(get_directory_path());
#           endif

            if (!fs::exists(dir_path) || !fs::is_directory(dir_path)) {
                return;
            }

            for (const auto& entry : fs::directory_iterator(dir_path)) {
                if (!fs::is_regular_file(entry.status())) continue;
                std::string filename = entry.path().filename().string();
                if (is_valid_log_filename(filename)) {
                    const int64_t file_ts = get_date_ts_from_filename(filename);
                    if (file_ts < threshold_ts) {
                        fs::remove(entry.path());
                    }
                }
            }
#           else
            const std::vector<std::string> file_list = get_list_files(get_directory_path());
            for (const auto& file_path : file_list) {
                std::string filename = get_file_name(file_path);
                if (is_valid_log_filename(filename)) {
                    const int64_t file_ts = get_date_ts_from_filename(filename);
                    if (file_ts < threshold_ts) {
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
            static const std::regex pattern(
                R"(^(\d{4}-\d{2}-\d{2}_\d{2}-\d{2}-\d{2}-\d{3})-[-_A-Za-z0-9]+\.log(\.gz|\.zst)?$)");
            return std::regex_match(filename, pattern);
        }

        /// \brief Extracts the date timestamp from the filename.
        /// \param filename The filename to extract the date from.
        /// \return The date timestamp.
        int64_t get_date_ts_from_filename(const std::string& filename) const {
            return time_shield::ts(filename.substr(0, 10));
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
            return LOGIT_MONOTONIC_MS() - m_last_log_mono_ts;
        }

    public:
        /// \brief Lists persisted log files owned by this backend.
        /// \return Persisted unique-log files sorted newest-first.
        std::vector<LogFileInfo> list_log_files() const override {
            const std::string directory = get_directory_path();
            std::vector<LogFileInfo> files;

#           if __cplusplus >= 201703L
#               if defined(_WIN32)
            fs::path dir_path = fs::u8path(directory);
#               else
            fs::path dir_path(directory);
#               endif
            if (!fs::exists(dir_path) || !fs::is_directory(dir_path)) {
                return files;
            }

            for (const auto& entry : fs::directory_iterator(dir_path)) {
                if (!fs::is_regular_file(entry.status())) continue;
#               if defined(_WIN32)
                const std::string file_path = entry.path().u8string();
#               else
                const std::string file_path = entry.path().string();
#               endif
                LogFileInfo info;
                if (try_make_log_file_info(file_path, info)) {
                    files.push_back(info);
                }
            }
#           else
            const std::vector<std::string> file_list = get_list_files(directory);
            for (size_t i = 0; i < file_list.size(); ++i) {
                LogFileInfo info;
                if (try_make_log_file_info(file_list[i], info)) {
                    files.push_back(info);
                }
            }
#           endif

            std::sort(files.begin(), files.end(), [this](const LogFileInfo& lhs, const LogFileInfo& rhs) {
                const UniqueFileSortKey lhs_key = make_file_sort_key(lhs);
                const UniqueFileSortKey rhs_key = make_file_sort_key(rhs);
                if (lhs_key.timestamp_prefix != rhs_key.timestamp_prefix) {
                    return lhs_key.timestamp_prefix > rhs_key.timestamp_prefix;
                }
                if (lhs_key.artifact_name == rhs_key.artifact_name &&
                    lhs.is_compressed != rhs.is_compressed) {
                    return !lhs.is_compressed && rhs.is_compressed;
                }
                if (lhs_key.artifact_name != rhs_key.artifact_name) {
                    return lhs_key.artifact_name > rhs_key.artifact_name;
                }
                return normalize_path_separators(lhs.path) > normalize_path_separators(rhs.path);
            });
            return files;
        }

        /// \brief Reads one persisted log file owned by this backend.
        /// \param path Full path returned by `list_log_files()`.
        /// \return Read result. Compressed files are listed but unreadable in v1.
        LogFileReadResult read_log_file(const std::string& path) const override {
            const std::vector<LogFileInfo> files = list_log_files();
            for (size_t i = 0; i < files.size(); ++i) {
                if (same_path(files[i].path, path)) {
                    return read_log_file_from_info(files[i]);
                }
            }

            LogFileReadResult result;
            result.file.path = path;
            result.file.name = get_file_name(path);
            result.ok = false;
            return result;
        }

        /// \brief Reads several persisted log files owned by this backend.
        /// \param paths Full paths returned by `list_log_files()`.
        /// \return Per-file results in the same order as the request.
        std::vector<LogFileReadResult> read_log_files(const std::vector<std::string>& paths) const override {
            const std::vector<LogFileInfo> files = list_log_files();
            std::vector<LogFileReadResult> results;
            results.reserve(paths.size());

            for (size_t i = 0; i < paths.size(); ++i) {
                bool found = false;
                for (size_t j = 0; j < files.size(); ++j) {
                    if (same_path(files[j].path, paths[i])) {
                        results.push_back(read_log_file_from_info(files[j]));
                        found = true;
                        break;
                    }
                }

                if (!found) {
                    LogFileReadResult result;
                    result.file.path = paths[i];
                    result.file.name = get_file_name(paths[i]);
                    result.ok = false;
                    results.push_back(result);
                }
            }

            return results;
        }


    }; // UniqueFileLogger

#endif // defined(__EMSCRIPTEN__)

}; // namespace logit

#endif // _LOGIT_UNIQUE_FILE_LOGGER_HPP_INCLUDED
