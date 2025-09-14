#define LOGIT_FILE_LOGGER_PATH "."
#include <LogIt.hpp>
#include <fstream>
#include <string>
#if __cplusplus >= 201703L
#include <filesystem>
#endif

int main() {
    const std::string dir = "remove_old_logs_test";
    LOGIT_ADD_FILE_LOGGER(dir, false, 0, "%v");
    LOGIT_INFO("init");
    std::string current = LOGIT_GET_LAST_FILE_PATH(0);
#if __cplusplus >= 201703L
    std::filesystem::path log_dir = std::filesystem::path(current).parent_path();
    std::filesystem::path old1 = log_dir / "2000-01-01.log";
    std::filesystem::path old2 = log_dir / "2000-01-01.1.log";
    std::filesystem::path old3 = log_dir / "2000-01-01.log.gz";
    std::ofstream(old1).close();
    std::ofstream(old2).close();
    std::ofstream(old3).close();
#else
    std::string log_dir = current.substr(0, current.find_last_of("/\\"));
    std::string old1 = log_dir + "/2000-01-01.log";
    std::string old2 = log_dir + "/2000-01-01.1.log";
    std::string old3 = log_dir + "/2000-01-01.log.gz";
    std::ofstream(old1.c_str()).close();
    std::ofstream(old2.c_str()).close();
    std::ofstream(old3.c_str()).close();
#endif
    LOGIT_INFO("remove");
    LOGIT_SHUTDOWN();
#if __cplusplus >= 201703L
    bool exists = std::filesystem::exists(old1) || std::filesystem::exists(old2) || std::filesystem::exists(old3);
#else
    bool exists = false;
    std::ifstream f1(old1.c_str());
    if (f1.good()) exists = true;
    std::ifstream f2(old2.c_str());
    if (f2.good()) exists = true;
    std::ifstream f3(old3.c_str());
    if (f3.good()) exists = true;
#endif
    return exists ? 1 : 0;
}
