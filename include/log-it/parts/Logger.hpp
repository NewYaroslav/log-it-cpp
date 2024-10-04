#pragma once
#ifndef _LOGIT_LOGGER_HPP_INCLUDED
#define _LOGIT_LOGGER_HPP_INCLUDED
/// \file Logger.hpp
/// \brief Defines the Logger class for managing multiple loggers and formatters.

#include "Logger/ILogger.hpp"
#include "Formatter/ILogFormatter.hpp"
#include <memory>
#include <mutex>
#include <sstream>

namespace logit {

    /// \class Logger
    /// \brief Singleton class that manages multiple loggers and formatters.
    ///
    /// The `Logger` class allows adding multiple logger and formatter pairs (strategies)
    /// and provides methods to log messages using these strategies. It supports both
    /// synchronous and asynchronous logging. The class is thread-safe.
    class Logger {
    public:

        /// \brief Retrieves the singleton instance of `Logger`.
        /// \return A reference to the single instance of the `Logger` class.
        static Logger& get_instance() {
            static Logger instance;
            return instance;
        }

        /// \brief Waits for all asynchronous loggers to finish processing.
        ///
        /// This method ensures that all log messages are fully processed before continuing.
        /// It calls the `wait()` function of each logger.
        void wait() {
            for (const auto& strategy : m_loggers) {
                strategy.logger->wait();
            }
        }

        /// \brief Adds a logger and its corresponding formatter.
        /// \param logger A unique pointer to the logger instance.
        /// \param formatter A unique pointer to the formatter instance.
        void add_logger(
                std::unique_ptr<ILogger> logger,
                std::unique_ptr<ILogFormatter> formatter) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_loggers.push_back(LoggerStrategy{std::move(logger), std::move(formatter)});
        }

        /// \brief Logs a `LogRecord` using all added loggers and formatters.
        ///
        /// The `log()` method formats the log message using each logger's corresponding formatter
        /// and sends the formatted message to each logger.
        ///
        /// \param record The log record to be logged.
        void log(const LogRecord& record) {
            std::lock_guard<std::mutex> lock(m_mutex);
            // Log to the specific logger if the index is valid
            if (record.logger_index >= 0 && record.logger_index < static_cast<int>(m_loggers.size())) {
                const auto& strategy = m_loggers[record.logger_index];
                strategy.logger->log(record, strategy.formatter->format(record));
                return;
            }
            for (const auto& strategy : m_loggers) {
                strategy.logger->log(record, strategy.formatter->format(record));
            }
        }

        /// \brief Logs the message and returns a tuple of arguments.
        /// \tparam Ts Types of the arguments.
        /// \param record The log record.
        /// \param args The arguments to be logged.
        /// \return A tuple containing the arguments.
        template <typename... Ts>
        auto log_and_return(const LogRecord& record, Ts&&... args) -> decltype(std::forward_as_tuple(std::forward<Ts>(args)...)) {
            this->print(record, args...);
            return std::forward_as_tuple(std::forward<Ts>(args)...);
        }

        /// \brief Logs the message and returns a single argument.
        /// \tparam T Type of the argument.
        /// \param record The log record.
        /// \param arg The argument to be logged.
        /// \return The logged argument.
        template <typename T>
        auto log_and_return(const LogRecord& record, T&& arg) -> decltype(arg) {
            this->print(record, arg);
            return std::forward<decltype(arg)>(arg);
        }

        /// \brief Logs a message without arguments and returns an empty tuple.
        /// \param record The log record.
        /// \return An empty tuple.
        auto log_and_return(const LogRecord& record) -> std::tuple<> {
            this->print(record);
            return {};
        }

    private:

        /// \struct LoggerStrategy
        /// \brief Structure to hold a logger-formatter pair.
        struct LoggerStrategy {
            std::unique_ptr<ILogger> logger;            ///< The logger instance.
            std::unique_ptr<ILogFormatter> formatter;   ///< The formatter instance.
        };

        std::vector<LoggerStrategy> m_loggers;  ///< Container for logger-formatter pairs.
        std::mutex m_mutex;                     ///< Mutex for thread safety during logging operations.

        /// \brief Logs a record with the given arguments.
        /// \tparam Ts Types of the arguments.
        /// \param record The log record.
        /// \param args The arguments to be logged.
        template <typename... Ts>
        void print(const LogRecord& record, Ts const&... args) {
            if (sizeof...(Ts) == 0) {
                log(record);
                return;
            }
            LogRecord mutable_record = record;
            auto var_names = split_arguments(mutable_record.arg_names);
            mutable_record.args_array = args_to_array(var_names.begin(), args...);
            log(mutable_record);
        }

        /// \brief Private constructor to enforce the singleton pattern.
        Logger() = default;

        ~Logger() {
            wait();
        }

        // Deleting copy and move constructors and assignment operators to enforce singleton.
        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;
        Logger(Logger&&) = delete;
        Logger& operator=(Logger&&) = delete;
    };

}; // namespace logit

#endif // _LOGIT_LOGGER_HPP_INCLUDED
