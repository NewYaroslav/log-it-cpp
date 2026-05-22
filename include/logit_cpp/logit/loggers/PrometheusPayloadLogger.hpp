#pragma once
#ifndef _LOGIT_PROMETHEUS_PAYLOAD_LOGGER_HPP_INCLUDED
#define _LOGIT_PROMETHEUS_PAYLOAD_LOGGER_HPP_INCLUDED

/// \file PrometheusPayloadLogger.hpp
/// \brief Prometheus text payload callback logger backend.

#ifndef LOGIT_WITH_PROMETHEUS
#   error "PrometheusPayloadLogger requires LOGIT_WITH_PROMETHEUS=1. Enable LOGIT_WITH_PROMETHEUS in CMake."
#endif

#include "ILogger.hpp"
#include "prometheus/PrometheusLoggerMetrics.hpp"
#include "prometheus/PrometheusTextSerializer.hpp"

#include <atomic>
#include <cstdint>
#include <functional>
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

        ~PrometheusPayloadLogger() override = default;

        PrometheusPayloadLogger(const PrometheusPayloadLogger&) = delete;
        PrometheusPayloadLogger& operator=(const PrometheusPayloadLogger&) = delete;

        /// \brief Updates internal metric counters for a log message.
        /// \param record Structured log record.
        /// \param message Formatted log message (unused by Prometheus metrics).
        void log(const LogRecord& record, const std::string& message) override {
            (void)message;
            m_metrics.on_log(record.timestamp_ms);

            if (m_config.emit_on_log && m_config.on_payload) {
                try {
                    std::string payload = collect_payload();
                    m_config.on_payload(std::move(payload));
                } catch (...) {
                    m_metrics.add_failed_export();
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
                    m_metrics.add_failed_export();
                }
            }
        }

        /// \brief Stops the logger (no worker thread to drain for payload logger).
        void shutdown() override {}

        /// \brief Collects current metrics and returns serialized Prometheus text payload.
        /// \return Complete Prometheus text exposition format string.
        std::string collect_payload() {
            std::vector<PrometheusMetricFamily> families;
            {
                std::lock_guard<std::mutex> lock(m_collect_mutex);
                m_metrics.build_builtin_metrics(families, m_config.format, "prometheus_payload");
                if (m_config.on_collect) {
                    try {
                        m_config.on_collect(families);
                    } catch (...) {
                        m_metrics.add_failed_export();
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
            case LoggerParam::LastLogTimestamp: return std::to_string(m_metrics.last_log_ts());
            case LoggerParam::TimeSinceLastLog: return std::to_string(m_metrics.time_since_last_log_ms());
            case LoggerParam::DroppedLogCount: return std::to_string(m_metrics.dropped_count());
            case LoggerParam::FailedExportCount: return std::to_string(m_metrics.failed_export_count());
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
            case LoggerParam::LastLogTimestamp: return m_metrics.last_log_ts();
            case LoggerParam::TimeSinceLastLog: return m_metrics.time_since_last_log_ms();
            case LoggerParam::DroppedLogCount: return PrometheusLoggerMetrics::counter_to_int64(m_metrics.dropped_count());
            case LoggerParam::FailedExportCount: return PrometheusLoggerMetrics::counter_to_int64(m_metrics.failed_export_count());
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
                return static_cast<double>(m_metrics.last_log_ts()) / 1000.0;
            case LoggerParam::TimeSinceLastLog:
                return static_cast<double>(m_metrics.time_since_last_log_ms()) / 1000.0;
            case LoggerParam::DroppedLogCount:
                return static_cast<double>(m_metrics.dropped_count());
            case LoggerParam::FailedExportCount:
                return static_cast<double>(m_metrics.failed_export_count());
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
        PrometheusLoggerMetrics m_metrics;

        std::atomic<int> m_log_level = ATOMIC_VAR_INIT(static_cast<int>(LogLevel::LOG_LVL_TRACE));
    };

} // namespace logit

#endif // _LOGIT_PROMETHEUS_PAYLOAD_LOGGER_HPP_INCLUDED
