#pragma once

/// \file MdbxProcessId.hpp
/// \brief Cross-platform current process id helper for MdbxLogger.

#include <cstdint>

#if defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace logit {
namespace detail {

inline uint64_t current_process_id() {
#if defined(_WIN32)
    return static_cast<uint64_t>(GetCurrentProcessId());
#else
    return static_cast<uint64_t>(getpid());
#endif
}

} // namespace detail
} // namespace logit
