#pragma once
#ifndef _LOGIT_LOGGERS_HPP_INCLUDED
#define _LOGIT_LOGGERS_HPP_INCLUDED

/// \file loggers.hpp
/// \brief Aggregates all logger implementations for convenient inclusion.

#include "detail/TaskExecutor.hpp"
#include "loggers/ILogger.hpp"
#include "loggers/ConsoleLogger.hpp"
#include "loggers/FileLogger.hpp"
#include "loggers/UniqueFileLogger.hpp"
#include "loggers/SyslogLogger.hpp"
#include "loggers/EventLogLogger.hpp"
#include "loggers/SystemLogger.hpp"

#endif // _LOGIT_LOGGERS_HPP_INCLUDED
