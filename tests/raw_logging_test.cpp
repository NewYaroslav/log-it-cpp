#define LOGIT_COMPILED_LEVEL LOGIT_LEVEL_FATAL
#include <logit.hpp>
#include <string>
#include <vector>

int main() {
    LOGIT_ADD_LOGGER(logit::MemoryLogger, (), logit::SimpleLogFormatter, ("formatted:%v:%l"));
    LOGIT_ADD_MEMORY_LOGGER_DEFAULT_SINGLE_MODE();

    LOGIT_SET_LOG_LEVEL(logit::LogLevel::LOG_LVL_FATAL);

    const std::string block =
        "[App]\n"
        "Name: sample.desktop.app\n"
        "Version: 3.3.106.wzr\n"
        "\n"
        "[Proxy]\n"
        "Proxy enabled: False";

    LOGIT_RAW("raw-text");
    LOGIT_SECTION("Proxy");
    LOGIT_RAW_IF(false, "hidden-raw");
    LOGIT_SECTION_IF(false, "Hidden");
    LOGIT_RAW_IF(true, "conditional-raw");
    LOGIT_SECTION_IF(true, "Automation");
    LOGIT_RAW(block);

    LOGIT_RAW_TO(1, "targeted-raw");
    LOGIT_SECTION_TO(1, "Target");

    LOGIT_WAIT();

    const std::vector<std::string> primary = LOGIT_GET_BUFFERED_STRINGS(0);
    const std::vector<std::string> targeted = LOGIT_GET_BUFFERED_STRINGS(1);

    LOGIT_SHUTDOWN();

    if (primary.size() != 5) return 1;
    if (primary[0] != "raw-text") return 2;
    if (primary[1] != "[Proxy]") return 3;
    if (primary[2] != "conditional-raw") return 4;
    if (primary[3] != "[Automation]") return 5;
    if (primary[4] != block) return 6;

    if (targeted.size() != 2) return 7;
    if (targeted[0] != "targeted-raw") return 8;
    if (targeted[1] != "[Target]") return 9;

    return 0;
}
