#include <logit.hpp>

int main() {
    LOGIT_ADD_CONSOLE_DEFAULT();
    LOGIT_TRACE("trace message");
    LOGIT_DEBUG("debug message");
    LOGIT_INFO("info message");
    LOGIT_WARN("warn message");
    LOGIT_ERROR("error message");
    LOGIT_FATAL("fatal message");
    return 0;
}
