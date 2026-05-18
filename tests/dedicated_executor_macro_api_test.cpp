#include <logit.hpp>

#include <iostream>

int main() {
    logit::ConsoleLogger::Config console_cfg;
    console_cfg.async = true;
    console_cfg.use_dedicated_executor = true;
    console_cfg.queue_capacity = 4;
    console_cfg.queue_policy = logit::detail::QueuePolicy::Block;
    LOGIT_ADD_CONSOLE_CONFIG(console_cfg, "%v");
    LOGIT_ADD_CONSOLE_DEDICATED("%v", 4, logit::detail::QueuePolicy::DropNewest);
    LOGIT_ADD_CONSOLE_EX("%v", true, true, 4, logit::detail::QueuePolicy::Block);

    logit::FileLogger::Config file_cfg;
    file_cfg.directory = "macro_api_file_config";
    file_cfg.async = true;
    file_cfg.auto_delete_days = 1;
    file_cfg.use_dedicated_executor = true;
    file_cfg.queue_capacity = 4;
    file_cfg.queue_policy = logit::detail::QueuePolicy::Block;
    LOGIT_ADD_FILE_LOGGER_CONFIG(file_cfg, "%v");
    LOGIT_ADD_FILE_LOGGER_DEDICATED(
            "macro_api_file_dedicated",
            1,
            "%v",
            4,
            logit::detail::QueuePolicy::DropNewest);
    LOGIT_ADD_FILE_LOGGER_EX(
            "macro_api_file_ex",
            true,
            1,
            "%v",
            true,
            4,
            logit::detail::QueuePolicy::Block);
    LOGIT_ADD_FILE_LOGGER_WITH_ROTATION_DEDICATED(
            "macro_api_file_rotation",
            1,
            "%v",
            1024,
            2,
            4,
            logit::detail::QueuePolicy::Block);

    logit::UniqueFileLogger::Config unique_cfg;
    unique_cfg.directory = "macro_api_unique_config";
    unique_cfg.async = true;
    unique_cfg.auto_delete_days = 1;
    unique_cfg.hash_length = 8;
    unique_cfg.use_dedicated_executor = true;
    unique_cfg.queue_capacity = 4;
    unique_cfg.queue_policy = logit::detail::QueuePolicy::Block;
    LOGIT_ADD_UNIQUE_FILE_LOGGER_CONFIG(unique_cfg, "%v");
    LOGIT_ADD_UNIQUE_FILE_LOGGER_DEDICATED(
            "macro_api_unique_dedicated",
            1,
            8,
            "%v",
            4,
            logit::detail::QueuePolicy::DropNewest);
    LOGIT_ADD_UNIQUE_FILE_LOGGER_EX(
            "macro_api_unique_ex",
            true,
            1,
            8,
            "%v",
            true,
            4,
            logit::detail::QueuePolicy::Block);

    logit::WindowsDebugLogger::Config debug_cfg(true);
    debug_cfg.use_dedicated_executor = true;
    debug_cfg.queue_capacity = 4;
    debug_cfg.queue_policy = logit::detail::QueuePolicy::Block;
    LOGIT_ADD_WINDOWS_DEBUG_CONFIG(debug_cfg);
    LOGIT_ADD_WINDOWS_DEBUG_DEDICATED(4, logit::detail::QueuePolicy::DropNewest);
    LOGIT_ADD_WINDOWS_DEBUG_EX(true, true, 4, logit::detail::QueuePolicy::Block);

    logit::SyslogLogger::Config syslog_cfg("macro-api", 0, true);
    syslog_cfg.use_dedicated_executor = true;
    syslog_cfg.queue_capacity = 4;
    syslog_cfg.queue_policy = logit::detail::QueuePolicy::Block;
    LOGIT_ADD_SYSLOG_CONFIG_SINGLE_MODE(syslog_cfg);
    LOGIT_ADD_SYSLOG_DEDICATED_SINGLE_MODE(
            "macro-api",
            0,
            4,
            logit::detail::QueuePolicy::DropNewest);
    LOGIT_ADD_SYSLOG_EX_SINGLE_MODE(
            "macro-api",
            0,
            true,
            true,
            4,
            logit::detail::QueuePolicy::Block);

    logit::EventLogLogger::Config event_cfg(L"LogItMacroApi", true);
    event_cfg.use_dedicated_executor = true;
    event_cfg.queue_capacity = 4;
    event_cfg.queue_policy = logit::detail::QueuePolicy::Block;
    LOGIT_ADD_EVENT_LOG_CONFIG_SINGLE_MODE(event_cfg);
    LOGIT_ADD_EVENT_LOG_DEDICATED_SINGLE_MODE(
            L"LogItMacroApi",
            4,
            logit::detail::QueuePolicy::DropNewest);
    LOGIT_ADD_EVENT_LOG_EX_SINGLE_MODE(
            L"LogItMacroApi",
            true,
            true,
            4,
            logit::detail::QueuePolicy::Block);

    LOGIT_INFO("dedicated executor macro API test");
    LOGIT_SHUTDOWN();

    std::cout << "PASS: dedicated_executor_macro_api" << std::endl;
    return 0;
}
