#pragma once
#ifndef _LOGIT_OTLP_RECORD_SNAPSHOT_HPP_INCLUDED
#define _LOGIT_OTLP_RECORD_SNAPSHOT_HPP_INCLUDED

/// \file OtlpRecordSnapshot.hpp
/// \brief Defines a stable snapshot of LogRecord data for asynchronous OTLP export.

#include <logit/utils.hpp>
#include <sstream>
#include <string>
#include <thread>

namespace logit {

    /// \struct OtlpRecordSnapshot
    /// \brief Owns LogRecord fields required after original logging call returns.
    struct OtlpRecordSnapshot {
        LogLevel log_level = LogLevel::LOG_LVL_TRACE; ///< Log severity level.
        int64_t timestamp_ms = 0;                     ///< Timestamp in milliseconds.
        std::string file;                             ///< Source file path.
        int line = 0;                                 ///< Source line number.
        std::string function;                         ///< Source function name.
        std::string format;                           ///< Original message or format string.
        std::string arg_names;                        ///< Original argument names.
        std::string thread_id;                        ///< Stringified std::thread::id.
        int logger_index = -1;                        ///< Target logger index, or -1 for all.
        bool print_mode = false;                      ///< Raw argument print mode flag.
        bool fmt_mode = false;                        ///< fmt formatting mode flag.
        bool raw_mode = false;                        ///< Raw log mode flag.
    };

    /// \brief Converts thread id to a portable string representation.
    /// \param id Thread id to convert.
    /// \return Stringified thread id.
    inline std::string otlp_thread_id_to_string(const std::thread::id& id) {
        std::ostringstream oss;
        oss << id;
        return oss.str();
    }

    /// \brief Creates an owned snapshot from a LogRecord.
    /// \param record Source log record.
    /// \return Snapshot safe to store in async queues.
    inline OtlpRecordSnapshot make_otlp_record_snapshot(const LogRecord& record) {
        OtlpRecordSnapshot out;
        out.log_level = record.log_level;
        out.timestamp_ms = record.timestamp_ms;
        out.file = record.file;
        out.line = record.line;
        out.function = record.function;
        out.format = record.format;
        out.arg_names = record.arg_names;
        out.thread_id = otlp_thread_id_to_string(record.thread_id);
        out.logger_index = record.logger_index;
        out.print_mode = record.print_mode;
        out.fmt_mode = record.fmt_mode;
        out.raw_mode = record.raw_mode;
        return out;
    }

} // namespace logit

#endif // _LOGIT_OTLP_RECORD_SNAPSHOT_HPP_INCLUDED
