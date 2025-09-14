#include <LogIt.hpp>
#include <regex>
#include <string>
#include <vector>
#include <cstdlib>

int main() {
    std::system("rm -rf rotation_ts");
    logit::FileLogger::Config cfg;
    cfg.directory = "rotation_ts";
    cfg.max_file_size_bytes = 20;
    cfg.naming = logit::RotationNaming::Timestamp;
    const std::string dir = cfg.directory;
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
