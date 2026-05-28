#include <logit.hpp>

#ifdef LOGIT_WITH_OTLP

#include <server_http.hpp>

#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;

namespace {

struct RequestCounter {
    std::mutex mutex;
    std::condition_variable cv;
    std::atomic<int> count{0};
    std::vector<std::string> bodies;
    std::string last_body;
    bool delay_response = false;
    int delay_ms = 0;
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

void start_server(HttpServer& server, std::thread& thread, RequestCounter& counter, unsigned short port, int status_code = 200) {
    server.config.port = port;

    server.resource["^/health$"]["GET"] = [](std::shared_ptr<HttpServer::Response> response,
                                              std::shared_ptr<HttpServer::Request>) {
        response->write(SimpleWeb::StatusCode::success_ok, "ok");
    };

    server.resource["^/v1/logs$"]["POST"] = [&counter, status_code](std::shared_ptr<HttpServer::Response> response,
                                                                     std::shared_ptr<HttpServer::Request> request) {
        if (counter.delay_response) {
            std::this_thread::sleep_for(std::chrono::milliseconds(counter.delay_ms));
        }
        {
            std::lock_guard<std::mutex> lock(counter.mutex);
            counter.last_body = request->content.string();
            counter.bodies.push_back(counter.last_body);
            counter.count.fetch_add(1);
        }
        counter.cv.notify_all();

        if (status_code >= 200 && status_code < 300) {
            response->write(static_cast<SimpleWeb::StatusCode>(status_code), "{}");
        } else {
            response->write(static_cast<SimpleWeb::StatusCode>(status_code), "error");
        }
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
    const unsigned short port = 43181;

    // Test a: Single batch callback export
    {
        RequestCounter counter;
        HttpServer server;
        std::thread server_thread;
        start_server(server, server_thread, counter, port);

        logit::OtlpHttpLogger::Config config;
        config.host = "http://127.0.0.1:" + std::to_string(port);
        config.path = "/v1/logs";
        config.format.service_name = "callback-test";
        config.max_batch_size = 256;
        config.export_interval_ms = 50;
        config.request_timeout_sec = 2;

        LOGIT_ADD_LOGGER(
            logit::OtlpHttpLogger,
            (config),
            logit::SimpleLogFormatter,
            ("%v")
        );

        LOGIT_WARN("single batch callback test");
        LOGIT_WAIT();

        {
            std::unique_lock<std::mutex> lock(counter.mutex);
            counter.cv.wait_for(lock, std::chrono::seconds(3), [&counter]() {
                return counter.count.load() >= 1;
            });
        }

        assert(counter.count.load() >= 1);
        assert(counter.last_body.find("\"resourceLogs\"") != std::string::npos);

        LOGIT_SHUTDOWN();
        stop_server(server, server_thread);
    }

    // Test b: max_in_flight_requests=1 blocking
    {
        RequestCounter counter;
        counter.delay_response = true;
        counter.delay_ms = 500;
        HttpServer server;
        std::thread server_thread;
        start_server(server, server_thread, counter, port);

        logit::OtlpHttpLogger::Config config;
        config.host = "http://127.0.0.1:" + std::to_string(port);
        config.path = "/v1/logs";
        config.format.service_name = "callback-test";
        config.max_batch_size = 1;
        config.max_in_flight_requests = 1;
        config.export_interval_ms = 50;
        config.request_timeout_sec = 5;

        LOGIT_ADD_LOGGER(
            logit::OtlpHttpLogger,
            (config),
            logit::SimpleLogFormatter,
            ("%v")
        );

        LOGIT_WARN("in-flight msg 1");
        LOGIT_WARN("in-flight msg 2");

        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        assert(counter.count.load() == 1);

        LOGIT_WAIT();
        LOGIT_SHUTDOWN();
        stop_server(server, server_thread);
    }

    // Test c: graceful shutdown drains queued backlog while in-flight is full
    {
        RequestCounter counter;
        counter.delay_response = true;
        counter.delay_ms = 300;
        HttpServer server;
        std::thread server_thread;
        start_server(server, server_thread, counter, port);

        logit::OtlpHttpLogger::Config config;
        config.host = "http://127.0.0.1:" + std::to_string(port);
        config.path = "/v1/logs";
        config.format.service_name = "callback-test";
        config.max_batch_size = 1;
        config.max_in_flight_requests = 1;
        config.export_interval_ms = 10;
        config.request_timeout_sec = 5;
        config.cancel_on_shutdown = false;

        auto logger = std::unique_ptr<logit::OtlpHttpLogger>(new logit::OtlpHttpLogger(config));

        logit::LogRecord record1(
            logit::LogLevel::LOG_LVL_WARN, 1710000000123LL,
            "test.cpp", 120, "test_func", "shutdown backlog 1", "",
            -1, false, false, false);
        logit::LogRecord record2(
            logit::LogLevel::LOG_LVL_WARN, 1710000000124LL,
            "test.cpp", 121, "test_func", "shutdown backlog 2", "",
            -1, false, false, false);

        logger->log(record1, "shutdown backlog msg 1");
        logger->log(record2, "shutdown backlog msg 2");

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        auto start = std::chrono::steady_clock::now();
        logger->shutdown();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();

        assert(counter.count.load() == 2);
        assert(elapsed >= 300);

        stop_server(server, server_thread);
    }

    // Test d: max_in_flight_requests=2 parallelism
    {
        RequestCounter counter;
        counter.delay_response = true;
        counter.delay_ms = 500;
        HttpServer server;
        std::thread server_thread;
        start_server(server, server_thread, counter, port);

        logit::OtlpHttpLogger::Config config;
        config.host = "http://127.0.0.1:" + std::to_string(port);
        config.path = "/v1/logs";
        config.format.service_name = "callback-test";
        config.max_batch_size = 1;
        config.max_in_flight_requests = 2;
        config.export_interval_ms = 50;
        config.request_timeout_sec = 5;

        LOGIT_ADD_LOGGER(
            logit::OtlpHttpLogger,
            (config),
            logit::SimpleLogFormatter,
            ("%v")
        );

        LOGIT_WARN("parallel msg 1");
        LOGIT_WARN("parallel msg 2");

        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        assert(counter.count.load() == 2);

        LOGIT_WAIT();
        LOGIT_SHUTDOWN();
        stop_server(server, server_thread);
    }

    // Test e: HTTP 500 failure counting
    {
        RequestCounter counter;
        HttpServer server;
        std::thread server_thread;
        start_server(server, server_thread, counter, port, 500);

        logit::OtlpHttpLogger::Config config;
        config.host = "http://127.0.0.1:" + std::to_string(port);
        config.path = "/v1/logs";
        config.format.service_name = "callback-test";
        config.max_batch_size = 256;
        config.export_interval_ms = 50;
        config.request_timeout_sec = 2;

        LOGIT_ADD_LOGGER(
            logit::OtlpHttpLogger,
            (config),
            logit::SimpleLogFormatter,
            ("%v")
        );

        LOGIT_WARN("failure test");
        LOGIT_WAIT();

        {
            std::unique_lock<std::mutex> lock(counter.mutex);
            counter.cv.wait_for(lock, std::chrono::seconds(3), [&counter]() {
                return counter.count.load() >= 1;
            });
        }

        uint64_t failed = static_cast<uint64_t>(LOGIT_GET_INT_PARAM(0, logit::LoggerParam::FailedExportCount));
        assert(failed > 0);

        LOGIT_SHUTDOWN();
        stop_server(server, server_thread);
    }

    // Test f: wait() waits for callbacks
    {
        RequestCounter counter;
        counter.delay_response = true;
        counter.delay_ms = 1000;
        HttpServer server;
        std::thread server_thread;
        start_server(server, server_thread, counter, port);

        logit::OtlpHttpLogger::Config config;
        config.host = "http://127.0.0.1:" + std::to_string(port);
        config.path = "/v1/logs";
        config.format.service_name = "callback-test";
        config.max_batch_size = 256;
        config.export_interval_ms = 50;
        config.request_timeout_sec = 5;

        LOGIT_ADD_LOGGER(
            logit::OtlpHttpLogger,
            (config),
            logit::SimpleLogFormatter,
            ("%v")
        );

        auto start = std::chrono::steady_clock::now();
        LOGIT_WARN("wait test");
        LOGIT_WAIT();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();

        assert(elapsed >= 800);
        assert(counter.count.load() >= 1);

        LOGIT_SHUTDOWN();
        stop_server(server, server_thread);
    }

    // Test g: Graceful shutdown no UAF
    {
        RequestCounter counter;
        counter.delay_response = true;
        counter.delay_ms = 5000;
        HttpServer server;
        std::thread server_thread;
        start_server(server, server_thread, counter, port);

        logit::OtlpHttpLogger::Config config;
        config.host = "http://127.0.0.1:" + std::to_string(port);
        config.path = "/v1/logs";
        config.format.service_name = "callback-test";
        config.max_batch_size = 256;
        config.export_interval_ms = 50;
        config.request_timeout_sec = 10;
        config.cancel_on_shutdown = false;

        LOGIT_ADD_LOGGER(
            logit::OtlpHttpLogger,
            (config),
            logit::SimpleLogFormatter,
            ("%v")
        );

        LOGIT_WARN("graceful shutdown test");
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        auto start = std::chrono::steady_clock::now();
        LOGIT_SHUTDOWN();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();

        assert(elapsed >= 4000);

        LOGIT_SHUTDOWN();
        stop_server(server, server_thread);
    }

    // Test h: cancel_on_shutdown=true fast shutdown
    {
        RequestCounter counter;
        counter.delay_response = true;
        counter.delay_ms = 5000;
        HttpServer server;
        std::thread server_thread;
        start_server(server, server_thread, counter, port);

        logit::OtlpHttpLogger::Config config;
        config.host = "http://127.0.0.1:" + std::to_string(port);
        config.path = "/v1/logs";
        config.format.service_name = "callback-test";
        config.max_batch_size = 256;
        config.export_interval_ms = 50;
        config.request_timeout_sec = 10;
        config.cancel_on_shutdown = true;

        LOGIT_ADD_LOGGER(
            logit::OtlpHttpLogger,
            (config),
            logit::SimpleLogFormatter,
            ("%v")
        );

        LOGIT_WARN("cancel shutdown test");
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        auto start = std::chrono::steady_clock::now();
        LOGIT_SHUTDOWN();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();

        assert(elapsed < 1000);

        LOGIT_SHUTDOWN();
        stop_server(server, server_thread);
    }

    // Test i: payload splitting produces multiple POSTs with small max_payload_bytes
    {
        RequestCounter counter;
        HttpServer server;
        std::thread server_thread;
        start_server(server, server_thread, counter, port);

        logit::OtlpHttpLogger::Config config;
        config.host = "http://127.0.0.1:" + std::to_string(port);
        config.path = "/v1/logs";
        config.format.service_name = "http-split-test";
        config.max_batch_size = 256;
        config.max_payload_bytes = 1024;
        config.export_interval_ms = 50;
        config.request_timeout_sec = 5;
        config.max_in_flight_requests = 8;

        auto logger = std::unique_ptr<logit::OtlpHttpLogger>(new logit::OtlpHttpLogger(config));

        for (int i = 0; i < 50; ++i) {
            logit::LogRecord record(
                logit::LogLevel::LOG_LVL_WARN, 1710000000123LL + i,
                "test.cpp", 200 + i, "test_func", "http split test", "",
                -1, false, false, false);
            logger->log(record, "http split payload test message number " + std::to_string(i));
        }

        logger->wait();
        logger->shutdown();

        {
            std::unique_lock<std::mutex> lock(counter.mutex);
            counter.cv.wait_for(lock, std::chrono::seconds(3), [&counter]() {
                return counter.count.load() >= 2;
            });
        }

        assert(counter.count.load() > 1);

        int body_count = 0;
        for (const auto& body : counter.bodies) {
            std::size_t pos = 0;
            while ((pos = body.find("\"body\"", pos)) != std::string::npos) {
                ++body_count;
                ++pos;
            }
        }
        assert(body_count == 50);

        stop_server(server, server_thread);
    }

    return 0;
}

#else

int main() {
    return 0;
}

#endif
