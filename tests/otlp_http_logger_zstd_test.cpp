#include <logit.hpp>

#if defined(LOGIT_WITH_OTLP) && defined(LOGIT_HAS_ZSTD)

#include <server_http.hpp>
#include <zstd.h>

#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;

namespace {

struct RequestCapture {
    std::mutex mutex;
    std::condition_variable cv;
    std::atomic<int> count{0};
    std::string last_body;
    bool has_content_encoding_zstd = false;
};

bool wait_for_server(unsigned short port) {
    for (int i = 0; i < 50; ++i) {
        try {
            kurlyk::HttpClient client("http://127.0.0.1:" + std::to_string(port));
            client.set_timeout(1);
            auto future = client.get("/health", {}, {});
            if (future.wait_for(std::chrono::seconds(2)) == std::future_status::ready) {
                auto response = future.get();
                if (response && response->status_code == 200) {
                    return true;
                }
            }
        } catch (...) {
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    return false;
}

void start_server(HttpServer& server, std::thread& thread, RequestCapture& capture, unsigned short port) {
    server.config.port = port;

    server.resource["^/health$"]["GET"] = [](std::shared_ptr<HttpServer::Response> response,
                                              std::shared_ptr<HttpServer::Request>) {
        response->write(SimpleWeb::StatusCode::success_ok, "ok");
    };

    server.resource["^/v1/logs$"]["POST"] = [&capture](std::shared_ptr<HttpServer::Response> response,
                                                         std::shared_ptr<HttpServer::Request> request) {
        {
            std::lock_guard<std::mutex> lock(capture.mutex);
            capture.last_body = request->content.string();

            auto it = request->header.find("Content-Encoding");
            if (it != request->header.end() && it->second == "zstd") {
                capture.has_content_encoding_zstd = true;
            }

            capture.count.fetch_add(1);
        }
        capture.cv.notify_all();

        response->write(SimpleWeb::StatusCode::success_ok, "{}");
    };

    thread = std::thread([&server]() {
        server.start();
    });

    assert(wait_for_server(port));
}

void stop_server(HttpServer& server, std::thread& thread) {
    server.stop();
    if (thread.joinable()) {
        thread.join();
    }
}

} // namespace

int main() {
    const unsigned short port = 43183;

    RequestCapture capture;
    HttpServer server;
    std::thread server_thread;
    start_server(server, server_thread, capture, port);

    logit::OtlpHttpLogger::Config config;
    config.host = "http://127.0.0.1:" + std::to_string(port);
    config.path = "/v1/logs";
    config.format.service_name = "zstd-test";
    config.compression = logit::OtlpCompression::Zstd;
    config.compression_level = 3;
    config.max_batch_size = 256;
    config.export_interval_ms = 50;
    config.request_timeout_sec = 2;

    auto logger = std::unique_ptr<logit::OtlpHttpLogger>(new logit::OtlpHttpLogger(config));

    for (int i = 0; i < 3; ++i) {
        logit::LogRecord record(
            logit::LogLevel::LOG_LVL_WARN, 1710000000123LL + i,
            "test.cpp", 100 + i, "test_func", "zstd test", "",
            -1, false, false, false);
        logger->log(record, "zstd compression test message " + std::to_string(i));
    }

    logger->wait();
    logger->shutdown();

    {
        std::unique_lock<std::mutex> lock(capture.mutex);
        capture.cv.wait_for(lock, std::chrono::seconds(3), [&capture]() {
            return capture.count.load() >= 1;
        });
    }

    assert(capture.count.load() >= 1);
    assert(capture.has_content_encoding_zstd);
    assert(capture.last_body.find("resourceLogs") == std::string::npos);

    unsigned long long raw_size =
        ZSTD_getFrameContentSize(capture.last_body.data(), capture.last_body.size());
    assert(raw_size != ZSTD_CONTENTSIZE_ERROR);
    assert(raw_size != ZSTD_CONTENTSIZE_UNKNOWN);

    std::vector<char> decompressed(static_cast<std::size_t>(raw_size));
    std::size_t result = ZSTD_decompress(
        decompressed.data(), decompressed.size(),
        capture.last_body.data(), capture.last_body.size());
    assert(!ZSTD_isError(result));

    std::string json(decompressed.data(), result);
    assert(json.find("\"resourceLogs\"") != std::string::npos);
    assert(json.find("zstd compression test message") != std::string::npos);

    stop_server(server, server_thread);

    return 0;
}

#else

int main() {
    return 0;
}

#endif
