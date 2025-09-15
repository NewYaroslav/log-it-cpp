#pragma once
#ifndef LOGIT_CRASH_LOGGER_HPP_INCLUDED
#define LOGIT_CRASH_LOGGER_HPP_INCLUDED

/// \\file CrashLogger.hpp
/// \\brief Platform-specific crash logger alias.

#include "CrashPosixLogger.hpp"
#include "CrashWindowsLogger.hpp"

namespace logit {

#if defined(_WIN32)
    using CrashLogger = CrashWindowsLogger;
#else
    using CrashLogger = CrashPosixLogger;
#endif

} // namespace logit

#endif // LOGIT_CRASH_LOGGER_HPP_INCLUDED
