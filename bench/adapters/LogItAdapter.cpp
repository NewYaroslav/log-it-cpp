#include "LogItAdapter.hpp"

#include <atomic>
#include <filesystem>
#include <fstream>
#include <limits>
#include <mutex>
#include <string>
#include <string_view>

#include <logit.hpp>

namespace logit_bench {
    namespace {
    
        constexpr const char* kFilePath = "bench/results/logit_sink.log";
        
        // record.line == -1 => not measured
        constexpr int kInactiveLine = -1;
    } // namespace

    class PassthroughFormatter : public logit::ILogFormatter {
    public:
        void set_timestamp_offset(int64_t) override {}
    
        std::string format(const logit::LogRecord& record) const override {
            return record.format;
        }
    
        bool is_passthrough() const noexcept override { return true; }
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
            const int slot_line = record.line;
    
            if (!m_async) {
                consume(slot_line, message);
                return;
            }
    
            if (m_sink == SinkKind::Null) {
                logit::detail::TaskExecutor::get_instance().add_task([this, slot_line]() {
                    consume(slot_line, std::string_view{});
                });
                return;
            }
    
            AsyncPayload payload;
            payload.slot_line = slot_line;
            payload.text = message;
    
            logit::detail::TaskExecutor::get_instance().add_task(
                [this, payload = std::move(payload)]() mutable {
                    consume(payload.slot_line, payload.text);
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
            int slot_line = kInactiveLine; // record.line copy
            std::string text;
        };
    
        void consume(int slot_line, std::string_view text) {
            // slot-only completion
            if (slot_line >= 0 && m_recorder) {
                m_recorder->complete_slot(static_cast<std::uint64_t>(slot_line));
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
            int line = kInactiveLine;
    
            // active => slot in record.line
            if (token.active) {
                // safety: avoid UB if someone sets huge totals
                if (token.slot <= static_cast<std::uint64_t>(std::numeric_limits<int>::max())) {
                    line = static_cast<int>(token.slot);
                } else {
                    line = kInactiveLine;
                }
            }
    
            logit::LogRecord record(
                logit::LogLevel::LOG_LVL_INFO,
                0,                 // logger index
                std::string(),     // file
                line,              // line = slot (or -1)
                std::string(),     // function
                std::string(message),
                std::string(),     // arg_names
                -1,
                false,
                false);
    
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
