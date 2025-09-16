#define LOGIT_FILE_LOGGER_PATH "."
#include <logit.hpp>
#include <fstream>
#include <string>

int main() {
    LOGIT_ADD_FILE_LOGGER_DEFAULT();
    LOGITF_INFO("fmt {}", 42);
    int value = 5;
    LOGIT_FMT_INFO("{:04}", value);
    {
        int scope_val = 7;
        LOGIT_SCOPE_FMT_INFO("scope {}", scope_val);
    }
    LOGIT_WAIT();
    std::ifstream in(LOGIT_GET_LAST_FILE_PATH(0));
    std::string line;
    bool found_fmt = false;
    bool found_fmt_arg = false;
    bool found_scope = false;
    while (std::getline(in, line)) {
        if (line.find("fmt 42") != std::string::npos) found_fmt = true;
        if (line.find("0005") != std::string::npos) found_fmt_arg = true;
        if (line.find("scope 7") != std::string::npos) found_scope = true;
    }
    LOGIT_SHUTDOWN();
    return (found_fmt && found_fmt_arg && found_scope) ? 0 : 1;
}
