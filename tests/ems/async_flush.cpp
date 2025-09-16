#include <logit.hpp>

int main() {
    LOGIT_ADD_CONSOLE(LOGIT_CONSOLE_PATTERN, true);
    for (int i = 0; i < 1000; ++i) {
        LOGIT_INFO("message");
    }
    LOGIT_WAIT();
    return 0;
}
