#define LOGIT_COMPILED_LEVEL LOGIT_LEVEL_INFO
#include <logit.hpp>

template <class T>
int should_not_compile_runtime_reenabled() {
    static_assert(sizeof(T) == 0, "compiled-out level was re-enabled at runtime");
    return 0;
}

int main() {
    LOGIT_ADD_MEMORY_LOGGER_DEFAULT();
    LOGIT_SET_LOG_LEVEL(logit::LogLevel::LOG_LVL_TRACE);
    LOGIT_SET_LOG_LEVEL_TO(0, logit::LogLevel::LOG_LVL_TRACE);

    LOGIT_TRACE(should_not_compile_runtime_reenabled<int>());
    LOGIT_DEBUG(should_not_compile_runtime_reenabled<int>());
    return 0;
}
