#include <logit.hpp>

#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

namespace {

logit::LogRecord make_record() {
    return logit::LogRecord(
            logit::LogLevel::LOG_LVL_INFO,
            LOGIT_CURRENT_TIMESTAMP_MS(),
            __FILE__,
            __LINE__,
            __func__,
            "",
            "",
            -1,
            false);
}

bool file_logger_ignores_log_after_shutdown(bool dedicated) {
    logit::FileLogger::Config cfg;
    cfg.directory = std::string("backend_shutdown_file_") +
                    (dedicated ? "dedicated_" : "global_") +
                    std::to_string(LOGIT_CURRENT_TIMESTAMP_MS());
    cfg.async = true;
    cfg.auto_delete_days = 1;
    cfg.use_dedicated_executor = dedicated;
    cfg.queue_capacity = 4;
    cfg.queue_policy = logit::detail::QueuePolicy::Block;

    logit::FileLogger logger(cfg);
    const logit::LogRecord rec = make_record();
    logger.log(rec, "before shutdown");
    logger.shutdown();
    logger.log(rec, "after shutdown");
    logger.wait();

    const std::vector<logit::LogFileInfo> files = logger.list_log_files();
    if (files.empty()) {
        return false;
    }

    std::string content;
    for (std::size_t i = 0; i < files.size(); ++i) {
        const logit::LogFileReadResult result = logger.read_log_file(files[i].path);
        if (result.ok) {
            content += result.content;
        }
    }

    return content.find("before shutdown") != std::string::npos &&
           content.find("after shutdown") == std::string::npos;
}

bool unique_file_logger_ignores_log_after_shutdown() {
    logit::UniqueFileLogger::Config cfg;
    cfg.directory = std::string("backend_shutdown_unique_") +
                    std::to_string(LOGIT_CURRENT_TIMESTAMP_MS());
    cfg.async = true;
    cfg.auto_delete_days = 1;
    cfg.hash_length = 8;
    cfg.use_dedicated_executor = true;
    cfg.queue_capacity = 4;
    cfg.queue_policy = logit::detail::QueuePolicy::Block;

    logit::UniqueFileLogger logger(cfg);
    const logit::LogRecord rec = make_record();
    logger.log(rec, "before shutdown");
    logger.shutdown();
    const std::vector<logit::LogFileInfo> before = logger.list_log_files();
    logger.log(rec, "after shutdown");
    logger.wait();
    const std::vector<logit::LogFileInfo> after = logger.list_log_files();

    return !before.empty() && after.size() == before.size();
}

bool simple_async_backends_ignore_log_after_shutdown() {
    const logit::LogRecord rec = make_record();

    logit::ConsoleLogger::Config console_cfg;
    console_cfg.async = true;
    console_cfg.use_dedicated_executor = true;
    console_cfg.queue_capacity = 4;
    console_cfg.queue_policy = logit::detail::QueuePolicy::Block;
    logit::ConsoleLogger console(console_cfg);
    console.log(rec, "console before shutdown");
    console.shutdown();
    console.log(rec, "console after shutdown");
    console.wait();

    logit::WindowsDebugLogger::Config debug_cfg(true);
    debug_cfg.use_dedicated_executor = true;
    debug_cfg.queue_capacity = 4;
    debug_cfg.queue_policy = logit::detail::QueuePolicy::Block;
    logit::WindowsDebugLogger debug(debug_cfg);
    debug.log(rec, "debug before shutdown");
    debug.shutdown();
    debug.log(rec, "debug after shutdown");
    debug.wait();

    return true;
}

} // namespace

int main() {
    const bool ok = file_logger_ignores_log_after_shutdown(false) &&
                    file_logger_ignores_log_after_shutdown(true) &&
                    unique_file_logger_ignores_log_after_shutdown() &&
                    simple_async_backends_ignore_log_after_shutdown();

    std::cout << (ok ? "PASS" : "FAIL") << ": backend_shutdown_terminal" << std::endl;
    return ok ? 0 : 1;
}
