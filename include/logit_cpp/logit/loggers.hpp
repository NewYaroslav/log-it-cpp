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
#include "detail/SingleThreadExecutor.hpp"
#ifndef __EMSCRIPTEN__
#include "detail/CompressionWorker.hpp"
#endif

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <vector>

#include "loggers/ILogReader.hpp"
#include "loggers/ILogger.hpp"
#include "loggers/ConsoleLogger.hpp"
#include "loggers/MemoryLogger.hpp"
#include "loggers/FileLogger.hpp"
#include "loggers/UniqueFileLogger.hpp"
#include "loggers/SyslogLogger.hpp"
#include "loggers/EventLogLogger.hpp"
#include "loggers/SystemLogger.hpp"
#include "loggers/CrashLogger.hpp"
#include "loggers/WindowsDebugLogger.hpp"

#ifdef LOGIT_WITH_OTLP
#include "loggers/OtlpHttpLogger.hpp"
#include "loggers/OtlpPayloadLogger.hpp"
#endif

#ifdef LOGIT_WITH_PROMETHEUS
#include "loggers/prometheus/PrometheusRegistry.hpp"
#include "loggers/PrometheusPayloadLogger.hpp"
#endif
#ifdef LOGIT_WITH_PROMETHEUS_SERVER
#include "loggers/PrometheusHttpServerLogger.hpp"
#endif

#ifdef LOGIT_WITH_MDBX
#include "detail/CompressionUtils.hpp"
#include "detail/MdbxByteIO.hpp"
#include "detail/MdbxKeyUtils.hpp"
#include "detail/MdbxProcessId.hpp"
#include "loggers/MdbxLogger.hpp"
#endif

#endif // _LOGIT_LOGGERS_HPP_INCLUDED
