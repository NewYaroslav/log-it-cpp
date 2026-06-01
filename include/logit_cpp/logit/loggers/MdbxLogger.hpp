#pragma once
#ifndef _LOGIT_MDBX_LOGGER_HPP_INCLUDED
#define _LOGIT_MDBX_LOGGER_HPP_INCLUDED

/// \file MdbxLogger.hpp
/// \brief MDBX structured log storage backend.

#include <mdbx_containers/KeyValueTable.hpp>
#include <algorithm>
#include <cstring>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace logit {

    /// \enum MdbxPayloadCompression
    /// \brief Compression algorithm used for payload rows.
    enum class MdbxPayloadCompression {
        None = 0, ///< Store payload bytes as-is.
        Gzip = 1, ///< Store gzip-compressed payload bytes.
        Zstd = 2  ///< Store zstd-compressed payload bytes.
    };

    namespace detail {
        class MdbxReadException : public std::runtime_error {
        public:
            MdbxReadException(LogReadError error, const std::string& message)
                : std::runtime_error(message), m_error(error) {}

            LogReadError error() const noexcept {
                return m_error;
            }

        private:
            LogReadError m_error;
        };
    }

    /// \class MdbxLogger
    /// \brief Stores formatted logs in MDBX tables with optional async batching.
    class MdbxLogger final : public ILogger, public ILogReader, public ILogSubscriber {
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
            std::function<void(const std::string&)> on_error; ///< Optional callback invoked on initialization and write errors instead of stderr.
        };

        /// \struct SessionView
        /// \brief Public read-only view of session metadata.
        struct SessionView {
            std::string app_name;       ///< Optional application name.
            int64_t start_time_ms = 0;  ///< Session start timestamp.
            int64_t end_time_ms = 0;    ///< Session end timestamp, or 0 while active.
            uint64_t process_id = 0;    ///< Process id that opened the session.
            uint32_t schema_version = 1;///< Storage schema version.
        };

        /// \struct PayloadView
        /// \brief Public read-only view of a large payload.
        struct PayloadView {
            uint64_t payload_id = 0; ///< Stable payload id.
            MdbxPayloadCompression compression = MdbxPayloadCompression::None; ///< Stored compression.
            std::string data;       ///< Stored payload bytes.
        };

        MdbxLogger() : MdbxLogger(Config()) {}

        explicit MdbxLogger(const Config& config)
            : m_config(config) {
            try {
                normalize_config();
                validate_compression_config();
                open_storage();
                m_session_id = open_session();
                if (m_config.async) {
                    m_worker = std::thread(&MdbxLogger::worker_loop, this);
                }
            } catch (const std::exception& e) {
                report_init_error(std::string("MdbxLogger initialization error: ") + e.what());
                throw;
            } catch (...) {
                report_init_error("MdbxLogger initialization error");
                throw;
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
        std::vector<LogRecordView> read_range(
                int64_t from_ms,
                int64_t to_ms,
                std::size_t limit = 0) const override {
            auto result = read_range_result(from_ms, to_ms, limit);
            return result.value ? *result.value : std::vector<LogRecordView>();
        }

        /// \brief Reads records and preserves storage/decode errors.
        LogReadResult<std::vector<LogRecordView>> read_range_result(
                int64_t from_ms,
                int64_t to_ms,
                std::size_t limit = 0) const override {
            LogReadResult<std::vector<LogRecordView>> result;
            result.value.emplace();
            if (to_ms <= from_ms) {
                return result;
            }

            try {
                const std::string from_key = detail::make_mdbx_record_key(from_ms, 0);
                const std::string to_key = detail::make_mdbx_record_key(
                    to_ms - 1,
                    (std::numeric_limits<uint32_t>::max)());

                std::lock_guard<std::mutex> db_lock(m_db_mutex);
                m_records->for_each_range(from_key, to_key,
                    [&result, limit](const std::string&, const Record& record) -> bool {
                        result.value->push_back(to_view(record));
                        return limit == 0 || result.value->size() < limit;
                    });
            } catch (const detail::MdbxReadException& e) {
                result.value.reset();
                result.error = e.error();
                result.message = e.what();
            } catch (const mdbxc::MdbxException& e) {
                result.value.reset();
                result.error = LogReadError::StorageError;
                result.message = e.what();
            } catch (const std::exception& e) {
                result.value.reset();
                result.error = LogReadError::DecodeError;
                result.message = e.what();
            } catch (...) {
                result.value.reset();
                result.error = LogReadError::DecodeError;
                result.message = "MdbxLogger: unknown read_range error";
            }

            return result;
        }

        /// \brief Reads the most recent records.
        /// \param limit     Maximum number of records (0 = unlimited).
        /// \param period_ms Time window in milliseconds from now backward (0 = unlimited).
        /// \param order     Ascending or descending result order.
        /// \return Matching records in the requested order.
        std::vector<LogRecordView> read_recent(
                std::size_t limit,
                int64_t period_ms = 0,
                LogReadOrder order = LogReadOrder::Ascending) const override {
            auto result = read_recent_result(limit, period_ms, order);
            return result.value ? *result.value : std::vector<LogRecordView>();
        }

        /// \brief Reads the most recent records and preserves storage/decode errors.
        LogReadResult<std::vector<LogRecordView>> read_recent_result(
                std::size_t limit,
                int64_t period_ms = 0,
                LogReadOrder order = LogReadOrder::Ascending) const override {
            const int64_t now_ms = LOGIT_CURRENT_TIMESTAMP_MS();
            const int64_t from_ms = (period_ms > 0) ? (now_ms - period_ms) : 0;
            auto result = read_range_result(from_ms, now_ms + 1, 0);
            if (!result.value) {
                return result;
            }
            if (limit > 0 && result.value->size() > limit) {
                result.value->erase(
                    result.value->begin(),
                    result.value->begin() + static_cast<std::ptrdiff_t>(result.value->size() - limit));
            }
            if (order == LogReadOrder::Descending && !result.value->empty()) {
                std::reverse(result.value->begin(), result.value->end());
            }
            return result;
        }

        /// \brief Reads a payload by id.
        std::optional<PayloadView> read_payload(uint64_t payload_id) const {
            return read_payload_result(payload_id).value;
        }

        /// \brief Reads a payload by id and preserves storage/decode errors.
        LogReadResult<PayloadView> read_payload_result(uint64_t payload_id) const {
            LogReadResult<PayloadView> result;
            if (payload_id == 0) {
                result.error = LogReadError::NotFound;
                result.message = "MdbxLogger: payload id is zero";
                return result;
            }

            try {
                std::lock_guard<std::mutex> db_lock(m_db_mutex);
#if __cplusplus >= 201703L
                auto value = m_payloads->find(payload_id);
                if (!value) return make_not_found_result<PayloadView>("MdbxLogger: payload not found");
                result.value = to_view(*value);
#else
                std::pair<bool, Payload> value = m_payloads->find(payload_id);
                if (!value.first) return make_not_found_result<PayloadView>("MdbxLogger: payload not found");
                result.value = to_view(value.second);
#endif
            } catch (const detail::MdbxReadException& e) {
                result.error = e.error();
                result.message = e.what();
            } catch (const mdbxc::MdbxException& e) {
                result.error = LogReadError::StorageError;
                result.message = e.what();
            } catch (const std::exception& e) {
                result.error = LogReadError::DecodeError;
                result.message = e.what();
            } catch (...) {
                result.error = LogReadError::DecodeError;
                result.message = "MdbxLogger: unknown payload read error";
            }
            return result;
        }

        /// \brief Reads and decompresses payload data by id.
        /// \return Original (decompressed) payload string, or std::nullopt if not found or decompression fails.
        std::optional<std::string> read_payload_data(uint64_t payload_id) const {
            return read_payload_data_result(payload_id).value;
        }

        /// \brief Reads and decompresses payload data while preserving failure details.
        LogReadResult<std::string> read_payload_data_result(uint64_t payload_id) const {
            LogReadResult<std::string> result;
            auto view = read_payload_result(payload_id);
            if (!view.value) {
                result.error = view.error;
                result.message = view.message;
                return result;
            }
            if (view.value->compression == MdbxPayloadCompression::None) {
                result.value = view.value->data;
                return result;
            }
            std::string output;
            bool ok = false;
            if (view.value->compression == MdbxPayloadCompression::Gzip) {
                ok = detail::decompress_string_gzip(view.value->data, output);
            } else if (view.value->compression == MdbxPayloadCompression::Zstd) {
                ok = detail::decompress_string_zstd(view.value->data, output);
            }
            if (ok) {
                result.value = std::move(output);
            } else {
                result.error = LogReadError::DecompressionError;
                result.message = "MdbxLogger: failed to decompress payload data";
            }
            return result;
        }

        /// \brief Reads session metadata by id.
        std::optional<SessionView> read_session(uint64_t session_id) const {
            return read_session_result(session_id).value;
        }

        /// \brief Reads session metadata by id and preserves storage/decode errors.
        LogReadResult<SessionView> read_session_result(uint64_t session_id) const {
            LogReadResult<SessionView> result;
            if (session_id == 0) {
                result.error = LogReadError::NotFound;
                result.message = "MdbxLogger: session id is zero";
                return result;
            }

            try {
                std::lock_guard<std::mutex> db_lock(m_db_mutex);
#if __cplusplus >= 201703L
                auto value = m_sessions->find(session_id);
                if (!value) return make_not_found_result<SessionView>("MdbxLogger: session not found");
                result.value = to_view(*value);
#else
                std::pair<bool, Session> value = m_sessions->find(session_id);
                if (!value.first) return make_not_found_result<SessionView>("MdbxLogger: session not found");
                result.value = to_view(value.second);
#endif
            } catch (const detail::MdbxReadException& e) {
                result.error = e.error();
                result.message = e.what();
            } catch (const mdbxc::MdbxException& e) {
                result.error = LogReadError::StorageError;
                result.message = e.what();
            } catch (const std::exception& e) {
                result.error = LogReadError::DecodeError;
                result.message = e.what();
            } catch (...) {
                result.error = LogReadError::DecodeError;
                result.message = "MdbxLogger: unknown session read error";
            }
            return result;
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

        LogClearResult clear_logs(const LogClearOptions& options = LogClearOptions()) override {
            wait();

            LogClearResult result;
            try {
                {
                    std::lock_guard<std::mutex> db_lock(m_db_mutex);
                    auto txn = m_connection->transaction(mdbxc::TransactionMode::WRITABLE);
                    if (options.include_persistent_records) {
                        result.cleared_records = m_records->count(txn);
                        m_records->clear(txn);
                    }
                    if (options.include_payloads) {
                        m_payloads->clear(txn);
                    }
                    if (options.include_sessions) {
                        m_sessions->clear(txn);
                    }
                    txn.commit();
                }

                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    m_next_sequence_by_timestamp.clear();
                }
                m_last_log_ts.store(0, std::memory_order_release);
                m_last_log_mono_ts.store(0, std::memory_order_release);

                result.ok = true;
                result.message = "cleared";
            } catch (const std::exception& e) {
                result.ok = false;
                result.message = std::string("MdbxLogger clear error: ") + e.what();
            } catch (...) {
                result.ok = false;
                result.message = "MdbxLogger clear error";
            }
            return result;
        }

        uint64_t dropped_count() const {
            return m_dropped.load(std::memory_order_acquire);
        }

        uint64_t failed_export_count() const {
            return m_failed_writes.load(std::memory_order_acquire);
        }

        uint64_t add_log_callback(Callback callback) override {
            std::lock_guard<std::mutex> lock(m_callbacks_mutex);
            const uint64_t id = m_next_callback_id.fetch_add(1, std::memory_order_relaxed);
            m_callbacks.emplace(id, std::move(callback));
            return id;
        }

        bool remove_log_callback(uint64_t callback_id) override {
            std::lock_guard<std::mutex> lock(m_callbacks_mutex);
            return m_callbacks.erase(callback_id) > 0;
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

        struct Session {
            std::string app_name;
            int64_t start_time_ms = 0;
            int64_t end_time_ms = 0;
            uint64_t process_id = 0;
            uint32_t schema_version = 1;

            std::vector<uint8_t> to_bytes() const;
            static Session from_bytes(const void* data, size_t size);
        };

        struct Record {
            uint64_t session_id = 0;
            int64_t timestamp_ms = 0;
            uint32_t sequence = 0;
            LogLevel level = LogLevel::LOG_LVL_TRACE;
            std::string message;
            uint64_t payload_id = 0;
            std::string file;
            std::string function;
            int line = 0;

            std::vector<uint8_t> to_bytes() const;
            static Record from_bytes(const void* data, size_t size);
        };

        struct Payload {
            uint64_t payload_id = 0;
            MdbxPayloadCompression compression = MdbxPayloadCompression::None;
            std::string data;

            std::vector<uint8_t> to_bytes() const;
            static Payload from_bytes(const void* data, size_t size);
        };

        typedef mdbxc::KeyValueTable<uint64_t, Session> SessionTable;
        typedef mdbxc::KeyValueTable<std::string, Record> RecordTable;
        typedef mdbxc::KeyValueTable<uint64_t, Payload> PayloadTable;

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

        mutable std::mutex m_callbacks_mutex;
        std::unordered_map<uint64_t, Callback> m_callbacks;
        std::atomic<uint64_t> m_next_callback_id{1};

        template <typename T>
        static LogReadResult<T> make_not_found_result(const std::string& message) {
            LogReadResult<T> result;
            result.error = LogReadError::NotFound;
            result.message = message;
            return result;
        }

        void notify_callbacks(const std::vector<LogRecordView>& views) const {
            std::vector<Callback> callbacks_copy;
            {
                std::lock_guard<std::mutex> lock(m_callbacks_mutex);
                callbacks_copy.reserve(m_callbacks.size());
                for (const auto& kv : m_callbacks) {
                    callbacks_copy.push_back(kv.second);
                }
            }
            for (const auto& view : views) {
                for (const auto& cb : callbacks_copy) {
                    try {
                        cb(view);
                    } catch (const std::exception& e) {
                        if (m_config.on_error) {
                            m_config.on_error(std::string("MdbxLogger callback error: ") + e.what());
                        }
                    } catch (...) {
                        if (m_config.on_error) {
                            m_config.on_error("MdbxLogger callback error");
                        }
                    }
                }
            }
        }

        static LogRecordView to_view(const Record& r) {
            LogRecordView v;
            v.session_id = r.session_id;
            v.timestamp_ms = r.timestamp_ms;
            v.sequence = r.sequence;
            v.level = r.level;
            v.message = r.message;
            v.payload_id = r.payload_id;
            v.file = r.file;
            v.function = r.function;
            v.line = r.line;
            return v;
        }

        static SessionView to_view(const Session& s) {
            SessionView v;
            v.app_name = s.app_name;
            v.start_time_ms = s.start_time_ms;
            v.end_time_ms = s.end_time_ms;
            v.process_id = s.process_id;
            v.schema_version = s.schema_version;
            return v;
        }

        static PayloadView to_view(const Payload& p) {
            PayloadView v;
            v.payload_id = p.payload_id;
            v.compression = p.compression;
            v.data = p.data;
            return v;
        }

        static std::vector<uint8_t> serialize_session(const Session& s) {
            detail::MdbxByteWriter out;
            out.write_u32(1);
            out.write_string(s.app_name);
            out.write_i64(s.start_time_ms);
            out.write_i64(s.end_time_ms);
            out.write_u64(s.process_id);
            out.write_u32(s.schema_version);
            return out.bytes();
        }

        static Session deserialize_session(const void* data, size_t size) {
            detail::MdbxByteReader in(data, size);
            const uint32_t version = in.read_u32();
            if (version != 1) {
                throw detail::MdbxReadException(
                    LogReadError::UnsupportedVersion,
                    "MdbxLogger: unsupported session value version");
            }
            Session s;
            s.app_name = in.read_string();
            s.start_time_ms = in.read_i64();
            s.end_time_ms = in.read_i64();
            s.process_id = in.read_u64();
            s.schema_version = in.read_u32();
            in.finish();
            return s;
        }

        static std::vector<uint8_t> serialize_record(const Record& r) {
            detail::MdbxByteWriter out;
            out.write_u32(1);
            out.write_u64(r.session_id);
            out.write_i64(r.timestamp_ms);
            out.write_u32(r.sequence);
            out.write_u32(static_cast<uint32_t>(r.level));
            out.write_string(r.message);
            out.write_u64(r.payload_id);
            out.write_string(r.file);
            out.write_string(r.function);
            out.write_i64(static_cast<int64_t>(r.line));
            return out.bytes();
        }

        static Record deserialize_record(const void* data, size_t size) {
            detail::MdbxByteReader in(data, size);
            const uint32_t version = in.read_u32();
            if (version != 1) {
                throw detail::MdbxReadException(
                    LogReadError::UnsupportedVersion,
                    "MdbxLogger: unsupported record value version");
            }
            Record r;
            r.session_id = in.read_u64();
            r.timestamp_ms = in.read_i64();
            r.sequence = in.read_u32();
            r.level = static_cast<LogLevel>(in.read_u32());
            r.message = in.read_string();
            r.payload_id = in.read_u64();
            r.file = in.read_string();
            r.function = in.read_string();
            r.line = static_cast<int>(in.read_i64());
            in.finish();
            return r;
        }

        static std::vector<uint8_t> serialize_payload(const Payload& p) {
            detail::MdbxByteWriter out;
            out.write_u32(1);
            out.write_u64(p.payload_id);
            out.write_u8(static_cast<uint8_t>(p.compression));
            out.write_string(p.data);
            return out.bytes();
        }

        static Payload deserialize_payload(const void* data, size_t size) {
            detail::MdbxByteReader in(data, size);
            const uint32_t version = in.read_u32();
            if (version != 1) {
                throw detail::MdbxReadException(
                    LogReadError::UnsupportedVersion,
                    "MdbxLogger: unsupported payload value version");
            }
            Payload p;
            p.payload_id = in.read_u64();
            const uint8_t compression = in.read_u8();
            if (compression > static_cast<uint8_t>(MdbxPayloadCompression::Zstd)) {
                throw detail::MdbxReadException(
                    LogReadError::DecodeError,
                    "MdbxLogger: unsupported payload compression");
            }
            p.compression = static_cast<MdbxPayloadCompression>(compression);
            p.data = in.read_string();
            in.finish();
            return p;
        }

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

            ensure_storage_parent(db_config);
            m_connection = mdbxc::Connection::create(db_config);
            m_sessions.reset(new SessionTable(m_connection, "log_sessions"));
            m_records.reset(new RecordTable(m_connection, "log_records_by_time"));
            m_payloads.reset(new PayloadTable(m_connection, "log_payloads"));
        }

        void ensure_storage_parent(const mdbxc::Config& db_config) const {
            if (!db_config.read_only && db_config.no_subdir) {
                mdbxc::create_directories(db_config.pathname);
            }
        }

        void report_init_error(const std::string& message) const noexcept {
            if (m_config.on_error) {
                try {
                    m_config.on_error(message);
                } catch (...) {
                }
            }
        }

        uint64_t open_session() {
            std::lock_guard<std::mutex> db_lock(m_db_mutex);
            auto txn = m_connection->transaction(mdbxc::TransactionMode::WRITABLE);
            const int64_t now_ms = LOGIT_CURRENT_TIMESTAMP_MS();
            const uint64_t pid = detail::current_process_id();

            if (m_config.session_id != 0) {
                Session session;
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
                Session session;
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

        bool find_session_locked(uint64_t session_id, Session& out, MDBX_txn* txn) const {
#if __cplusplus >= 201703L
            auto value = m_sessions->find(session_id, txn);
            if (!value) return false;
            out = *value;
            return true;
#else
            std::pair<bool, Session> value = m_sessions->find(session_id, txn);
            if (!value.first) return false;
            out = value.second;
            return true;
#endif
        }

        void update_session_end() {
            std::lock_guard<std::mutex> db_lock(m_db_mutex);
            auto txn = m_connection->transaction(mdbxc::TransactionMode::WRITABLE);
            Session session;
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

            std::vector<LogRecordView> written_views;
            try {
                std::lock_guard<std::mutex> db_lock(m_db_mutex);
                auto txn = m_connection->transaction(mdbxc::TransactionMode::WRITABLE);
                for (size_t i = 0; i < batch.size(); ++i) {
                    Record record = write_item_locked(batch[i], txn);
                    written_views.push_back(to_view(record));
                }
                txn.commit();
            } catch (const std::exception& e) {
                m_failed_writes.fetch_add(1, std::memory_order_acq_rel);
                if (m_config.on_error) {
                    m_config.on_error(std::string("MdbxLogger write error: ") + e.what());
                }
                return;
            } catch (...) {
                m_failed_writes.fetch_add(1, std::memory_order_acq_rel);
                if (m_config.on_error) {
                    m_config.on_error("MdbxLogger write error");
                }
                return;
            }

            notify_callbacks(written_views);
        }

        Record write_item_locked(const MdbxLogItem& item, mdbxc::Transaction& txn) {
            Record record;
            record.session_id = m_session_id;
            record.timestamp_ms = item.timestamp_ms;
            record.level = item.level;
            record.file = item.file;
            record.function = item.function;
            record.line = item.line;
            record.message = item.message;

            if (should_spill_payload(item.message)) {
                Payload payload;
                payload.payload_id = allocate_payload_id_locked(txn);
                fill_payload(item.message, payload);
                m_payloads->insert(payload.payload_id, payload, txn);
                record.payload_id = payload.payload_id;
                record.message = make_payload_preview(item.message);
            }

            for (;;) {
                const std::string key = next_record_key_locked(record.timestamp_ms, record.sequence, txn);
                if (m_records->insert(key, record, txn)) {
                    break;
                }
                advance_sequence_after_collision(record.timestamp_ms, record.sequence);
            }
            return record;
        }

        bool should_spill_payload(const std::string& message) const {
            return m_config.store_large_payloads_separately &&
                   message.size() > m_config.large_payload_threshold;
        }

        std::string make_payload_preview(const std::string& message) const {
            const size_t count = std::min(message.size(), m_config.payload_preview_size);
            return message.substr(0, count);
        }

        void fill_payload(const std::string& message, Payload& payload) {
            payload.compression = MdbxPayloadCompression::None;
            payload.data = message;

            if (m_config.payload_compression == MdbxPayloadCompression::None) {
                return;
            }

            std::string compressed;
            bool ok = false;
            if (m_config.payload_compression == MdbxPayloadCompression::Gzip) {
                ok = detail::compress_string_gzip(message, compressed, m_config.payload_compression_level);
            } else if (m_config.payload_compression == MdbxPayloadCompression::Zstd) {
                ok = detail::compress_string_zstd(message, compressed, m_config.payload_compression_level);
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

    inline std::vector<uint8_t> MdbxLogger::Session::to_bytes() const {
        return MdbxLogger::serialize_session(*this);
    }

    inline MdbxLogger::Session MdbxLogger::Session::from_bytes(const void* data, size_t size) {
        return MdbxLogger::deserialize_session(data, size);
    }

    inline std::vector<uint8_t> MdbxLogger::Record::to_bytes() const {
        return MdbxLogger::serialize_record(*this);
    }

    inline MdbxLogger::Record MdbxLogger::Record::from_bytes(const void* data, size_t size) {
        return MdbxLogger::deserialize_record(data, size);
    }

    inline std::vector<uint8_t> MdbxLogger::Payload::to_bytes() const {
        return MdbxLogger::serialize_payload(*this);
    }

    inline MdbxLogger::Payload MdbxLogger::Payload::from_bytes(const void* data, size_t size) {
        return MdbxLogger::deserialize_payload(data, size);
    }

} // namespace logit

#endif // _LOGIT_MDBX_LOGGER_HPP_INCLUDED
