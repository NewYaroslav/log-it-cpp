#include <logit.hpp>
#include <chrono>
#include <fstream>
#include <iterator>
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

std::string normalize_newlines(const std::string& value) {
    std::string normalized;
    normalized.reserve(value.size());
    for (size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '\r' && (i + 1) < value.size() && value[i + 1] == '\n') {
            continue;
        }
        normalized.push_back(value[i]);
    }
    return normalized;
}

} // namespace

int main() {
    const std::string directory_name = make_unique_directory_name("raw_file_logs");
    const std::string block =
        "[App]\n"
        "Name: sample.desktop.app\n"
        "Version: 3.3.106.wzr\n"
        "\n"
        "[Network Settings]\n"
        "Protocol: https\n"
        "Proxy enabled: False";

    LOGIT_ADD_FILE_LOGGER(directory_name, true, 30, "prefix:%v:%l");
    LOGIT_RAW(block);
    LOGIT_WAIT();

    const std::string log_path = LOGIT_GET_LAST_FILE_PATH(0);

    std::ifstream in(log_path.c_str(), std::ios_base::binary);
    const std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    LOGIT_SHUTDOWN();

#if __cplusplus >= 201703L
    std::error_code ec;
    std::filesystem::remove_all(std::filesystem::path(log_path).parent_path(), ec);
#endif

    return normalize_newlines(content) == block + "\n" ? 0 : 1;
}
