#pragma once
#ifndef _LOGIT_PROMETHEUS_TEXT_FORMAT_CONFIG_HPP_INCLUDED
#define _LOGIT_PROMETHEUS_TEXT_FORMAT_CONFIG_HPP_INCLUDED

/// \file PrometheusTextFormatConfig.hpp
/// \brief Defines Prometheus text exposition format types and configuration.

#include <cstdint>
#include <string>
#include <vector>

namespace logit {

    /// \enum PrometheusMetricType
    /// \brief Prometheus metric type identifiers.
    enum class PrometheusMetricType { Counter, Gauge, Untyped };

    /// \struct PrometheusLabel
    /// \brief A single Prometheus label key-value pair.
    struct PrometheusLabel {
        std::string name;  ///< Label name.
        std::string value; ///< Label value.
    };

    /// \struct PrometheusSample
    /// \brief A single metric sample with optional labels and timestamp.
    struct PrometheusSample {
        std::string name;                       ///< Full metric name (including suffixes).
        double value = 0.0;                     ///< Sample value.
        std::vector<PrometheusLabel> labels;    ///< Labels for this sample.
        int64_t timestamp_ms = 0;               ///< Timestamp in ms; 0 = omit.
    };

    /// \struct PrometheusMetricFamily
    /// \brief A group of related samples sharing a name, help, and type.
    struct PrometheusMetricFamily {
        std::string name;                                ///< Metric family name.
        std::string help;                                ///< HELP text.
        PrometheusMetricType type = PrometheusMetricType::Untyped; ///< Metric type.
        std::vector<PrometheusSample> samples;           ///< Samples in this family.
    };

    /// \struct PrometheusTextFormatConfig
    /// \brief Configuration for Prometheus text exposition format output.
    struct PrometheusTextFormatConfig {
        bool include_help = true;            ///< Emit HELP lines.
        bool include_type = true;            ///< Emit TYPE lines.
        bool include_timestamp = false;      ///< Emit sample timestamps.
        std::string metric_prefix = "logit_";///< Prefix applied to built-in metric names.
        std::vector<PrometheusLabel> const_labels; ///< Labels added to every sample.
        bool include_logger_label = true;    ///< Add logger label to built-in metrics.
        std::string logger_label_name = "logger";   ///< Name of logger label.
        bool include_instance_label = false; ///< Add instance label to built-in metrics.
        std::string instance_label_name = "instance";  ///< Name of instance label.
        std::string instance_label_value;    ///< Value of instance label.
        bool include_build_info = true;      ///< Emit logit_build_info metric.
    };

} // namespace logit

#endif // _LOGIT_PROMETHEUS_TEXT_FORMAT_CONFIG_HPP_INCLUDED
