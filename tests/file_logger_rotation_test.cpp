#define LOGIT_FILE_LOGGER_PATH "."
#include <logit.hpp>
#include <chrono>
#include <fstream>
#include <string>
#if __cplusplus >= 201703L
#include <filesystem>
#endif

namespace {

std::string make_unique_directory_name(const std::string& prefix) {
    const long long stamp = static_cast<long long>(
        std::chrono::steady_clock::now().time_since_epoch().count());
    return prefix + "_" + std::to_string(stamp);
}

} // namespace

int main() {
    const std::string directory_name = make_unique_directory_name("file_rotation_logs");

    LOGIT_ADD_FILE_LOGGER_WITH_ROTATION(directory_name, true, 30, "%v", 20, 10);
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
    std::error_code ec;
    std::filesystem::remove_all(current.parent_path(), ec);
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
