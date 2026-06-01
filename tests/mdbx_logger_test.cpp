#include <logit/loggers.hpp>
#include <logit/loggers/MdbxLogger.hpp>

#ifdef LOGIT_WITH_MDBX

#include <cassert>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#if __cplusplus >= 201703L
#include <filesystem>
#endif

namespace {

std::string make_db_path(const std::string& suffix) {
    std::ostringstream os;
    os << logit::get_exec_dir()
       << "/mdbx_logger_test_"
       << suffix
       << "_"
       << LOGIT_CURRENT_TIMESTAMP_MS()
       << ".mdbx";
    return os.str();
}

std::string make_nested_db_path(const std::string& suffix) {
    std::ostringstream os;
    os << logit::get_exec_dir()
       << "/mdbx_logger_test_"
       << suffix
       << "_"
       << LOGIT_CURRENT_TIMESTAMP_MS()
       << "/nested/logs.mdbx";
    return os.str();
}

std::string make_nested_db_root(const std::string& path) {
#if __cplusplus >= 202002L
    const auto root = std::filesystem::u8path(path).parent_path().parent_path().u8string();
    return std::string(root.begin(), root.end());
#else
    return std::filesystem::u8path(path).parent_path().parent_path().u8string();
#endif
}

void cleanup_db(const std::string& path) {
    std::remove(path.c_str());
    std::remove((path + "-lck").c_str());
}

void cleanup_path_tree(const std::string& path) {
    cleanup_db(path);
#if __cplusplus >= 201703L
    std::filesystem::remove_all(std::filesystem::u8path(path).parent_path().parent_path());
#endif
}

logit::LogRecord make_record(logit::LogLevel level, int64_t timestamp_ms, int line) {
    return logit::LogRecord(
        level,
        timestamp_ms,
        "mdbx_logger_test.cpp",
        line,
        "make_record",
        "message",
        "",
        -1,
        false,
        false,
        false);
}

void test_sync_range_session_and_sequences() {
    const std::string path = make_db_path("sync");
    cleanup_db(path);

    {
        logit::MdbxLogger::Config config;
        config.path = path;
        config.app_name = "mdbx-test";
        config.async = false;

        logit::MdbxLogger logger(config);
        const uint64_t session_id = logger.session_id();
        assert(session_id != 0);

        logger.log(make_record(logit::LogLevel::LOG_LVL_INFO, 2001, 10), "later-1");
        logger.log(make_record(logit::LogLevel::LOG_LVL_WARN, 2000, 11), "backward");
        logger.log(make_record(logit::LogLevel::LOG_LVL_ERROR, 2001, 12), "later-2");

        auto records = logger.read_range(2000, 2002);
        assert(records.size() == 3);
        assert(records[0].timestamp_ms == 2000);
        assert(records[0].message == "backward");
        assert(records[1].timestamp_ms == 2001);
        assert(records[2].timestamp_ms == 2001);
        assert(records[1].sequence != records[2].sequence);
        assert(records[2].sequence > records[1].sequence);
        assert(records[2].file == "mdbx_logger_test.cpp");
        assert(records[2].function == "make_record");
        assert(records[2].line == 12);
        assert(records[2].session_id == session_id);

        auto limited = logger.read_range(2000, 2002, 2);
        assert(limited.size() == 2);

        auto session_opt = logger.read_session(session_id);
        assert(session_opt);
        assert(session_opt->app_name == "mdbx-test");
        assert(session_opt->start_time_ms > 0);
        assert(session_opt->end_time_ms == 0);
        assert(session_opt->schema_version == 1);

        logger.shutdown();
        session_opt = logger.read_session(session_id);
        assert(session_opt);
        assert(session_opt->end_time_ms >= session_opt->start_time_ms);
    }

    cleanup_db(path);
}

void test_async_large_payload_spill() {
    const std::string path = make_db_path("payload");
    cleanup_db(path);

    {
        logit::MdbxLogger::Config config;
        config.path = path;
        config.async = true;
        config.flush_interval_ms = 5;
        config.max_batch_size = 8;
        config.large_payload_threshold = 10;
        config.payload_preview_size = 5;
        config.store_large_payloads_separately = true;

        logit::MdbxLogger logger(config);

        const std::string small = "small";
        const std::string large = "abcdefghijklmnopqrstuvwxyz";
        logger.log(make_record(logit::LogLevel::LOG_LVL_INFO, 3000, 20), small);
        logger.log(make_record(logit::LogLevel::LOG_LVL_INFO, 3001, 21), large);
        logger.wait();

        auto records = logger.read_range(3000, 3002);
        assert(records.size() == 2);
        assert(records[0].message == small);
        assert(records[0].payload_id == 0);
        assert(records[1].message == "abcde");
        assert(records[1].payload_id != 0);

        auto payload_opt = logger.read_payload(records[1].payload_id);
        assert(payload_opt);
        assert(payload_opt->payload_id == records[1].payload_id);
        assert(payload_opt->compression == logit::MdbxPayloadCompression::None);
        assert(payload_opt->data == large);

        auto data_opt = logger.read_payload_data(records[1].payload_id);
        assert(data_opt);
        assert(*data_opt == large);

        logger.shutdown();
    }

    cleanup_db(path);
}

#if defined(LOGIT_HAS_ZLIB)
void test_gzip_payload_compression() {
    const std::string path = make_db_path("gzip");
    cleanup_db(path);

    {
        logit::MdbxLogger::Config config;
        config.path = path;
        config.async = false;
        config.large_payload_threshold = 4;
        config.payload_preview_size = 3;
        config.payload_compression = logit::MdbxPayloadCompression::Gzip;
        config.payload_compression_level = 6;

        logit::MdbxLogger logger(config);
        const std::string large(128, 'x');
        logger.log(make_record(logit::LogLevel::LOG_LVL_INFO, 4000, 30), large);

        auto records = logger.read_range(4000, 4001);
        assert(records.size() == 1);
        assert(records[0].payload_id != 0);

        auto payload_opt = logger.read_payload(records[0].payload_id);
        assert(payload_opt);
        assert(payload_opt->compression == logit::MdbxPayloadCompression::Gzip);
        assert(!payload_opt->data.empty());
        assert(payload_opt->data != large);

        auto data_opt = logger.read_payload_data(records[0].payload_id);
        assert(data_opt);
        assert(*data_opt == large);

        logger.shutdown();
    }

    cleanup_db(path);
}
#endif

void test_counters_zero_for_sync_writes() {
    const std::string path = make_db_path("counters");
    cleanup_db(path);

    {
        logit::MdbxLogger::Config config;
        config.path = path;
        config.async = false;

        logit::MdbxLogger logger(config);
        assert(logger.dropped_count() == 0);
        assert(logger.failed_export_count() == 0);

        logger.log(make_record(logit::LogLevel::LOG_LVL_INFO, 5000, 40), "first");
        logger.log(make_record(logit::LogLevel::LOG_LVL_INFO, 5001, 41), "second");

        assert(logger.dropped_count() == 0);
        assert(logger.failed_export_count() == 0);
        logger.shutdown();
    }

    cleanup_db(path);
}

void test_on_error_callback() {
    const std::string path = make_db_path("error_cb");
    cleanup_db(path);

    {
        std::vector<std::string> errors;
        logit::MdbxLogger::Config config;
        config.path = path;
        config.async = false;
        config.on_error = [&errors](const std::string& msg) {
            errors.push_back(msg);
        };

        logit::MdbxLogger logger(config);
        logger.log(make_record(logit::LogLevel::LOG_LVL_INFO, 6000, 50), "ok");
        logger.shutdown();
        assert(errors.empty());
    }

    cleanup_db(path);
}

void test_nested_parent_directory_created() {
    const std::string path = make_nested_db_path("nested");
    cleanup_path_tree(path);

    {
        logit::MdbxLogger::Config config;
        config.path = path;
        config.async = false;

        logit::MdbxLogger logger(config);
        logger.log(make_record(logit::LogLevel::LOG_LVL_INFO, 6100, 51), "nested-ok");

        auto records = logger.read_range(6100, 6101);
        assert(records.size() == 1);
        assert(records[0].message == "nested-ok");
        logger.shutdown();
    }

    cleanup_path_tree(path);
}

void test_init_error_callback_and_rethrow() {
    const std::string path = make_nested_db_path("init_error");
    cleanup_path_tree(path);

    std::vector<std::string> errors;
    logit::MdbxLogger::Config config;
    config.path = path;
    config.async = false;
    config.on_error = [&errors](const std::string& msg) {
        errors.push_back(msg);
    };

    const std::string blocker = make_nested_db_root(path);
    FILE* blocker_file = std::fopen(blocker.c_str(), "wb");
    assert(blocker_file != nullptr);
    std::fclose(blocker_file);

    bool thrown = false;
    try {
        logit::MdbxLogger logger(config);
    } catch (const std::exception&) {
        thrown = true;
    }

    assert(thrown);
    assert(!errors.empty());
    assert(errors[0].find("MdbxLogger initialization error") != std::string::npos);

    std::remove(blocker.c_str());
    cleanup_path_tree(path);
}

void test_read_range_empty_and_limits() {
    const std::string path = make_db_path("range");
    cleanup_db(path);

    {
        logit::MdbxLogger::Config config;
        config.path = path;
        config.async = false;

        logit::MdbxLogger logger(config);
        logger.log(make_record(logit::LogLevel::LOG_LVL_INFO, 7000, 60), "a");
        logger.log(make_record(logit::LogLevel::LOG_LVL_INFO, 7001, 61), "b");
        logger.log(make_record(logit::LogLevel::LOG_LVL_INFO, 7002, 62), "c");

        auto empty = logger.read_range(8000, 9000);
        assert(empty.empty());

        auto all = logger.read_range(7000, 7003);
        assert(all.size() == 3);

        auto limited = logger.read_range(7000, 7003, 2);
        assert(limited.size() == 2);

        auto zero_span = logger.read_range(7001, 7001);
        assert(zero_span.empty());

        logger.shutdown();
    }

    cleanup_db(path);
}

void test_read_recent() {
    const std::string path = make_db_path("recent");
    cleanup_db(path);

    {
        logit::MdbxLogger::Config config;
        config.path = path;
        config.async = false;

        logit::MdbxLogger logger(config);
        const int64_t now = LOGIT_CURRENT_TIMESTAMP_MS();
        logger.log(make_record(logit::LogLevel::LOG_LVL_INFO, now - 3000, 70), "old");
        logger.log(make_record(logit::LogLevel::LOG_LVL_INFO, now - 1000, 71), "mid");
        logger.log(make_record(logit::LogLevel::LOG_LVL_INFO, now, 72), "new");

        auto all_asc = logger.read_recent(0, 0, logit::LogReadOrder::Ascending);
        assert(all_asc.size() == 3);
        assert(all_asc[0].message == "old");
        assert(all_asc[2].message == "new");

        auto all_desc = logger.read_recent(0, 0, logit::LogReadOrder::Descending);
        assert(all_desc.size() == 3);
        assert(all_desc[0].message == "new");
        assert(all_desc[2].message == "old");

        auto limited = logger.read_recent(2, 0, logit::LogReadOrder::Ascending);
        assert(limited.size() == 2);
        assert(limited[0].message == "mid");
        assert(limited[1].message == "new");

        auto limited_desc = logger.read_recent(2, 0, logit::LogReadOrder::Descending);
        assert(limited_desc.size() == 2);
        assert(limited_desc[0].message == "new");
        assert(limited_desc[1].message == "mid");

        auto period = logger.read_recent(10, 1500, logit::LogReadOrder::Ascending);
        assert(period.size() == 2);
        assert(period[0].message == "mid");
        assert(period[1].message == "new");

        logger.shutdown();
    }

    cleanup_db(path);
}

void test_callback_sync() {
    const std::string path = make_db_path("cb_sync");
    cleanup_db(path);

    {
        logit::MdbxLogger::Config config;
        config.path = path;
        config.async = false;

        logit::MdbxLogger logger(config);
        std::vector<logit::LogRecordView> received;
        const uint64_t cb_id = logger.add_log_callback(
            [&received](const logit::LogRecordView& v) {
                received.push_back(v);
            });
        assert(cb_id != 0);

        logger.log(make_record(logit::LogLevel::LOG_LVL_INFO, 8000, 80), "hello");
        assert(received.size() == 1);
        assert(received[0].message == "hello");
        assert(received[0].level == logit::LogLevel::LOG_LVL_INFO);

        assert(logger.remove_log_callback(cb_id));
        logger.log(make_record(logit::LogLevel::LOG_LVL_INFO, 8001, 81), "after-remove");
        assert(received.size() == 1);

        logger.shutdown();
    }

    cleanup_db(path);
}

void test_callback_async() {
    const std::string path = make_db_path("cb_async");
    cleanup_db(path);

    {
        logit::MdbxLogger::Config config;
        config.path = path;
        config.async = true;
        config.flush_interval_ms = 5;
        config.max_batch_size = 64;

        logit::MdbxLogger logger(config);
        std::vector<logit::LogRecordView> received;
        const uint64_t cb_id = logger.add_log_callback(
            [&received](const logit::LogRecordView& v) {
                received.push_back(v);
            });
        assert(cb_id != 0);

        logger.log(make_record(logit::LogLevel::LOG_LVL_WARN, 9000, 90), "async-1");
        logger.log(make_record(logit::LogLevel::LOG_LVL_ERROR, 9001, 91), "async-2");
        logger.wait();

        assert(received.size() == 2);
        assert(received[0].message == "async-1");
        assert(received[1].message == "async-2");

        assert(logger.remove_log_callback(cb_id));
        logger.shutdown();
    }

    cleanup_db(path);
}

void test_callback_exception_safe() {
    const std::string path = make_db_path("cb_ex");
    cleanup_db(path);

    {
        logit::MdbxLogger::Config config;
        config.path = path;
        config.async = false;
        std::vector<std::string> errors;
        config.on_error = [&errors](const std::string& msg) {
            errors.push_back(msg);
        };

        logit::MdbxLogger logger(config);
        bool second_called = false;

        logger.add_log_callback(
            [](const logit::LogRecordView&) {
                throw std::runtime_error("boom");
            });
        logger.add_log_callback(
            [&second_called](const logit::LogRecordView&) {
                second_called = true;
            });

        logger.log(make_record(logit::LogLevel::LOG_LVL_INFO, 10000, 100), "ok");
        assert(second_called);
        assert(!errors.empty());

        logger.shutdown();
    }

    cleanup_db(path);
}

} // namespace

int main() {
    test_sync_range_session_and_sequences();
    test_async_large_payload_spill();
#if defined(LOGIT_HAS_ZLIB)
    test_gzip_payload_compression();
#endif
    test_counters_zero_for_sync_writes();
    test_on_error_callback();
    test_nested_parent_directory_created();
    test_init_error_callback_and_rethrow();
    test_read_range_empty_and_limits();
    test_read_recent();
    test_callback_sync();
    test_callback_async();
    test_callback_exception_safe();
    std::cout << "PASS: mdbx_logger_test" << std::endl;
    return 0;
}

#else

int main() {
    return 0;
}

#endif
