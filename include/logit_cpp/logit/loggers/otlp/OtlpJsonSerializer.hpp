#pragma once
#ifndef _LOGIT_OTLP_JSON_SERIALIZER_HPP_INCLUDED
#define _LOGIT_OTLP_JSON_SERIALIZER_HPP_INCLUDED

/// \file OtlpJsonSerializer.hpp
/// \brief Defines OTLP/HTTP JSON serialization helpers for logs.

#include "OtlpJsonFormatConfig.hpp"
#include "OtlpRecordSnapshot.hpp"
#include <cctype>
#include <cstdint>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace logit {

    /// \struct OtlpLogItem
    /// \brief Owns one formatted log message and its structured metadata.
    struct OtlpLogItem {
        OtlpRecordSnapshot record; ///< Structured log metadata snapshot.
        std::string message;       ///< Formatted message body.
    };

    /// \brief Escapes a string for JSON output.
    /// \param value Input string.
    /// \return Escaped JSON string content without surrounding quotes.
    inline std::string otlp_json_escape(const std::string& value) {
        std::string out;
        out.reserve(value.size() + 16);

        for (size_t i = 0; i < value.size(); ++i) {
            const unsigned char c = static_cast<unsigned char>(value[i]);

            switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) {
                    static const char* hex = "0123456789abcdef";
                    out += "\\u00";
                    out += hex[(c >> 4) & 0x0F];
                    out += hex[c & 0x0F];
                } else {
                    out += static_cast<char>(c);
                }
                break;
            }
        }

        return out;
    }

    /// \brief Maps LogIt++ levels to OpenTelemetry severity numbers.
    /// \param level LogIt++ severity level.
    /// \return OpenTelemetry severity number.
    inline int otlp_severity_number(LogLevel level) {
        switch (level) {
        case LogLevel::LOG_LVL_TRACE: return 1;
        case LogLevel::LOG_LVL_DEBUG: return 5;
        case LogLevel::LOG_LVL_INFO:  return 9;
        case LogLevel::LOG_LVL_WARN:  return 13;
        case LogLevel::LOG_LVL_ERROR: return 17;
        case LogLevel::LOG_LVL_FATAL: return 21;
        default: return 0;
        }
    }

    /// \brief Writes a comma before the next JSON value when needed.
    /// \param os Output stream.
    /// \param first First-item flag to update.
    inline void otlp_write_comma_if_needed(std::ostringstream& os, bool& first) {
        if (!first) {
            os << ',';
        }
        first = false;
    }

    /// \brief Writes a string OTLP attribute.
    /// \param os Output stream.
    /// \param key Attribute key.
    /// \param value Attribute string value.
    inline void otlp_write_string_attr(std::ostringstream& os, const std::string& key, const std::string& value) {
        os << "{\"key\":\"" << otlp_json_escape(key)
           << "\",\"value\":{\"stringValue\":\"" << otlp_json_escape(value) << "\"}}";
    }

    /// \brief Writes an int OTLP attribute.
    /// \param os Output stream.
    /// \param key Attribute key.
    /// \param value Attribute integer value.
    inline void otlp_write_int_attr(std::ostringstream& os, const std::string& key, int64_t value) {
        os << "{\"key\":\"" << otlp_json_escape(key)
           << "\",\"value\":{\"intValue\":\"" << value << "\"}}";
    }

    /// \brief Writes a bool OTLP attribute.
    /// \param os Output stream.
    /// \param key Attribute key.
    /// \param value Attribute boolean value.
    inline void otlp_write_bool_attr(std::ostringstream& os, const std::string& key, bool value) {
        os << "{\"key\":\"" << otlp_json_escape(key)
           << "\",\"value\":{\"boolValue\":" << (value ? "true" : "false") << "}}";
    }

    /// \brief Writes a double OTLP attribute.
    /// \param os Output stream.
    /// \param key Attribute key.
    /// \param value Attribute double value.
    inline void otlp_write_double_attr(std::ostringstream& os, const std::string& key, double value) {
        if (std::isfinite(value)) {
            os << "{\"key\":\"" << otlp_json_escape(key)
               << "\",\"value\":{\"doubleValue\":"
               << std::setprecision(std::numeric_limits<double>::max_digits10)
               << value << "}}";
        } else {
            os << "{\"key\":\"" << otlp_json_escape(key)
               << "\",\"value\":{\"stringValue\":\"" << otlp_json_escape(std::to_string(value)) << "\"}}";
        }
    }

    /// \brief Writes a uint64 OTLP attribute.
    /// \param os Output stream.
    /// \param key Attribute key.
    /// \param value Attribute uint64 value.
    inline void otlp_write_uint_attr(std::ostringstream& os, const std::string& key, uint64_t value) {
        if (value <= static_cast<uint64_t>((std::numeric_limits<int64_t>::max)())) {
            otlp_write_int_attr(os, key, static_cast<int64_t>(value));
        } else {
            os << "{\"key\":\"" << otlp_json_escape(key)
               << "\",\"value\":{\"stringValue\":\"" << value << "\"}}";
        }
    }

    /// \brief Sanitizes an attribute key for OTLP.
    /// \param name Raw key name.
    /// \return Sanitized key with invalid chars replaced by underscore.
    inline std::string sanitize_otlp_key(const std::string& name) {
        std::string result = name;
        for (char& c : result) {
            if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '.' && c != '-') {
                c = '_';
            }
        }
        return result;
    }

    /// \brief Writes one OTLP JSON log record.
    /// \param os Output stream.
    /// \param item Log item to serialize.
    /// \param config Export configuration.
    inline void otlp_write_log_record_json(
            std::ostringstream& os,
            const OtlpLogItem& item,
            const OtlpJsonFormatConfig& config) {
        const OtlpRecordSnapshot& r = item.record;
        const int64_t time_unix_nano = r.timestamp_ms * 1000000LL;

        os << '{';
        os << "\"timeUnixNano\":\"" << time_unix_nano << "\",";
        os << "\"severityText\":\"" << otlp_json_escape(to_c_str(r.log_level)) << "\",";
        os << "\"severityNumber\":" << otlp_severity_number(r.log_level) << ',';
        os << "\"body\":{\"stringValue\":\"" << otlp_json_escape(item.message) << "\"},";
        os << "\"attributes\":[";

        bool first = true;

        if (config.include_source) {
            otlp_write_comma_if_needed(os, first);
            otlp_write_string_attr(os, "code.file.path", r.file);

            otlp_write_comma_if_needed(os, first);
            otlp_write_int_attr(os, "code.line.number", r.line);

            otlp_write_comma_if_needed(os, first);
            otlp_write_string_attr(os, "code.function.name", r.function);
        }

        if (config.include_thread_id) {
            otlp_write_comma_if_needed(os, first);
            otlp_write_string_attr(os, "thread.id", r.thread_id);
        }

        if (config.include_format) {
            otlp_write_comma_if_needed(os, first);
            otlp_write_string_attr(os, "logit.format", r.format);
        }

        if (config.include_arg_names && !r.arg_names.empty()) {
            otlp_write_comma_if_needed(os, first);
            otlp_write_string_attr(os, "logit.arg_names", r.arg_names);
        }

        if (config.include_args && !r.args_array.empty()) {
            std::unordered_map<std::string, std::size_t> key_count;

            for (std::size_t i = 0; i < r.args_array.size(); ++i) {
                const VariableValue& arg = r.args_array[i];

                std::string sanitized = sanitize_otlp_key(arg.name);

                bool all_underscore = true;
                for (std::size_t j = 0; j < sanitized.size(); ++j) {
                    if (sanitized[j] != '_') {
                        all_underscore = false;
                        break;
                    }
                }

                std::string key;
                if (sanitized.empty() || all_underscore) {
                    key = config.args_prefix + std::to_string(i);
                } else {
                    key = config.args_prefix + sanitized;
                }

                auto it = key_count.find(key);
                if (it != key_count.end()) {
                    ++(it->second);
                    key += "." + std::to_string(it->second);
                }
                key_count[key] = 0;

                otlp_write_comma_if_needed(os, first);

                switch (arg.type) {
                case VariableValue::ValueType::BOOL_VAL:
                    otlp_write_bool_attr(os, key, arg.pod_value.bool_value);
                    break;
                case VariableValue::ValueType::INT8_VAL:
                    otlp_write_int_attr(os, key, static_cast<int64_t>(arg.pod_value.int8_value));
                    break;
                case VariableValue::ValueType::INT16_VAL:
                    otlp_write_int_attr(os, key, static_cast<int64_t>(arg.pod_value.int16_value));
                    break;
                case VariableValue::ValueType::INT32_VAL:
                    otlp_write_int_attr(os, key, static_cast<int64_t>(arg.pod_value.int32_value));
                    break;
                case VariableValue::ValueType::INT64_VAL:
                    otlp_write_int_attr(os, key, arg.pod_value.int64_value);
                    break;
                case VariableValue::ValueType::UINT8_VAL:
                    otlp_write_uint_attr(os, key, static_cast<uint64_t>(arg.pod_value.uint8_value));
                    break;
                case VariableValue::ValueType::UINT16_VAL:
                    otlp_write_uint_attr(os, key, static_cast<uint64_t>(arg.pod_value.uint16_value));
                    break;
                case VariableValue::ValueType::UINT32_VAL:
                    otlp_write_uint_attr(os, key, static_cast<uint64_t>(arg.pod_value.uint32_value));
                    break;
                case VariableValue::ValueType::UINT64_VAL:
                    otlp_write_uint_attr(os, key, arg.pod_value.uint64_value);
                    break;
                case VariableValue::ValueType::FLOAT_VAL:
                    otlp_write_double_attr(os, key, static_cast<double>(arg.pod_value.float_value));
                    break;
                case VariableValue::ValueType::DOUBLE_VAL:
                    otlp_write_double_attr(os, key, arg.pod_value.double_value);
                    break;
                case VariableValue::ValueType::LONG_DOUBLE_VAL:
                    otlp_write_double_attr(os, key, static_cast<double>(arg.pod_value.long_double_value));
                    break;
                default:
                    otlp_write_string_attr(os, key, arg.to_string());
                    break;
                }
            }
        }

        otlp_write_comma_if_needed(os, first);
        otlp_write_int_attr(os, "logit.logger_index", r.logger_index);

        otlp_write_comma_if_needed(os, first);
        otlp_write_bool_attr(os, "logit.raw_mode", r.raw_mode);

        otlp_write_comma_if_needed(os, first);
        otlp_write_bool_attr(os, "logit.print_mode", r.print_mode);

        otlp_write_comma_if_needed(os, first);
        otlp_write_bool_attr(os, "logit.fmt_mode", r.fmt_mode);

        os << ']';
        os << '}';
    }

    /// \brief Builds an OTLP ExportLogsServiceRequest JSON payload.
    /// \param batch Log batch to serialize.
    /// \param config Export configuration.
    /// \return OTLP/HTTP JSON payload.
    inline std::string build_otlp_logs_json_payload(
            const std::vector<OtlpLogItem>& batch,
            const OtlpJsonFormatConfig& config) {
        std::ostringstream os;

        os << '{';
        os << "\"resourceLogs\":[{";
        os << "\"resource\":{\"attributes\":[";

        bool resource_first = true;
        otlp_write_comma_if_needed(os, resource_first);
        otlp_write_string_attr(os, "service.name", config.service_name);

        if (!config.service_namespace.empty()) {
            otlp_write_comma_if_needed(os, resource_first);
            otlp_write_string_attr(os, "service.namespace", config.service_namespace);
        }

        if (!config.service_instance_id.empty()) {
            otlp_write_comma_if_needed(os, resource_first);
            otlp_write_string_attr(os, "service.instance.id", config.service_instance_id);
        }

        if (!config.deployment_environment.empty()) {
            otlp_write_comma_if_needed(os, resource_first);
            otlp_write_string_attr(os, "deployment.environment.name", config.deployment_environment);
        }

        os << "]},";
        os << "\"scopeLogs\":[{";
        os << "\"scope\":{\"name\":\"logit-cpp\"},";
        os << "\"logRecords\":[";

        for (size_t i = 0; i < batch.size(); ++i) {
            if (i != 0) {
                os << ',';
            }
            otlp_write_log_record_json(os, batch[i], config);
        }

        os << ']';
        os << "}]";
        os << "}]";
        os << '}';

        return os.str();
    }

} // namespace logit

#endif // _LOGIT_OTLP_JSON_SERIALIZER_HPP_INCLUDED
