#include <logit.hpp>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>

namespace {

std::string capture_stream(std::ostream& stream, const std::function<void()>& body) {
    std::stringstream buffer;
    auto* old = stream.rdbuf(buffer.rdbuf());
    body();
    stream.flush();
    stream.rdbuf(old);
    return buffer.str();
}

logit::LogRecord make_record(logit::LogLevel level, const char* message) {
    return logit::LogRecord(
        level,
        0,
        "console_logger_stream_test.cpp",
        0,
        "test",
        message,
        "",
        -1,
        false,
        false,
        false);
}

void test_default_constructor_writes_to_cout() {
    logit::ConsoleLogger logger;
    const std::string captured = capture_stream(std::cout, [&]() {
        logger.log(make_record(logit::LogLevel::LOG_LVL_INFO, "to-cout"),
                   "default-cout-message");
        logger.wait();
    });
    if (captured.find("default-cout-message") == std::string::npos) {
        std::cerr << "FAIL: default constructor should write to std::cout" << std::endl;
        std::exit(1);
    }
}

void test_explicit_cout_stream() {
    std::stringstream sink;
    logit::ConsoleLogger logger(sink);
    logger.log(make_record(logit::LogLevel::LOG_LVL_INFO, "explicit-cout"),
               "explicit-cout-message");
    logger.wait();
    if (sink.str().find("explicit-cout-message") == std::string::npos) {
        std::cerr << "FAIL: explicit ostringstream should receive message" << std::endl;
        std::exit(1);
    }
}

void test_cerr_stream_routing() {
    logit::ConsoleLogger cerr_logger(std::cerr);
    const std::string cerr_captured = capture_stream(std::cerr, [&]() {
        cerr_logger.log(make_record(logit::LogLevel::LOG_LVL_INFO, "to-cerr"),
                        "cerr-message");
        cerr_logger.wait();
    });
    if (cerr_captured.find("cerr-message") == std::string::npos) {
        std::cerr << "FAIL: cerr logger should write to std::cerr" << std::endl;
        std::exit(1);
    }
}

void test_cout_does_not_pick_up_cerr_output() {
    logit::ConsoleLogger cerr_logger(std::cerr);
    const std::string cout_captured = capture_stream(std::cout, [&]() {
        cerr_logger.log(make_record(logit::LogLevel::LOG_LVL_INFO, "to-cerr"),
                        "isolation-check");
        cerr_logger.wait();
    });
    if (cout_captured.find("isolation-check") != std::string::npos) {
        std::cerr << "FAIL: cerr logger should not write to std::cout" << std::endl;
        std::exit(1);
    }
}

void test_level_based_routing_to_cerr() {
    logit::ConsoleLogger::Config config;
    logit::ConsoleStreamRoute err_route;
    err_route.min_level = logit::LogLevel::LOG_LVL_ERROR;
    err_route.max_level = logit::LogLevel::LOG_LVL_FATAL;
    err_route.kind = logit::ConsoleStreamKind::Cerr;
    config.routes.push_back(err_route);

    logit::ConsoleLogger primary(std::cout, config);

    const std::string cout_captured = capture_stream(std::cout, [&]() {
        const std::string cerr_captured = capture_stream(std::cerr, [&]() {
            primary.log(make_record(logit::LogLevel::LOG_LVL_INFO, "info-record"),
                        "info-route");
            primary.log(make_record(logit::LogLevel::LOG_LVL_ERROR, "error-record"),
                        "error-route");
            primary.wait();
        });
        if (cerr_captured.find("error-route") == std::string::npos) {
            std::cerr << "FAIL: ERROR level should route to std::cerr" << std::endl;
            std::exit(1);
        }
        if (cerr_captured.find("info-route") != std::string::npos) {
            std::cerr << "FAIL: INFO level should not leak into std::cerr" << std::endl;
            std::exit(1);
        }
    });
    if (cout_captured.find("info-route") == std::string::npos) {
        std::cerr << "FAIL: INFO level should fall back to primary stream (cout)" << std::endl;
        std::exit(1);
    }
    if (cout_captured.find("error-route") != std::string::npos) {
        std::cerr << "FAIL: ERROR level should not fall through to primary stream" << std::endl;
        std::exit(1);
    }
}

void test_level_based_routing_to_custom_stream() {
    std::stringstream warn_sink;
    logit::ConsoleLogger::Config config;
    logit::ConsoleStreamRoute warn_route;
    warn_route.min_level = logit::LogLevel::LOG_LVL_WARN;
    warn_route.max_level = logit::LogLevel::LOG_LVL_WARN;
    warn_route.kind = logit::ConsoleStreamKind::Custom;
    warn_route.custom_stream = &warn_sink;
    config.routes.push_back(warn_route);

    logit::ConsoleLogger primary(std::cout, config);
    const std::string cout_captured = capture_stream(std::cout, [&]() {
        primary.log(make_record(logit::LogLevel::LOG_LVL_INFO, "info-record"),
                    "info-fallback");
        primary.log(make_record(logit::LogLevel::LOG_LVL_WARN, "warn-record"),
                    "warn-routed");
        primary.log(make_record(logit::LogLevel::LOG_LVL_ERROR, "error-record"),
                    "error-fallback");
        primary.wait();
    });
    if (warn_sink.str().find("warn-routed") == std::string::npos) {
        std::cerr << "FAIL: WARN should route to custom stream" << std::endl;
        std::exit(1);
    }
    if (cout_captured.find("info-fallback") == std::string::npos ||
        cout_captured.find("error-fallback") == std::string::npos) {
        std::cerr << "FAIL: non-matching levels should fall back to primary stream" << std::endl;
        std::exit(1);
    }
}

void test_null_custom_route_falls_back_to_primary_stream() {
    logit::ConsoleLogger::Config config;
    logit::ConsoleStreamRoute route;
    route.min_level = logit::LogLevel::LOG_LVL_ERROR;
    route.max_level = logit::LogLevel::LOG_LVL_FATAL;
    route.kind = logit::ConsoleStreamKind::Custom;
    route.custom_stream = nullptr;
    config.routes.push_back(route);

    std::stringstream sink;
    logit::ConsoleLogger logger(sink, config);

    logger.log(make_record(logit::LogLevel::LOG_LVL_ERROR, "error-record"),
               "fallback-error");
    logger.wait();

    if (sink.str().find("fallback-error") == std::string::npos) {
        std::cerr << "FAIL: null custom stream should fall back to primary stream" << std::endl;
        std::exit(1);
    }
}

void test_sync_custom_stream() {
    std::stringstream sink;
    logit::ConsoleLogger::Config config;
    config.async = false;

    logit::ConsoleLogger logger(sink, config);
    logger.log(make_record(logit::LogLevel::LOG_LVL_INFO, "sync-record"),
               "sync-message");

    if (sink.str().find("sync-message") == std::string::npos) {
        std::cerr << "FAIL: sync logger should write to custom stream" << std::endl;
        std::exit(1);
    }
}

void test_sync_level_based_routing_to_custom_stream() {
    std::stringstream primary_sink;
    std::stringstream error_sink;

    logit::ConsoleLogger::Config config;
    config.async = false;
    config.routes.push_back(logit::ConsoleStreamRoute::to_stream(
        logit::LogLevel::LOG_LVL_ERROR,
        logit::LogLevel::LOG_LVL_FATAL,
        error_sink));

    logit::ConsoleLogger logger(primary_sink, config);

    logger.log(make_record(logit::LogLevel::LOG_LVL_INFO, "info-record"),
               "sync-info");
    logger.log(make_record(logit::LogLevel::LOG_LVL_ERROR, "error-record"),
               "sync-error");

    if (primary_sink.str().find("sync-info") == std::string::npos) {
        std::cerr << "FAIL: INFO should go to primary stream" << std::endl;
        std::exit(1);
    }
    if (error_sink.str().find("sync-error") == std::string::npos) {
        std::cerr << "FAIL: ERROR should route to custom stream" << std::endl;
        std::exit(1);
    }
}

void test_route_helpers_to_cerr() {
    logit::ConsoleStreamRoute route = logit::ConsoleStreamRoute::to_cerr(
        logit::LogLevel::LOG_LVL_ERROR,
        logit::LogLevel::LOG_LVL_FATAL);

    if (route.kind != logit::ConsoleStreamKind::Cerr ||
        route.min_level != logit::LogLevel::LOG_LVL_ERROR ||
        route.max_level != logit::LogLevel::LOG_LVL_FATAL) {
        std::cerr << "FAIL: to_cerr helper should initialize route" << std::endl;
        std::exit(1);
    }
}

} // namespace

int main() {
    test_default_constructor_writes_to_cout();
    test_explicit_cout_stream();
    test_cerr_stream_routing();
    test_cout_does_not_pick_up_cerr_output();
    test_level_based_routing_to_cerr();
    test_level_based_routing_to_custom_stream();
    test_null_custom_route_falls_back_to_primary_stream();
    test_sync_custom_stream();
    test_sync_level_based_routing_to_custom_stream();
    test_route_helpers_to_cerr();
    return 0;
}
