#pragma once
#ifndef _LOGIT_LOGGERS_HPP_INCLUDED
#define _LOGIT_LOGGERS_HPP_INCLUDED

/// \file loggers.hpp
/// \brief Aggregates all public logger backends.
///
/// This header is self-contained: it prepares common configuration, utility and detail
/// dependencies before including each backend implementation. Include it prior to including
/// any header under `loggers/` to satisfy the nearest-header requirement.

#include "config.hpp"
#include "utils.hpp"
#include "detail/TaskExecutor.hpp"
#ifndef __EMSCRIPTEN__
#include "detail/CompressionWorker.hpp"
#endif

#include "loggers/ILogger.hpp"
#include "loggers/ConsoleLogger.hpp"
#include "loggers/FileLogger.hpp"
#include "loggers/UniqueFileLogger.hpp"
#include "loggers/SyslogLogger.hpp"
#include "loggers/EventLogLogger.hpp"
#include "loggers/SystemLogger.hpp"
#include "loggers/CrashLogger.hpp"

#endif // _LOGIT_LOGGERS_HPP_INCLUDED
