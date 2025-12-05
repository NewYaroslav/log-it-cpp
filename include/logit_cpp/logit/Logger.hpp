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
            if (m_shutdown) return;
            auto strategy = std::make_shared<LoggerStrategy>();
            strategy->logger = std::move(logger);
            strategy->formatter = std::move(formatter);
            strategy->single_mode = single_mode;
            strategy->enabled = true;

            LoggerWriteLock lock(m_loggers_mx);
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
            if (m_shutdown) return;

            const bool targeted = record.logger_index >= 0;

            std::vector<std::shared_ptr<LoggerStrategy>> snapshot;
            snapshot.reserve(targeted ? 1 : 0);

            LoggerReadLock lock(m_loggers_mx);
            if (targeted) {
                if (record.logger_index < (int)m_loggers.size())
                    snapshot.push_back(m_loggers[record.logger_index]);
            } else {
                snapshot = m_loggers; // copy shared_ptrs
            }
            lock.unlock();

            if (targeted) {
                if (snapshot.empty() || !snapshot[0]) return;
                auto& strategy = snapshot[0];

                std::lock_guard<std::mutex> exec_lock(strategy->exec_mx);
                if (!strategy->enabled) return;
                if ((int)record.log_level < (int)strategy->logger->get_log_level()) return;
                dispatch_to_strategy(*strategy, record);
                return;
            }

            for (const auto& strategy : snapshot) {
                if (!strategy) continue;

                std::lock_guard<std::mutex> exec_lock(strategy->exec_mx);
                if (strategy->single_mode) continue;
                if (!strategy->enabled) continue;
                if ((int)record.log_level < (int)strategy->logger->get_log_level()) continue;

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
            LoggerReadLock lock(m_loggers_mx);
            for (const auto& strategy : m_loggers) {
                if (!strategy) continue;
                strategy->logger->wait();
            }
        }

        /// \brief Shuts down logger system.
        ///
        /// Disables further logging, waits for asynchronous tasks to complete,
        /// and shuts down TaskExecutor.
        void shutdown() {
            if (m_shutdown) return;
            m_shutdown = true;
            wait();
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
            if (strategy.formatter && strategy.formatter->is_passthrough()) {
                strategy.logger->log(record, record.format);
                return;
            }
            const std::string msg = strategy.formatter ? strategy.formatter->format(record) : std::string();
            strategy.logger->log(record, msg);
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
