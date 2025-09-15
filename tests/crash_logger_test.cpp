#include <atomic>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>
#include <system_error>

#ifndef _WIN32
#    include <csignal>
#endif

namespace logit_test {
    static int g_exit_code = -1;

    inline void reset_exit_code() { g_exit_code = -1; }

    inline void fake_exit(int code) { g_exit_code = code; }
} // namespace logit_test

#define _exit ::logit_test::fake_exit

#ifdef _WIN32
#    define private public
#    include <logit_cpp/logit/loggers/CrashWindowsLogger.hpp>
#    undef private
#else
#    define private public
#    include <logit_cpp/logit/loggers/CrashPosixLogger.hpp>
#    undef private
#endif

#undef _exit

#include <LogIt.hpp>

int main() {
    namespace fs = std::filesystem;

    logit_test::reset_exit_code();

    std::error_code ec;
    const fs::path temp_dir = fs::temp_directory_path(ec);
    if (ec) {
        return 1;
    }

    const auto unique_suffix = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const fs::path log_path = temp_dir / ("logit_crash_logger_test_" + std::to_string(unique_suffix) + ".log");
    fs::remove(log_path, ec);

    logit::CrashLogger::Config config;
    config.log_path = log_path.string();
    config.buffer_size = 4096;

    LOGIT_ADD_LOGGER(logit::CrashLogger, (config), logit::SimpleLogFormatter, ("%v"));

    const std::vector<std::string> messages = {
        "Crash logger message 1",
        "Crash logger message 2",
        "Crash logger message 3",
    };
    for (const auto& message : messages) {
        LOGIT_INFO(message);
    }
    LOGIT_WAIT();

    int expected_exit_code = 0;
    std::string marker;

#ifdef _WIN32
    auto& active_logger = logit::CrashLogger::active_logger();
    logit::CrashLogger* crash_logger = active_logger.load(std::memory_order_acquire);
    if (crash_logger == nullptr) {
        LOGIT_SHUTDOWN();
        return 1;
    }

    EXCEPTION_RECORD record{};
    record.ExceptionCode = 0xC0000005;
    EXCEPTION_POINTERS pointers{};
    pointers.ExceptionRecord = &record;

    (void)logit::CrashLogger::exception_filter(&pointers);
    expected_exit_code = EXIT_FAILURE;

    std::ostringstream marker_builder;
    marker_builder << "\n== CRASH EXCEPTION 0x" << std::uppercase << std::hex << record.ExceptionCode << " ==\n";
    marker = marker_builder.str();
#else
    logit::CrashLogger* crash_logger = logit::CrashLogger::s_active_logger.load(std::memory_order_acquire);
    if (crash_logger == nullptr) {
        LOGIT_SHUTDOWN();
        return 1;
    }

    constexpr int kSignalNumber = SIGABRT;
    logit::CrashLogger::signal_handler(kSignalNumber, nullptr, nullptr);
    expected_exit_code = 128 + kSignalNumber;
    marker = "\n== CRASH SIGNAL " + std::to_string(kSignalNumber) + " ==\n";
#endif

    if (logit_test::g_exit_code != expected_exit_code) {
        LOGIT_SHUTDOWN();
        return 1;
    }

    LOGIT_SHUTDOWN();

    std::ifstream input(log_path);
    if (!input.is_open()) {
        return 1;
    }
    const std::string contents((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());

    for (const auto& message : messages) {
        if (contents.find(message) == std::string::npos) {
            return 1;
        }
    }
    if (contents.find(marker) == std::string::npos) {
        return 1;
    }

    fs::remove(log_path, ec);

    return 0;
}
