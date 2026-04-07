#pragma once
#ifndef _LOGIT_LOG_FILE_INFO_HPP_INCLUDED
#define _LOGIT_LOG_FILE_INFO_HPP_INCLUDED

/// \file LogFileInfo.hpp
/// \brief Public DTO that describes a persisted log file exposed by a file-based backend.

#include <string>
#include <cstdint>

namespace logit {

    /// \brief Metadata for a persisted log file owned by a logger backend.
    struct LogFileInfo {
        std::string path;          ///< Full path to the file.
        std::string name;          ///< File name without the parent directory.
        int64_t     day_start_ms = 0; ///< Start-of-day UTC timestamp derived from the file name.
        bool        is_current = false; ///< True when this is the file currently used for appends.
        bool        is_compressed = false; ///< True when the file is a compressed rotated artifact.
    };

} // namespace logit

#endif // _LOGIT_LOG_FILE_INFO_HPP_INCLUDED
