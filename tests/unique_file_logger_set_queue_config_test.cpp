#include <logit.hpp>

#include <iostream>

static bool test_dedicated_executor_returns_true() {
    logit::UniqueFileLogger::Config cfg;
    cfg.directory = "test_unique_queue_cfg";
    cfg.async = true;
    cfg.use_dedicated_executor = true;
    cfg.queue_capacity = 0;
    cfg.queue_policy = logit::detail::QueuePolicy::Block;
    logit::UniqueFileLogger logger(cfg);

    const bool ok = logger.set_queue_config(1, logit::detail::QueuePolicy::DropNewest);
    logger.shutdown();
    return ok;
}

static bool test_global_executor_returns_false() {
    logit::UniqueFileLogger::Config cfg;
    cfg.directory = "test_unique_queue_cfg_global";
    cfg.async = true;
    cfg.use_dedicated_executor = false;
    cfg.queue_capacity = 0;
    cfg.queue_policy = logit::detail::QueuePolicy::Block;
    logit::UniqueFileLogger logger(cfg);

    const bool ok = logger.set_queue_config(2, logit::detail::QueuePolicy::DropNewest);
    logger.shutdown();
    return !ok;
}

static bool test_shutdown_returns_false() {
    logit::UniqueFileLogger::Config cfg;
    cfg.directory = "test_unique_queue_cfg_shut";
    cfg.async = true;
    cfg.use_dedicated_executor = true;
    logit::UniqueFileLogger logger(cfg);
    logger.shutdown();

    const bool ok = logger.set_queue_config(1, logit::detail::QueuePolicy::DropNewest);
    return !ok;
}

int main() {
    const bool ok = test_dedicated_executor_returns_true() &&
                    test_global_executor_returns_false() &&
                    test_shutdown_returns_false();
    std::cout << (ok ? "PASS" : "FAIL") << ": unique_file_logger_set_queue_config" << std::endl;
    return ok ? 0 : 1;
}
