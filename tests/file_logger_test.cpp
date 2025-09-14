#define LOGIT_FILE_LOGGER_PATH "."
#include <LogIt.hpp>
#include <fstream>
#include <string>

int main() {
    LOGIT_ADD_FILE_LOGGER_DEFAULT();
    const std::string message = "test log message";
    LOGIT_INFO(message);
    LOGIT_WAIT();
    const std::string log_path = LOGIT_GET_LAST_FILE_PATH(0);
    std::ifstream in(log_path);
    std::string line;
    while (std::getline(in, line)) {
        if (line.find(message) != std::string::npos) {
            return 0;
        }
    }
    return 1;
}
