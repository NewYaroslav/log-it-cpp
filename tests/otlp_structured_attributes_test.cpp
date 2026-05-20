#include <logit/loggers/otlp/OtlpJsonSerializer.hpp>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace {

bool json_contains(const std::string& json, const std::string& fragment) {
    return json.find(fragment) != std::string::npos;
}

bool json_not_contains(const std::string& json, const std::string& fragment) {
    return json.find(fragment) == std::string::npos;
}

logit::OtlpLogItem make_item_with_args(
        const std::vector<logit::VariableValue>& args) {
    logit::OtlpLogItem item;
    item.record.log_level = logit::LogLevel::LOG_LVL_INFO;
    item.record.timestamp_ms = 1710000000123LL;
    item.record.file = "test.cpp";
    item.record.line = 1;
    item.record.function = "test_func";
    item.record.format = "test";
    item.record.logger_index = -1;
    item.record.print_mode = false;
    item.record.fmt_mode = false;
    item.record.raw_mode = false;
    item.record.args_array = args;
    item.message = "test message";
    return item;
}

std::string serialize_single(const logit::OtlpLogItem& item,
                              const logit::OtlpHttpLoggerConfig& config) {
    std::vector<logit::OtlpLogItem> batch;
    batch.push_back(item);
    return logit::build_otlp_logs_json_payload(batch, config);
}

} // namespace

int main() {
    // string attr
    {
        logit::OtlpHttpLoggerConfig config;
        config.include_arg_names = false;
        std::vector<logit::VariableValue> args;
        args.push_back(logit::VariableValue("sym", std::string("AAPL")));
        std::string json = serialize_single(make_item_with_args(args), config);
        assert(json_contains(json, "\"key\":\"logit.arg.sym\",\"value\":{\"stringValue\":\"AAPL\"}"));
    }

    // int attr
    {
        logit::OtlpHttpLoggerConfig config;
        config.include_arg_names = false;
        std::vector<logit::VariableValue> args;
        args.push_back(logit::VariableValue("vol", 100));
        std::string json = serialize_single(make_item_with_args(args), config);
        assert(json_contains(json, "\"key\":\"logit.arg.vol\",\"value\":{\"intValue\":\"100\"}"));
    }

    // uint64 > INT64_MAX
    {
        logit::OtlpHttpLoggerConfig config;
        config.include_arg_names = false;
        std::vector<logit::VariableValue> args;
        args.push_back(logit::VariableValue("ts", 18446744073709551615ULL));
        std::string json = serialize_single(make_item_with_args(args), config);
        assert(json_contains(json, "\"key\":\"logit.arg.ts\",\"value\":{\"stringValue\":\"18446744073709551615\"}"));
    }

    // double finite
    {
        logit::OtlpHttpLoggerConfig config;
        config.include_arg_names = false;
        std::vector<logit::VariableValue> args;
        args.push_back(logit::VariableValue("px", 3.14));
        std::string json = serialize_single(make_item_with_args(args), config);
        assert(json_contains(json, "\"key\":\"logit.arg.px\",\"value\":{\"doubleValue\":\"3.14\"}"));
    }

    // double NaN
    {
        logit::OtlpHttpLoggerConfig config;
        config.include_arg_names = false;
        std::vector<logit::VariableValue> args;
        args.push_back(logit::VariableValue("bad", NAN));
        std::string json = serialize_single(make_item_with_args(args), config);
        assert(json_contains(json, "\"key\":\"logit.arg.bad\",\"value\":{\"stringValue\""));
        assert(json_not_contains(json, "\"key\":\"logit.arg.bad\",\"value\":{\"doubleValue\""));
    }

    // bool attr
    {
        logit::OtlpHttpLoggerConfig config;
        config.include_arg_names = false;
        std::vector<logit::VariableValue> args;
        args.push_back(logit::VariableValue("ok", true));
        std::string json = serialize_single(make_item_with_args(args), config);
        assert(json_contains(json, "\"key\":\"logit.arg.ok\",\"value\":{\"boolValue\":true}"));
    }

    // char attr (stored as string, serializer emits stringValue)
    {
        logit::OtlpHttpLoggerConfig config;
        config.include_arg_names = false;
        std::vector<logit::VariableValue> args;
        args.push_back(logit::VariableValue("ch", std::string("x")));
        std::string json = serialize_single(make_item_with_args(args), config);
        assert(json_contains(json, "\"key\":\"logit.arg.ch\",\"value\":{\"stringValue\":\"x\"}"));
    }

    // enum attr
    {
        logit::OtlpHttpLoggerConfig config;
        config.include_arg_names = false;
        enum Color { RED = 2, GREEN = 5 };
        std::vector<logit::VariableValue> args;
        args.push_back(logit::VariableValue("color", Color::GREEN));
        std::string json = serialize_single(make_item_with_args(args), config);
        assert(json_contains(json, "\"key\":\"logit.arg.color\",\"value\":{\"stringValue\":\"5\"}"));
    }

    // duplicate names
    {
        logit::OtlpHttpLoggerConfig config;
        config.include_arg_names = false;
        std::vector<logit::VariableValue> args;
        args.push_back(logit::VariableValue("px", 1));
        args.push_back(logit::VariableValue("px", 2));
        std::string json = serialize_single(make_item_with_args(args), config);
        assert(json_contains(json, "\"key\":\"logit.arg.px\",\"value\":{\"intValue\":\"1\"}"));
        assert(json_contains(json, "\"key\":\"logit.arg.px.1\",\"value\":{\"intValue\":\"2\"}"));
    }

    // empty names (positional fallback)
    {
        logit::OtlpHttpLoggerConfig config;
        config.include_arg_names = false;
        std::vector<logit::VariableValue> args;
        args.push_back(logit::VariableValue("", 1));
        args.push_back(logit::VariableValue("", 2));
        std::string json = serialize_single(make_item_with_args(args), config);
        assert(json_contains(json, "\"key\":\"logit.arg.0\",\"value\":{\"intValue\":\"1\"}"));
        assert(json_contains(json, "\"key\":\"logit.arg.1\",\"value\":{\"intValue\":\"2\"}"));
    }

    // sanitized invalid chars
    {
        logit::OtlpHttpLoggerConfig config;
        config.include_arg_names = false;
        std::vector<logit::VariableValue> args;
        args.push_back(logit::VariableValue("a b", std::string("x")));
        std::string json = serialize_single(make_item_with_args(args), config);
        assert(json_contains(json, "\"key\":\"logit.arg.a_b\",\"value\":{\"stringValue\":\"x\"}"));
    }

    // custom prefix
    {
        logit::OtlpHttpLoggerConfig config;
        config.include_arg_names = false;
        config.args_prefix = "user.";
        std::vector<logit::VariableValue> args;
        args.push_back(logit::VariableValue("sym", std::string("AAPL")));
        std::string json = serialize_single(make_item_with_args(args), config);
        assert(json_contains(json, "\"key\":\"user.sym\",\"value\":{\"stringValue\":\"AAPL\"}"));
    }

    // include_args=false, include_arg_names=false: no arg-related attributes
    {
        logit::OtlpHttpLoggerConfig config;
        config.include_args = false;
        config.include_arg_names = false;
        std::vector<logit::VariableValue> args;
        args.push_back(logit::VariableValue("x", 42));
        std::string json = serialize_single(make_item_with_args(args), config);
        assert(json_not_contains(json, "logit.arg.x"));
        assert(json_not_contains(json, "logit.arg_names"));
    }

    // include_arg_names legacy (include_args=false)
    {
        logit::OtlpHttpLoggerConfig config;
        config.include_args = false;
        config.include_arg_names = true;
        logit::OtlpLogItem item;
        item.record.log_level = logit::LogLevel::LOG_LVL_INFO;
        item.record.timestamp_ms = 1710000000123LL;
        item.record.file = "test.cpp";
        item.record.line = 1;
        item.record.function = "test_func";
        item.record.format = "test";
        item.record.arg_names = "x,y";
        item.record.logger_index = -1;
        item.record.print_mode = false;
        item.record.fmt_mode = false;
        item.record.raw_mode = false;
        item.message = "test";
        std::string json = serialize_single(item, config);
        assert(json_contains(json, "\"key\":\"logit.arg_names\",\"value\":{\"stringValue\":\"x,y\"}"));
        assert(json_not_contains(json, "logit.arg."));
    }

    return 0;
}
