#define LOGIT_FILE_LOGGER_PATH "."
#include <LogIt.hpp>
#include <fstream>
#include <string>

int main() {
    LOGIT_ADD_FILE_LOGGER_WITH_ROTATION(".", true, 30, "%v", 20, 10);
    const std::string msg = "0123456789"; // 10 bytes
    LOGIT_INFO(msg);
    LOGIT_INFO(msg);
    LOGIT_WAIT();
    std::string current = LOGIT_GET_LAST_FILE_PATH(0);
    std::string rotated = current;
    size_t pos = rotated.rfind(".log");
    rotated.insert(pos, ".1");
    std::ifstream in(rotated);
    return in.good() ? 0 : 1;
}
