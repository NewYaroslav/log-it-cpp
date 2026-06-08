/// \file example_logit_mdbx_logger.cpp
/// \brief Demonstrates structured log storage with MdbxLogger, including macros,
///        querying by date range, and reading sessions/payloads.

// #define LOGIT_BASE_PATH "E:\\_repoz\\log-it-cpp"  <- set via CMake

#include <logit.hpp>

#ifdef LOGIT_WITH_MDBX

#include <chrono>
#include <cstdio>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

namespace {

std::string make_db_path() {
    std::ostringstream os;
    os << logit::get_exec_dir() << "/example_mdbx_logger_" << LOGIT_CURRENT_TIMESTAMP_MS() << ".mdbx";
    return os.str();
}

void cleanup_db(const std::string& path) {
    std::remove(path.c_str());
    std::remove((path + "-lck").c_str());
}

} // namespace

int main() {
    const std::string db_path = make_db_path();
    cleanup_db(db_path);

    std::cout << "MdbxLogger example starting, db=" << db_path << std::endl;

    // ------------------------------------------------------------------
    // 1. Configure and add the MDBX backend
    // ------------------------------------------------------------------
    logit::MdbxLogger::Config mdbx_config;
    mdbx_config.path = db_path;
    mdbx_config.app_name = "example-mdbx";
    mdbx_config.async = true;
    mdbx_config.max_queue_size = 256;
    mdbx_config.max_batch_size = 32;
    mdbx_config.large_payload_threshold = 256;
    mdbx_config.store_large_payloads_separately = true;

    // Optional: capture write errors instead of writing to stderr
    mdbx_config.on_error = [](const std::string& msg) {
        std::cerr << "[MdbxLogger error callback] " << msg << std::endl;
    };

    // Add a console logger for live observation (optional).
    LOGIT_ADD_CONSOLE_DEFAULT();

    // Add to the registry as a regular backend so standard LOGIT_* macros
    // write both to MDBX and to the console logger above.
    LOGIT_ADD_LOGGER(
        logit::MdbxLogger,
        (mdbx_config),
        logit::SimpleLogFormatter,
        ("[%l] %v"));

    // The MDBX logger is now the last added backend; its index is:
    const int mdbx_index = static_cast<int>(logit::Logger::get_instance().logger_count()) - 1;

    // ------------------------------------------------------------------
    // 2. Log messages via standard macros
    // ------------------------------------------------------------------
    LOGIT_INFO("Application started");
    LOGIT_WARN("Disk usage is above 80%%");

    {
        LOGIT_SCOPE_INFO("process_batch");
        LOGIT_INFO("Processing 42 items");
        LOGIT_DEBUG("Detail: item id=7, status=pending");

        // This message is larger than large_payload_threshold and will spill
        // into the separate log_payloads table with a preview kept inline.
        std::string big;
        big.reserve(512);
        for (int i = 0; i < 50; ++i) {
            big += "chunk-" + std::to_string(i) + " ";
        }
        LOGIT_INFO(big);
    }

    LOGIT_ERROR("Connection timeout to upstream-3");
    LOGIT_FATAL("Critical: unable to recover transaction state");

    // Wait until the async queue is flushed.
    LOGIT_WAIT();

    // ------------------------------------------------------------------
    // 3. Read through the common ILogReader API, then use MDBX-only extras
    // ------------------------------------------------------------------
    LOGIT_WITH_LOGGER_AS(mdbx_index, logit::MdbxLogger, mdbx) {
        // Session metadata
        auto session_opt = mdbx->read_session(mdbx->session_id());
        if (session_opt) {
            std::cout << "\n--- Session ---" << std::endl;
            std::cout << "  app_name:    " << session_opt->app_name << std::endl;
            std::cout << "  process_id:  " << session_opt->process_id << std::endl;
            std::cout << "  started_ms:  " << session_opt->start_time_ms << std::endl;
            std::cout << "  schema_ver:  " << session_opt->schema_version << std::endl;
        }

        // The LOGIT_READ_* macros work with both MemoryLogger and MdbxLogger.
        // All records in a wide time window (last 24 hours).
        const int64_t now_ms = LOGIT_CURRENT_TIMESTAMP_MS();
        auto all_records = LOGIT_READ_RANGE(
            mdbx_index,
            now_ms - 24LL * 60 * 60 * 1000,
            now_ms + 1,
            0);

        std::cout << "\n--- All records (" << all_records.size() << ") ---" << std::endl;
        for (const auto& r : all_records) {
            std::cout << "  [" << logit::to_string(r.level) << "] "
                      << r.timestamp_ms << " seq=" << r.sequence
                      << " msg=\"" << r.message << "\"";
            if (r.payload_id != 0) {
                std::cout << " [payload_id=" << r.payload_id << "]";
            }
            std::cout << std::endl;
        }

        // Level-based client-side filter
        std::cout << "\n--- Records with level >= WARN ---" << std::endl;
        for (const auto& r : all_records) {
            if (static_cast<int>(r.level) >= static_cast<int>(logit::LogLevel::LOG_LVL_WARN)) {
                std::cout << "  [" << logit::to_string(r.level) << "] "
                          << r.timestamp_ms << " " << r.message << std::endl;
            }
        }

        // Spilled payloads (decompressed transparently)
        std::cout << "\n--- Spilled payloads ---" << std::endl;
        for (const auto& r : all_records) {
            if (r.payload_id != 0) {
                auto data_opt = mdbx->read_payload_data(r.payload_id);
                if (data_opt) {
                    std::cout << "  payload_id=" << r.payload_id
                              << " size=" << data_opt->size()
                              << " preview=\"" << r.message << "\""
                              << std::endl;
                } else {
                    std::cout << "  payload_id=" << r.payload_id << " FAILED to read/decompress"
                              << std::endl;
                }
            }
        }

        // Query by today's local midnight
        try {
            auto now_local = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            std::tm* tm_now = std::localtime(&now_local);
            std::tm tm_midnight = *tm_now;
            tm_midnight.tm_hour = 0;
            tm_midnight.tm_min = 0;
            tm_midnight.tm_sec = 0;
            std::time_t midnight_t = std::mktime(&tm_midnight);

            auto today_ms = std::chrono::milliseconds(static_cast<int64_t>(midnight_t) * 1000);
            auto tomorrow_ms = today_ms + std::chrono::hours(24);

            auto day_records = LOGIT_READ_RANGE(
                mdbx_index,
                today_ms.count(),
                tomorrow_ms.count(),
                0);

            std::cout << "\n--- Records for today (" << day_records.size() << ") ---" << std::endl;
            for (const auto& r : day_records) {
                std::cout << "  [" << logit::to_string(r.level) << "] "
                          << r.timestamp_ms << " " << r.message << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Date query example skipped: " << e.what() << std::endl;
        }

        // read_recent: last 100 records in ascending order, through ILogReader.
        auto recent = LOGIT_READ_RECENT_ASC(mdbx_index, 100, 0);
        std::cout << "\n--- LOGIT_READ_RECENT_ASC(100) (" << recent.size() << ") ---" << std::endl;
        for (const auto& r : recent) {
            std::cout << "  [" << logit::to_string(r.level) << "] "
                      << r.timestamp_ms << " " << r.message << std::endl;
        }

        // Live subscription: the LOGIT_* callback macros also work with both
        // MemoryLogger and MdbxLogger.
        std::vector<logit::LogRecordSnapshot> live_updates;
        const uint64_t cb_id = LOGIT_ADD_LOG_CALLBACK(
            mdbx_index,
            ([&live_updates](const logit::LogRecordSnapshot& v) {
                live_updates.push_back(v);
            }));

        LOGIT_INFO("Live event 1 via callback");
        LOGIT_INFO("Live event 2 via callback");
        LOGIT_WAIT();

        std::cout << "\n--- Live updates received (" << live_updates.size()
                  << ") ---" << std::endl;
        for (const auto& r : live_updates) {
            std::cout << "  [" << logit::to_string(r.level) << "] "
                      << r.message << std::endl;
        }

        if (LOGIT_REMOVE_LOG_CALLBACK(mdbx_index, cb_id)) {
            std::cout << "Callback removed" << std::endl;
        }

        LOGIT_INFO("Event after unsubscribe");
        LOGIT_WAIT();
        std::cout << "Live updates after unsubscribe: " << live_updates.size()
                  << std::endl;

        // Statistics
        std::cout << "\n--- Statistics ---" << std::endl;
        std::cout << "  dropped:       " << mdbx->dropped_count() << std::endl;
        std::cout << "  failed_writes: " << mdbx->failed_export_count() << std::endl;
    } else {
        std::cerr << "MDBX logger not found at index " << mdbx_index << std::endl;
        LOGIT_SHUTDOWN();
        cleanup_db(db_path);
        return 1;
    }

    // ------------------------------------------------------------------
    // 5. Graceful shutdown and cleanup
    // ------------------------------------------------------------------
    LOGIT_SHUTDOWN();

    // After shutdown the session end_time_ms is persisted.
    LOGIT_WITH_LOGGER_AS(mdbx_index, logit::MdbxLogger, mdbx) {
        auto session_opt = mdbx->read_session(mdbx->session_id());
        if (session_opt) {
            std::cout << "  session end_ms: " << session_opt->end_time_ms << std::endl;
        }
    }

    cleanup_db(db_path);
    std::cout << "\nMdbxLogger example completed." << std::endl;
    return 0;
}

#else

#include <iostream>

int main() {
    std::cout << "MdbxLogger example requires LOGIT_WITH_MDBX=ON." << std::endl;
    return 0;
}

#endif
