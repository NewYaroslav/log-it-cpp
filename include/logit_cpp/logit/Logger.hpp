#pragma once
#ifndef _LOGIT_LOGGER_HPP_INCLUDED
#define _LOGIT_LOGGER_HPP_INCLUDED

/// \file Logger.hpp
/// \brief Defines the Logger class for managing multiple loggers and formatters.

#include "config.hpp"
#include "loggers/ILogger.hpp"
#include "formatter.hpp"
#include "detail/TaskExecutor.hpp"
#include <memory>
#include <mutex>
#include <sstream>
#include <atomic>
#include <cstddef>

#if __cplusplus >= 201703L
#include <shared_mutex>
#define LOGIT_HAS_SHARED_MUTEX 1
#else
#define LOGIT_HAS_SHARED_MUTEX 0
#endif

namespace logit {

#if LOGIT_HAS_SHARED_MUTEX
    using LoggerMutex = std::shared_mutex;
    using LoggerReadLock = std::shared_lock<LoggerMutex>;
    using LoggerWriteLock = std::unique_lock<LoggerMutex>;
#else
    using LoggerMutex = std::mutex;
    using LoggerReadLock = std::unique_lock<LoggerMutex>;
    using LoggerWriteLock = std::unique_lock<LoggerMutex>;
#endif

    /// \class Logger
    /// \brief Singleton class managing multiple loggers and formatters.
    ///
    /// Allows adding multiple logger and formatter pairs.
    /// Provides methods to log messages using these strategies and supports
    /// both synchronous and asynchronous logging. Class is thread-safe.
    class Logger {
    public:

        /// \brief Retrieves singleton instance of Logger.
        /// \return Reference to singleton Logger instance.
        static Logger& get_instance() {
            static Logger* instance = new Logger();
            return *instance;
        }

        /// \brief Adds a logger and its corresponding formatter.
        /// \param logger Unique pointer to a logger instance.
        /// \param formatter Unique pointer to a formatter instance.
        /// \param single_mode If true, this logger will only be invoked by specific log macros
        /// (e.g., LOGIT_TRACE_TO) that explicitly target it using the logger's index.
        /// It will not process logs from general log macros (e.g., LOGIT_TRACE).
        void add_logger(
                std::unique_ptr<ILogger> logger,
                std::unique_ptr<ILogFormatter> formatter,
                bool single_mode = false) {
            if (m_shutdown.load(std::memory_order_acquire)) return;
            auto strategy = std::make_shared<LoggerStrategy>();
            strategy->logger = std::move(logger);
            strategy->formatter = std::move(formatter);
            strategy->single_mode = single_mode;
            strategy->enabled = true;

            LoggerWriteLock lock(m_loggers_mx);
            if (m_shutdown.load(std::memory_order_acquire)) return;
            m_loggers.push_back(std::move(strategy));
        }

        /// \brief Enables or disables a logger by index.
        /// \param logger_index Index of logger.
        /// \param enabled True to enable, false to disable.
        void set_logger_enabled(int logger_index, bool enabled) {
            if (m_shutdown) return;
            LoggerWriteLock lock(m_loggers_mx);
            if (logger_index >= 0 && logger_index < static_cast<int>(m_loggers.size()) && m_loggers[logger_index]) {
                m_loggers[logger_index]->enabled = enabled;
            }
        }

        /// \brief Checks if a logger is enabled.
        /// \param logger_index Index of logger.
        /// \return True if logger is enabled, false otherwise.
        bool is_logger_enabled(int logger_index) const {
            LoggerReadLock lock(m_loggers_mx);
            if (logger_index >= 0 && logger_index < static_cast<int>(m_loggers.size()) && m_loggers[logger_index]) {
                return m_loggers[logger_index]->enabled;
            }
            return false;
        }

        /// \brief Sets single-mode flag for a logger.
        /// \param logger_index Index of logger to modify.
        /// \param single_mode True to enable single mode, false to disable.
        void set_logger_single_mode(int logger_index, bool single_mode) {
            if (m_shutdown) return;
            LoggerWriteLock lock(m_loggers_mx);
            if (logger_index >= 0 && logger_index < static_cast<int>(m_loggers.size()) && m_loggers[logger_index]) {
                m_loggers[logger_index]->single_mode = single_mode;
            }
        }

        /// \brief Sets timestamp offset for a specific logger.
        /// \param logger_index Index of logger to modify.
        /// \param offset_ms Offset in milliseconds.
        void set_timestamp_offset(int logger_index, int64_t offset_ms) {
            if (m_shutdown) return;
            LoggerWriteLock lock(m_loggers_mx);
            if (logger_index >= 0 && logger_index < static_cast<int>(m_loggers.size()) && m_loggers[logger_index]) {
                std::lock_guard<std::mutex> exec_lock(m_loggers[logger_index]->exec_mx);
                m_loggers[logger_index]->formatter->set_timestamp_offset(offset_ms);
            }
        }

        /// \brief Sets minimal log level for all loggers.
        /// \param level Minimum log level.
        void set_log_level(LogLevel level) {
            if (m_shutdown) return;
            LoggerReadLock lock(m_loggers_mx);
            for (auto& strategy : m_loggers) {
                if (!strategy) continue;
                std::lock_guard<std::mutex> exec_lock(strategy->exec_mx);
                strategy->logger->set_log_level(level);
            }
        }

        /// \brief Sets minimal log level for a specific logger.
        /// \param logger_index Index of logger.
        /// \param level Minimum log level.
        void set_log_level(int logger_index, LogLevel level) {
            if (m_shutdown) return;
            LoggerWriteLock lock(m_loggers_mx);
            if (logger_index >= 0 && logger_index < static_cast<int>(m_loggers.size()) && m_loggers[logger_index]) {
                std::lock_guard<std::mutex> exec_lock(m_loggers[logger_index]->exec_mx);
                m_loggers[logger_index]->logger->set_log_level(level);
            }
        }

        /// \brief Checks whether a logger is in single mode.
        /// \param logger_index Index of logger.
        /// \return True if logger is in single mode, false otherwise.
        bool is_logger_single_mode(int logger_index) const {
            if (m_shutdown) return false;
            LoggerReadLock lock(m_loggers_mx);
            if (logger_index >= 0 && logger_index < static_cast<int>(m_loggers.size()) && m_loggers[logger_index]) {
                return m_loggers[logger_index]->single_mode;
            }
            return false;
        }

        /// \brief Logs a LogRecord using added loggers and formatters.
        ///
        /// Formats the log message using each logger's corresponding formatter and sends
        /// the formatted message to the logger.
        /// \param record Log record to be logged.
        void log(const LogRecord& record) {
            if (m_shutdown.load(std::memory_order_acquire)) return;

            const bool targeted = record.logger_index >= 0;

            std::vector<std::shared_ptr<LoggerStrategy>> snapshot;
            snapshot.reserve(targeted ? 1 : 0);

            LoggerReadLock lock(m_loggers_mx);
            if (targeted) {
                if (record.logger_index < static_cast<int>(m_loggers.size()))
                    snapshot.push_back(m_loggers[record.logger_index]);
            } else {
                snapshot = m_loggers; // copy shared_ptrs
            }
            lock.unlock();

            if (targeted) {
                if (snapshot.empty() || !snapshot[0]) return;
                auto& strategy = snapshot[0];

                std::lock_guard<std::mutex> exec_lock(strategy->exec_mx);
                if (m_shutdown.load(std::memory_order_acquire)) return;
                if (!strategy->enabled) return;
                if (!record.raw_mode &&
                    static_cast<int>(record.log_level) < static_cast<int>(strategy->logger->get_log_level())) return;
                dispatch_to_strategy(*strategy, record);
                return;
            }

            for (const auto& strategy : snapshot) {
                if (!strategy) continue;

                std::lock_guard<std::mutex> exec_lock(strategy->exec_mx);
                if (m_shutdown.load(std::memory_order_acquire)) return;
                if (strategy->single_mode) continue;
                if (!strategy->enabled) continue;
                if (!record.raw_mode &&
                    static_cast<int>(record.log_level) < static_cast<int>(strategy->logger->get_log_level())) continue;

                dispatch_to_strategy(*strategy, record);
            }
        }

        /// \brief Retrieves a string parameter from a logger.
        /// \param logger_index Index of logger.
        /// \param param Logger parameter to retrieve.
        /// \return Requested parameter as a string, or empty string if unsupported.
        std::string get_string_param(int logger_index, const LoggerParam& param) const {
            if (m_shutdown) return std::string();
            LoggerReadLock lock(m_loggers_mx);
            if (logger_index >= 0 && logger_index < static_cast<int>(m_loggers.size()) && m_loggers[logger_index]) {
                const auto& strategy = m_loggers[logger_index];
                return strategy->logger->get_string_param(param);
            }
            return std::string();
        }

        /// \brief Retrieves an integer parameter from a logger.
        /// \param logger_index Index of logger.
        /// \param param Logger parameter to retrieve.
        /// \return Requested parameter as an integer, or 0 if unsupported.
        int64_t get_int_param(int logger_index, const LoggerParam& param) const {
            if (m_shutdown) return 0;
            LoggerReadLock lock(m_loggers_mx);
            if (logger_index >= 0 && logger_index < static_cast<int>(m_loggers.size()) && m_loggers[logger_index]) {
                const auto& strategy = m_loggers[logger_index];
                return strategy->logger->get_int_param(param);
            }
            return 0;
        }

        /// \brief Retrieves a floating-point parameter from a logger.
        /// \param logger_index Index of logger.
        /// \param param Logger parameter to retrieve.
        /// \return Requested parameter as a double, or 0.0 if unsupported.
        double get_float_param(int logger_index, const LoggerParam& param) const {
            if (m_shutdown) return 0.0;
            LoggerReadLock lock(m_loggers_mx);
            if (logger_index >= 0 && logger_index < static_cast<int>(m_loggers.size()) && m_loggers[logger_index]) {
                const auto& strategy = m_loggers[logger_index];
                return strategy->logger->get_float_param(param);
            }
            return 0.0;
        }

        /// \brief Retrieves buffered formatted messages from a logger.
        /// \param logger_index Index of logger.
        /// \return Buffered messages in chronological order, or an empty vector if unsupported.
        std::vector<std::string> get_buffered_strings(int logger_index) const {
            if (m_shutdown) return std::vector<std::string>();

            auto strategy = get_strategy_snapshot(logger_index);
            if (!strategy) {
                return std::vector<std::string>();
            }

            return strategy->logger->get_buffered_strings();
        }

        /// \brief Retrieves buffered structured entries from a logger.
        /// \param logger_index Index of logger.
        /// \return Buffered entries in chronological order, or an empty vector if unsupported.
        std::vector<BufferedLogEntry> get_buffered_entries(int logger_index) const {
            if (m_shutdown) return std::vector<BufferedLogEntry>();

            auto strategy = get_strategy_snapshot(logger_index);
            if (!strategy) {
                return std::vector<BufferedLogEntry>();
            }

            return strategy->logger->get_buffered_entries();
        }

        /// \brief Lists persisted log files exposed by a logger backend.
        /// \param logger_index Index of logger.
        /// \return Persisted file metadata, or an empty vector if unsupported.
        std::vector<LogFileInfo> list_log_files(int logger_index) const {
            if (m_shutdown) return std::vector<LogFileInfo>();

            auto strategy = get_strategy_snapshot(logger_index);
            if (!strategy) {
                return std::vector<LogFileInfo>();
            }

            return strategy->logger->list_log_files();
        }

        /// \brief Reads one persisted log file exposed by a logger backend.
        /// \param logger_index Index of logger.
        /// \param path Full path returned by `list_log_files()`.
        /// \return Read result with `ok=false` when unavailable or unsupported.
        LogFileReadResult read_log_file(int logger_index, const std::string& path) const {
            if (m_shutdown) {
                LogFileReadResult result;
                result.file.path = path;
                result.ok = false;
                return result;
            }

            auto strategy = get_strategy_snapshot(logger_index);
            if (!strategy) {
                LogFileReadResult result;
                result.file.path = path;
                result.ok = false;
                return result;
            }

            return strategy->logger->read_log_file(path);
        }

        /// \brief Reads several persisted log files exposed by a logger backend.
        /// \param logger_index Index of logger.
        /// \param paths Full paths returned by `list_log_files()`.
        /// \return Per-file read results in request order.
        std::vector<LogFileReadResult> read_log_files(int logger_index, const std::vector<std::string>& paths) const {
            if (m_shutdown) {
                std::vector<LogFileReadResult> results;
                results.reserve(paths.size());
                for (size_t i = 0; i < paths.size(); ++i) {
                    LogFileReadResult result;
                    result.file.path = paths[i];
                    result.ok = false;
                    results.push_back(result);
                }
                return results;
            }

            auto strategy = get_strategy_snapshot(logger_index);
            if (!strategy) {
                std::vector<LogFileReadResult> results;
                results.reserve(paths.size());
                for (size_t i = 0; i < paths.size(); ++i) {
                    LogFileReadResult result;
                    result.file.path = paths[i];
                    result.ok = false;
                    results.push_back(result);
                }
                return results;
            }

            return strategy->logger->read_log_files(paths);
        }

        /// \brief Retrieves the current minimal log level for a logger.
        /// \param logger_index Index of logger.
        /// \return Current minimal log level, or TRACE when the logger index is invalid.
        LogLevel get_log_level(int logger_index) const {
            if (m_shutdown) return LogLevel::LOG_LVL_TRACE;

            auto strategy = get_strategy_snapshot(logger_index);
            if (!strategy) {
                return LogLevel::LOG_LVL_TRACE;
            }

            return strategy->logger->get_log_level();
        }

        /// \brief Logs message and returns tuple of arguments.
        /// \tparam Ts Types of arguments.
        /// \param record Log record.
        /// \param args Arguments to be logged.
        /// \return Tuple containing logged arguments.
        template <typename... Ts>
        auto log_and_return(const LogRecord& record, Ts&&... args) -> decltype(std::forward_as_tuple(std::forward<Ts>(args)...)) {
            this->print(record, args...);
            return std::forward_as_tuple(std::forward<Ts>(args)...);
        }

        /// \brief Logs message and returns argument.
        ///
        /// Logs provided argument and returns it.
        /// \tparam T Type of argument.
        /// \param record Log record.
        /// \param args Argument to be logged.
        /// \return Logged argument.
        template <typename T>
        auto log_and_return(const LogRecord& record, T&& args) -> decltype(args) {
            this->print(record, args);
            return std::forward<decltype(args)>(args);
        }

        /// \brief Logs message without arguments and returns empty tuple.
        /// \param record Log record.
        /// \return Empty tuple.
        auto log_and_return(const LogRecord& record) -> std::tuple<> {
            this->print(record);
            return {};
        }

        /// \brief Waits for all asynchronous loggers to finish processing.
        ///
        /// Ensures that all log messages are fully processed before continuing.
        void wait() {
            const auto snapshot = get_all_strategy_snapshots();
            for (const auto& strategy : snapshot) {
                if (!strategy) continue;
                strategy->logger->wait();
            }
        }

        /// \brief Clears logger-owned records for a specific logger.
        /// \param logger_index Index of logger.
        /// \param options Data categories to clear.
        /// \return Cleanup result for the selected logger.
        LogClearResult clear_logger(int logger_index, const LogClearOptions& options = LogClearOptions()) {
            LogClearResult result;
            if (m_shutdown.load(std::memory_order_acquire)) {
                result.ok = false;
                result.status = LogClearStatus::Failed;
                result.message = "logger system is shut down";
                return result;
            }

            auto strategy = get_strategy_snapshot(logger_index);
            if (!strategy) {
                result.ok = false;
                result.status = LogClearStatus::Failed;
                result.message = "logger index not found";
                return result;
            }

            std::lock_guard<std::mutex> exec_lock(strategy->exec_mx);
            if (m_shutdown.load(std::memory_order_acquire)) {
                result.ok = false;
                result.status = LogClearStatus::Failed;
                result.message = "logger system is shut down";
                return result;
            }
            return strategy->logger->clear_logs(options);
        }

        /// \brief Clears logger-owned records for all registered loggers.
        /// \param options Data categories to clear.
        /// \return Aggregated cleanup result. Unsupported loggers are skipped.
        LogClearResult clear_all_loggers(const LogClearOptions& options = LogClearOptions()) {
            LogClearResult result;
            if (m_shutdown.load(std::memory_order_acquire)) {
                result.ok = false;
                result.status = LogClearStatus::Failed;
                result.message = "logger system is shut down";
                return result;
            }

            const auto snapshot = get_all_strategy_snapshots();
            std::size_t supported = 0;
            std::size_t failed = 0;
            std::size_t unsupported = 0;
            for (const auto& strategy : snapshot) {
                if (!strategy) continue;
                std::lock_guard<std::mutex> exec_lock(strategy->exec_mx);
                if (m_shutdown.load(std::memory_order_acquire)) {
                    result.ok = false;
                    result.status = LogClearStatus::Failed;
                    result.message = "logger system is shut down";
                    return result;
                }
                const LogClearResult one = strategy->logger->clear_logs(options);
                if (one.status == LogClearStatus::Cleared && one.ok) {
                    ++supported;
                    result.cleared_records += one.cleared_records;
                } else if (one.status == LogClearStatus::Unsupported) {
                    ++unsupported;
                } else {
                    ++failed;
                    if (result.message.empty()) {
                        result.message = one.message;
                    }
                }
            }

            result.ok = supported > 0 && failed == 0;
            if (result.ok) {
                result.status = LogClearStatus::Cleared;
                result.message = unsupported > 0 ? "cleared supported loggers; skipped unsupported loggers" : "cleared";
            } else {
                result.status = unsupported > 0 && failed == 0 ? LogClearStatus::Unsupported : LogClearStatus::Failed;
                if (result.message.empty()) {
                    result.message = unsupported > 0 ? "no logger supports clearing" : "no loggers to clear";
                }
            }
            return result;
        }

        /// \brief Returns the number of registered logger strategies.
        std::size_t logger_count() const {
            LoggerReadLock lock(m_loggers_mx);
            return m_loggers.size();
        }

        /// \brief Retrieves a typed backend pointer from a logger by index.
        template <typename LoggerT>
        LoggerT* get_logger_as(int logger_index) {
            auto strategy = get_strategy_snapshot(logger_index);
            return (strategy && strategy->logger)
                ? dynamic_cast<LoggerT*>(strategy->logger.get())
                : nullptr;
        }

        /// \brief Retrieves a typed backend pointer from a logger by index.
        template <typename LoggerT>
        const LoggerT* get_logger_as(int logger_index) const {
            auto strategy = get_strategy_snapshot(logger_index);
            return (strategy && strategy->logger)
                ? dynamic_cast<const LoggerT*>(strategy->logger.get())
                : nullptr;
        }

        /// \brief Shuts down logger system.
        ///
        /// Disables further logging, waits for asynchronous tasks to complete,
        /// and shuts down TaskExecutor.
        void shutdown() {
            if (m_shutdown.exchange(true, std::memory_order_acq_rel)) return;

            const auto snapshot = get_all_strategy_snapshots();
            for (const auto& strategy : snapshot) {
                if (!strategy) continue;
                std::lock_guard<std::mutex> exec_lock(strategy->exec_mx);
                strategy->logger->shutdown();
            }
            detail::TaskExecutor::get_instance().shutdown();
        }

    private:

        /// \struct LoggerStrategy
        /// \brief Structure to hold a logger-formatter pair.
        struct LoggerStrategy {
            std::unique_ptr<ILogger> logger;            ///< The logger instance.
            std::unique_ptr<ILogFormatter> formatter;   ///< The formatter instance.
            bool single_mode = false;                   ///< Flag indicating if the logger is in single mode.
            bool enabled = true;                        ///< Flag indicating if the logger is enabled.
            mutable std::mutex exec_mx;                 ///< Protects formatter+logger invocation.
        };

        void dispatch_to_strategy(LoggerStrategy& strategy, const LogRecord& record) {
            if (record.raw_mode) {
                strategy.logger->log(record, record.format);
                return;
            }
            if (strategy.formatter && strategy.formatter->is_passthrough()) {
                strategy.logger->log(record, record.format);
                return;
            }
            const std::string msg = strategy.formatter ? strategy.formatter->format(record) : std::string();
            strategy.logger->log(record, msg);
        }

        std::shared_ptr<LoggerStrategy> get_strategy_snapshot(int logger_index) const {
            LoggerReadLock lock(m_loggers_mx);
            if (logger_index >= 0 &&
                logger_index < static_cast<int>(m_loggers.size()) &&
                m_loggers[logger_index]) {
                return m_loggers[logger_index];
            }
            return std::shared_ptr<LoggerStrategy>();
        }

        std::vector<std::shared_ptr<LoggerStrategy>> get_all_strategy_snapshots() const {
            LoggerReadLock lock(m_loggers_mx);
            return m_loggers;
        }

        std::vector<std::shared_ptr<LoggerStrategy>> m_loggers;        ///< Container for logger-formatter pairs.
        mutable LoggerMutex m_loggers_mx;                        ///< Protects access to logger strategies.
        std::atomic<bool> m_shutdown = ATOMIC_VAR_INIT(false); ///< Flag indicating if shutdown was requested.

        void print(const LogRecord& record) {
            log(record);
        }
        
#ifdef _MSC_VER
#	pragma warning(push)
#	pragma warning(disable: 4127) // conditional expression is constant
#endif

        /// \brief Logs a record with given arguments.
        /// \tparam Ts Types of arguments.
        /// \param record Log record.
        /// \param args Arguments to be logged.
        template <typename... Ts>
        void print(const LogRecord& record, Ts const&... args) {
            if (sizeof...(Ts) == 0) {
                log(record);
                return;
            }
            // args_array is mutable cache inside LogRecord
            auto var_names = split_arguments(record.arg_names);
            record.args_array = args_to_array(var_names.begin(), args...);
            log(record);
        }
        
#ifdef _MSC_VER
#	pragma warning(pop)
#endif

        Logger() {
            std::atexit(Logger::on_exit_handler);
        }

        ~Logger() {
            shutdown();
        }

        // Deleting copy and move constructors and assignment operators to enforce singleton.
        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;
        Logger(Logger&&) = delete;
        Logger& operator=(Logger&&) = delete;

        /// \brief Atexit shutdown handler for Logger and TaskExecutor.
        /// \details Called automatically at program exit.
        static void on_exit_handler() {
            Logger::get_instance().shutdown();
        }
    };

}; // namespace logit

#endif // _LOGIT_LOGGER_HPP_INCLUDED
