#pragma once
#ifndef _LOGIT_HPP_INCLUDED
#define _LOGIT_HPP_INCLUDED
/// \file LogIt.hpp
/// \brief Main header file for the LogIt++ library.

#include "LogItConfig.hpp"
#include "parts/Enums.hpp"
#include "parts/Utils.hpp"
#include "parts/TaskExecutor.hpp"
#include "parts/Logger.hpp"
#include "parts/LogStream.hpp"
#include "parts/LogMacros.hpp"
#include "parts/Formatter/SimpleLogFormatter.hpp"
#include "parts/Logger/ConsoleLogger.hpp"
#include "parts/Logger/FileLogger.hpp"
#include "parts/Logger/UniqueFileLogger.hpp"

/// \namespace logit
/// \brief The primary namespace for the LogIt++ library.
namespace logit {};

#endif // _LOGIT_HPP_INCLUDED
