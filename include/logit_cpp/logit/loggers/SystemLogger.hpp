#pragma once
#include "SyslogLogger.hpp"
#include "EventLogLogger.hpp"

namespace logit {
#if defined(_WIN32)
using SystemLogger = EventLogLogger;
#else
using SystemLogger = SyslogLogger;
#endif
}

