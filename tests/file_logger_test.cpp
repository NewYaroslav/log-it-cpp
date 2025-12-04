#define LOGIT_FILE_LOGGER_PATH "file_logger_test_logs"
#include <logit.hpp>
#include <fstream>
#include <string>

#if defined(_WIN32)
#    include <direct.h>
#else
#    include <sys/stat.h>
#endif

int main() {
#if defined(_WIN32)
    _mkdir(LOGIT_FILE_LOGGER_PATH);
#else
    mkdir(LOGIT_FILE_LOGGER_PATH, 0777);
#endif

    LOGIT_ADD_FILE_LOGGER_DEFAULT();
    const std::string message = "test log message";
    LOGIT_INFO(message);
    LOGIT_WAIT();
    const std::string log_path = LOGIT_GET_LAST_FILE_PATH(0);
    std::ifstream in(log_path);
    std::string line;
    bool found = false;
    while (std::getline(in, line)) {
        if (line.find(message) != std::string::npos) {
            found = true;
            break;
        }
    }
    LOGIT_SHUTDOWN();
    return found ? 0 : 1;
}
