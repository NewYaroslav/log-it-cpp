/// \file example_logit_queue_limit.cpp
/// \brief Shows how to limit the log queue size.

#include <logit.hpp>

int main() {
    LOGIT_ADD_CONSOLE_DEFAULT();
    LOGIT_SET_MAX_QUEUE(2);
    LOGIT_SET_QUEUE_POLICY(LOGIT_QUEUE_DROP);

    for (int i = 0; i < 10; ++i) {
        LOGIT_INFO("queue message", i);
    }

    LOGIT_WAIT();
    LOGIT_SHUTDOWN();
    return 0;
}
