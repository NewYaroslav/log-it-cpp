#pragma once
#ifndef _LOGIT_MEMORY_LOGGER_HPP_INCLUDED
#define _LOGIT_MEMORY_LOGGER_HPP_INCLUDED

/// \file MemoryLogger.hpp
/// \brief In-memory logger backend that stores recent log snapshots.

#include "ILogger.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <vector>

namespace logit {

    /// \class MemoryLogger
    /// \ingroup LogBackends
    /// \brief Stores the latest formatted log messages in memory.
    /// \details Snapshots are returned oldest-to-newest. Read operations avoid
    /// `Logger`-level execution serialization, but still synchronize on this
    /// backend's own mutex while copying the current buffer.
    class MemoryLogger : public ILogger {
    public:
        /// \struct Config
        /// \brief Retention limits for the in-memory buffer.
        struct Config {
            std::size_t max_records = 1000;            ///< Maximum number of buffered entries (0 = unlimited).
            std::size_t max_bytes   = 1024 * 1024;    ///< Maximum buffered formatted-message bytes (0 = unlimited).
            int64_t     max_age_ms  = 24LL * 60 * 60 * 1000; ///< Maximum age of buffered entries (0 = unlimited).
        };

        /// \brief Construct with default retention settings.
        MemoryLogger() = default;

        /// \brief Construct with explicit configuration.
        explicit MemoryLogger(const Config& config) : m_config(config) {}

        /// \brief Construct with explicit count/size/age limits.
        MemoryLogger(std::size_t max_records, std::size_t max_bytes, int64_t max_age_ms) :
                m_config{max_records, max_bytes, max_age_ms} {}

        /// \brief Store a formatted message in the buffer and evict old entries.
        void log(const LogRecord& record, const std::string& message) override {
            if (static_cast<int>(record.log_level) < m_log_level.load(std::memory_order_relaxed)) {
                return;
            }

            BufferedLogEntry entry;
            entry.level = record.log_level;
            entry.timestamp_ms = record.timestamp_ms;
            entry.file = record.file;
            entry.line = record.line;
            entry.function = record.function;
            entry.message = message;

            std::lock_guard<std::mutex> lock(m_mutex);
            m_evict_expired_locked(record.timestamp_ms);

            m_total_bytes += m_entry_bytes(entry);
            m_entries.push_back(std::move(entry));
            m_last_log_ts.store(record.timestamp_ms, std::memory_order_relaxed);
            m_last_log_mono_ts.store(LOGIT_MONOTONIC_MS(), std::memory_order_relaxed);

            m_enforce_limits_locked(record.timestamp_ms);
        }

        /// \brief Retrieve legacy string-based metadata.
        std::string get_string_param(const LoggerParam& param) const override {
            switch (param) {
            case LoggerParam::LastLogTimestamp:
                return std::to_string(get_last_log_ts());
            case LoggerParam::TimeSinceLastLog:
                return std::to_string(get_time_since_last_log());
            default:
                break;
            }
            return std::string();
        }

        /// \brief Retrieve legacy integer-based metadata.
        int64_t get_int_param(const LoggerParam& param) const override {
            switch (param) {
            case LoggerParam::LastLogTimestamp:
                return get_last_log_ts();
            case LoggerParam::TimeSinceLastLog:
                return get_time_since_last_log();
            default:
                break;
            }
            return 0;
        }

        /// \brief Retrieve legacy floating-point metadata.
        double get_float_param(const LoggerParam& param) const override {
            switch (param) {
            case LoggerParam::LastLogTimestamp:
                return static_cast<double>(get_last_log_ts()) / 1000.0;
            case LoggerParam::TimeSinceLastLog:
                return static_cast<double>(get_time_since_last_log()) / 1000.0;
            default:
                break;
            }
            return 0.0;
        }

        /// \brief Set the minimum accepted log level.
        void set_log_level(LogLevel level) override {
            m_log_level.store(static_cast<int>(level), std::memory_order_relaxed);
        }

        /// \brief Get the minimum accepted log level.
        LogLevel get_log_level() const override {
            return static_cast<LogLevel>(m_log_level.load(std::memory_order_relaxed));
        }

        /// \brief Return buffered formatted strings in chronological order.
        /// \details Age-based cleanup runs before copying the snapshot.
        std::vector<std::string> get_buffered_strings() const override {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_evict_expired_locked(LOGIT_CURRENT_TIMESTAMP_MS());

            std::vector<std::string> snapshot;
            snapshot.reserve(m_entries.size());
            for (const auto& entry : m_entries) {
                snapshot.push_back(entry.message);
            }
            return snapshot;
        }

        /// \brief Return buffered structured entries in chronological order.
        /// \details Age-based cleanup runs before copying the snapshot.
        std::vector<BufferedLogEntry> get_buffered_entries() const override {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_evict_expired_locked(LOGIT_CURRENT_TIMESTAMP_MS());
            return std::vector<BufferedLogEntry>(m_entries.begin(), m_entries.end());
        }

        /// \brief Memory logger is synchronous, so no flush step is needed.
        void wait() override {}

    private:
        // Count only the retained formatted payload, not the full object footprint.
        static std::size_t m_entry_bytes(const BufferedLogEntry& entry) {
            return entry.message.size();
        }

        void m_pop_front_locked() const {
            if (m_entries.empty()) {
                return;
            }
            m_total_bytes -= m_entry_bytes(m_entries.front());
            m_entries.pop_front();
        }

        void m_evict_expired_locked(int64_t now_ms) const {
            if (m_config.max_age_ms <= 0) {
                return;
            }

            const int64_t cutoff = now_ms - m_config.max_age_ms;
            while (!m_entries.empty() && m_entries.front().timestamp_ms < cutoff) {
                m_pop_front_locked();
            }
        }

        void m_enforce_limits_locked(int64_t now_ms) const {
            m_evict_expired_locked(now_ms);

            if (m_config.max_records > 0) {
                while (m_entries.size() > m_config.max_records) {
                    m_pop_front_locked();
                }
            }

            if (m_config.max_bytes > 0) {
                // If a single message exceeds the byte budget, eviction removes it too.
                while (m_total_bytes > m_config.max_bytes && !m_entries.empty()) {
                    m_pop_front_locked();
                }
            }
        }

        int64_t get_last_log_ts() const {
            return m_last_log_ts.load(std::memory_order_relaxed);
        }

        int64_t get_time_since_last_log() const {
            const int64_t last = m_last_log_mono_ts.load(std::memory_order_relaxed);
            if (last <= 0) {
                return 0;
            }
            const int64_t now = LOGIT_MONOTONIC_MS();
            return now > last ? (now - last) : 0;
        }

        Config m_config;
        mutable std::mutex m_mutex;
        mutable std::deque<BufferedLogEntry> m_entries;
        mutable std::size_t m_total_bytes = 0;
        std::atomic<int64_t> m_last_log_ts = ATOMIC_VAR_INIT(0);
        std::atomic<int64_t> m_last_log_mono_ts = ATOMIC_VAR_INIT(0);
        std::atomic<int> m_log_level = ATOMIC_VAR_INIT(static_cast<int>(LogLevel::LOG_LVL_TRACE));
    };

} // namespace logit

#endif // _LOGIT_MEMORY_LOGGER_HPP_INCLUDED
