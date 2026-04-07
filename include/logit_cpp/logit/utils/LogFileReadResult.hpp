#pragma once
#ifndef _LOGIT_LOG_FILE_READ_RESULT_HPP_INCLUDED
#define _LOGIT_LOG_FILE_READ_RESULT_HPP_INCLUDED

/// \file LogFileReadResult.hpp
/// \brief Public DTO that contains a persisted log-file read result.

#include "LogFileInfo.hpp"
#include <string>

namespace logit {

    /// \brief Result of reading a persisted log file from a file-based backend.
    struct LogFileReadResult {
        LogFileInfo  file;       ///< Metadata for the requested file.
        std::string  content;    ///< File contents when the read succeeded.
        bool         ok = false; ///< Whether the read succeeded.
    };

} // namespace logit

#endif // _LOGIT_LOG_FILE_READ_RESULT_HPP_INCLUDED
