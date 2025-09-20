#include "SpdlogAdapter.hpp"

#ifdef LOGIT_BENCH_HAVE_SPDLOG

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <string_view>

#include <spdlog/async.h>
#include <spdlog/async_logger.h>
#include <spdlog/sinks/sink.h>
#include <spdlog/spdlog.h>

namespace logit_bench {
namespace {
constexpr const char* kFilePath = "bench/results/spdlog_sink.log";
constexpr std::size_t kDefaultQueue = 8192;

struct MessagePayload {
    LatencyRecorder::Token token;
    std::string text;
};
} // namespace

class SpdlogAdapter::MeasuringSink : public spdlog::sinks::sink {
public:
    MeasuringSink() = default;

    void configure(const Scenario& scenario, LatencyRecorder& recorder) {
        m_sink = scenario.sink;
        m_recorder = &recorder;
        if (m_sink == SinkKind::File) {
            std::filesystem::create_directories("bench/results");
            std::lock_guard<std::mutex> lock(m_mutex);
            m_file.close();
            m_file.open(kFilePath, std::ios::out | std::ios::trunc);
        } else {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_file.close();
        }
    }

    void log(const spdlog::details::log_msg& msg) override {
        const auto* payload_ptr = reinterpret_cast<const MessagePayload*>(msg.source.funcname);
        if (!payload_ptr) {
            return;
        }
        auto* payload = const_cast<MessagePayload*>(payload_ptr);
        consume(*payload);
        delete payload;
    }

    void set_pattern(const std::string&) override {}

    void set_formatter(std::unique_ptr<spdlog::formatter>) override {}

    void flush() override {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_file.is_open()) {
            m_file.flush();
        }
    }

private:
    void consume(const MessagePayload& payload) {
        if (payload.token.active && m_recorder) {
            m_recorder->complete(payload.token);
        }
        if (m_sink == SinkKind::File) {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_file.is_open()) {
                m_file << payload.text << '\n';
            }
        }
    }

    SinkKind m_sink = SinkKind::Null;
    LatencyRecorder* m_recorder = nullptr;
    std::ofstream m_file;
    std::mutex m_mutex;
};

SpdlogAdapter::SpdlogAdapter() = default;

SpdlogAdapter::~SpdlogAdapter() {
    flush();
    spdlog::shutdown();
}

void SpdlogAdapter::prepare(const Scenario& scenario, LatencyRecorder& recorder) {
    m_logger.reset();
    m_sink.reset();
    spdlog::shutdown();

    m_sink = std::make_shared<MeasuringSink>();
    m_sink->configure(scenario, recorder);
    m_async = scenario.async;

    std::string logger_name = m_async ? "logit_bench_async" : "logit_bench_sync";
    if (m_async) {
        const std::size_t queue_size = std::max<std::size_t>(kDefaultQueue, scenario.total_messages * 2);
        spdlog::init_thread_pool(queue_size, 1);
        auto async_logger = std::make_shared<spdlog::async_logger>(
            logger_name,
            m_sink,
            spdlog::thread_pool(),
            spdlog::async_overflow_policy::block);
        async_logger->set_level(spdlog::level::trace);
        async_logger->set_pattern("%v");
        m_logger = std::move(async_logger);
    } else {
        auto logger = std::make_shared<spdlog::logger>(logger_name, m_sink);
        logger->set_level(spdlog::level::trace);
        logger->set_pattern("%v");
        m_logger = std::move(logger);
    }
}

void SpdlogAdapter::log(const LatencyRecorder::Token& token, std::string_view message) {
    if (!m_logger) {
        return;
    }
    auto* payload = new MessagePayload();
    payload->token = token;
    payload->text.assign(message.data(), message.size());
    spdlog::source_loc loc{nullptr, 0, reinterpret_cast<const char*>(payload)};
    m_logger->log(loc, spdlog::level::info, spdlog::string_view_t(payload->text));
}

void SpdlogAdapter::flush() {
    if (m_logger) {
        m_logger->flush();
    }
    if (m_sink) {
        m_sink->flush();
    }
}

} // namespace logit_bench

#endif // LOGIT_BENCH_HAVE_SPDLOG
