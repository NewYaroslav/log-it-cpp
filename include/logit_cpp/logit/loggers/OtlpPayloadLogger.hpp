#pragma once
#ifndef _LOGIT_OTLP_PAYLOAD_LOGGER_HPP_INCLUDED
#define _LOGIT_OTLP_PAYLOAD_LOGGER_HPP_INCLUDED

/// \file OtlpPayloadLogger.hpp
/// \brief OTLP payload callback logger backend for exporting logs via user-provided callback.

#ifndef LOGIT_WITH_OTLP
#   error "OtlpPayloadLogger requires LOGIT_WITH_OTLP=1. Enable LOGIT_WITH_OTLP in CMake."
#endif

#include "ILogger.hpp"
#include "otlp/OtlpJsonFormatConfig.hpp"
#include "otlp/OtlpJsonSerializer.hpp"
#include "otlp/OtlpPayloadSplitter.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <limits>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace logit {

    /// \class OtlpPayloadLogger
    /// \ingroup LogBackends
    /// \brief Exports logs as OTLP JSON payloads via a user-provided callback.
    ///
    /// This backend serializes records to OTLP/HTTP JSON and passes each batch
    /// to a user-provided callback instead of sending HTTP itself.
    class OtlpPayloadLogger final : public ILogger {
    public:
        struct Config {
            OtlpJsonFormatConfig format;
            std::function<void(std::string)> on_payload;
            bool async = true;
            std::size_t max_payload_bytes = 1024 * 1024;
            std::size_t max_batch_size = 256;
            std::size_t max_queue_size = 1024;
            bool drop_on_overflow = true;
            unsigned export_interval_ms = 100;
        };

        /// \brief Constructs OTLP payload logger with default configuration.
        OtlpPayloadLogger() : OtlpPayloadLogger(Config()) {}

        /// \brief Constructs OTLP payload logger with custom configuration.
        /// \param config Export configuration.
        explicit OtlpPayloadLogger(const Config& config)
            : m_config(config) {
            if (m_config.async) {
                m_worker = std::thread(&OtlpPayloadLogger::worker_loop, this);
            }
        }

        /// \brief Stops worker and drains queue.
        ~OtlpPayloadLogger() override {
            stop();
        }

        OtlpPayloadLogger(const OtlpPayloadLogger&) = delete;
        OtlpPayloadLogger& operator=(const OtlpPayloadLogger&) = delete;

        /// \brief Queues or exports a log message.
        /// \param record Structured log record.
        /// \param message Formatted log message used as OTLP body.
        void log(const LogRecord& record, const std::string& message) override {
            if (!m_config.on_payload) {
                return;
            }

            m_last_log_ts = record.timestamp_ms;

            OtlpLogItem item;
            item.record = make_otlp_record_snapshot(record);
            item.message = message;

            if (!m_config.async) {
                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    if (m_stopping) {
                        ++m_dropped;
                        return;
                    }
                }
                std::vector<OtlpLogItem> batch;
                batch.push_back(item);
                auto chunks = build_otlp_logs_json_payload_chunks(
                    batch, m_config.format, m_config.max_payload_bytes);
                try {
                    if (m_config.on_payload) {
                        for (auto& chunk : chunks) {
                            m_config.on_payload(std::move(chunk));
                        }
                    }
                } catch (...) {
                    ++m_failed_exports;
                }
                return;
            }

            std::unique_lock<std::mutex> lock(m_mutex);
            if (m_stopping) {
                ++m_dropped;
                return;
            }

            if (m_queue.size() >= m_config.max_queue_size) {
                if (m_config.drop_on_overflow) {
                    ++m_dropped;
                    return;
                }

                m_space_cv.wait(lock, [this]() {
                    return m_stopping || m_queue.size() < m_config.max_queue_size;
                });

                if (m_stopping) {
                    ++m_dropped;
                    return;
                }
            }

            m_queue.push_back(item);
            lock.unlock();
            m_cv.notify_one();
        }

        /// \brief Waits until queue is empty and worker is idle.
        void wait() override {
            if (!m_config.async) {
                return;
            }

            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this]() {
                return m_queue.empty() && m_idle;
            });
        }

        /// \brief Stops the OTLP worker after draining queued items.
        void shutdown() override {
            stop();
        }

        /// \brief Retrieves a string parameter from the logger.
        /// \param param Parameter to retrieve.
        /// \return Parameter value, or empty string when unsupported.
        std::string get_string_param(const LoggerParam& param) const override {
            switch (param) {
            case LoggerParam::LastLogTimestamp: return std::to_string(get_last_log_ts());
            case LoggerParam::TimeSinceLastLog: return std::to_string(get_time_since_last_log());
            case LoggerParam::DroppedLogCount: return std::to_string(dropped_count());
            case LoggerParam::FailedExportCount: return std::to_string(failed_export_count());
            default:
                break;
            }
            return std::string();
        }

        /// \brief Retrieves an integer parameter from the logger.
        /// \param param Parameter to retrieve.
        /// \return Parameter value, or 0 when unsupported.
        int64_t get_int_param(const LoggerParam& param) const override {
            switch (param) {
            case LoggerParam::LastLogTimestamp: return get_last_log_ts();
            case LoggerParam::TimeSinceLastLog: return get_time_since_last_log();
            case LoggerParam::DroppedLogCount: return counter_to_int64(dropped_count());
            case LoggerParam::FailedExportCount: return counter_to_int64(failed_export_count());
            default:
                break;
            }
            return 0;
        }

        /// \brief Retrieves a floating-point parameter from the logger.
        /// \param param Parameter to retrieve.
        /// \return Parameter value in seconds for time params, raw count for counter params, or 0.0 when unsupported.
        double get_float_param(const LoggerParam& param) const override {
            switch (param) {
            case LoggerParam::LastLogTimestamp:
                return static_cast<double>(get_last_log_ts()) / 1000.0;
            case LoggerParam::TimeSinceLastLog:
                return static_cast<double>(get_time_since_last_log()) / 1000.0;
            case LoggerParam::DroppedLogCount:
                return static_cast<double>(dropped_count());
            case LoggerParam::FailedExportCount:
                return static_cast<double>(failed_export_count());
            default:
                break;
            }
            return 0.0;
        }

        /// \brief Sets minimal log level for this logger.
        /// \param level Minimum log level.
        void set_log_level(LogLevel level) override {
            m_log_level = static_cast<int>(level);
        }

        /// \brief Gets minimal log level for this logger.
        /// \return Current minimal log level.
        LogLevel get_log_level() const override {
            return static_cast<LogLevel>(m_log_level.load());
        }

        /// \brief Returns number of dropped records.
        /// \return Dropped record count.
        uint64_t dropped_count() const {
            return m_dropped.load();
        }

        /// \brief Returns number of failed export attempts.
        /// \return Failed export count.
        uint64_t failed_export_count() const {
            return m_failed_exports.load();
        }

    private:
        Config m_config;

        std::mutex m_mutex;
        std::condition_variable m_cv;
        std::condition_variable m_space_cv;
        std::deque<OtlpLogItem> m_queue;
        bool m_stopping = false;
        bool m_idle = true;

        std::thread m_worker;

        std::atomic<int> m_log_level = ATOMIC_VAR_INIT(static_cast<int>(LogLevel::LOG_LVL_TRACE));
        std::atomic<int64_t> m_last_log_ts = ATOMIC_VAR_INIT(0);
        std::atomic<uint64_t> m_dropped = ATOMIC_VAR_INIT(0);
        std::atomic<uint64_t> m_failed_exports = ATOMIC_VAR_INIT(0);

        /// \brief Worker thread loop.
        void worker_loop() {
            while (true) {
                std::vector<OtlpLogItem> batch;
                batch.reserve(m_config.max_batch_size);

                {
                    std::unique_lock<std::mutex> lock(m_mutex);
                    m_cv.wait_for(
                        lock,
                        std::chrono::milliseconds(m_config.export_interval_ms),
                        [this]() {
                            return m_stopping || !m_queue.empty();
                        });

                    if (m_stopping && m_queue.empty()) {
                        m_idle = true;
                        m_cv.notify_all();
                        return;
                    }

                    if (m_queue.empty()) {
                        continue;
                    }

                    m_idle = false;

                    while (!m_queue.empty() && batch.size() < m_config.max_batch_size) {
                        batch.push_back(m_queue.front());
                        m_queue.pop_front();
                    }

                    m_space_cv.notify_all();
                }

                if (!batch.empty()) {
                    auto chunks = build_otlp_logs_json_payload_chunks(
                        batch, m_config.format, m_config.max_payload_bytes);
                    try {
                        if (m_config.on_payload) {
                            for (auto& chunk : chunks) {
                                m_config.on_payload(std::move(chunk));
                            }
                        }
                    } catch (...) {
                        ++m_failed_exports;
                    }
                }

                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    m_idle = true;
                }
                m_cv.notify_all();
            }
        }

        /// \brief Stops worker and drains remaining queue.
        void stop() {
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                if (m_stopping) {
                    return;
                }
                m_stopping = true;
            }

            m_cv.notify_all();
            m_space_cv.notify_all();

            if (m_worker.joinable()) {
                m_worker.join();
            }
        }

        /// \brief Returns last log timestamp.
        /// \return Last log timestamp in milliseconds.
        int64_t get_last_log_ts() const {
            return m_last_log_ts.load();
        }

        /// \brief Returns elapsed time since last log.
        /// \return Elapsed time in milliseconds.
        int64_t get_time_since_last_log() const {
            const int64_t last = get_last_log_ts();
            if (last <= 0) {
                return 0;
            }
            const int64_t now = LOGIT_CURRENT_TIMESTAMP_MS();
            return now > last ? now - last : 0;
        }

        /// \brief Converts unsigned counter value to int64_t with saturation.
        /// \param value Counter value.
        /// \return Counter value clamped to int64_t max.
        static int64_t counter_to_int64(uint64_t value) {
            const uint64_t max_value = static_cast<uint64_t>((std::numeric_limits<int64_t>::max)());
            return value > max_value ? (std::numeric_limits<int64_t>::max)() : static_cast<int64_t>(value);
        }
    };

} // namespace logit

#endif // _LOGIT_OTLP_PAYLOAD_LOGGER_HPP_INCLUDED
