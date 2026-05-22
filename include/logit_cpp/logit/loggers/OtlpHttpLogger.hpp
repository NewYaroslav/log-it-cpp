#pragma once
#ifndef _LOGIT_OTLP_HTTP_LOGGER_HPP_INCLUDED
#define _LOGIT_OTLP_HTTP_LOGGER_HPP_INCLUDED

/// \file OtlpHttpLogger.hpp
/// \brief OTLP/HTTP logger backend for exporting logs to OpenTelemetry collectors.

#ifndef LOGIT_WITH_OTLP
#   error "OtlpHttpLogger requires LOGIT_WITH_OTLP=1 and the optional kurlyk dependency. Enable LOGIT_WITH_OTLP in CMake."
#endif

#include "ILogger.hpp"
#include "otlp/OtlpCompression.hpp"
#include "otlp/OtlpJsonFormatConfig.hpp"
#include "otlp/OtlpJsonSerializer.hpp"
#include "otlp/OtlpPayloadSplitter.hpp"

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
#include <limits>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace logit {

    struct OtlpHttpLoggerState {
        std::mutex mutex;
        std::condition_variable cv;
        std::condition_variable space_cv;
        std::deque<OtlpLogItem> queue;
        std::size_t http_in_flight = 0;
        std::atomic<uint64_t> failed_exports{0};
        bool stopping = false;
    };

    /// \class OtlpHttpLogger
    /// \ingroup LogBackends
    /// \brief Exports logs to an OTLP/HTTP endpoint using kurlyk.
    ///
    /// This backend is an outbound OTLP exporter/client. It serializes records to
    /// OTLP/HTTP JSON and sends batches to an OpenTelemetry Collector-compatible endpoint.
    class OtlpHttpLogger final : public ILogger {
    public:
        struct Config {
            OtlpJsonFormatConfig format;
            std::string host = "http://localhost:4318";
            std::string path = "/v1/logs";
            std::size_t max_queue_size = 8192;
            std::size_t max_batch_size = 256;
            std::size_t max_in_flight_requests = 1;
            int export_interval_ms = 1000;
            int request_timeout_sec = 3;
            long retry_attempts = 2;
            long retry_delay_ms = 250;
            std::size_t max_payload_bytes = 1024 * 1024;
            OtlpCompression compression = OtlpCompression::None;
            int compression_level = 6;
            bool drop_on_overflow = true;
            bool async = true;
            bool cancel_on_shutdown = false;
        };

        /// \brief Constructs OTLP HTTP logger with default configuration.
        OtlpHttpLogger() : OtlpHttpLogger(Config()) {}

        /// \brief Constructs OTLP HTTP logger with custom configuration.
        /// \param config Export configuration.
        explicit OtlpHttpLogger(const Config& config)
            : m_config(config),
              m_client(config.host),
              m_state(std::make_shared<OtlpHttpLoggerState>()) {
            if (m_config.max_in_flight_requests == 0) {
                m_config.max_in_flight_requests = 1;
            }
            m_client.set_content_type("application/json");
            m_client.set_timeout(config.request_timeout_sec);
            m_client.set_retry_attempts(config.retry_attempts, config.retry_delay_ms);

            if (m_config.compression == OtlpCompression::Gzip) {
#if !defined(LOGIT_HAS_ZLIB)
                throw std::runtime_error("OtlpHttpLogger: gzip compression requested but LOGIT_WITH_GZIP is not enabled");
#endif
            }
            if (m_config.compression == OtlpCompression::Zstd) {
#if !defined(LOGIT_HAS_ZSTD)
                throw std::runtime_error("OtlpHttpLogger: zstd compression requested but LOGIT_WITH_ZSTD is not enabled");
#endif
            }

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

            if (m_state->stopping) {
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
                submit_batch_async(batch);
                try {
                    m_client.wait_requests();
                } catch (...) {
                }
                return;
            }

            for (;;) {
                std::unique_lock<std::mutex> lock(m_state->mutex);
                if (m_state->stopping) {
                    ++m_dropped;
                    return;
                }

                if (m_state->queue.size() >= m_config.max_queue_size) {
                    if (m_config.drop_on_overflow) {
                        ++m_dropped;
                        return;
                    }

                    lifecycle_lock.unlock();
                    m_state->space_cv.wait(lock, [this]() {
                        return m_state->stopping || m_state->queue.size() < m_config.max_queue_size;
                    });
                    lock.unlock();
                    lifecycle_lock.lock();

                    if (m_state->stopping) {
                        ++m_dropped;
                        return;
                    }

                    continue;
                }

                m_state->queue.push_back(item);
                lock.unlock();
                m_state->cv.notify_one();
                return;
            }
        }

        /// \brief Waits until queued and in-flight exports finish.
        void wait() override {
            if (!m_config.async) {
                return;
            }

            std::unique_lock<std::mutex> lock(m_state->mutex);
            m_state->cv.wait(lock, [this]() {
                return m_state->queue.empty() && m_state->http_in_flight == 0;
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
        Config m_config;       ///< Export configuration.
        kurlyk::HttpClient   m_client;       ///< HTTP client used for OTLP export.

        mutable std::mutex m_lifecycle_mutex;///< Serializes log() with shutdown.
        std::thread m_worker;                ///< Export worker thread.

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
                    std::unique_lock<std::mutex> lock(m_state->mutex);
                    m_state->cv.wait_for(
                        lock,
                        std::chrono::milliseconds(m_config.export_interval_ms),
                        [this]() {
                            return m_state->stopping ||
                                   (!m_state->queue.empty() &&
                                    m_state->http_in_flight < m_config.max_in_flight_requests);
                        });

                    if (m_state->stopping && m_state->queue.empty() && m_state->http_in_flight == 0) {
                        return;
                    }

                    if (m_state->stopping && m_config.cancel_on_shutdown) {
                        return;
                    }

                    if (m_state->queue.empty() ||
                        m_state->http_in_flight >= m_config.max_in_flight_requests) {
                        continue;
                    }

                    while (!m_state->queue.empty() && batch.size() < m_config.max_batch_size) {
                        batch.push_back(m_state->queue.front());
                        m_state->queue.pop_front();
                    }

                    m_state->space_cv.notify_all();
                }

                if (!batch.empty()) {
                    submit_batch_async(batch);
                }
            }
        }

        /// \brief Submits one batch asynchronously, splitting into payload chunks.
        /// \param batch Batch to export.
        void submit_batch_async(const std::vector<OtlpLogItem>& batch) {
            auto chunks = build_otlp_logs_json_payload_chunks(
                batch, m_config.format, m_config.max_payload_bytes);

            if (chunks.empty()) {
                return;
            }

            kurlyk::Headers headers;
            headers.emplace("Content-Type", "application/json");

            auto weak_state = std::weak_ptr<OtlpHttpLoggerState>(m_state);

            for (auto& chunk : chunks) {
                std::string post_content;
                kurlyk::Headers chunk_headers = headers;

                if (m_config.compression == OtlpCompression::Gzip) {
                    if (!compress_string_gzip(chunk, post_content, m_config.compression_level)) {
                        post_content = std::move(chunk);
                    } else {
                        chunk_headers.emplace("Content-Encoding", "gzip");
                    }
                } else if (m_config.compression == OtlpCompression::Zstd) {
                    if (!compress_string_zstd(chunk, post_content, m_config.compression_level)) {
                        post_content = std::move(chunk);
                    } else {
                        chunk_headers.emplace("Content-Encoding", "zstd");
                    }
                } else {
                    post_content = std::move(chunk);
                }

                {
                    std::unique_lock<std::mutex> lock(m_state->mutex);
                    m_state->cv.wait(lock, [this]() {
                        return (m_state->stopping && m_config.cancel_on_shutdown) ||
                               m_state->http_in_flight < m_config.max_in_flight_requests;
                    });

                    if (m_state->stopping && m_config.cancel_on_shutdown) {
                        m_state->failed_exports.fetch_add(1);
                        continue;
                    }

                    ++m_state->http_in_flight;
                }

                bool submitted = false;

                try {
                    submitted = m_client.post(
                        m_config.path, {}, chunk_headers, std::move(post_content),
                        [weak_state](kurlyk::HttpResponsePtr response) {
                            auto state = weak_state.lock();
                            if (!state) {
                                return;
                            }

                            std::lock_guard<std::mutex> lock(state->mutex);
                            if (!response || response->status_code < 200 || response->status_code >= 300) {
                                state->failed_exports.fetch_add(1);
                            }
                            if (state->http_in_flight > 0) {
                                --state->http_in_flight;
                            }
                            state->cv.notify_all();
                        });
                } catch (...) {
                    submitted = false;
                }

                if (!submitted) {
                    auto state = m_state;
                    std::lock_guard<std::mutex> lock(state->mutex);
                    state->failed_exports.fetch_add(1);
                    if (state->http_in_flight > 0) {
                        --state->http_in_flight;
                    }
                    state->cv.notify_all();
                }
            }
        }

        /// \brief Stops worker and cancels pending requests.
        void stop() {
            {
                std::lock_guard<std::mutex> lifecycle_lock(m_lifecycle_mutex);
                std::lock_guard<std::mutex> lock(m_state->mutex);
                if (m_state->stopping) {
                    return;
                }
                m_state->stopping = true;

                if (m_config.cancel_on_shutdown) {
                    m_dropped.fetch_add(m_state->queue.size());
                    m_state->queue.clear();
                    m_state->space_cv.notify_all();
                }
            }

            if (m_config.cancel_on_shutdown) {
                try {
                    m_client.cancel_requests();
                } catch (...) {
                }
            }

            m_state->cv.notify_all();
            m_state->space_cv.notify_all();

            if (m_worker.joinable()) {
                m_worker.join();
            }

            if (!m_config.cancel_on_shutdown) {
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
