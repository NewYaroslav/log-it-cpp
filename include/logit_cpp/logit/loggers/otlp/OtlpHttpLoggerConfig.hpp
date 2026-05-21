#pragma once
#ifndef _LOGIT_OTLP_HTTP_LOGGER_CONFIG_HPP_INCLUDED
#define _LOGIT_OTLP_HTTP_LOGGER_CONFIG_HPP_INCLUDED

/// \file OtlpHttpLoggerConfig.hpp
/// \brief Defines configuration for OTLP/HTTP log export.

#include <cstddef>
#include <string>

namespace logit {

    /// \struct OtlpHttpLoggerConfig
    /// \brief Configuration for OtlpHttpLogger.
    struct OtlpHttpLoggerConfig {
        std::string host = "http://localhost:4318"; ///< OTLP HTTP host, without `/v1/logs`.
        std::string path = "/v1/logs";              ///< OTLP logs endpoint path.

        std::string service_name = "logit-app";       ///< `service.name` resource attribute.
        std::string service_namespace;                ///< Optional `service.namespace` resource attribute.
        std::string service_instance_id;              ///< Optional `service.instance.id` resource attribute.
        std::string deployment_environment;           ///< Optional `deployment.environment.name` resource attribute.

        std::size_t max_queue_size = 8192; ///< Maximum pending records before overflow policy is applied.
        std::size_t max_batch_size = 256;  ///< Maximum records per OTLP export request.
        std::size_t max_in_flight_requests = 1; ///< Maximum concurrent HTTP export requests.

        int export_interval_ms = 1000; ///< Maximum delay before exporting a non-empty batch.
        int request_timeout_sec = 3;   ///< HTTP request timeout in seconds.

        long retry_attempts = 2;  ///< Retry attempts passed to kurlyk HttpClient.
        long retry_delay_ms = 250; ///< Retry delay passed to kurlyk HttpClient.

        bool drop_on_overflow = true; ///< Drop incoming records when queue is full.
        bool async = true;            ///< Export records from a dedicated worker thread.

        bool include_source = true;     ///< Export source file, line, and function attributes.
        bool include_thread_id = true;  ///< Export thread id attribute.
        bool include_format = true;     ///< Export original format string as `logit.format`.
        bool include_arg_names = false; ///< Export original argument names as `logit.arg_names` (legacy).
        bool include_args = true;       ///< Export structured typed arg attributes.
        std::string args_prefix = "logit.arg."; ///< Key prefix for structured arg attributes.
        bool cancel_on_shutdown = false; ///< If true, cancel in-flight HTTP requests on shutdown instead of waiting.
    };

} // namespace logit

#endif // _LOGIT_OTLP_HTTP_LOGGER_CONFIG_HPP_INCLUDED
