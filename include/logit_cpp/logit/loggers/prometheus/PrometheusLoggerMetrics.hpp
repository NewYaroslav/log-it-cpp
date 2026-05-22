#pragma once
#ifndef _LOGIT_PROMETHEUS_LOGGER_METRICS_HPP_INCLUDED
#define _LOGIT_PROMETHEUS_LOGGER_METRICS_HPP_INCLUDED

/// \file PrometheusLoggerMetrics.hpp
/// \brief Shared built-in metrics state and serialization for Prometheus loggers.

#include "PrometheusTextFormatConfig.hpp"
#include "../../config.hpp"

#include <atomic>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace logit {

    /// \class PrometheusLoggerMetrics
    /// \brief Tracks log-record counters and builds built-in Prometheus metric families.
    class PrometheusLoggerMetrics {
    public:
        /// \brief Update metrics when a log record is processed.
        void on_log(int64_t timestamp_ms) {
            m_last_log_ts.store(timestamp_ms);
            ++m_log_records_total;
        }

        /// \brief Increment dropped log counter.
        void add_dropped(uint64_t count = 1) {
            m_dropped.fetch_add(count);
        }

        /// \brief Increment failed export/collect counter.
        void add_failed_export(uint64_t count = 1) {
            m_failed_exports.fetch_add(count);
        }

        /// \brief Total log records processed.
        uint64_t log_records_total() const {
            return m_log_records_total.load();
        }

        /// \brief Total dropped log records.
        uint64_t dropped_count() const {
            return m_dropped.load();
        }

        /// \brief Total failed export/collect attempts.
        uint64_t failed_export_count() const {
            return m_failed_exports.load();
        }

        /// \brief Last log timestamp (ms).
        int64_t last_log_ts() const {
            return m_last_log_ts.load();
        }

        /// \brief Milliseconds since last log record (0 if none).
        int64_t time_since_last_log_ms() const {
            const int64_t last = last_log_ts();
            if (last <= 0) {
                return 0;
            }
            const int64_t now = LOGIT_CURRENT_TIMESTAMP_MS();
            return now > last ? now - last : 0;
        }

        /// \brief Append the six built-in metric families to \p families.
        /// \param families Destination vector.
        /// \param config Format configuration (prefix, labels, build-info flag).
        /// \param logger_name Value for the logger label (e.g. "prometheus_payload").
        void build_builtin_metrics(
            std::vector<PrometheusMetricFamily>& families,
            const PrometheusTextFormatConfig& config,
            const std::string& logger_name) const {

            const std::string& prefix = config.metric_prefix;

            // logit_log_records_total (counter)
            {
                PrometheusMetricFamily mf;
                mf.name = prefix + "log_records_total";
                mf.help = "Total number of log records processed";
                mf.type = PrometheusMetricType::Counter;
                PrometheusSample s;
                s.name = prefix + "log_records_total";
                s.value = static_cast<double>(m_log_records_total.load());
                add_common_labels(s, config, logger_name);
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
                add_common_labels(s, config, logger_name);
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
                add_common_labels(s, config, logger_name);
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
                add_common_labels(s, config, logger_name);
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
                s.value = static_cast<double>(time_since_last_log_ms());
                add_common_labels(s, config, logger_name);
                mf.samples.push_back(s);
                families.push_back(mf);
            }

            // logit_build_info (gauge, value=1) with version/compiler labels
            if (config.include_build_info) {
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
                add_common_labels(s, config, logger_name);
                mf.samples.push_back(s);
                families.push_back(mf);
            }
        }

        /// \brief Safely convert uint64_t counter to int64_t (clamping on overflow).
        static int64_t counter_to_int64(uint64_t value) {
            const uint64_t max_value = static_cast<uint64_t>((std::numeric_limits<int64_t>::max)());
            return value > max_value ? (std::numeric_limits<int64_t>::max)() : static_cast<int64_t>(value);
        }

    private:
        std::atomic<uint64_t> m_log_records_total{0};
        std::atomic<uint64_t> m_dropped{0};
        std::atomic<uint64_t> m_failed_exports{0};
        std::atomic<int64_t>  m_last_log_ts{0};

        static void add_common_labels(
            PrometheusSample& sample,
            const PrometheusTextFormatConfig& config,
            const std::string& logger_name) {
            if (config.include_logger_label) {
                sample.labels.push_back(
                    {config.logger_label_name, logger_name});
            }
            if (config.include_instance_label) {
                sample.labels.push_back(
                    {config.instance_label_name, config.instance_label_value});
            }
        }
    };

} // namespace logit

#endif // _LOGIT_PROMETHEUS_LOGGER_METRICS_HPP_INCLUDED
