#include <logit.hpp>

#include <atomic>
#include <iostream>
#include <thread>

static bool test_set_config_switches_dedicated_executor() {
    logit::ConsoleLogger logger(false);
    logit::LogRecord rec(logit::LogLevel::LOG_LVL_INFO, 0, "", 0, "", "", "", -1, false);

    logit::ConsoleLogger::Config cfg;
    cfg.async = true;
    cfg.use_dedicated_executor = true;
    cfg.queue_capacity = 1;
    cfg.queue_policy = logit::detail::QueuePolicy::Block;
    logger.set_config(cfg);
    logger.log(rec, "console dedicated on");
    logger.wait();

    cfg.use_dedicated_executor = false;
    logger.set_config(cfg);
    logger.log(rec, "console dedicated off");
    logger.wait();

    cfg.use_dedicated_executor = true;
    cfg.queue_policy = logit::detail::QueuePolicy::DropNewest;
    logger.set_config(cfg);
    logger.log(rec, "console dedicated on again");
    logger.wait();

    const logit::ConsoleLogger::Config observed = logger.get_config();
    logger.shutdown();

    return observed.async &&
           observed.use_dedicated_executor &&
           observed.queue_capacity == 1 &&
           observed.queue_policy == logit::detail::QueuePolicy::DropNewest;
}

static bool test_set_config_concurrent_log_does_not_hang() {
    logit::ConsoleLogger logger(false);
    logit::LogRecord rec(logit::LogLevel::LOG_LVL_INFO, 0, "", 0, "", "", "", -1, false);

    logit::ConsoleLogger::Config cfg;
    cfg.async = true;
    cfg.use_dedicated_executor = true;
    cfg.queue_capacity = 1;
    cfg.queue_policy = logit::detail::QueuePolicy::Block;
    logger.set_config(cfg);

    std::atomic<bool> producer_done(false);
    std::thread producer([&]() {
        for (int i = 0; i < 32; ++i) {
            logger.log(rec, "console concurrent dedicated config");
        }
        producer_done.store(true);
    });

    for (int i = 0; i < 16; ++i) {
        cfg.use_dedicated_executor = (i % 2) == 0;
        cfg.queue_policy = (i % 3) == 0
                ? logit::detail::QueuePolicy::DropNewest
                : logit::detail::QueuePolicy::Block;
        logger.set_config(cfg);
    }

    producer.join();
    logger.wait();
    logger.shutdown();
    return producer_done.load();
}

int main() {
    const bool ok = test_set_config_switches_dedicated_executor() &&
                    test_set_config_concurrent_log_does_not_hang();
    std::cout << (ok ? "PASS" : "FAIL") << ": console_logger_dedicated_config" << std::endl;
    return ok ? 0 : 1;
}
