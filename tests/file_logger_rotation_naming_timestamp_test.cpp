#include <LogIt.hpp>
#include <regex>
#include <string>
#include <vector>
#if __cplusplus >= 201703L
#include <filesystem>
#else
#include <cstdlib>
#endif

int main() {
#if __cplusplus >= 201703L
    std::filesystem::remove_all(logit::get_exec_dir() + "/rotation_ts");
#else
#ifdef _WIN32
    std::string clean = logit::get_exec_dir() + "\\rotation_ts";
    std::string cmd = "rmdir /s /q \"" + clean + "\"";
#else
    std::string clean = logit::get_exec_dir() + "/rotation_ts";
    std::string cmd = "rm -rf \"" + clean + "\"";
#endif
    std::system(cmd.c_str());
#endif
    const std::string dir = logit::get_exec_dir() + "/rotation_ts";
    logit::FileLogger::Config cfg;
    cfg.directory = "rotation_ts";
    cfg.max_file_size_bytes = 20;
    cfg.naming = logit::RotationNaming::Timestamp;
    logit::Logger::get_instance().add_logger(
        std::unique_ptr<logit::FileLogger>(new logit::FileLogger(cfg)),
        std::unique_ptr<logit::SimpleLogFormatter>(new logit::SimpleLogFormatter("%v")));
    const std::string msg = "0123456789";
    LOGIT_INFO(msg);
    LOGIT_INFO(msg);
    LOGIT_WAIT();
    LOGIT_SHUTDOWN();
    std::vector<std::string> files = logit::get_list_files(dir);
    std::regex re("\\d{4}-\\d{2}-\\d{2}_\\d{6}(?:\\.\\d+)?\\.log");
    for (const auto& path : files) {
        std::string name = path.substr(path.find_last_of("/\\") + 1);
        if (std::regex_match(name, re)) return 0;
    }
    return 1;
}
