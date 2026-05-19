#include <logit.hpp>

#ifdef LOGIT_WITH_OTLP

#include <server_http.hpp>

#include <cassert>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;

namespace {

struct CapturedRequest {
    std::mutex mutex;
    std::condition_variable cv;
    bool received = false;
    std::string body;
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

} // namespace

int main() {
    const unsigned short port = 43180;

    CapturedRequest captured;
    HttpServer server;
    server.config.port = port;

    server.resource["^/health$"]["GET"] = [](std::shared_ptr<HttpServer::Response> response,
                                              std::shared_ptr<HttpServer::Request>) {
        response->write(SimpleWeb::StatusCode::success_ok, "ok");
    };

    server.resource["^/v1/logs$"]["POST"] = [&captured](std::shared_ptr<HttpServer::Response> response,
                                                        std::shared_ptr<HttpServer::Request> request) {
        {
            std::lock_guard<std::mutex> lock(captured.mutex);
            captured.body = request->content.string();
            captured.received = true;
        }
        captured.cv.notify_one();
        response->write(SimpleWeb::StatusCode::success_ok, "{}");
    };

    std::thread server_thread([&server]() {
        server.start();
    });

    assert(wait_for_server(port));

    logit::OtlpHttpLoggerConfig config;
    config.host = "http://127.0.0.1:" + std::to_string(port);
    config.path = "/v1/logs";
    config.service_name = "logit-otlp-test";
    config.deployment_environment = "test";
    config.max_batch_size = 8;
    config.export_interval_ms = 50;
    config.request_timeout_sec = 2;

    LOGIT_ADD_LOGGER(
        logit::OtlpHttpLogger,
        (config),
        logit::SimpleLogFormatter,
        ("%v")
    );

    LOGIT_WARN("OTLP integration test message");
    LOGIT_WAIT();

    {
        std::unique_lock<std::mutex> lock(captured.mutex);
        captured.cv.wait_for(lock, std::chrono::seconds(3), [&captured]() {
            return captured.received;
        });
    }

    server.stop();
    if (server_thread.joinable()) {
        server_thread.join();
    }

    assert(captured.received);
    assert(captured.body.find("\"resourceLogs\"") != std::string::npos);
    assert(captured.body.find("\"service.name\",\"value\":{\"stringValue\":\"logit-otlp-test\"}") != std::string::npos);
    assert(captured.body.find("\"deployment.environment.name\",\"value\":{\"stringValue\":\"test\"}") != std::string::npos);
    assert(captured.body.find("\"severityText\":\"WARN\"") != std::string::npos);
    assert(captured.body.find("\"severityNumber\":13") != std::string::npos);
    assert(captured.body.find("OTLP integration test message") != std::string::npos);

    LOGIT_SHUTDOWN();

    return 0;
}

#else

int main() {
    return 0;
}

#endif
