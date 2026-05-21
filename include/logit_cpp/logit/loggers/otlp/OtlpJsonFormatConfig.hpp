#pragma once
#ifndef _LOGIT_OTLP_JSON_FORMAT_CONFIG_HPP_INCLUDED
#define _LOGIT_OTLP_JSON_FORMAT_CONFIG_HPP_INCLUDED

/// \file OtlpJsonFormatConfig.hpp
/// \brief Defines serialization-related configuration shared by OTLP loggers.

#include <string>

namespace logit {

    /// \struct OtlpJsonFormatConfig
    /// \brief Serialization settings for OTLP JSON payload construction.
    struct OtlpJsonFormatConfig {
        std::string service_name = "logit-app";       ///< `service.name` resource attribute.
        std::string service_namespace;                ///< Optional `service.namespace` resource attribute.
        std::string service_instance_id;              ///< Optional `service.instance.id` resource attribute.
        std::string deployment_environment;           ///< Optional `deployment.environment.name` resource attribute.
        bool include_source = true;     ///< Export source file, line, and function attributes.
        bool include_thread_id = true;  ///< Export thread id attribute.
        bool include_format = true;     ///< Export original format string as `logit.format`.
        bool include_arg_names = false; ///< Export original argument names as `logit.arg_names` (legacy).
        bool include_args = true;       ///< Export structured typed arg attributes.
        std::string args_prefix = "logit.arg."; ///< Key prefix for structured arg attributes.
    };

} // namespace logit

#endif // _LOGIT_OTLP_JSON_FORMAT_CONFIG_HPP_INCLUDED
