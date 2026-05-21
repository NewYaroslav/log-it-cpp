#pragma once
#ifndef _LOGIT_OTLP_PAYLOAD_LOGGER_CONFIG_HPP_INCLUDED
#define _LOGIT_OTLP_PAYLOAD_LOGGER_CONFIG_HPP_INCLUDED

/// \file OtlpPayloadLoggerConfig.hpp
/// \brief Defines configuration for OTLP payload callback logger.

#include "OtlpHttpLoggerConfig.hpp"
#include <cstddef>
#include <functional>
#include <string>

namespace logit {

    /// \struct OtlpPayloadLoggerConfig
    /// \brief Configuration for OtlpPayloadLogger.
    struct OtlpPayloadLoggerConfig {
        OtlpHttpLoggerConfig format;                        ///< Serialization settings (service_name, resource attrs, etc.).
        std::function<void(std::string)> on_payload;        ///< Callback invoked with OTLP JSON payload.
        bool async = true;                                  ///< Export records from a dedicated worker thread.
        std::size_t max_batch_size = 256;                   ///< Maximum records per OTLP payload.
        std::size_t max_queue_size = 1024;                  ///< Maximum pending records before overflow policy is applied.
        bool drop_on_overflow = true;                       ///< Drop incoming records when queue is full.
        unsigned export_interval_ms = 100;                  ///< Maximum delay before exporting a non-empty batch.
    };

} // namespace logit

#endif // _LOGIT_OTLP_PAYLOAD_LOGGER_CONFIG_HPP_INCLUDED
