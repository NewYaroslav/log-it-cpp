#pragma once
#ifndef LOGIT_CPP_LOGIT_HPP
#define LOGIT_CPP_LOGIT_HPP

/// \file logit.hpp
/// \brief Unified umbrella header for the LogIt++ library.
///
/// Including this header provides a fully self-contained entry point that
/// aggregates configuration, utilities, formatters, loggers and the logging
/// façade. No additional includes are required to start using the library.

#include "logit/config.hpp"
#include "logit/enums.hpp"
#include "logit/utils.hpp"
#include "logit/formatter.hpp"
#include "logit/loggers.hpp"
#include "logit/Logger.hpp"
#include "logit/log_macros.hpp"

/// \namespace logit
/// \brief The primary namespace for the LogIt++ library.
namespace logit {};

#endif // LOGIT_CPP_LOGIT_HPP
