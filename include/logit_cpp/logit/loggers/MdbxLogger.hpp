#pragma once
#ifndef _LOGIT_MDBX_LOGGER_HPP_INCLUDED
#define _LOGIT_MDBX_LOGGER_HPP_INCLUDED

/// \file MdbxLogger.hpp
/// \brief MDBX structured log storage backend.

#include "ILogger.hpp"
#include "otlp/OtlpCompression.hpp"
#include <mdbx_containers/KeyValueTable.hpp>
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <deque>
#include <iostream>
#include <limits>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace logit {

    /// \enum MdbxPayloadCompression
    /// \brief Compression algorithm used for payload rows.
    enum class MdbxPayloadCompression {
        None = 0, ///< Store payload bytes as-is.
        Gzip = 1, ///< Store gzip-compressed payload bytes.
        Zstd = 2  ///< Store zstd-compressed payload bytes.
    };

    /// \struct MdbxLogSessionV1
    /// \brief Session metadata stored in the `log_sessions` table.
    struct MdbxLogSessionV1 {
        std::string app_name;       ///< Optional application name.
        int64_t start_time_ms = 0;  ///< Session start timestamp.
        int64_t end_time_ms = 0;    ///< Session end timestamp, or 0 while active.
        uint64_t process_id = 0;    ///< Process id that opened the session.
        uint32_t schema_version = 1;///< Storage schema version.

        std::vector<uint8_t> to_bytes() const;
        static MdbxLogSessionV1 from_bytes(const void* data, size_t size);
    };

    /// \struct MdbxLogRecordV1
    /// \brief Log record stored in the `log_records_by_time` table.
    struct MdbxLogRecordV1 {
        uint64_t session_id = 0;                     ///< Owning session id.
        int64_t timestamp_ms = 0;                    ///< Log timestamp in milliseconds.
        uint32_t sequence = 0;                       ///< Per-timestamp sequence.
        LogLevel level = LogLevel::LOG_LVL_TRACE;    ///< Log severity level.
        std::string message;                         ///< Formatted message or payload preview.
        uint64_t payload_id = 0;                     ///< Payload row id, or 0 when absent.
        std::string file;                            ///< Source file path.
        std::string function;                        ///< Source function name.
        int line = 0;                                ///< Source line number.

        std::vector<uint8_t> to_bytes() const;
        static MdbxLogRecordV1 from_bytes(const void* data, size_t size);
    };

    /// \struct MdbxLogPayloadV1
    /// \brief Large payload stored in the `log_payloads` table.
    struct MdbxLogPayloadV1 {
        uint64_t payload_id = 0; ///< Stable payload id.
        MdbxPayloadCompression compression = MdbxPayloadCompression::None; ///< Stored compression.
        std::string data;       ///< Stored payload bytes.

        std::vector<uint8_t> to_bytes() const;
        static MdbxLogPayloadV1 from_bytes(const void* data, size_t size);
    };

namespace detail {

    class MdbxByteWriter {
    public:
        void write_u8(uint8_t value) {
            m_out.push_back(value);
        }

        void write_u32(uint32_t value) {
            for (int shift = 24; shift >= 0; shift -= 8) {
                m_out.push_back(static_cast<uint8_t>((value >> shift) & 0xFFu));
            }
        }

        void write_u64(uint64_t value) {
            for (int shift = 56; shift >= 0; shift -= 8) {
                m_out.push_back(static_cast<uint8_t>((value >> shift) & 0xFFu));
            }
        }

        void write_i64(int64_t value) {
            uint64_t bits = 0;
            std::memcpy(&bits, &value, sizeof(bits));
            write_u64(bits);
        }

        void write_string(const std::string& value) {
            if (value.size() > static_cast<size_t>((std::numeric_limits<uint32_t>::max)())) {
                throw std::length_error("MdbxLogger: string field is too large");
            }
            write_u32(static_cast<uint32_t>(value.size()));
            m_out.insert(m_out.end(), value.begin(), value.end());
        }

        const std::vector<uint8_t>& bytes() const {
            return m_out;
        }

    private:
        std::vector<uint8_t> m_out;
    };

    class MdbxByteReader {
    public:
        MdbxByteReader(const void* data, size_t size)
            : m_cur(static_cast<const uint8_t*>(data)),
              m_end(static_cast<const uint8_t*>(data)) {
            if (m_cur == nullptr && size != 0) {
                throw std::runtime_error("MdbxLogger: null serialized value");
            }
            m_end = m_cur == nullptr ? m_cur : m_cur + size;
        }

        uint8_t read_u8() {
            require(1);
            return *m_cur++;
        }

        uint32_t read_u32() {
            require(4);
            uint32_t value = 0;
            for (int i = 0; i < 4; ++i) {
                value = (value << 8) | static_cast<uint32_t>(*m_cur++);
            }
            return value;
        }

        uint64_t read_u64() {
            require(8);
            uint64_t value = 0;
            for (int i = 0; i < 8; ++i) {
                value = (value << 8) | static_cast<uint64_t>(*m_cur++);
            }
            return value;
        }

        int64_t read_i64() {
            const uint64_t bits = read_u64();
            int64_t value = 0;
            std::memcpy(&value, &bits, sizeof(value));
            return value;
        }

        std::string read_string() {
            const uint32_t size = read_u32();
            require(size);
            const char* begin = reinterpret_cast<const char*>(m_cur);
            m_cur += size;
            return std::string(begin, size);
        }

        void finish() const {
            if (m_cur != m_end) {
                throw std::runtime_error("MdbxLogger: trailing bytes in serialized value");
            }
        }

    private:
        const uint8_t* m_cur;
        const uint8_t* m_end;

        void require(size_t size) const {
            if (static_cast<size_t>(m_end - m_cur) < size) {
                throw std::runtime_error("MdbxLogger: corrupted serialized value");
            }
        }
    };

    inline void mdbx_write_record_key_be(std::string& key, uint64_t value, size_t offset) {
        for (int shift = 56; shift >= 0; shift -= 8) {
            key[offset++] = static_cast<char>((value >> shift) & 0xFFu);
        }
    }

    inline void mdbx_write_record_sequence_be(std::string& key, uint32_t value, size_t offset) {
        for (int shift = 24; shift >= 0; shift -= 8) {
            key[offset++] = static_cast<char>((value >> shift) & 0xFFu);
        }
    }

    inline std::string make_mdbx_record_key(int64_t timestamp_ms, uint32_t sequence) {
        std::string key(12, '\0');
        const uint64_t sortable_ts = static_cast<uint64_t>(timestamp_ms) ^ 0x8000000000000000ULL;
        mdbx_write_record_key_be(key, sortable_ts, 0);
        mdbx_write_record_sequence_be(key, sequence, 8);
        return key;
    }

    inline uint64_t current_process_id() {
#       if defined(_WIN32)
        return static_cast<uint64_t>(GetCurrentProcessId());
#       else
        return static_cast<uint64_t>(getpid());
#       endif
    }

} // namespace detail

    inline std::vector<uint8_t> MdbxLogSessionV1::to_bytes() const {
        detail::MdbxByteWriter out;
        out.write_u32(1);
        out.write_string(app_name);
        out.write_i64(start_time_ms);
        out.write_i64(end_time_ms);
        out.write_u64(process_id);
        out.write_u32(schema_version);
        return out.bytes();
    }

    inline MdbxLogSessionV1 MdbxLogSessionV1::from_bytes(const void* data, size_t size) {
        detail::MdbxByteReader in(data, size);
        const uint32_t version = in.read_u32();
        if (version != 1) {
            throw std::runtime_error("MdbxLogger: unsupported session value version");
        }
        MdbxLogSessionV1 out;
        out.app_name = in.read_string();
        out.start_time_ms = in.read_i64();
        out.end_time_ms = in.read_i64();
        out.process_id = in.read_u64();
        out.schema_version = in.read_u32();
        in.finish();
        return out;
    }

    inline std::vector<uint8_t> MdbxLogRecordV1::to_bytes() const {
        detail::MdbxByteWriter out;
        out.write_u32(1);
        out.write_u64(session_id);
        out.write_i64(timestamp_ms);
        out.write_u32(sequence);
        out.write_u32(static_cast<uint32_t>(level));
        out.write_string(message);
        out.write_u64(payload_id);
        out.write_string(file);
        out.write_string(function);
        out.write_i64(static_cast<int64_t>(line));
        return out.bytes();
    }

    inline MdbxLogRecordV1 MdbxLogRecordV1::from_bytes(const void* data, size_t size) {
        detail::MdbxByteReader in(data, size);
        const uint32_t version = in.read_u32();
        if (version != 1) {
            throw std::runtime_error("MdbxLogger: unsupported record value version");
        }
        MdbxLogRecordV1 out;
        out.session_id = in.read_u64();
        out.timestamp_ms = in.read_i64();
        out.sequence = in.read_u32();
        out.level = static_cast<LogLevel>(in.read_u32());
        out.message = in.read_string();
        out.payload_id = in.read_u64();
        out.file = in.read_string();
        out.function = in.read_string();
        out.line = static_cast<int>(in.read_i64());
        in.finish();
        return out;
    }

    inline std::vector<uint8_t> MdbxLogPayloadV1::to_bytes() const {
        detail::MdbxByteWriter out;
        out.write_u32(1);
        out.write_u64(payload_id);
        out.write_u8(static_cast<uint8_t>(compression));
        out.write_string(data);
        return out.bytes();
    }

    inline MdbxLogPayloadV1 MdbxLogPayloadV1::from_bytes(const void* data, size_t size) {
        detail::MdbxByteReader in(data, size);
        const uint32_t version = in.read_u32();
        if (version != 1) {
            throw std::runtime_error("MdbxLogger: unsupported payload value version");
        }
        MdbxLogPayloadV1 out;
        out.payload_id = in.read_u64();
        const uint8_t compression = in.read_u8();
        if (compression > static_cast<uint8_t>(MdbxPayloadCompression::Zstd)) {
            throw std::runtime_error("MdbxLogger: unsupported payload compression");
        }
        out.compression = static_cast<MdbxPayloadCompression>(compression);
        out.data = in.read_string();
        in.finish();
        return out;
    }

    /// \class MdbxLogger
    /// \brief Stores formatted logs in MDBX tables with optional async batching.
    class MdbxLogger final : public ILogger {
    public:
        /// \struct Config
        /// \brief Configuration for the MDBX logger backend.
        struct Config {
            std::string path = "logs.mdbx"; ///< MDBX environment path.
            std::string app_name;           ///< Optional application name stored in session metadata.
            uint64_t session_id = 0;        ///< Existing session id, or 0 to auto-create.
            bool async = true;              ///< Queue records and write them from a worker thread.
            bool drop_on_overflow = true;   ///< Drop newest record when the async queue is full.
            std::size_t max_queue_size = 8192; ///< Maximum async queue size; 0 means unlimited.
            std::size_t max_batch_size = 256;  ///< Maximum records per write transaction.
            int flush_interval_ms = 100;       ///< Worker wake interval.
            std::size_t large_payload_threshold = 4096; ///< Spill messages larger than this.
            std::size_t payload_preview_size = 512;     ///< Bytes kept inline when a message spills.
            bool store_large_payloads_separately = true;///< Store large messages in `log_payloads`.
            MdbxPayloadCompression payload_compression = MdbxPayloadCompression::None; ///< Payload compression.
            int payload_compression_level = 6; ///< Compression level for gzip/zstd.
        };

        MdbxLogger() : MdbxLogger(Config()) {}

        explicit MdbxLogger(const Config& config)
            : m_config(config) {
            normalize_config();
            validate_compression_config();
            open_storage();
            m_session_id = open_session();
            if (m_config.async) {
                m_worker = std::thread(&MdbxLogger::worker_loop, this);
            }
        }

        ~MdbxLogger() override {
            shutdown();
        }

        MdbxLogger(const MdbxLogger&) = delete;
        MdbxLogger& operator=(const MdbxLogger&) = delete;

        /// \brief Queues or writes a formatted log record.
        void log(const LogRecord& record, const std::string& message) override {
            MdbxLogItem item;
            item.level = record.log_level;
            item.timestamp_ms = record.timestamp_ms;
            item.file = record.file;
            item.function = record.function;
            item.line = record.line;
            item.message = message;

            if (!m_config.async) {
                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    if (m_stopping) {
                        ++m_dropped;
                        return;
                    }
                }
                mark_last_log(record.timestamp_ms);
                std::vector<MdbxLogItem> batch;
                batch.push_back(std::move(item));
                write_batch(batch);
                return;
            }

            std::unique_lock<std::mutex> lock(m_mutex);
            if (m_stopping) {
                ++m_dropped;
                return;
            }

            mark_last_log(record.timestamp_ms);

            if (m_config.max_queue_size > 0 && m_queue.size() >= m_config.max_queue_size) {
                if (m_config.drop_on_overflow) {
                    ++m_dropped;
                    return;
                }

                m_space_cv.wait(lock, [this]() {
                    return m_stopping ||
                           m_config.max_queue_size == 0 ||
                           m_queue.size() < m_config.max_queue_size;
                });

                if (m_stopping) {
                    ++m_dropped;
                    return;
                }
            }

            m_queue.push_back(std::move(item));
            lock.unlock();
            m_cv.notify_one();
        }

        /// \brief Waits until accepted async records are written.
        void wait() override {
            if (!m_config.async) {
                return;
            }

            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this]() {
                return m_queue.empty() && m_idle;
            });
        }

        /// \brief Stops the worker after draining accepted records.
        void shutdown() override {
            bool expected = false;
            if (!m_shutdown.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
                return;
            }

            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_stopping = true;
            }
            m_cv.notify_all();
            m_space_cv.notify_all();

            if (m_worker.joinable()) {
                m_worker.join();
            }

            try {
                update_session_end();
            } catch (...) {
                ++m_failed_writes;
            }
        }

        /// \brief Returns this logger session id.
        uint64_t session_id() const {
            return m_session_id;
        }

        /// \brief Reads records in `[from_ms, to_ms)` ordered by timestamp and sequence.
        std::vector<MdbxLogRecordV1> read_range(
                int64_t from_ms,
                int64_t to_ms,
                std::size_t limit = 0) const {
            std::vector<MdbxLogRecordV1> out;
            if (to_ms <= from_ms) {
                return out;
            }

            try {
                const std::string from_key = detail::make_mdbx_record_key(from_ms, 0);
                const std::string to_key = detail::make_mdbx_record_key(
                    to_ms - 1,
                    (std::numeric_limits<uint32_t>::max)());

                std::lock_guard<std::mutex> db_lock(m_db_mutex);
                m_records->for_each_range(from_key, to_key,
                    [&out, limit](const std::string&, const MdbxLogRecordV1& record) -> bool {
                        out.push_back(record);
                        return limit == 0 || out.size() < limit;
                    });
            } catch (...) {
                out.clear();
            }

            return out;
        }

        /// \brief Reads a payload by id.
        bool read_payload(uint64_t payload_id, MdbxLogPayloadV1& out) const {
            if (payload_id == 0) {
                return false;
            }

            try {
                std::lock_guard<std::mutex> db_lock(m_db_mutex);
#if __cplusplus >= 201703L
                auto value = m_payloads->find(payload_id);
                if (!value) return false;
                out = *value;
                return true;
#else
                std::pair<bool, MdbxLogPayloadV1> value = m_payloads->find(payload_id);
                if (!value.first) return false;
                out = value.second;
                return true;
#endif
            } catch (...) {
                return false;
            }
        }

        /// \brief Reads session metadata by id.
        bool read_session(uint64_t session_id, MdbxLogSessionV1& out) const {
            if (session_id == 0) {
                return false;
            }

            try {
                std::lock_guard<std::mutex> db_lock(m_db_mutex);
#if __cplusplus >= 201703L
                auto value = m_sessions->find(session_id);
                if (!value) return false;
                out = *value;
                return true;
#else
                std::pair<bool, MdbxLogSessionV1> value = m_sessions->find(session_id);
                if (!value.first) return false;
                out = value.second;
                return true;
#endif
            } catch (...) {
                return false;
            }
        }

        std::string get_string_param(const LoggerParam& param) const override {
            switch (param) {
            case LoggerParam::LastLogTimestamp: return std::to_string(get_last_log_ts());
            case LoggerParam::TimeSinceLastLog: return std::to_string(get_time_since_last_log());
            case LoggerParam::DroppedLogCount: return std::to_string(dropped_count());
            case LoggerParam::FailedExportCount: return std::to_string(failed_export_count());
            default:
                break;
            }
            return std::string();
        }

        int64_t get_int_param(const LoggerParam& param) const override {
            switch (param) {
            case LoggerParam::LastLogTimestamp: return get_last_log_ts();
            case LoggerParam::TimeSinceLastLog: return get_time_since_last_log();
            case LoggerParam::DroppedLogCount: return counter_to_int64(dropped_count());
            case LoggerParam::FailedExportCount: return counter_to_int64(failed_export_count());
            default:
                break;
            }
            return 0;
        }

        double get_float_param(const LoggerParam& param) const override {
            switch (param) {
            case LoggerParam::LastLogTimestamp:
                return static_cast<double>(get_last_log_ts()) / 1000.0;
            case LoggerParam::TimeSinceLastLog:
                return static_cast<double>(get_time_since_last_log()) / 1000.0;
            case LoggerParam::DroppedLogCount:
                return static_cast<double>(dropped_count());
            case LoggerParam::FailedExportCount:
                return static_cast<double>(failed_export_count());
            default:
                break;
            }
            return 0.0;
        }

        void set_log_level(LogLevel level) override {
            m_log_level.store(static_cast<int>(level), std::memory_order_release);
        }

        LogLevel get_log_level() const override {
            return static_cast<LogLevel>(m_log_level.load(std::memory_order_acquire));
        }

        uint64_t dropped_count() const {
            return m_dropped.load(std::memory_order_acquire);
        }

        uint64_t failed_export_count() const {
            return m_failed_writes.load(std::memory_order_acquire);
        }

    private:
        struct MdbxLogItem {
            LogLevel level = LogLevel::LOG_LVL_TRACE;
            int64_t timestamp_ms = 0;
            std::string file;
            std::string function;
            int line = 0;
            std::string message;
        };

        typedef mdbxc::KeyValueTable<uint64_t, MdbxLogSessionV1> SessionTable;
        typedef mdbxc::KeyValueTable<std::string, MdbxLogRecordV1> RecordTable;
        typedef mdbxc::KeyValueTable<uint64_t, MdbxLogPayloadV1> PayloadTable;

        Config m_config;
        std::shared_ptr<mdbxc::Connection> m_connection;
        std::unique_ptr<SessionTable> m_sessions;
        std::unique_ptr<RecordTable> m_records;
        std::unique_ptr<PayloadTable> m_payloads;

        mutable std::mutex m_db_mutex;

        std::mutex m_mutex;
        std::condition_variable m_cv;
        std::condition_variable m_space_cv;
        std::deque<MdbxLogItem> m_queue;
        bool m_stopping = false;
        bool m_idle = true;
        std::thread m_worker;

        uint64_t m_session_id = 0;
        std::unordered_map<int64_t, uint32_t> m_next_sequence_by_timestamp;

        std::atomic<int> m_log_level = ATOMIC_VAR_INIT(static_cast<int>(LogLevel::LOG_LVL_TRACE));
        std::atomic<int64_t> m_last_log_ts = ATOMIC_VAR_INIT(0);
        std::atomic<int64_t> m_last_log_mono_ts = ATOMIC_VAR_INIT(0);
        std::atomic<uint64_t> m_dropped = ATOMIC_VAR_INIT(0);
        std::atomic<uint64_t> m_failed_writes = ATOMIC_VAR_INIT(0);
        std::atomic<bool> m_shutdown = ATOMIC_VAR_INIT(false);

        void normalize_config() {
            if (m_config.max_batch_size == 0) {
                m_config.max_batch_size = 1;
            }
            if (m_config.flush_interval_ms <= 0) {
                m_config.flush_interval_ms = 1;
            }
        }

        void validate_compression_config() const {
            if (m_config.payload_compression == MdbxPayloadCompression::Gzip) {
#if !defined(LOGIT_HAS_ZLIB)
                throw std::runtime_error("MdbxLogger: gzip payload compression requested but LOGIT_WITH_GZIP is not enabled");
#endif
            }
            if (m_config.payload_compression == MdbxPayloadCompression::Zstd) {
#if !defined(LOGIT_HAS_ZSTD)
                throw std::runtime_error("MdbxLogger: zstd payload compression requested but LOGIT_WITH_ZSTD is not enabled");
#endif
            }
        }

        void open_storage() {
            mdbxc::Config db_config;
            db_config.pathname = m_config.path;
            db_config.max_dbs = 4;
            db_config.no_subdir = true;
            db_config.sync_durable = true;

            m_connection = mdbxc::Connection::create(db_config);
            m_sessions.reset(new SessionTable(m_connection, "log_sessions"));
            m_records.reset(new RecordTable(m_connection, "log_records_by_time"));
            m_payloads.reset(new PayloadTable(m_connection, "log_payloads"));
        }

        uint64_t open_session() {
            std::lock_guard<std::mutex> db_lock(m_db_mutex);
            auto txn = m_connection->transaction(mdbxc::TransactionMode::WRITABLE);
            const int64_t now_ms = LOGIT_CURRENT_TIMESTAMP_MS();
            const uint64_t pid = detail::current_process_id();

            if (m_config.session_id != 0) {
                MdbxLogSessionV1 session;
                if (find_session_locked(m_config.session_id, session, txn.handle())) {
                    if (!m_config.app_name.empty()) {
                        session.app_name = m_config.app_name;
                    }
                    if (session.start_time_ms == 0) {
                        session.start_time_ms = now_ms;
                    }
                    session.end_time_ms = 0;
                    session.process_id = pid;
                    session.schema_version = 1;
                } else {
                    session.app_name = m_config.app_name;
                    session.start_time_ms = now_ms;
                    session.end_time_ms = 0;
                    session.process_id = pid;
                    session.schema_version = 1;
                }
                m_sessions->insert_or_assign(m_config.session_id, session, txn);
                txn.commit();
                return m_config.session_id;
            }

            for (int attempt = 0; attempt < 1024; ++attempt) {
                const uint64_t candidate = make_unique_id();
                MdbxLogSessionV1 session;
                session.app_name = m_config.app_name;
                session.start_time_ms = now_ms;
                session.end_time_ms = 0;
                session.process_id = pid;
                session.schema_version = 1;
                if (m_sessions->insert(candidate, session, txn)) {
                    txn.commit();
                    return candidate;
                }
            }

            throw std::runtime_error("MdbxLogger: failed to allocate unique session id");
        }

        bool find_session_locked(uint64_t session_id, MdbxLogSessionV1& out, MDBX_txn* txn) const {
#if __cplusplus >= 201703L
            auto value = m_sessions->find(session_id, txn);
            if (!value) return false;
            out = *value;
            return true;
#else
            std::pair<bool, MdbxLogSessionV1> value = m_sessions->find(session_id, txn);
            if (!value.first) return false;
            out = value.second;
            return true;
#endif
        }

        void update_session_end() {
            std::lock_guard<std::mutex> db_lock(m_db_mutex);
            auto txn = m_connection->transaction(mdbxc::TransactionMode::WRITABLE);
            MdbxLogSessionV1 session;
            if (!find_session_locked(m_session_id, session, txn.handle())) {
                session.app_name = m_config.app_name;
                session.start_time_ms = LOGIT_CURRENT_TIMESTAMP_MS();
                session.process_id = detail::current_process_id();
                session.schema_version = 1;
            }
            session.end_time_ms = LOGIT_CURRENT_TIMESTAMP_MS();
            m_sessions->insert_or_assign(m_session_id, session, txn);
            txn.commit();
        }

        void worker_loop() {
            while (true) {
                std::vector<MdbxLogItem> batch;
                batch.reserve(m_config.max_batch_size);

                {
                    std::unique_lock<std::mutex> lock(m_mutex);
                    m_cv.wait_for(
                        lock,
                        std::chrono::milliseconds(m_config.flush_interval_ms),
                        [this]() { return m_stopping || !m_queue.empty(); });

                    if (m_stopping && m_queue.empty()) {
                        m_idle = true;
                        m_cv.notify_all();
                        return;
                    }

                    if (m_queue.empty()) {
                        continue;
                    }

                    m_idle = false;
                    while (!m_queue.empty() && batch.size() < m_config.max_batch_size) {
                        batch.push_back(std::move(m_queue.front()));
                        m_queue.pop_front();
                    }
                    m_space_cv.notify_all();
                }

                write_batch(batch);

                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    m_idle = true;
                }
                m_cv.notify_all();
            }
        }

        void write_batch(const std::vector<MdbxLogItem>& batch) {
            if (batch.empty()) {
                return;
            }

            try {
                std::lock_guard<std::mutex> db_lock(m_db_mutex);
                auto txn = m_connection->transaction(mdbxc::TransactionMode::WRITABLE);
                for (size_t i = 0; i < batch.size(); ++i) {
                    write_item_locked(batch[i], txn);
                }
                txn.commit();
            } catch (const std::exception& e) {
                m_failed_writes.fetch_add(1, std::memory_order_acq_rel);
                std::cerr << "MdbxLogger write error: " << e.what() << std::endl;
            } catch (...) {
                m_failed_writes.fetch_add(1, std::memory_order_acq_rel);
                std::cerr << "MdbxLogger write error" << std::endl;
            }
        }

        void write_item_locked(const MdbxLogItem& item, mdbxc::Transaction& txn) {
            MdbxLogRecordV1 record;
            record.session_id = m_session_id;
            record.timestamp_ms = item.timestamp_ms;
            record.level = item.level;
            record.file = item.file;
            record.function = item.function;
            record.line = item.line;
            record.message = item.message;

            if (should_spill_payload(item.message)) {
                MdbxLogPayloadV1 payload;
                payload.payload_id = allocate_payload_id_locked(txn);
                fill_payload(item.message, payload);
                m_payloads->insert(payload.payload_id, payload, txn);
                record.payload_id = payload.payload_id;
                record.message = make_payload_preview(item.message);
            }

            for (;;) {
                const std::string key = next_record_key_locked(record.timestamp_ms, record.sequence, txn);
                if (m_records->insert(key, record, txn)) {
                    return;
                }
                advance_sequence_after_collision(record.timestamp_ms, record.sequence);
            }
        }

        bool should_spill_payload(const std::string& message) const {
            return m_config.store_large_payloads_separately &&
                   message.size() > m_config.large_payload_threshold;
        }

        std::string make_payload_preview(const std::string& message) const {
            const size_t count = std::min(message.size(), m_config.payload_preview_size);
            return message.substr(0, count);
        }

        void fill_payload(const std::string& message, MdbxLogPayloadV1& payload) {
            payload.compression = MdbxPayloadCompression::None;
            payload.data = message;

            if (m_config.payload_compression == MdbxPayloadCompression::None) {
                return;
            }

            std::string compressed;
            bool ok = false;
            if (m_config.payload_compression == MdbxPayloadCompression::Gzip) {
                ok = compress_string_gzip(message, compressed, m_config.payload_compression_level);
            } else if (m_config.payload_compression == MdbxPayloadCompression::Zstd) {
                ok = compress_string_zstd(message, compressed, m_config.payload_compression_level);
            }

            if (ok) {
                payload.compression = m_config.payload_compression;
                payload.data = std::move(compressed);
            } else {
                m_failed_writes.fetch_add(1, std::memory_order_acq_rel);
            }
        }

        uint64_t allocate_payload_id_locked(mdbxc::Transaction& txn) {
            for (int attempt = 0; attempt < 1024; ++attempt) {
                const uint64_t candidate = make_unique_id();
                if (!m_payloads->contains(candidate, txn)) {
                    return candidate;
                }
            }
            throw std::runtime_error("MdbxLogger: failed to allocate unique payload id");
        }

        std::string next_record_key_locked(
                int64_t timestamp_ms,
                uint32_t& sequence,
                mdbxc::Transaction& txn) {
            std::unordered_map<int64_t, uint32_t>::iterator it =
                m_next_sequence_by_timestamp.find(timestamp_ms);
            uint32_t candidate = it == m_next_sequence_by_timestamp.end() ? 0 : it->second;

            for (;;) {
                const std::string key = detail::make_mdbx_record_key(timestamp_ms, candidate);
                if (!m_records->contains(key, txn)) {
                    sequence = candidate;
                    if (candidate != (std::numeric_limits<uint32_t>::max)()) {
                        m_next_sequence_by_timestamp[timestamp_ms] = candidate + 1;
                    } else {
                        m_next_sequence_by_timestamp[timestamp_ms] = candidate;
                    }
                    return key;
                }
                if (candidate == (std::numeric_limits<uint32_t>::max)()) {
                    throw std::runtime_error("MdbxLogger: sequence exhausted for timestamp");
                }
                ++candidate;
            }
        }

        void advance_sequence_after_collision(int64_t timestamp_ms, uint32_t sequence) {
            if (sequence == (std::numeric_limits<uint32_t>::max)()) {
                throw std::runtime_error("MdbxLogger: sequence exhausted for timestamp");
            }
            m_next_sequence_by_timestamp[timestamp_ms] = sequence + 1;
        }

        static uint64_t make_unique_id() {
            static std::atomic<uint64_t> counter(0);
            const uint64_t now = static_cast<uint64_t>(LOGIT_CURRENT_TIMESTAMP_MS());
            const uint64_t n = counter.fetch_add(1, std::memory_order_relaxed) & 0xFFFFu;
            const uint64_t id = (now << 16) ^ (detail::current_process_id() << 1) ^ n;
            return id == 0 ? (n + 1) : id;
        }

        void mark_last_log(int64_t timestamp_ms) {
            m_last_log_ts.store(timestamp_ms, std::memory_order_release);
            m_last_log_mono_ts.store(LOGIT_MONOTONIC_MS(), std::memory_order_release);
        }

        int64_t get_last_log_ts() const {
            return m_last_log_ts.load(std::memory_order_acquire);
        }

        int64_t get_time_since_last_log() const {
            const int64_t last_mono = m_last_log_mono_ts.load(std::memory_order_acquire);
            if (last_mono <= 0) {
                return 0;
            }
            const int64_t now = LOGIT_MONOTONIC_MS();
            return now > last_mono ? now - last_mono : 0;
        }

        static int64_t counter_to_int64(uint64_t value) {
            const uint64_t max_value = static_cast<uint64_t>((std::numeric_limits<int64_t>::max)());
            return value > max_value ? (std::numeric_limits<int64_t>::max)() : static_cast<int64_t>(value);
        }
    };

} // namespace logit

#endif // _LOGIT_MDBX_LOGGER_HPP_INCLUDED
