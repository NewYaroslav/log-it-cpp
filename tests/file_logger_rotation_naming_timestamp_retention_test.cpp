#include <LogIt.hpp>
#include <regex>
#include <string>
#include <vector>
#include <algorithm>
#if __cplusplus >= 201703L
#include <filesystem>
#else
#include <cstdlib>
#endif

int main() {
#if __cplusplus >= 201703L
    std::filesystem::remove_all(logit::get_exec_dir() + "/rotation_ts_ret");
#else
#ifdef _WIN32
    std::string clean = logit::get_exec_dir() + "\\rotation_ts_ret";
    std::string cmd = "rmdir /s /q \"" + clean + "\"";
#else
    std::string clean = logit::get_exec_dir() + "/rotation_ts_ret";
    std::string cmd = "rm -rf \"" + clean + "\"";
#endif
    std::system(cmd.c_str());
#endif
    const std::string dir = logit::get_exec_dir() + "/rotation_ts_ret";
    logit::FileLogger::Config cfg;
    cfg.directory = "rotation_ts_ret";
    cfg.max_file_size_bytes = 20;
    cfg.max_rotated_files = 2;
    cfg.naming = logit::RotationNaming::Timestamp;
    logit::Logger::get_instance().add_logger(
        std::unique_ptr<logit::FileLogger>(new logit::FileLogger(cfg)),
        std::unique_ptr<logit::SimpleLogFormatter>(new logit::SimpleLogFormatter("%v")));
    const std::string msg = "0123456789";
    LOGIT_INFO(msg);
    LOGIT_INFO(msg);
    LOGIT_INFO(msg);
    LOGIT_INFO(msg);
    LOGIT_INFO(msg);
    LOGIT_INFO(msg);
    LOGIT_WAIT();
    LOGIT_SHUTDOWN();
    std::vector<std::string> files = logit::get_list_files(dir);
    std::regex re_idx("^\\d{4}-\\d{2}-\\d{2}_(\\d{6})\\.(\\d+)\\.log$");
    std::regex re_base("^\\d{4}-\\d{2}-\\d{2}_\\d{6}\\.log$");
    std::vector<std::string> rotated;
    std::string base_ts;
    for (const auto& path : files) {
        std::string name = path.substr(path.find_last_of("/\\") + 1);
        if (std::regex_match(name, re_base)) return 1;
        std::smatch m;
        if (std::regex_match(name, m, re_idx)) {
            if (base_ts.empty()) base_ts = m.str(1);
            else if (base_ts != m.str(1)) return 1;
            rotated.push_back(name);
        }
    }
    if (rotated.size() != 2) return 1;
    std::sort(rotated.begin(), rotated.end());
    if (rotated[0].find(".1.log") == std::string::npos) return 1;
    if (rotated[1].find(".2.log") == std::string::npos) return 1;
    return 0;
}
