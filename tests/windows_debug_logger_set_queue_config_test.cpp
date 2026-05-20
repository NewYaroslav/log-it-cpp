#include <logit.hpp>

#include <iostream>

static bool test_dedicated_executor_returns_true() {
    logit::WindowsDebugLogger::Config cfg;
    cfg.async = true;
    cfg.use_dedicated_executor = true;
    cfg.queue_capacity = 0;
    cfg.queue_policy = logit::detail::QueuePolicy::Block;
    logit::WindowsDebugLogger logger(cfg);

    const bool ok = logger.set_queue_config(1, logit::detail::QueuePolicy::DropNewest);
    logger.shutdown();
    return ok;
}

static bool test_global_executor_returns_false() {
    logit::WindowsDebugLogger::Config cfg;
    cfg.async = true;
    cfg.use_dedicated_executor = false;
    cfg.queue_capacity = 0;
    cfg.queue_policy = logit::detail::QueuePolicy::Block;
    logit::WindowsDebugLogger logger(cfg);

    const bool ok = logger.set_queue_config(2, logit::detail::QueuePolicy::DropNewest);
    logger.shutdown();
    return !ok;
}

static bool test_shutdown_returns_false() {
    logit::WindowsDebugLogger::Config cfg;
    cfg.async = true;
    cfg.use_dedicated_executor = true;
    logit::WindowsDebugLogger logger(cfg);
    logger.shutdown();

    const bool ok = logger.set_queue_config(1, logit::detail::QueuePolicy::DropNewest);
    return !ok;
}

int main() {
    const bool ok = test_dedicated_executor_returns_true() &&
                    test_global_executor_returns_false() &&
                    test_shutdown_returns_false();
    std::cout << (ok ? "PASS" : "FAIL") << ": windows_debug_logger_set_queue_config" << std::endl;
    return ok ? 0 : 1;
}
