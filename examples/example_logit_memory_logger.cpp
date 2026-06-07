/// \file example_logit_memory_logger.cpp
/// \brief Demonstrates in-memory snapshots, range reads, and live subscriptions.

// #define LOGIT_BASE_PATH "E:\\_repoz\\log-it-cpp" <- set via CMake

#include <cstdint>
#include <iostream>
#include <mutex>
#include <vector>
#include <logit.hpp>

int main() {
    std::cout << "Starting in-memory snapshot logger example..." << std::endl;

    LOGIT_ADD_CONSOLE_DEFAULT();
    LOGIT_ADD_MEMORY_LOGGER_SINGLE_MODE(100, 64 * 1024, 60 * 60 * 1000);
    const int memory_index = static_cast<int>(logit::Logger::get_instance().logger_count()) - 1;

    LOGIT_INFO("This goes to the regular backends");
    LOGIT_INFO_TO(memory_index, "Remote control can fetch this later");
    LOGIT_WARN_TO(memory_index, "Latest warning for the control plane");
    LOGIT_SECTION_TO(memory_index, "Network Settings");
    LOGIT_RAW_TO(memory_index, "Protocol: https");
    LOGIT_RAW_TO(memory_index, "Transport Mode: standard");
    LOGIT_RAW_TO(memory_index, "DNS Server: 1.1.1.1");
    LOGIT_WAIT();

    const auto recent_lines = LOGIT_GET_BUFFERED_STRINGS(memory_index);
    const auto recent_entries = LOGIT_GET_BUFFERED_ENTRIES(memory_index);

    std::cout << "Buffered strings:" << std::endl;
    for (const auto& line : recent_lines) {
        std::cout << "  " << line << std::endl;
    }

    std::cout << "Buffered structured entries:" << std::endl;
    for (const auto& entry : recent_entries) {
        std::cout << "  [" << logit::to_string(entry.level) << "] "
                  << entry.timestamp_ms << " " << entry.message << std::endl;
    }

    const auto recent_records = LOGIT_READ_RECENT_ASC(memory_index, 3, 0);
    std::cout << "Recent records via ILogReader:" << std::endl;
    for (const auto& record : recent_records) {
        std::cout << "  [" << logit::to_string(record.level) << "] "
                  << record.timestamp_ms << " " << record.message << std::endl;
    }

    const int64_t now_ms = LOGIT_CURRENT_TIMESTAMP_MS();
    const auto range_records = LOGIT_READ_RANGE(
        memory_index,
        now_ms - 60LL * 60 * 1000,
        now_ms + 1,
        0);
    std::cout << "Range records for the last hour: "
              << range_records.size() << std::endl;

    std::vector<logit::LogRecordSnapshot> live_updates;
    std::mutex live_mutex;
    const uint64_t callback_id = LOGIT_ADD_LOG_CALLBACK(
        memory_index,
        ([&live_updates, &live_mutex](const logit::LogRecordSnapshot& view) {
            std::lock_guard<std::mutex> lock(live_mutex);
            live_updates.push_back(view);
        }));

    LOGIT_INFO_TO(memory_index, "Live callback event 1");
    LOGIT_ERROR_TO(memory_index, "Live callback event 2");
    LOGIT_WAIT();

    std::size_t live_count = 0;
    {
        std::lock_guard<std::mutex> lock(live_mutex);
        live_count = live_updates.size();
        std::cout << "Live updates received:" << std::endl;
        for (const auto& record : live_updates) {
            std::cout << "  [" << logit::to_string(record.level) << "] "
                      << record.message << std::endl;
        }
    }

    if (LOGIT_REMOVE_LOG_CALLBACK(memory_index, callback_id)) {
        std::cout << "Callback removed" << std::endl;
    }

    LOGIT_WARN_TO(memory_index, "Stored after callback removal");
    LOGIT_WAIT();

    {
        std::lock_guard<std::mutex> lock(live_mutex);
        std::cout << "Live updates after unsubscribe: "
                  << live_updates.size() << " (was " << live_count << ")"
                  << std::endl;
    }

    LOGIT_SHUTDOWN();
    return 0;
}
