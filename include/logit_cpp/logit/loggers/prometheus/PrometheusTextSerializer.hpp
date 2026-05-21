#pragma once
#ifndef _LOGIT_PROMETHEUS_TEXT_SERIALIZER_HPP_INCLUDED
#define _LOGIT_PROMETHEUS_TEXT_SERIALIZER_HPP_INCLUDED

/// \file PrometheusTextSerializer.hpp
/// \brief Prometheus text exposition format serialization helpers.

#include "PrometheusTextFormatConfig.hpp"

#include <cctype>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace logit {

    /// \brief Escapes help string for Prometheus text format.
    /// \param value Raw help string.
    /// \return Escaped string with backslash and newline escaped.
    inline std::string prometheus_escape_help(const std::string& value) {
        std::string out;
        out.reserve(value.size());
        for (size_t i = 0; i < value.size(); ++i) {
            const char c = value[i];
            if (c == '\\') {
                out += "\\\\";
            } else if (c == '\n') {
                out += "\\n";
            } else {
                out += c;
            }
        }
        return out;
    }

    /// \brief Escapes label value for Prometheus text format.
    /// \param value Raw label value.
    /// \return Escaped string with backslash, double quote, and newline escaped.
    inline std::string prometheus_escape_label_value(const std::string& value) {
        std::string out;
        out.reserve(value.size());
        for (size_t i = 0; i < value.size(); ++i) {
            const char c = value[i];
            if (c == '\\') {
                out += "\\\\";
            } else if (c == '"') {
                out += "\\\"";
            } else if (c == '\n') {
                out += "\\n";
            } else {
                out += c;
            }
        }
        return out;
    }

    /// \brief Sanitizes a metric name to match Prometheus naming rules.
    /// \param name Raw metric name.
    /// \return Sanitized name: [a-zA-Z_:][a-zA-Z0-9_:]*; invalid chars replaced by _.
    inline std::string prometheus_sanitize_metric_name(const std::string& name) {
        std::string out;
        out.reserve(name.size());
        for (size_t i = 0; i < name.size(); ++i) {
            const unsigned char c = static_cast<unsigned char>(name[i]);
            if (i == 0) {
                if (std::isalpha(c) || c == '_' || c == ':') {
                    out += static_cast<char>(c);
                } else {
                    out += '_';
                }
            } else {
                if (std::isalnum(c) || c == '_' || c == ':') {
                    out += static_cast<char>(c);
                } else {
                    out += '_';
                }
            }
        }
        if (out.empty()) {
            out = "_";
        }
        return out;
    }

    /// \brief Sanitizes a label name to match Prometheus naming rules.
    /// \param name Raw label name.
    /// \return Sanitized name; first char must be letter or underscore.
    inline std::string prometheus_sanitize_label_name(const std::string& name) {
        std::string out;
        out.reserve(name.size());
        for (size_t i = 0; i < name.size(); ++i) {
            const unsigned char c = static_cast<unsigned char>(name[i]);
            if (i == 0) {
                if (std::isalpha(c) || c == '_') {
                    out += static_cast<char>(c);
                } else {
                    out += '_';
                }
            } else {
                if (std::isalnum(c) || c == '_') {
                    out += static_cast<char>(c);
                } else {
                    out += '_';
                }
            }
        }
        if (out.empty()) {
            out = "_";
        }
        return out;
    }

    /// \brief Formats a Prometheus sample value according to text format rules.
    /// \param os Output stream.
    /// \param value Double value to format.
    inline void prometheus_format_value(std::ostringstream& os, double value) {
        if (std::isnan(value)) {
            os << "NaN";
        } else if (std::isinf(value)) {
            if (value > 0) {
                os << "+Inf";
            } else {
                os << "-Inf";
            }
        } else {
            os << std::setprecision(std::numeric_limits<double>::max_digits10) << value;
        }
    }

    /// \brief Writes one PrometheusMetricFamily to an output stream.
    /// \param os Output string stream.
    /// \param family Metric family to write.
    /// \param config Format configuration.
    inline void prometheus_write_metric_family(
            std::ostringstream& os,
            const PrometheusMetricFamily& family,
            const PrometheusTextFormatConfig& config) {
        const std::string safe_name = prometheus_sanitize_metric_name(family.name);

        if (config.include_help && !family.help.empty()) {
            os << "# HELP " << safe_name << " " << prometheus_escape_help(family.help) << "\n";
        }

        if (config.include_type) {
            const char* type_str = "untyped";
            switch (family.type) {
            case PrometheusMetricType::Counter: type_str = "counter"; break;
            case PrometheusMetricType::Gauge:   type_str = "gauge";   break;
            default: break;
            }
            os << "# TYPE " << safe_name << " " << type_str << "\n";
        }

        for (size_t si = 0; si < family.samples.size(); ++si) {
            const PrometheusSample& sample = family.samples[si];
            const std::string sample_name = prometheus_sanitize_metric_name(sample.name);

            os << sample_name;

            // Merge labels: const_labels first, then sample labels, then logger/instance.
            std::vector<PrometheusLabel> merged;
            if (!config.const_labels.empty()) {
                merged.insert(merged.end(), config.const_labels.begin(), config.const_labels.end());
            }
            if (!sample.labels.empty()) {
                merged.insert(merged.end(), sample.labels.begin(), sample.labels.end());
            }

            if (!merged.empty()) {
                os << "{";
                for (size_t li = 0; li < merged.size(); ++li) {
                    if (li != 0) {
                        os << ",";
                    }
                    os << prometheus_sanitize_label_name(merged[li].name)
                       << "=\"" << prometheus_escape_label_value(merged[li].value) << "\"";
                }
                os << "}";
            }

            os << " ";
            prometheus_format_value(os, sample.value);

            if (config.include_timestamp && sample.timestamp_ms != 0) {
                os << " " << sample.timestamp_ms;
            }

            os << "\n";
        }
    }

    /// \brief Builds a complete Prometheus text exposition format payload.
    /// \param families Metric families to serialize.
    /// \param config Format configuration.
    /// \return Complete text exposition payload string.
    inline std::string build_prometheus_text_payload(
            const std::vector<PrometheusMetricFamily>& families,
            const PrometheusTextFormatConfig& config) {
        std::ostringstream os;
        for (size_t i = 0; i < families.size(); ++i) {
            prometheus_write_metric_family(os, families[i], config);
        }
        return os.str();
    }

} // namespace logit

#endif // _LOGIT_PROMETHEUS_TEXT_SERIALIZER_HPP_INCLUDED
