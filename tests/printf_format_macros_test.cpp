#define LOGIT_FILE_LOGGER_PATH "."
#include <LogIt.hpp>
#include <fstream>
#include <string>

int main() {
    LOGIT_ADD_FILE_LOGGER_DEFAULT();
    float f = 123.456f;
    int i = 789;
    LOGIT_PRINTF_INFO("%.2f %d", f, i);
    LOGIT_FORMAT_INFO("%.1f", f, 654.321f);
    LOGIT_WAIT();
    std::ifstream in(LOGIT_GET_LAST_FILE_PATH(0));
    std::string line;
    bool found_printf = false;
    bool found_format = false;
    while (std::getline(in, line)) {
        if (line.find("123.46 789") != std::string::npos) found_printf = true;
        if (line.find("f: 123.5, 654.3") != std::string::npos) found_format = true;
    }
    LOGIT_SHUTDOWN();
    return (found_printf && found_format) ? 0 : 1;
}
