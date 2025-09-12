#define LOGIT_FILE_LOGGER_PATH "."
#include <LogIt.hpp>
#include <time_shield.hpp>
#include <fstream>
#include <string>

int main() {
    LOGIT_ADD_FILE_LOGGER_DEFAULT();
    const std::string message = "test log message";
    LOGIT_INFO(message);
    LOGIT_WAIT();
    const auto date_ts = time_shield::start_of_day(time_shield::ms_to_sec(time_shield::ts_ms()));
    const std::string filename = time_shield::to_iso8601_date(date_ts) + ".log";
    std::ifstream in(filename);
    std::string line;
    std::getline(in, line);
    return line.find(message) != std::string::npos ? 0 : 1;
}
