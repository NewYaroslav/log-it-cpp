#define LOGIT_COMPILED_LEVEL logit::LogLevel::LOG_LVL_INFO
#include <LogIt.hpp>

template <class T>
int should_not_compile() {
    static_assert(sizeof(T) == 0, "disabled level compiled");
    return 0;
}

int main() {
    LOGIT_TRACE(should_not_compile<int>());
    LOGIT_DEBUG(should_not_compile<int>());
    LOGIT_TRACE0();
    LOGIT_DEBUG0();
    return 0;
}
