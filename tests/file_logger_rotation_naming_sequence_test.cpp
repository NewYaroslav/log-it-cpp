#include <logit.hpp>
#include <regex>
#include <string>
#include <fstream>
#include <cstdlib>

int main() {
    std::system("rm -rf rotation_seq");
    logit::FileLogger::Config cfg;
    cfg.directory = "rotation_seq";
    cfg.max_file_size_bytes = 20;
    cfg.naming = logit::RotationNaming::Sequence;
    logit::Logger::get_instance().add_logger(
        std::unique_ptr<logit::FileLogger>(new logit::FileLogger(cfg)),
        std::unique_ptr<logit::SimpleLogFormatter>(new logit::SimpleLogFormatter("%v")));
    const std::string msg = "0123456789";
    LOGIT_INFO(msg);
    LOGIT_INFO(msg);
    LOGIT_WAIT();
    std::string current = LOGIT_GET_LAST_FILE_PATH(0);
    LOGIT_SHUTDOWN();
    size_t pos = current.rfind(".log");
    if (pos == std::string::npos) return 1;
    std::string rotated = current;
    rotated.insert(pos, ".001");
    std::string name = rotated.substr(rotated.find_last_of("/\\") + 1);
    std::regex re("\\d{4}-\\d{2}-\\d{2}\\.001\\.log");
    if (!std::regex_match(name, re)) return 1;
    std::ifstream f(rotated.c_str());
    return f.good() ? 0 : 1;
}
