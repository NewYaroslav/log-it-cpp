#pragma once
#ifndef _LOGIT_ILOG_READER_HPP_INCLUDED
#define _LOGIT_ILOG_READER_HPP_INCLUDED

/// \file ILogReader.hpp
/// \brief Optional read-only interface for log backends that support querying stored records.

#include <logit/enums.hpp>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace logit {

    /// \struct LogRecordView
    /// \brief Common read-only view of a stored log record.
    struct LogRecordView {
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

    /// \class ILogReader
    /// \brief Optional interface for backends that expose stored records.
    ///
    /// Implementations should provide read_range for time-window queries
    /// and read_recent for "tail -n" style access.  Both return copies
    /// so the caller does not depend on backend locking details.
    class ILogReader {
    public:
        virtual ~ILogReader() = default;

        /// \brief Reads records in `[from_ms, to_ms)` ordered by timestamp.
        /// \param from_ms Inclusive start timestamp.
        /// \param to_ms   Exclusive end timestamp.
        /// \param limit   Maximum number of records (0 = unlimited).
        /// \return Matching records in backend-defined order (usually ascending).
        virtual std::vector<LogRecordView> read_range(
            int64_t from_ms,
            int64_t to_ms,
            std::size_t limit = 0) const = 0;

        /// \brief Reads the most recent records.
        /// \param limit     Maximum number of records (0 = unlimited).
        /// \param period_ms Time window in milliseconds from now backward (0 = unlimited).
        /// \param order     Ascending (oldest first) or Descending (newest first).
        /// \return Matching records in the requested order.
        virtual std::vector<LogRecordView> read_recent(
            std::size_t limit,
            int64_t period_ms = 0,
            LogReadOrder order = LogReadOrder::Ascending) const = 0;
    };

} // namespace logit

#endif // _LOGIT_ILOG_READER_HPP_INCLUDED
