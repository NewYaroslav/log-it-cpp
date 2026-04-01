// #define LOGIT_BASE_PATH "E:\\_repoz\\log-it-cpp" <- set via CMake

#include <iostream>
#include <logit.hpp>

int main() {
    std::cout << "Starting in-memory snapshot logger example..." << std::endl;

    LOGIT_ADD_CONSOLE_DEFAULT();
    LOGIT_ADD_MEMORY_LOGGER_SINGLE_MODE(100, 64 * 1024, 60 * 60 * 1000);

    LOGIT_INFO("This goes to the regular backends");
    LOGIT_INFO_TO(1, "Remote control can fetch this later");
    LOGIT_WARN_TO(1, "Latest warning for the control plane");

    const auto recent_lines = LOGIT_GET_BUFFERED_STRINGS(1);
    const auto recent_entries = LOGIT_GET_BUFFERED_ENTRIES(1);

    std::cout << "Buffered strings:" << std::endl;
    for (const auto& line : recent_lines) {
        std::cout << "  " << line << std::endl;
    }

    std::cout << "Buffered structured entries:" << std::endl;
    for (const auto& entry : recent_entries) {
        std::cout << "  [" << logit::to_string(entry.level) << "] "
                  << entry.timestamp_ms << " " << entry.message << std::endl;
    }

    return 0;
}
