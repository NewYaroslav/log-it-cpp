#pragma once
#ifndef _LOGIT_PROMETHEUS_PAYLOAD_LOGGER_HPP_INCLUDED
#define _LOGIT_PROMETHEUS_PAYLOAD_LOGGER_HPP_INCLUDED

/// \file PrometheusPayloadLogger.hpp
/// \brief Prometheus text payload callback logger backend.

#ifndef LOGIT_WITH_PROMETHEUS
#   error "PrometheusPayloadLogger requires LOGIT_WITH_PROMETHEUS=1. Enable LOGIT_WITH_PROMETHEUS in CMake."
#endif

#include "ILogger.hpp"
#include "prometheus/PrometheusTextFormatConfig.hpp"
#include "prometheus/PrometheusTextSerializer.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <limits>
#include <mutex>
#include <string>
#include <vector>

namespace logit {

    /// \class PrometheusPayloadLogger
    /// \ingroup LogBackends
    /// \brief Exposes log metrics as Prometheus text exposition format via user-provided callback.
    ///
    /// This backend tracks internal counters and gauges (log records total, dropped,
    /// failed exports, last log timestamp, time since last log) and serializes them
    /// to Prometheus text exposition format. The payload is passed to a user-provided
    /// callback on wait() or on-demand via collect_payload().
    class PrometheusPayloadLogger final : public ILogger {
    public:
        struct Config {
            PrometheusTextFormatConfig format;
            std::function<void(std::string)> on_payload;
            std::function<void(std::vector<PrometheusMetricFamily>&)> on_collect;
            bool emit_on_log = false;
            bool emit_on_wait = true;
        };

        /// \brief Constructs Prometheus payload logger with default configuration.
        PrometheusPayloadLogger() : PrometheusPayloadLogger(Config()) {}

        /// \brief Constructs Prometheus payload logger with custom configuration.
        /// \param config Export configuration.
        explicit PrometheusPayloadLogger(const Config& config)
            : m_config(config) {}

        ~PrometheusPayloadLogger() override {
            stop();
        }

        PrometheusPayloadLogger(const PrometheusPayloadLogger&) = delete;
        PrometheusPayloadLogger& operator=(const PrometheusPayloadLogger&) = delete;

        /// \brief Updates internal metric counters for a log message.
        /// \param record Structured log record.
        /// \param message Formatted log message (unused by Prometheus metrics).
        void log(const LogRecord& record, const std::string& message) override {
            (void)message;
            m_last_log_ts.store(record.timestamp_ms);
            ++m_log_records_total;

            if (m_config.emit_on_log && m_config.on_payload) {
                try {
                    std::string payload = collect_payload();
                    m_config.on_payload(std::move(payload));
                } catch (...) {
                    ++m_failed_collects;
                }
            }
        }

        /// \brief If emit_on_wait, collects metrics and invokes on_payload callback.
        void wait() override {
            if (m_config.emit_on_wait && m_config.on_payload) {
                try {
                    std::string payload = collect_payload();
                    m_config.on_payload(std::move(payload));
                } catch (...) {
                    ++m_failed_collects;
                }
            }
        }

        /// \brief Stops the logger (no worker thread to drain for payload logger).
        void shutdown() override {
            stop();
        }

        /// \brief Collects current metrics and returns serialized Prometheus text payload.
        /// \return Complete Prometheus text exposition format string.
        std::string collect_payload() {
            std::vector<PrometheusMetricFamily> families;
            {
                std::lock_guard<std::mutex> lock(m_collect_mutex);
                build_builtin_metrics(families);
                if (m_config.on_collect) {
                    try {
                        m_config.on_collect(families);
                    } catch (...) {
                        ++m_failed_collects;
                    }
                }
            }
            return build_prometheus_text_payload(families, m_config.format);
        }

        /// \brief Retrieves a string parameter from the logger.
        /// \param param Parameter to retrieve.
        /// \return Parameter value, or empty string when unsupported.
        std::string get_string_param(const LoggerParam& param) const override {
            switch (param) {
            case LoggerParam::LastLogTimestamp: return std::to_string(get_last_log_ts());
            case LoggerParam::TimeSinceLastLog: return std::to_string(get_time_since_last_log());
            case LoggerParam::DroppedLogCount: return std::to_string(m_dropped.load());
            case LoggerParam::FailedExportCount: return std::to_string(m_failed_collects.load());
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
            case LoggerParam::DroppedLogCount: return counter_to_int64(m_dropped.load());
            case LoggerParam::FailedExportCount: return counter_to_int64(m_failed_collects.load());
            default:
                break;
            }
            return 0;
        }

        /// \brief Retrieves a floating-point parameter from the logger.
        /// \param param Parameter to retrieve.
        /// \return Parameter value in seconds for time params, or 0.0 when unsupported.
        double get_float_param(const LoggerParam& param) const override {
            switch (param) {
            case LoggerParam::LastLogTimestamp:
                return static_cast<double>(get_last_log_ts()) / 1000.0;
            case LoggerParam::TimeSinceLastLog:
                return static_cast<double>(get_time_since_last_log()) / 1000.0;
            case LoggerParam::DroppedLogCount:
                return static_cast<double>(m_dropped.load());
            case LoggerParam::FailedExportCount:
                return static_cast<double>(m_failed_collects.load());
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

    private:
        Config m_config;
        std::mutex m_collect_mutex;
        bool m_stopped = false;

        std::atomic<int> m_log_level = ATOMIC_VAR_INIT(static_cast<int>(LogLevel::LOG_LVL_TRACE));
        std::atomic<int64_t> m_last_log_ts = ATOMIC_VAR_INIT(0);
        std::atomic<uint64_t> m_log_records_total = ATOMIC_VAR_INIT(0);
        std::atomic<uint64_t> m_dropped = ATOMIC_VAR_INIT(0);
        std::atomic<uint64_t> m_failed_collects = ATOMIC_VAR_INIT(0);

        void stop() {
            std::lock_guard<std::mutex> lock(m_collect_mutex);
            m_stopped = true;
        }

        int64_t get_last_log_ts() const {
            return m_last_log_ts.load();
        }

        int64_t get_time_since_last_log() const {
            const int64_t last = get_last_log_ts();
            if (last <= 0) {
                return 0;
            }
            const int64_t now = LOGIT_CURRENT_TIMESTAMP_MS();
            return now > last ? now - last : 0;
        }

        static int64_t counter_to_int64(uint64_t value) {
            const uint64_t max_value = static_cast<uint64_t>((std::numeric_limits<int64_t>::max)());
            return value > max_value ? (std::numeric_limits<int64_t>::max)() : static_cast<int64_t>(value);
        }

        void build_builtin_metrics(std::vector<PrometheusMetricFamily>& families) const {
            const std::string& prefix = m_config.format.metric_prefix;

            // logit_log_records_total (counter)
            {
                PrometheusMetricFamily mf;
                mf.name = prefix + "log_records_total";
                mf.help = "Total number of log records processed";
                mf.type = PrometheusMetricType::Counter;
                PrometheusSample s;
                s.name = prefix + "log_records_total";
                s.value = static_cast<double>(m_log_records_total.load());
                add_common_labels(s);
                mf.samples.push_back(s);
                families.push_back(mf);
            }

            // logit_dropped_logs_total (counter)
            {
                PrometheusMetricFamily mf;
                mf.name = prefix + "dropped_logs_total";
                mf.help = "Total number of dropped log records";
                mf.type = PrometheusMetricType::Counter;
                PrometheusSample s;
                s.name = prefix + "dropped_logs_total";
                s.value = static_cast<double>(m_dropped.load());
                add_common_labels(s);
                mf.samples.push_back(s);
                families.push_back(mf);
            }

            // logit_failed_exports_total (counter)
            {
                PrometheusMetricFamily mf;
                mf.name = prefix + "failed_exports_total";
                mf.help = "Total number of failed export attempts";
                mf.type = PrometheusMetricType::Counter;
                PrometheusSample s;
                s.name = prefix + "failed_exports_total";
                s.value = static_cast<double>(m_failed_collects.load());
                add_common_labels(s);
                mf.samples.push_back(s);
                families.push_back(mf);
            }

            // logit_last_log_timestamp_ms (gauge)
            {
                PrometheusMetricFamily mf;
                mf.name = prefix + "last_log_timestamp_ms";
                mf.help = "Timestamp of the last log record in milliseconds";
                mf.type = PrometheusMetricType::Gauge;
                PrometheusSample s;
                s.name = prefix + "last_log_timestamp_ms";
                s.value = static_cast<double>(m_last_log_ts.load());
                add_common_labels(s);
                mf.samples.push_back(s);
                families.push_back(mf);
            }

            // logit_time_since_last_log_ms (gauge)
            {
                PrometheusMetricFamily mf;
                mf.name = prefix + "time_since_last_log_ms";
                mf.help = "Milliseconds since the last log record";
                mf.type = PrometheusMetricType::Gauge;
                PrometheusSample s;
                s.name = prefix + "time_since_last_log_ms";
                s.value = static_cast<double>(get_time_since_last_log());
                add_common_labels(s);
                mf.samples.push_back(s);
                families.push_back(mf);
            }

            // logit_build_info (gauge, value=1) with version/compiler labels
            if (m_config.format.include_build_info) {
                PrometheusMetricFamily mf;
                mf.name = prefix + "build_info";
                mf.help = "Build information for logit-cpp";
                mf.type = PrometheusMetricType::Gauge;
                PrometheusSample s;
                s.name = prefix + "build_info";
                s.value = 1.0;
#ifdef LOGIT_VERSION
                s.labels.push_back({"version", LOGIT_VERSION});
#else
                s.labels.push_back({"version", "1.0.2"});
#endif
#if defined(__GNUC__) && !defined(__clang__)
                s.labels.push_back({"compiler", "gcc"});
#elif defined(__clang__)
                s.labels.push_back({"compiler", "clang"});
#elif defined(_MSC_VER)
                s.labels.push_back({"compiler", "msvc"});
#else
                s.labels.push_back({"compiler", "unknown"});
#endif
                add_common_labels(s);
                mf.samples.push_back(s);
                families.push_back(mf);
            }
        }

        void add_common_labels(PrometheusSample& sample) const {
            if (m_config.format.include_logger_label) {
                sample.labels.push_back(
                    {m_config.format.logger_label_name, "prometheus_payload"});
            }
            if (m_config.format.include_instance_label) {
                sample.labels.push_back(
                    {m_config.format.instance_label_name, m_config.format.instance_label_value});
            }
        }
    };

} // namespace logit

#endif // _LOGIT_PROMETHEUS_PAYLOAD_LOGGER_HPP_INCLUDED
