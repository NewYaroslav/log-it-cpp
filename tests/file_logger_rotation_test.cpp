#define LOGIT_FILE_LOGGER_PATH "."
#include <LogIt.hpp>
#include <fstream>
#include <string>
#include <filesystem>

int main() {
    LOGIT_ADD_FILE_LOGGER_WITH_ROTATION(".", true, 30, "%v", 20, 10);
    const std::string msg = "0123456789"; // 10 bytes
    LOGIT_INFO(msg);
    LOGIT_INFO(msg);
    LOGIT_WAIT();
    std::filesystem::path current = LOGIT_GET_LAST_FILE_PATH(0);
    std::filesystem::path rotated = current;
    rotated.replace_filename(current.stem().string() + ".001.log");
    return std::filesystem::exists(rotated) ? 0 : 1;
}
