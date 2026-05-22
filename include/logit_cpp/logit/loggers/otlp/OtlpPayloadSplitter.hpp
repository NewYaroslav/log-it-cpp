#pragma once
#ifndef _LOGIT_OTLP_PAYLOAD_SPLITTER_HPP_INCLUDED
#define _LOGIT_OTLP_PAYLOAD_SPLITTER_HPP_INCLUDED

/// \file OtlpPayloadSplitter.hpp
/// \brief Splits oversized OTLP log batches into multiple JSON payload chunks.

#include "OtlpJsonSerializer.hpp"
#include <string>
#include <vector>

namespace logit {

    /// \brief Builds OTLP JSON payload chunks from a batch, splitting when a chunk
    ///        would exceed max_payload_bytes.
    /// \param batch Log items to serialize.
    /// \param format Serialization configuration.
    /// \param max_payload_bytes Maximum serialized size per chunk (0 = no splitting).
    /// \return Vector of JSON payload strings, one per chunk.
    inline std::vector<std::string> build_otlp_logs_json_payload_chunks(
            const std::vector<OtlpLogItem>& batch,
            const OtlpJsonFormatConfig& format,
            std::size_t max_payload_bytes) {
        if (max_payload_bytes == 0) {
            std::vector<std::string> chunks;
            chunks.push_back(build_otlp_logs_json_payload(batch, format));
            return chunks;
        }

        if (batch.empty()) {
            return {};
        }

        std::vector<std::string> chunks;
        std::vector<OtlpLogItem> current_payload;

        for (std::size_t i = 0; i < batch.size(); ++i) {
            std::vector<OtlpLogItem> candidate_payload = current_payload;
            candidate_payload.push_back(batch[i]);

            std::string candidate_json = build_otlp_logs_json_payload(candidate_payload, format);

            if (candidate_json.size() <= max_payload_bytes) {
                current_payload = std::move(candidate_payload);
            } else if (current_payload.empty()) {
                // Single oversized record: emit it as its own chunk anyway.
                chunks.push_back(std::move(candidate_json));
            } else {
                // Close current chunk and start a new one with this record.
                chunks.push_back(build_otlp_logs_json_payload(current_payload, format));
                current_payload.clear();
                current_payload.push_back(batch[i]);
            }
        }

        if (!current_payload.empty()) {
            chunks.push_back(build_otlp_logs_json_payload(current_payload, format));
        }

        return chunks;
    }

} // namespace logit

#endif // _LOGIT_OTLP_PAYLOAD_SPLITTER_HPP_INCLUDED
