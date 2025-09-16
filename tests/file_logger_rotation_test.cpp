#define LOGIT_FILE_LOGGER_PATH "."
#include <logit.hpp>
#include <fstream>
#include <string>
#if __cplusplus >= 201703L
#include <filesystem>
#endif

int main() {
    LOGIT_ADD_FILE_LOGGER_WITH_ROTATION(".", true, 30, "%v", 20, 10);
    const std::string msg = "0123456789"; // 10 bytes
    LOGIT_INFO(msg);
    LOGIT_INFO(msg);
    LOGIT_WAIT();
    
#if __cplusplus >= 201703L
    std::filesystem::path current = LOGIT_GET_LAST_FILE_PATH(0);
    std::filesystem::path rotated = current;
    rotated.replace_filename(current.stem().string() + ".001.log");
    LOGIT_SHUTDOWN();
    bool exists = std::filesystem::exists(rotated);
#else
    std::string current = LOGIT_GET_LAST_FILE_PATH(0);
    std::string rotated = current;
    size_t pos = rotated.rfind(".log");
    if (pos != std::string::npos) {
        rotated.insert(pos, ".001");
    }
    LOGIT_SHUTDOWN();
    bool exists = false;
    if (pos != std::string::npos) {
        std::ifstream f(rotated.c_str());
        exists = f.good();
    }
#endif
    return exists ? 0 : 1;
}
