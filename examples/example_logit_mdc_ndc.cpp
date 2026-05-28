#include <logit.hpp>

#include <thread>

int main() {
    LOGIT_ADD_CONSOLE(
        "[%T] [%l] request=%K{request_id} user=%K{user_id} ndc=[%J] %v",
        false);

    LOGIT_MDC_PUT("request_id", "req-42");
    LOGIT_MDC_PUT("user_id", "alice");
    LOGIT_NDC_PUSH("http");
    LOGIT_NDC_PUSH("POST /checkout");

    LOGIT_INFO("request accepted");

    {
        LOGIT_NDC_GUARD("payment");
        LOGIT_WARN("payment provider latency is high");
    }

    LOGIT_INFO("payment scope has ended");

    std::thread worker([]() {
        LOGIT_MDC_PUT("request_id", "worker-7");
        LOGIT_MDC_PUT("user_id", "background");
        LOGIT_NDC_PUSH("worker");
        LOGIT_INFO("background task has its own thread-local context");
        LOGIT_MDC_CLEAR();
        LOGIT_NDC_CLEAR();
    });
    worker.join();

    LOGIT_INFO("main thread context is unchanged");

    LOGIT_MDC_REMOVE("user_id");
    LOGIT_NDC_POP();
    LOGIT_INFO("specific MDC keys can be removed and NDC can be popped");

    LOGIT_MDC_CLEAR();
    LOGIT_NDC_CLEAR();
    LOGIT_SHUTDOWN();
    return 0;
}
