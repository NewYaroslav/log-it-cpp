#include "LogItAdapter.hpp"

#include <atomic>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <string_view>

#include <logit.hpp>

namespace logit_bench {
namespace {
constexpr const char* kFilePath = "bench/results/logit_sink.log";
constexpr std::size_t kSlotIndex = 0;
constexpr std::size_t kT0Index = 1;
constexpr std::size_t kActiveIndex = 2;
} // namespace

class PassthroughFormatter : public logit::ILogFormatter {
public:
    void set_timestamp_offset(int64_t) override {}

    std::string format(const logit::LogRecord& record) const override {
        return record.format;
    }
};

class MeasuringSink : public logit::ILogger {
public:
    MeasuringSink() = default;

    void configure(const Scenario& scenario, LatencyRecorder& recorder) {
        m_async = scenario.async;
        m_sink = scenario.sink;
        m_recorder = &recorder;
        if (m_sink == SinkKind::File) {
            std::filesystem::create_directories("bench/results");
            std::lock_guard<std::mutex> lock(m_file_mutex);
            m_file.close();
            m_file.open(kFilePath, std::ios::out | std::ios::trunc);
        } else {
            std::lock_guard<std::mutex> lock(m_file_mutex);
            m_file.close();
        }
    }

    void log(const logit::LogRecord& record, const std::string& message) override {
        LatencyRecorder::Token token = extract(record);
        if (!m_async) {
            consume(token, message);
            return;
        }
        if (m_sink == SinkKind::Null) {
            logit::detail::TaskExecutor::get_instance().add_task([this, token]() {
                consume(token, std::string_view{});
            });
            return;
        }

        AsyncPayload payload;
        payload.token = token;
        payload.text = message;
        logit::detail::TaskExecutor::get_instance().add_task([this, payload = std::move(payload)]() mutable {
            consume(payload.token, payload.text);
        });
    }

    std::string get_string_param(const logit::LoggerParam&) const override { return std::string(); }
    int64_t get_int_param(const logit::LoggerParam&) const override { return 0; }
    double get_float_param(const logit::LoggerParam&) const override { return 0.0; }

    void set_log_level(logit::LogLevel level) override {
        m_level.store(static_cast<int>(level), std::memory_order_relaxed);
    }

    logit::LogLevel get_log_level() const override {
        return static_cast<logit::LogLevel>(m_level.load(std::memory_order_relaxed));
    }

    void wait() override {
        if (m_async) {
            logit::detail::TaskExecutor::get_instance().wait();
        }
        std::lock_guard<std::mutex> lock(m_file_mutex);
        if (m_file.is_open()) {
            m_file.flush();
        }
    }

private:
    struct AsyncPayload {
        LatencyRecorder::Token token;
        std::string text;
    };

    static LatencyRecorder::Token extract(const logit::LogRecord& record) {
        LatencyRecorder::Token token;
        if (record.args_array.size() <= kActiveIndex) {
            return token;
        }
        const auto& slot = record.args_array[kSlotIndex];
        const auto& t0 = record.args_array[kT0Index];
        const auto& active = record.args_array[kActiveIndex];
        token.slot = read_u64(slot);
        token.t0_ns = read_u64(t0);
        token.active = read_u64(active) != 0;
        return token;
    }

    static std::uint64_t read_u64(const logit::VariableValue& value) {
        using VT = logit::VariableValue::ValueType;
        switch (value.type) {
        case VT::UINT64_VAL:
            return value.pod_value.uint64_value;
        case VT::INT64_VAL:
            return static_cast<std::uint64_t>(value.pod_value.int64_value);
        case VT::UINT32_VAL:
            return value.pod_value.uint32_value;
        case VT::INT32_VAL:
            return static_cast<std::uint64_t>(value.pod_value.int32_value);
        default:
            break;
        }
        return 0;
    }

    void consume(const LatencyRecorder::Token& token, std::string_view text) {
        if (token.active && m_recorder) {
            m_recorder->complete(token);
        }
        if (m_sink == SinkKind::File) {
            std::lock_guard<std::mutex> lock(m_file_mutex);
            if (m_file.is_open()) {
                m_file << text << '\n';
            }
        }
    }

    bool m_async = false;
    SinkKind m_sink = SinkKind::Null;
    LatencyRecorder* m_recorder = nullptr;
    std::ofstream m_file;
    mutable std::mutex m_file_mutex;
    std::atomic<int> m_level{static_cast<int>(logit::LogLevel::LOG_LVL_TRACE)};
};

class LogItAdapter::Impl {
public:
    Impl()
        : logger(logit::Logger::get_instance()) {
        auto sink_ptr = std::make_unique<MeasuringSink>();
        sink = sink_ptr.get();
        auto formatter = std::unique_ptr<logit::ILogFormatter>(new PassthroughFormatter());
        logger.add_logger(std::move(sink_ptr), std::move(formatter));
    }

    void prepare(const Scenario& scenario, LatencyRecorder& recorder) {
        if (sink) {
            sink->configure(scenario, recorder);
        }
    }

    void log(const LatencyRecorder::Token& token, std::string_view message) {
        logit::LogRecord record(
            logit::LogLevel::LOG_LVL_INFO,
            0,
            std::string(),
            0,
            std::string(),
            std::string(message),
            std::string(),
            -1,
            false,
            false);
        record.args_array.reserve(3);
        record.args_array.emplace_back("slot", static_cast<std::uint64_t>(token.slot));
        record.args_array.emplace_back("t0", static_cast<std::uint64_t>(token.t0_ns));
        record.args_array.emplace_back("active", static_cast<std::uint64_t>(token.active ? 1 : 0));
        logger.log(record);
    }

    void flush() {
        if (sink) {
            sink->wait();
        }
    }

    logit::Logger& logger;
    MeasuringSink* sink = nullptr;
};

LogItAdapter::LogItAdapter()
    : m_impl(std::make_unique<Impl>()) {}

LogItAdapter::~LogItAdapter() = default;

void LogItAdapter::prepare(const Scenario& scenario, LatencyRecorder& recorder) {
    if (m_impl) {
        m_impl->prepare(scenario, recorder);
    }
}

void LogItAdapter::log(const LatencyRecorder::Token& token, std::string_view message) {
    if (m_impl) {
        m_impl->log(token, message);
    }
}

void LogItAdapter::flush() {
    if (m_impl) {
        m_impl->flush();
    }
}

} // namespace logit_bench
