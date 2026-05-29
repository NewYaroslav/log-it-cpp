#include <logit.hpp>

#include <cassert>
#include <memory>
#include <string>
#include <thread>
#include <vector>

int main() {
#ifndef LOGIT_WITH_CONTEXT
    return 0;
#else
    logit::MemoryLogger::Config mem_cfg;
    logit::Logger::get_instance().add_logger(
        std::unique_ptr<logit::MemoryLogger>(new logit::MemoryLogger(mem_cfg)),
        std::unique_ptr<logit::SimpleLogFormatter>(new logit::SimpleLogFormatter("%v [%K] [%J] req=%K{request_id}")));

    LOGIT_MDC_PUT("request_id", "abc123");
    LOGIT_MDC_PUT("user_id", "42");
    LOGIT_INFO("msg1");

    LOGIT_NDC_PUSH("outer");
    LOGIT_NDC_PUSH("inner");
    LOGIT_INFO("msg2");

    logit::MemoryLogger::Config mem_cfg2;
    logit::Logger::get_instance().add_logger(
        std::unique_ptr<logit::MemoryLogger>(new logit::MemoryLogger(mem_cfg2)),
        std::unique_ptr<logit::SimpleLogFormatter>(new logit::SimpleLogFormatter("%v req=%K{request_id}")));
    LOGIT_INFO("msg3");

    {
        LOGIT_NDC_GUARD("guard_scope");
        LOGIT_INFO("msg4");
    }
    LOGIT_INFO("msg5");

    std::thread worker([]() {
        LOGIT_MDC_PUT("request_id", "other");
        LOGIT_NDC_PUSH("worker");
        LOGIT_INFO("thread_msg");
        LOGIT_MDC_CLEAR();
        LOGIT_NDC_CLEAR();
    });
    worker.join();

    LOGIT_INFO("msg6");

    std::vector<std::string> logs0 =
        logit::Logger::get_instance().get_buffered_strings(0);
    std::vector<std::string> logs1 =
        logit::Logger::get_instance().get_buffered_strings(1);

    LOGIT_MDC_CLEAR();
    LOGIT_NDC_CLEAR();
    LOGIT_SHUTDOWN();

    assert(logs0.size() == 7);

    assert(logs0[0].find("msg1") != std::string::npos);
    assert(logs0[0].find("request_id=abc123") != std::string::npos);
    assert(logs0[0].find("user_id=42") != std::string::npos);
    assert(logs0[0].find("req=abc123") != std::string::npos);
    assert(logs0[0].find("[]") != std::string::npos);

    assert(logs0[1].find("msg2") != std::string::npos);
    assert(logs0[1].find("outer > inner") != std::string::npos);

    assert(logs0[2].find("msg3") != std::string::npos);

    assert(logs0[3].find("msg4") != std::string::npos);
    assert(logs0[3].find("guard_scope") != std::string::npos);

    assert(logs0[4].find("msg5") != std::string::npos);
    assert(logs0[4].find("guard_scope") == std::string::npos);

    assert(logs0[5].find("thread_msg") != std::string::npos);
    assert(logs0[5].find("request_id=other") != std::string::npos);
    assert(logs0[5].find("worker") != std::string::npos);

    assert(logs0[6].find("msg6") != std::string::npos);
    assert(logs0[6].find("request_id=abc123") != std::string::npos);
    assert(logs0[6].find("request_id=other") == std::string::npos);

    assert(logs1.size() == 5);
    assert(logs1[0].find("msg3") != std::string::npos);
    assert(logs1[0].find("req=abc123") != std::string::npos);
    assert(logs1[3].find("thread_msg") != std::string::npos);
    assert(logs1[3].find("req=other") != std::string::npos);
    assert(logs1[4].find("msg6") != std::string::npos);
    assert(logs1[4].find("req=abc123") != std::string::npos);

    return 0;
#endif
}
