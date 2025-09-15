#define LOGIT_FILE_LOGGER_PATH "."
#include <LogIt.hpp>
#include <fstream>
#include <string>

int main() {
    LOGIT_ADD_FILE_LOGGER_DEFAULT();

    LOGIT_PRINT_TRACE_IF(true, "[trace-if-true] ", 1);
    LOGIT_PRINT_TRACE_IF(false, "[trace-if-false]");
    LOGIT_PRINT_INFO_IF(true, "[info-if-true] ", 2);
    LOGIT_PRINT_INFO_IF(false, "[info-if-false]");
    LOGIT_PRINT_DEBUG_IF(true, "[debug-if-true] ", 3);
    LOGIT_PRINT_DEBUG_IF(false, "[debug-if-false]");
    LOGIT_PRINT_WARN_IF(true, "[warn-if-true] ", 4);
    LOGIT_PRINT_WARN_IF(false, "[warn-if-false]");
    LOGIT_PRINT_ERROR_IF(true, "[error-if-true] ", 5);
    LOGIT_PRINT_ERROR_IF(false, "[error-if-false]");
    LOGIT_PRINT_FATAL_IF(true, "[fatal-if-true] ", 6);
    LOGIT_PRINT_FATAL_IF(false, "[fatal-if-false]");

    if (false)
        LOGIT_PRINT_INFO_IF(true, "[info-if-false-branch] ");
    else
        LOGIT_PRINT_INFO_IF(true, "[info-if-else-branch] ", 7);

    LOGIT_WAIT();

    std::ifstream in(LOGIT_GET_LAST_FILE_PATH(0));
    std::string line;
    bool found_trace_true = false;
    bool found_info_true = false;
    bool found_debug_true = false;
    bool found_warn_true = false;
    bool found_error_true = false;
    bool found_fatal_true = false;
    bool found_else_branch = false;
    bool found_trace_false = false;
    bool found_info_false = false;
    bool found_debug_false = false;
    bool found_warn_false = false;
    bool found_error_false = false;
    bool found_fatal_false = false;
    bool found_info_false_branch = false;

    while (std::getline(in, line)) {
        if (line.find("[trace-if-true] 1") != std::string::npos) found_trace_true = true;
        if (line.find("[info-if-true] 2") != std::string::npos) found_info_true = true;
        if (line.find("[debug-if-true] 3") != std::string::npos) found_debug_true = true;
        if (line.find("[warn-if-true] 4") != std::string::npos) found_warn_true = true;
        if (line.find("[error-if-true] 5") != std::string::npos) found_error_true = true;
        if (line.find("[fatal-if-true] 6") != std::string::npos) found_fatal_true = true;
        if (line.find("[info-if-else-branch] 7") != std::string::npos) found_else_branch = true;
        if (line.find("[trace-if-false]") != std::string::npos) found_trace_false = true;
        if (line.find("[info-if-false]") != std::string::npos) found_info_false = true;
        if (line.find("[debug-if-false]") != std::string::npos) found_debug_false = true;
        if (line.find("[warn-if-false]") != std::string::npos) found_warn_false = true;
        if (line.find("[error-if-false]") != std::string::npos) found_error_false = true;
        if (line.find("[fatal-if-false]") != std::string::npos) found_fatal_false = true;
        if (line.find("[info-if-false-branch]") != std::string::npos) found_info_false_branch = true;
    }

    LOGIT_SHUTDOWN();

    const bool found_all_true =
        found_trace_true && found_info_true && found_debug_true && found_warn_true && found_error_true && found_fatal_true &&
        found_else_branch;
    const bool found_any_false =
        found_trace_false || found_info_false || found_debug_false || found_warn_false || found_error_false ||
        found_fatal_false || found_info_false_branch;

    return (found_all_true && !found_any_false) ? 0 : 1;
}
