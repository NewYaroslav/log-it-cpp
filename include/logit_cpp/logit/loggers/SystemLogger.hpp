#pragma once
#include "SyslogLogger.hpp"
#include "EventLogLogger.hpp"

/// \file SystemLogger.hpp
/// \brief Defines alias to platform system logger.

namespace logit {
#   if defined(_WIN32)
    /// \brief Windows system logger alias.
    using SystemLogger = EventLogLogger;
#   else
    /// \brief POSIX system logger alias.
    using SystemLogger = SyslogLogger;
#   endif
}

