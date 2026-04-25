#include <logit.hpp>
#include <chrono>
#include <fstream>
#include <iterator>
#include <string>
#include <thread>
#include <vector>
#if __cplusplus >= 201703L
#include <filesystem>
#endif

namespace {

std::string make_unique_directory_name(const std::string& prefix) {
    const long long stamp = static_cast<long long>(
        std::chrono::steady_clock::now().time_since_epoch().count());
    return prefix + "_" + std::to_string(stamp);
}

bool contains_scope_message(const std::string& value) {
    return value.find("scope_phase | duration_ms=") != std::string::npos;
}

} // namespace

int main() {
    const std::string directory_name = make_unique_directory_name("scope_timer_logs");

    LOGIT_ADD_MEMORY_LOGGER(10, 64 * 1024, 60 * 60 * 1000);
    LOGIT_ADD_FILE_LOGGER(directory_name, true, 30, "%v");

    {
        LOGIT_SCOPE_INFO("scope_phase");
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    LOGIT_WAIT();

    const std::vector<std::string> buffered = LOGIT_GET_BUFFERED_STRINGS(0);
    const std::string log_path = LOGIT_GET_LAST_FILE_PATH(1);

    std::ifstream in(log_path.c_str(), std::ios_base::binary);
    const std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    LOGIT_SHUTDOWN();

#if __cplusplus >= 201703L
    std::error_code ec;
    std::filesystem::remove_all(std::filesystem::path(log_path).parent_path(), ec);
#endif

    if (buffered.size() != 1) return 1;
    if (!contains_scope_message(buffered[0])) return 2;
    if (!contains_scope_message(content)) return 3;

    return 0;
}
