#include <logit/utils.hpp>

#ifdef LOGIT_WITH_OTLP

#include <logit/loggers/otlp/OtlpPayloadSplitter.hpp>
#include <cassert>
#include <cstddef>
#include <string>
#include <vector>

namespace {

logit::OtlpLogItem make_item(const std::string& message) {
    logit::OtlpLogItem item;
    item.record.log_level = logit::LogLevel::LOG_LVL_INFO;
    item.record.timestamp_ms = 1710000000123LL;
    item.record.file = "test.cpp";
    item.record.line = 1;
    item.record.function = "test_func";
    item.record.format = "%v";
    item.record.thread_id = "t1";
    item.record.logger_index = -1;
    item.record.print_mode = false;
    item.record.fmt_mode = false;
    item.record.raw_mode = false;
    item.message = message;
    return item;
}

} // namespace

int main() {
    logit::OtlpJsonFormatConfig config;
    config.service_name = "splitter-test";

    // Test a: max_payload_bytes == 0 => single chunk for 10-record batch
    {
        std::vector<logit::OtlpLogItem> batch;
        for (int i = 0; i < 10; ++i) {
            batch.push_back(make_item("msg " + std::to_string(i)));
        }

        auto chunks = logit::build_otlp_logs_json_payload_chunks(batch, config, 0);
        assert(chunks.size() == 1);
        assert(chunks[0].find("\"resourceLogs\"") != std::string::npos);
    }

    // Test b: small limit (1024 bytes) => batch of 50 records with long messages
    //         splits into >1 chunk; verify each chunk size <= 1024 OR oversized
    {
        std::vector<logit::OtlpLogItem> batch;
        for (int i = 0; i < 50; ++i) {
            batch.push_back(make_item("payload-split-test-message-" + std::to_string(i)));
        }

        auto chunks = logit::build_otlp_logs_json_payload_chunks(batch, config, 1024);
        assert(chunks.size() > 1);

        // Each chunk must be <= 1024 bytes OR it's an oversized single-record chunk
        for (const auto& chunk : chunks) {
            // Count "body" occurrences to know how many records are in this chunk
            std::size_t body_count = 0;
            std::size_t pos = 0;
            while ((pos = chunk.find("\"body\"", pos)) != std::string::npos) {
                ++body_count;
                ++pos;
            }
            if (body_count == 1) {
                // Oversized single-record chunk is allowed
            } else {
                assert(chunk.size() <= 1024);
            }
        }

        // All 50 records must appear across all chunks
        int total_bodies = 0;
        for (const auto& chunk : chunks) {
            std::size_t pos = 0;
            while ((pos = chunk.find("\"body\"", pos)) != std::string::npos) {
                ++total_bodies;
                ++pos;
            }
        }
        assert(total_bodies == 50);
    }

    // Test c: empty batch => empty chunks vector
    {
        std::vector<logit::OtlpLogItem> batch;
        auto chunks = logit::build_otlp_logs_json_payload_chunks(batch, config, 1024);
        assert(chunks.empty());
    }

    // Test d: single oversized record (5000-char message, limit 1024) => 1 chunk emitted
    {
        std::vector<logit::OtlpLogItem> batch;
        batch.push_back(make_item(std::string(5000, 'X')));

        auto chunks = logit::build_otlp_logs_json_payload_chunks(batch, config, 1024);
        assert(chunks.size() == 1);
        assert(chunks[0].size() > 1024);
        assert(chunks[0].find("\"resourceLogs\"") != std::string::npos);
    }

    return 0;
}

#else

int main() {
    return 0;
}

#endif
