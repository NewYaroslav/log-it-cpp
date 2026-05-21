#include <logit/loggers/otlp/OtlpJsonSerializer.hpp>
#include <cassert>
#include <string>
#include <vector>

int main() {
    logit::OtlpJsonFormatConfig config;
    config.include_arg_names = true;
    config.service_name = "trade-bot";
    config.service_namespace = "tests";
    config.service_instance_id = "instance-1";
    config.deployment_environment = "dev";

    logit::OtlpLogItem item;
    item.record.log_level = logit::LogLevel::LOG_LVL_WARN;
    item.record.timestamp_ms = 1710000000123LL;
    item.record.file = "src/main.cpp";
    item.record.line = 42;
    item.record.function = "main";
    item.record.format = "Spread too high: %d";
    item.record.arg_names = "spread";
    item.record.thread_id = "test-thread";
    item.record.logger_index = -1;
    item.record.print_mode = false;
    item.record.fmt_mode = false;
    item.record.raw_mode = false;
    item.message = "Spread too high: 12\nquoted \"value\"";

    std::vector<logit::OtlpLogItem> batch;
    batch.push_back(item);

    const std::string payload = logit::build_otlp_logs_json_payload(batch, config);

    assert(payload.find("\"resourceLogs\"") != std::string::npos);
    assert(payload.find("\"service.name\",\"value\":{\"stringValue\":\"trade-bot\"}") != std::string::npos);
    assert(payload.find("\"service.namespace\",\"value\":{\"stringValue\":\"tests\"}") != std::string::npos);
    assert(payload.find("\"timeUnixNano\":\"1710000000123000000\"") != std::string::npos);
    assert(payload.find("\"severityText\":\"WARN\"") != std::string::npos);
    assert(payload.find("\"severityNumber\":13") != std::string::npos);
    assert(payload.find("Spread too high: 12\\nquoted \\\"value\\\"") != std::string::npos);
    assert(payload.find("\"code.file.path\",\"value\":{\"stringValue\":\"src/main.cpp\"}") != std::string::npos);
    assert(payload.find("\"code.line.number\",\"value\":{\"intValue\":\"42\"}") != std::string::npos);
    assert(payload.find("\"code.function.name\",\"value\":{\"stringValue\":\"main\"}") != std::string::npos);
    assert(payload.find("\"logit.arg_names\",\"value\":{\"stringValue\":\"spread\"}") != std::string::npos);

    return 0;
}
