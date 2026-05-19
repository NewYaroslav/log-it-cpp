#pragma once
#ifndef _LOGIT_OTLP_JSON_SERIALIZER_HPP_INCLUDED
#define _LOGIT_OTLP_JSON_SERIALIZER_HPP_INCLUDED

/// \file OtlpJsonSerializer.hpp
/// \brief Defines OTLP/HTTP JSON serialization helpers for logs.

#include "OtlpHttpLoggerConfig.hpp"
#include "OtlpRecordSnapshot.hpp"
#include <cstdint>
#include <sstream>
#include <string>
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

    /// \brief Writes one OTLP JSON log record.
    /// \param os Output stream.
    /// \param item Log item to serialize.
    /// \param config Export configuration.
    inline void otlp_write_log_record_json(
            std::ostringstream& os,
            const OtlpLogItem& item,
            const OtlpHttpLoggerConfig& config) {
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
            const OtlpHttpLoggerConfig& config) {
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
