#pragma once
#ifndef _LOGIT_BUFFERED_LOG_ENTRY_HPP_INCLUDED
#define _LOGIT_BUFFERED_LOG_ENTRY_HPP_INCLUDED

/// \file BufferedLogEntry.hpp
/// \brief Structured snapshot entry used by in-memory log buffers.

#include <cstdint>
#include <string>

namespace logit {

    /// \struct BufferedLogEntry
    /// \brief Structured representation of a buffered log snapshot entry.
    /// \details This is a small public DTO for snapshot-style APIs such as
    /// `MemoryLogger` and related query helpers.
    struct BufferedLogEntry {
        LogLevel    level = LogLevel::LOG_LVL_TRACE; ///< Severity of the log entry.
        int64_t     timestamp_ms = 0;                ///< Wall-clock timestamp in milliseconds.
        std::string file;                            ///< Source file path or leaf name.
        int         line = 0;                        ///< Source line number.
        std::string function;                        ///< Function name captured at the call site.
        std::string message;                         ///< Formatted message text stored in the buffer.
    };

} // namespace logit

#endif // _LOGIT_BUFFERED_LOG_ENTRY_HPP_INCLUDED
