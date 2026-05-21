#pragma once
#ifndef _LOGIT_OTLP_HTTP_LOGGER_HPP_INCLUDED
#define _LOGIT_OTLP_HTTP_LOGGER_HPP_INCLUDED

/// \file OtlpHttpLogger.hpp
/// \brief OTLP/HTTP logger backend for exporting logs to OpenTelemetry collectors.

#ifndef LOGIT_WITH_OTLP
#   error "OtlpHttpLogger requires LOGIT_WITH_OTLP=1 and the optional kurlyk dependency. Enable LOGIT_WITH_OTLP in CMake."
#endif

#include "ILogger.hpp"
#include "otlp/OtlpHttpLoggerConfig.hpp"
#include "otlp/OtlpJsonSerializer.hpp"

#ifndef KURLYK_WEBSOCKET_SUPPORT
#   define KURLYK_WEBSOCKET_SUPPORT 0
#endif
#ifndef KURLYK_HTTP_SUPPORT
#   define KURLYK_HTTP_SUPPORT 1
#endif
#include <kurlyk.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <future>
#include <limits>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace logit {

    struct OtlpHttpLoggerState {
        std::mutex mutex;
        std::condition_variable cv;
        std::atomic<std::size_t> http_in_flight{0};
        std::atomic<uint64_t> failed_exports{0};
    };

    /// \class OtlpHttpLogger
    /// \ingroup LogBackends
    /// \brief Exports logs to an OTLP/HTTP endpoint using kurlyk.
    ///
    /// This backend is an outbound OTLP exporter/client. It serializes records to
    /// OTLP/HTTP JSON and sends batches to an OpenTelemetry Collector-compatible endpoint.
    class OtlpHttpLogger final : public ILogger {
    public:
        /// \brief Constructs OTLP HTTP logger with default configuration.
        OtlpHttpLogger() : OtlpHttpLogger(OtlpHttpLoggerConfig()) {}

        /// \brief Constructs OTLP HTTP logger with custom configuration.
        /// \param config Export configuration.
        explicit OtlpHttpLogger(const OtlpHttpLoggerConfig& config)
            : m_config(config),
              m_client(config.host),
              m_state(std::make_shared<OtlpHttpLoggerState>()) {
            m_client.set_content_type("application/json");
            m_client.set_timeout(config.request_timeout_sec);
            m_client.set_retry_attempts(config.retry_attempts, config.retry_delay_ms);

            if (m_config.async) {
                m_worker = std::thread(&OtlpHttpLogger::worker_loop, this);
            }
        }

        /// \brief Stops worker and cancels pending HTTP requests.
        ~OtlpHttpLogger() override {
            stop();
        }

        OtlpHttpLogger(const OtlpHttpLogger&) = delete;
        OtlpHttpLogger& operator=(const OtlpHttpLogger&) = delete;

        /// \brief Queues or exports a log message.
        /// \param record Structured log record.
        /// \param message Formatted log message used as OTLP body.
        void log(const LogRecord& record, const std::string& message) override {
            std::unique_lock<std::mutex> lifecycle_lock(m_lifecycle_mutex);

            if (m_stopping) {
                ++m_dropped;
                return;
            }
            m_last_log_ts = record.timestamp_ms;

            OtlpLogItem item;
            item.record = make_otlp_record_snapshot(record);
            item.message = message;

            if (!m_config.async) {
                std::vector<OtlpLogItem> batch;
                batch.push_back(item);
                m_state->http_in_flight.fetch_add(1);
                submit_batch_async(batch);
                try {
                    m_client.wait_requests();
                } catch (...) {
                }
                return;
            }

            for (;;) {
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

                    lifecycle_lock.unlock();
                    m_cv_space.wait(lock, [this]() {
                        return m_stopping || m_queue.size() < m_config.max_queue_size;
                    });
                    lock.unlock();
                    lifecycle_lock.lock();

                    if (m_stopping) {
                        ++m_dropped;
                        return;
                    }

                    continue;
                }

                m_queue.push_back(item);
                lock.unlock();
                m_cv.notify_one();
                return;
            }
        }

        /// \brief Waits until queued and in-flight exports finish.
        void wait() override {
            if (!m_config.async) {
                return;
            }

            std::unique_lock<std::mutex> state_lock(m_state->mutex);
            m_state->cv.wait(state_lock, [this, state = m_state.get()]() {
                if (state->http_in_flight.load() > 0) {
                    return false;
                }
                std::lock_guard<std::mutex> queue_lock(m_mutex);
                return m_queue.empty();
            });
        }

        /// \brief Stops the OTLP worker after draining queued exports.
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
            return m_state->failed_exports.load();
        }

    private:
        OtlpHttpLoggerConfig m_config;       ///< Export configuration.
        kurlyk::HttpClient   m_client;       ///< HTTP client used for OTLP export.

        mutable std::mutex m_lifecycle_mutex;///< Serializes log() with shutdown.
        mutable std::mutex m_mutex;          ///< Protects queue and worker state.
        std::condition_variable m_cv;        ///< Signals queued records.
        std::condition_variable m_cv_space;  ///< Signals available queue space.
        std::condition_variable m_cv_drained;///< Signals drained queue and exports.
        std::deque<OtlpLogItem> m_queue;     ///< Pending records.

        std::thread m_worker;                ///< Export worker thread.
        bool m_stopping = false;             ///< Stop flag protected by lifecycle/mutex locks.
        std::shared_ptr<OtlpHttpLoggerState> m_state;

        std::atomic<int> m_log_level = ATOMIC_VAR_INIT(static_cast<int>(LogLevel::LOG_LVL_TRACE));
        std::atomic<int64_t> m_last_log_ts = ATOMIC_VAR_INIT(0);
        std::atomic<uint64_t> m_dropped = ATOMIC_VAR_INIT(0);

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
                            return m_stopping ||
                                   (!m_queue.empty() &&
                                    m_state->http_in_flight.load() < m_config.max_in_flight_requests);
                        });

                    if (m_stopping && m_queue.empty() && m_state->http_in_flight.load() == 0) {
                        m_cv_drained.notify_all();
                        return;
                    }

                    if (m_queue.empty() ||
                        m_state->http_in_flight.load() >= m_config.max_in_flight_requests) {
                        continue;
                    }

                    while (!m_queue.empty() && batch.size() < m_config.max_batch_size) {
                        batch.push_back(m_queue.front());
                        m_queue.pop_front();
                    }

                    m_cv_space.notify_all();
                    m_state->http_in_flight.fetch_add(1);
                }

                if (!batch.empty()) {
                    submit_batch_async(batch);
                }
            }
        }

        /// \brief Submits one batch asynchronously to the configured OTLP endpoint.
        /// \param batch Batch to export.
        void submit_batch_async(const std::vector<OtlpLogItem>& batch) {
            const std::string payload = build_otlp_logs_json_payload(batch, m_config);
            kurlyk::Headers headers;
            headers.emplace("Content-Type", "application/json");

            auto weak_state = std::weak_ptr<OtlpHttpLoggerState>(m_state);
            bool submitted = false;

            try {
                submitted = m_client.post(
                    m_config.path, {}, headers, payload,
                    [weak_state](kurlyk::HttpResponsePtr response) {
                        auto state = weak_state.lock();
                        if (!state) {
                            return;
                        }

                        std::lock_guard<std::mutex> lock(state->mutex);
                        if (!response || response->status_code < 200 || response->status_code >= 300) {
                            state->failed_exports.fetch_add(1);
                        }
                        state->http_in_flight.fetch_sub(1);
                        state->cv.notify_all();
                    });
            } catch (...) {
                submitted = false;
            }

            if (!submitted) {
                auto state = m_state;
                std::lock_guard<std::mutex> lock(state->mutex);
                state->failed_exports.fetch_add(1);
                state->http_in_flight.fetch_sub(1);
                state->cv.notify_all();
            }
        }

        /// \brief Stops worker and cancels pending requests.
        void stop() {
            std::lock_guard<std::mutex> lifecycle_lock(m_lifecycle_mutex);
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                if (m_stopping) {
                    return;
                }
                m_stopping = true;
            }

            m_cv.notify_all();
            m_cv_space.notify_all();

            if (m_worker.joinable()) {
                m_worker.join();
            }

            if (m_config.cancel_on_shutdown) {
                try {
                    m_client.cancel_requests();
                } catch (...) {
                }
            } else {
                try {
                    m_client.wait_requests();
                } catch (...) {
                }
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

#endif // _LOGIT_OTLP_HTTP_LOGGER_HPP_INCLUDED
