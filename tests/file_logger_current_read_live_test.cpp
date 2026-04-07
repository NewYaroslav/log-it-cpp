#include <logit.hpp>
#include <atomic>
#include <chrono>
#include <string>
#include <thread>
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
    const std::string directory_name = make_unique_directory_name("file_api_live_logs");

    logit::FileLogger::Config cfg;
    cfg.directory = directory_name;
    cfg.async = true;

    logit::Logger::get_instance().add_logger(
        std::unique_ptr<logit::FileLogger>(new logit::FileLogger(cfg)),
        std::unique_ptr<logit::SimpleLogFormatter>(new logit::SimpleLogFormatter("%v")));

    LOGIT_INFO("seed");
    std::this_thread::sleep_for(std::chrono::milliseconds(30));

    const std::string current_path = LOGIT_GET_LAST_FILE_PATH(0);
    if (current_path.empty()) {
        return 1;
    }

    std::atomic<bool> started(false);
    std::thread writer([&started]() {
        started = true;
        for (int i = 0; i < 120; ++i) {
            LOGIT_INFO("live-write");
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
    });

    while (!started.load()) {
        std::this_thread::yield();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    const std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    const logit::LogFileReadResult read_result = LOGIT_READ_LOG_FILE(0, current_path);
    const std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

    writer.join();
    LOGIT_WAIT();

    const std::chrono::milliseconds elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - begin);
    if (!read_result.ok) {
        return 1;
    }
    if (elapsed.count() >= 500) {
        return 1;
    }
    if (read_result.file.path != current_path) {
        return 1;
    }

    LOGIT_SHUTDOWN();
    return 0;
}
