#pragma once
#ifndef _LOGIT_PROMETHEUS_METRIC_BUILDERS_HPP_INCLUDED
#define _LOGIT_PROMETHEUS_METRIC_BUILDERS_HPP_INCLUDED

/// \file PrometheusMetricBuilders.hpp
/// \brief Convenience helpers for constructing Prometheus metric families and samples.

#include "PrometheusTextFormatConfig.hpp"

#include <string>
#include <vector>

namespace logit {

    /// \brief Create a single label pair.
    inline PrometheusLabel make_prometheus_label(
        const std::string& name,
        const std::string& value) {
        PrometheusLabel label;
        label.name = name;
        label.value = value;
        return label;
    }

    /// \brief Create a sample for a Prometheus metric family.
    inline PrometheusSample make_prometheus_sample(
        const std::string& name,
        double value,
        std::vector<PrometheusLabel> labels = {},
        int64_t timestamp_ms = 0) {
        PrometheusSample s;
        s.name = name;
        s.value = value;
        s.labels = std::move(labels);
        s.timestamp_ms = timestamp_ms;
        return s;
    }

    /// \brief Create a Counter metric family with one unlabeled sample.
    inline PrometheusMetricFamily make_prometheus_counter(
        const std::string& name,
        const std::string& help,
        double value) {
        PrometheusMetricFamily mf;
        mf.name = name;
        mf.help = help;
        mf.type = PrometheusMetricType::Counter;
        mf.samples.push_back(make_prometheus_sample(name, value));
        return mf;
    }

    /// \brief Create a Counter metric family with one labeled sample.
    inline PrometheusMetricFamily make_prometheus_counter(
        const std::string& name,
        const std::string& help,
        double value,
        std::vector<PrometheusLabel> labels) {
        PrometheusMetricFamily mf;
        mf.name = name;
        mf.help = help;
        mf.type = PrometheusMetricType::Counter;
        mf.samples.push_back(make_prometheus_sample(name, value, std::move(labels)));
        return mf;
    }

    /// \brief Create a Gauge metric family with one unlabeled sample.
    inline PrometheusMetricFamily make_prometheus_gauge(
        const std::string& name,
        const std::string& help,
        double value) {
        PrometheusMetricFamily mf;
        mf.name = name;
        mf.help = help;
        mf.type = PrometheusMetricType::Gauge;
        mf.samples.push_back(make_prometheus_sample(name, value));
        return mf;
    }

    /// \brief Create a Gauge metric family with one labeled sample.
    inline PrometheusMetricFamily make_prometheus_gauge(
        const std::string& name,
        const std::string& help,
        double value,
        std::vector<PrometheusLabel> labels) {
        PrometheusMetricFamily mf;
        mf.name = name;
        mf.help = help;
        mf.type = PrometheusMetricType::Gauge;
        mf.samples.push_back(make_prometheus_sample(name, value, std::move(labels)));
        return mf;
    }

    /// \brief Create an Untyped metric family with one unlabeled sample.
    inline PrometheusMetricFamily make_prometheus_untyped(
        const std::string& name,
        const std::string& help,
        double value) {
        PrometheusMetricFamily mf;
        mf.name = name;
        mf.help = help;
        mf.type = PrometheusMetricType::Untyped;
        mf.samples.push_back(make_prometheus_sample(name, value));
        return mf;
    }

    /// \brief Create an Untyped metric family with one labeled sample.
    inline PrometheusMetricFamily make_prometheus_untyped(
        const std::string& name,
        const std::string& help,
        double value,
        std::vector<PrometheusLabel> labels) {
        PrometheusMetricFamily mf;
        mf.name = name;
        mf.help = help;
        mf.type = PrometheusMetricType::Untyped;
        mf.samples.push_back(make_prometheus_sample(name, value, std::move(labels)));
        return mf;
    }

    /// \brief Append a Counter metric family to the families vector (convenience for on_collect).
    inline void add_prometheus_counter(
        std::vector<PrometheusMetricFamily>& families,
        const std::string& name,
        const std::string& help,
        double value,
        std::vector<PrometheusLabel> labels = {}) {
        families.push_back(make_prometheus_counter(name, help, value, std::move(labels)));
    }

    /// \brief Append a Gauge metric family to the families vector (convenience for on_collect).
    inline void add_prometheus_gauge(
        std::vector<PrometheusMetricFamily>& families,
        const std::string& name,
        const std::string& help,
        double value,
        std::vector<PrometheusLabel> labels = {}) {
        families.push_back(make_prometheus_gauge(name, help, value, std::move(labels)));
    }

    /// \brief Append an Untyped metric family to the families vector (convenience for on_collect).
    inline void add_prometheus_untyped(
        std::vector<PrometheusMetricFamily>& families,
        const std::string& name,
        const std::string& help,
        double value,
        std::vector<PrometheusLabel> labels = {}) {
        families.push_back(make_prometheus_untyped(name, help, value, std::move(labels)));
    }

} // namespace logit

#endif // _LOGIT_PROMETHEUS_METRIC_BUILDERS_HPP_INCLUDED
