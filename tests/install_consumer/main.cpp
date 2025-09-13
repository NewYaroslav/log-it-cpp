#define LOGIT_SHORT_NAME
#include <LogIt.hpp>

int main() {
    LOGIT_ADD_CONSOLE_DEFAULT();
    LOG_INFO("consumer works");
    LOGIT_WAIT();
    return 0;
}
