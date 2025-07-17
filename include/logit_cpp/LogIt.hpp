#pragma once
#ifndef _LOGIT_HPP_INCLUDED
#define _LOGIT_HPP_INCLUDED

/// \file LogIt.hpp
/// \brief Main header file for the LogIt++ library.

#include "LogItConfig.hpp"
#include "logit/enums.hpp"
#include "logit/utils.hpp"
#include "logit/TaskExecutor.hpp"
#include "logit/Logger.hpp"
#include "logit/LogStream.hpp"
#include "logit/LogMacros.hpp"
#include "logit/formatter/SimpleLogFormatter.hpp"
#include "logit/loggers/ConsoleLogger.hpp"
#include "logit/loggers/FileLogger.hpp"
#include "logit/loggers/UniqueFileLogger.hpp"

/// \namespace logit
/// \brief The primary namespace for the LogIt++ library.
namespace logit {};

#endif // _LOGIT_HPP_INCLUDED
