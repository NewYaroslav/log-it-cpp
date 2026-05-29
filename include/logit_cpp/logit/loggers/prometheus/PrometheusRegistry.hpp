#pragma once
#ifndef _LOGIT_PROMETHEUS_REGISTRY_HPP_INCLUDED
#define _LOGIT_PROMETHEUS_REGISTRY_HPP_INCLUDED

/// \file PrometheusRegistry.hpp
/// \brief Declarative registry for custom Prometheus application metrics.

#include "PrometheusTextFormatConfig.hpp"

#include <cstddef>
#include <exception>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace logit {

    /// \class PrometheusRegistry
    /// \brief Stores custom gauge and counter callbacks for Prometheus collection.
    class PrometheusRegistry {
    public:
        /// \brief Constructs registry with an optional metric prefix.
        explicit PrometheusRegistry(std::string metric_prefix = std::string())
            : m_metric_prefix(std::move(metric_prefix)) {}

        /// \brief Sets prefix applied to metrics collected by this registry.
        void set_metric_prefix(const std::string& metric_prefix) {
            m_metric_prefix = metric_prefix;
        }

        /// \brief Returns prefix applied to metrics collected by this registry.
        const std::string& metric_prefix() const {
            return m_metric_prefix;
        }

        /// \brief Registers or replaces a gauge metric callback.
        void set_gauge(
            const std::string& name,
            const std::string& help,
            std::function<double()> value_fn,
            std::vector<PrometheusLabel> labels = {}) {
            set_metric(
                PrometheusMetricType::Gauge,
                name,
                help,
                std::move(value_fn),
                std::move(labels));
        }

        /// \brief Registers or replaces a counter metric callback.
        void set_counter(
            const std::string& name,
            const std::string& help,
            std::function<double()> value_fn,
            std::vector<PrometheusLabel> labels = {}) {
            set_metric(
                PrometheusMetricType::Counter,
                name,
                help,
                std::move(value_fn),
                std::move(labels));
        }

        /// \brief Appends current registry metrics to the output vector.
        void collect(std::vector<PrometheusMetricFamily>& out) const {
            std::vector<PrometheusMetricFamily> collected;
            std::exception_ptr first_exception;

            for (std::size_t i = 0; i < m_entries.size(); ++i) {
                const Entry& entry = m_entries[i];
                const std::string full_name = m_metric_prefix + entry.name;
                double value = 0.0;

                try {
                    value = entry.value_fn();
                } catch (...) {
                    if (!first_exception) {
                        first_exception = std::current_exception();
                    }
                    continue;
                }

                PrometheusMetricFamily* family =
                    find_family(collected, full_name, entry.type);
                if (family == nullptr) {
                    PrometheusMetricFamily mf;
                    mf.name = full_name;
                    mf.help = entry.help;
                    mf.type = entry.type;
                    collected.push_back(std::move(mf));
                    family = &collected.back();
                }

                PrometheusSample sample;
                sample.name = full_name;
                sample.value = value;
                sample.labels = entry.labels;
                family->samples.push_back(std::move(sample));
            }

            out.insert(out.end(), collected.begin(), collected.end());
            if (first_exception) {
                std::rethrow_exception(first_exception);
            }
        }

    private:
        struct Entry {
            PrometheusMetricType type = PrometheusMetricType::Untyped;
            std::string name;
            std::string help;
            std::function<double()> value_fn;
            std::vector<PrometheusLabel> labels;
        };

        std::string m_metric_prefix;
        std::vector<Entry> m_entries;

        void set_metric(
            PrometheusMetricType type,
            const std::string& name,
            const std::string& help,
            std::function<double()> value_fn,
            std::vector<PrometheusLabel> labels) {
            for (std::size_t i = 0; i < m_entries.size(); ++i) {
                Entry& entry = m_entries[i];
                if (entry.name == name && labels_equal(entry.labels, labels)) {
                    entry.type = type;
                    entry.help = help;
                    entry.value_fn = std::move(value_fn);
                    entry.labels = std::move(labels);
                    return;
                }
            }

            Entry entry;
            entry.type = type;
            entry.name = name;
            entry.help = help;
            entry.value_fn = std::move(value_fn);
            entry.labels = std::move(labels);
            m_entries.push_back(std::move(entry));
        }

        static bool labels_equal(
            const std::vector<PrometheusLabel>& lhs,
            const std::vector<PrometheusLabel>& rhs) {
            if (lhs.size() != rhs.size()) {
                return false;
            }
            for (std::size_t i = 0; i < lhs.size(); ++i) {
                if (lhs[i].name != rhs[i].name || lhs[i].value != rhs[i].value) {
                    return false;
                }
            }
            return true;
        }

        static PrometheusMetricFamily* find_family(
            std::vector<PrometheusMetricFamily>& families,
            const std::string& name,
            PrometheusMetricType type) {
            for (std::size_t i = 0; i < families.size(); ++i) {
                if (families[i].name == name && families[i].type == type) {
                    return &families[i];
                }
            }
            return nullptr;
        }
    };

} // namespace logit

#endif // _LOGIT_PROMETHEUS_REGISTRY_HPP_INCLUDED
