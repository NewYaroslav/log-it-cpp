#pragma once
#ifndef _LOGIT_PROMETHEUS_HTTP_SERVER_LOGGER_HPP_INCLUDED
#define _LOGIT_PROMETHEUS_HTTP_SERVER_LOGGER_HPP_INCLUDED

/// \file PrometheusHttpServerLogger.hpp
/// \brief Prometheus HTTP server logger backend exposing /metrics endpoint.

#ifndef LOGIT_WITH_PROMETHEUS_SERVER
#   error "PrometheusHttpServerLogger requires LOGIT_WITH_PROMETHEUS_SERVER=1. Enable LOGIT_WITH_PROMETHEUS_SERVER in CMake."
#endif

#include "ILogger.hpp"
#include "prometheus/PrometheusTextFormatConfig.hpp"
#include "prometheus/PrometheusTextSerializer.hpp"

#include <server_http.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <limits>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace logit {

    /// \class PrometheusHttpServerLogger
    /// \ingroup LogBackends
    /// \brief Serves Prometheus metrics via an embedded HTTP server.
    ///
    /// This backend starts a Simple-Web-Server HTTP server and serves Prometheus
    /// text exposition format on the configured path (default: /metrics).
    /// It tracks the same internal counters and gauges as PrometheusPayloadLogger.
    class PrometheusHttpServerLogger final : public ILogger {
    public:
        using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;

        struct Config {
            PrometheusTextFormatConfig format;
            std::string address = "0.0.0.0";
            unsigned short port = 9090;
            std::string path = "/metrics";
            std::string health_path = "/health";
            bool enable_health_endpoint = true;
            std::function<void(std::vector<PrometheusMetricFamily>&)> on_collect;
            bool start_immediately = true;
        };

        /// \brief Constructs Prometheus HTTP server logger with default configuration.
        PrometheusHttpServerLogger() : PrometheusHttpServerLogger(Config()) {}

        /// \brief Constructs Prometheus HTTP server logger with custom configuration.
        /// \param config Server and format configuration.
        explicit PrometheusHttpServerLogger(const Config& config)
            : m_config(config) {

            m_server.config.address = m_config.address;
            m_server.config.port = m_config.port;

            // Lifecycle note: this lambda captures `this`. The destructor calls stop()
            // which invokes server.stop() and joins m_server_thread, ensuring no
            // active handlers reference the logger after destruction.
            m_server.resource[m_config.path]["GET"] =
                [this](std::shared_ptr<HttpServer::Response> response,
                       std::shared_ptr<HttpServer::Request>) {
                    try {
                        std::string payload = this->collect_payload();
                        response->write(
                            SimpleWeb::StatusCode::success_ok,
                            payload,
                            {{"Content-Type", "text/plain; version=0.0.4; charset=utf-8"},
                             {"Cache-Control", "no-store"}});
                    } catch (...) {
                        response->write(SimpleWeb::StatusCode::server_error_internal_server_error);
                    }
                };

            if (m_config.enable_health_endpoint) {
                m_server.resource[m_config.health_path]["GET"] =
                    [](std::shared_ptr<HttpServer::Response> response,
                       std::shared_ptr<HttpServer::Request>) {
                        response->write(SimpleWeb::StatusCode::success_ok, "ok");
                    };
            }

            m_server.default_resource["GET"] =
                [](std::shared_ptr<HttpServer::Response> response,
                   std::shared_ptr<HttpServer::Request>) {
                    response->write(
                        SimpleWeb::StatusCode::client_error_not_found,
                        "not found");
                };

            if (m_config.start_immediately) {
                start();
            }
        }

        ~PrometheusHttpServerLogger() override {
            stop();
        }

        PrometheusHttpServerLogger(const PrometheusHttpServerLogger&) = delete;
        PrometheusHttpServerLogger& operator=(const PrometheusHttpServerLogger&) = delete;

        /// \brief Starts the HTTP server thread.
        void start() {
            if (m_running.load()) {
                return;
            }
            m_running.store(true);
            m_server_thread = std::thread([this]() {
                m_server.start();
            });
        }

        /// \brief Updates internal metric counters for a log message.
        /// \param record Structured log record.
        /// \param message Formatted log message (unused by Prometheus metrics).
        void log(const LogRecord& record, const std::string& message) override {
            (void)message;
            m_last_log_ts.store(record.timestamp_ms);
            ++m_log_records_total;
        }

        /// \brief No-op; server is independently serving metrics.
        void wait() override {}

        /// \brief Stops the HTTP server and joins the server thread.
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
                        ++m_failed_exports;
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
        /// \return Parameter value in seconds for time params, or 0.0 when unsupported.
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
        uint64_t dropped_count() const {
            return m_dropped.load();
        }

        /// \brief Returns number of failed export attempts.
        uint64_t failed_export_count() const {
            return m_failed_exports.load();
        }

    private:
        Config m_config;
        HttpServer m_server;
        std::thread m_server_thread;
        std::mutex m_collect_mutex;
        std::atomic<bool> m_running = ATOMIC_VAR_INIT(false);

        std::atomic<int> m_log_level = ATOMIC_VAR_INIT(static_cast<int>(LogLevel::LOG_LVL_TRACE));
        std::atomic<int64_t> m_last_log_ts = ATOMIC_VAR_INIT(0);
        std::atomic<uint64_t> m_log_records_total = ATOMIC_VAR_INIT(0);
        std::atomic<uint64_t> m_dropped = ATOMIC_VAR_INIT(0);
        std::atomic<uint64_t> m_failed_exports = ATOMIC_VAR_INIT(0);

        void stop() {
            if (!m_running.exchange(false)) {
                return;
            }
            m_server.stop();
            if (m_server_thread.joinable()) {
                m_server_thread.join();
            }
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
                s.value = static_cast<double>(m_failed_exports.load());
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

            // logit_build_info (gauge, value=1)
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
                    {m_config.format.logger_label_name, "prometheus_http_server"});
            }
            if (m_config.format.include_instance_label) {
                sample.labels.push_back(
                    {m_config.format.instance_label_name, m_config.format.instance_label_value});
            }
        }
    };

} // namespace logit

#endif // _LOGIT_PROMETHEUS_HTTP_SERVER_LOGGER_HPP_INCLUDED
