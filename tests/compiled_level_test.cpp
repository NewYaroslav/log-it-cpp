#define LOGIT_COMPILED_LEVEL LOGIT_LEVEL_INFO
#include <LogIt.hpp>

template <class T>
int should_not_compile() {
    static_assert(sizeof(T) == 0, "disabled level compiled");
    return 0;
}

int main() {
    // TRACE macros
    LOGIT_TRACE(should_not_compile<int>());
    LOGIT_TRACE0();
    LOGIT_0TRACE();
    LOGIT_0_TRACE();
    LOGIT_NOARGS_TRACE();
    LOGIT_FORMAT_TRACE("%d", should_not_compile<int>());
    LOGIT_PRINT_TRACE(should_not_compile<int>());
    LOGIT_PRINTF_TRACE("%d", should_not_compile<int>());
    LOGITF_TRACE("{}", should_not_compile<int>());
    LOGIT_FMT_TRACE("{}", should_not_compile<int>());
    LOGIT_TRACE_TO(0, should_not_compile<int>());
    LOGIT_TRACE0_TO(0);
    LOGIT_0TRACE_TO(0);
    LOGIT_0_TRACE_TO(0);
    LOGIT_NOARGS_TRACE_TO(0);
    LOGIT_FORMAT_TRACE_TO(0, "%d", should_not_compile<int>());
    LOGIT_PRINT_TRACE_TO(0, should_not_compile<int>());
    LOGIT_PRINTF_TRACE_TO(0, "%d", should_not_compile<int>());
    LOGITF_TRACE_TO(0, "{}", should_not_compile<int>());
    LOGIT_FMT_TRACE_TO(0, "{}", should_not_compile<int>());
    LOGIT_TRACE_IF(false, should_not_compile<int>());
    LOGIT_TRACE0_IF(false);
    LOGIT_0TRACE_IF(false);
    LOGIT_0_TRACE_IF(false);
    LOGIT_NOARGS_TRACE_IF(false);
    LOGIT_FORMAT_TRACE_IF(false, "%d", should_not_compile<int>());
    LOGIT_PRINT_TRACE_IF(false, should_not_compile<int>());
    LOGIT_PRINTF_TRACE_IF(false, "%d", should_not_compile<int>());
    LOGITF_TRACE_IF(false, "{}", should_not_compile<int>());
    LOGIT_FMT_TRACE_IF(false, "{}", should_not_compile<int>());
    LOGIT_SCOPE_TRACE(should_not_compile<int>());
    LOGIT_SCOPE_TRACE_T(0, should_not_compile<int>());
    LOGIT_SCOPE_FMT_TRACE("{}", should_not_compile<int>());
    LOGIT_SCOPE_FMT_TRACE_T(0, "{}", should_not_compile<int>());

    // DEBUG macros
    LOGIT_DEBUG(should_not_compile<int>());
    LOGIT_DEBUG0();
    LOGIT_0DEBUG();
    LOGIT_0_DEBUG();
    LOGIT_NOARGS_DEBUG();
    LOGIT_FORMAT_DEBUG("%d", should_not_compile<int>());
    LOGIT_PRINT_DEBUG(should_not_compile<int>());
    LOGIT_PRINTF_DEBUG("%d", should_not_compile<int>());
    LOGITF_DEBUG("{}", should_not_compile<int>());
    LOGIT_FMT_DEBUG("{}", should_not_compile<int>());
    LOGIT_DEBUG_TO(0, should_not_compile<int>());
    LOGIT_DEBUG0_TO(0);
    LOGIT_0DEBUG_TO(0);
    LOGIT_0_DEBUG_TO(0);
    LOGIT_NOARGS_DEBUG_TO(0);
    LOGIT_FORMAT_DEBUG_TO(0, "%d", should_not_compile<int>());
    LOGIT_PRINT_DEBUG_TO(0, should_not_compile<int>());
    LOGIT_PRINTF_DEBUG_TO(0, "%d", should_not_compile<int>());
    LOGITF_DEBUG_TO(0, "{}", should_not_compile<int>());
    LOGIT_FMT_DEBUG_TO(0, "{}", should_not_compile<int>());
    LOGIT_DEBUG_IF(false, should_not_compile<int>());
    LOGIT_DEBUG0_IF(false);
    LOGIT_0DEBUG_IF(false);
    LOGIT_0_DEBUG_IF(false);
    LOGIT_NOARGS_DEBUG_IF(false);
    LOGIT_FORMAT_DEBUG_IF(false, "%d", should_not_compile<int>());
    LOGIT_PRINT_DEBUG_IF(false, should_not_compile<int>());
    LOGIT_PRINTF_DEBUG_IF(false, "%d", should_not_compile<int>());
    LOGITF_DEBUG_IF(false, "{}", should_not_compile<int>());
    LOGIT_FMT_DEBUG_IF(false, "{}", should_not_compile<int>());
    LOGIT_SCOPE_DEBUG(should_not_compile<int>());
    LOGIT_SCOPE_DEBUG_T(0, should_not_compile<int>());
    LOGIT_SCOPE_FMT_DEBUG("{}", should_not_compile<int>());
    LOGIT_SCOPE_FMT_DEBUG_T(0, "{}", should_not_compile<int>());
    return 0;
}
