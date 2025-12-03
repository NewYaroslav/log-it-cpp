#include "SpdlogAdapter.hpp"

#ifdef LOGIT_BENCH_HAVE_SPDLOG

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

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
        {
            std::lock_guard<std::mutex> lock(m_pending_mx);
            m_pending.clear();
        }
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

    void track_token(const LatencyRecorder::Token& token, std::unique_ptr<MessagePayload> payload) {
        std::lock_guard<std::mutex> lock(m_pending_mx);
        m_pending.push_back(Pending{std::move(payload), token});
    }

    void log(const spdlog::details::log_msg& msg) override {
        const char* func = msg.source.funcname;
        if (msg.payload.size() == 0 || !func || *func == '\0') {
            return; // Flush/control messages have no payload attached.
        }

        const auto* payload_ptr = reinterpret_cast<const MessagePayload*>(func);
        auto* payload = const_cast<MessagePayload*>(payload_ptr);
        consume(*payload, payload);
    }

    void set_pattern(const std::string&) override {}

    void set_formatter(std::unique_ptr<spdlog::formatter>) override {}

    void flush() override {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_file.is_open()) {
            m_file.flush();
        }
    }

    void complete_pending() {
        std::vector<Pending> pending;
        {
            std::lock_guard<std::mutex> lock(m_pending_mx);
            pending.swap(m_pending);
        }
        for (const auto& entry : pending) {
            if (entry.token.active && m_recorder) {
                m_recorder->complete(entry.token);
            }
        }
    }

private:
    void consume(const MessagePayload& payload, MessagePayload* payload_ptr) {
        LatencyRecorder::Token token = payload.token;
        std::unique_ptr<MessagePayload> owned;
        {
            std::lock_guard<std::mutex> lock(m_pending_mx);
            auto it = std::find_if(m_pending.begin(), m_pending.end(), [&](const Pending& p){ return p.payload.get() == payload_ptr; });
            if (it != m_pending.end()) {
                token = it->token;
                owned = std::move(it->payload);
                m_pending.erase(it);
            }
        }

        const MessagePayload& msg = owned ? *owned : payload;

        if (token.active && m_recorder) {
            m_recorder->complete(token);
        }
        if (m_sink == SinkKind::File) {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_file.is_open()) {
                m_file << msg.text << '\n';
            }
        }
    }

    SinkKind m_sink = SinkKind::Null;
    LatencyRecorder* m_recorder = nullptr;
    std::ofstream m_file;
    std::mutex m_mutex;
    struct Pending {
        std::unique_ptr<MessagePayload> payload;
        LatencyRecorder::Token token;
    };
    std::vector<Pending> m_pending;
    std::mutex m_pending_mx;
};

SpdlogAdapter::SpdlogAdapter() = default;

SpdlogAdapter::~SpdlogAdapter() {
    flush();
    spdlog::shutdown();
}

void SpdlogAdapter::set_recorder_handle(std::shared_ptr<LatencyRecorder> recorder) {
    m_recorder_handle = std::move(recorder);
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
    auto payload = std::make_unique<MessagePayload>();
    payload->token = token;
    payload->text.assign(message.data(), message.size());
    MessagePayload* payload_ptr = payload.get();
    if (m_sink) {
        m_sink->track_token(token, std::move(payload));
    }
    spdlog::source_loc loc{nullptr, 0, reinterpret_cast<const char*>(payload_ptr)};
    m_logger->log(loc, spdlog::level::info, spdlog::string_view_t(payload_ptr->text));
}

void SpdlogAdapter::flush() {
    if (m_logger) {
        m_logger->flush();
    }
    if (m_sink) {
        m_sink->complete_pending();
        m_sink->flush();
    }
}

} // namespace logit_bench

#endif // LOGIT_BENCH_HAVE_SPDLOG
