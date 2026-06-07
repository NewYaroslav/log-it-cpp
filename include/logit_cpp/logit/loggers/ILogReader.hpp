#pragma once
#ifndef _LOGIT_ILOG_READER_HPP_INCLUDED
#define _LOGIT_ILOG_READER_HPP_INCLUDED

/// \file ILogReader.hpp
/// \brief Optional read-only interface for log backends that support querying stored records.

#include "../enums.hpp"
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#if __cplusplus >= 201703L
#include <optional>
#endif

namespace logit {

    /// \struct LogRecordSnapshot
    /// \brief Common owning snapshot of a stored log record.
    ///
    /// String fields are copied from the backend, so callers may store the
    /// snapshot by value without depending on backend locks or record lifetime.
    struct LogRecordSnapshot {
        uint64_t session_id = 0;                     ///< Owning session id, or 0 if unused.
        int64_t timestamp_ms = 0;                    ///< Log timestamp in milliseconds.
        uint32_t sequence = 0;                       ///< Per-timestamp sequence, or 0 if unused.
        LogLevel level = LogLevel::LOG_LVL_TRACE;    ///< Log severity level.
        std::string message;                         ///< Formatted message or payload preview.
        uint64_t payload_id = 0;                     ///< Payload row id, or 0 when absent.
        std::string file;                            ///< Source file path.
        std::string function;                        ///< Source function name.
        int line = 0;                                ///< Source line number.
    };

    /// \enum LogReadOrder
    /// \brief Ordering for query results returned by ILogReader.
    enum class LogReadOrder {
        Ascending,   ///< Oldest first.
        Descending   ///< Newest first.
    };

#if __cplusplus >= 201703L
    /// \enum LogReadError
    /// \brief Result status for log read APIs that preserve failure details.
    enum class LogReadError {
        None,
        NotFound,
        StorageError,
        DecodeError,
        UnsupportedVersion,
        DecompressionError
    };

    /// \struct LogReadResult
    /// \brief Value-or-error result returned by detailed log read APIs.
    template <typename T>
    struct LogReadResult {
        std::optional<T> value; ///< Present when the read succeeded with a value.
        LogReadError error = LogReadError::None; ///< Error status, or None on success.
        std::string message; ///< Optional diagnostic message for failed reads.
    };
#endif

    /// \class ILogReader
    /// \brief Optional interface for backends that expose stored records.
    ///
    /// Implementations should provide read_range for time-window queries
    /// and read_recent for "tail -n" style access. Both return owning
    /// snapshots so the caller does not depend on backend locking details.
    class ILogReader {
    public:
        virtual ~ILogReader() = default;

        /// \brief Reads records in `[from_ms, to_ms)` ordered by timestamp.
        /// \param from_ms Inclusive start timestamp.
        /// \param to_ms   Exclusive end timestamp.
        /// \param limit   Maximum number of records (0 = unlimited).
        /// \return Matching records in backend-defined order (usually ascending).
        virtual std::vector<LogRecordSnapshot> read_range(
            int64_t from_ms,
            int64_t to_ms,
            std::size_t limit = 0) const = 0;

#if __cplusplus >= 201703L
        /// \brief Reads records and preserves backend-specific read errors.
        virtual LogReadResult<std::vector<LogRecordSnapshot>> read_range_result(
            int64_t from_ms,
            int64_t to_ms,
            std::size_t limit = 0) const {
            LogReadResult<std::vector<LogRecordSnapshot>> result;
            result.value = read_range(from_ms, to_ms, limit);
            return result;
        }
#endif

        /// \brief Reads the most recent records.
        /// \param limit     Maximum number of records (0 = unlimited).
        /// \param period_ms Time window in milliseconds from now backward (0 = unlimited).
        /// \param order     Ascending (oldest first) or Descending (newest first).
        /// \return Matching records in the requested order.
        virtual std::vector<LogRecordSnapshot> read_recent(
            std::size_t limit,
            int64_t period_ms = 0,
            LogReadOrder order = LogReadOrder::Ascending) const = 0;

#if __cplusplus >= 201703L
        /// \brief Reads recent records and preserves backend-specific read errors.
        virtual LogReadResult<std::vector<LogRecordSnapshot>> read_recent_result(
            std::size_t limit,
            int64_t period_ms = 0,
            LogReadOrder order = LogReadOrder::Ascending) const {
            LogReadResult<std::vector<LogRecordSnapshot>> result;
            result.value = read_recent(limit, period_ms, order);
            return result;
        }
#endif
    };

} // namespace logit

#endif // _LOGIT_ILOG_READER_HPP_INCLUDED
