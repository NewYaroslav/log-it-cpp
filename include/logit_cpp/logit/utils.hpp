#pragma once
#ifndef _LOGIT_UTILS_HPP_INCLUDED
#define _LOGIT_UTILS_HPP_INCLUDED

/// \file utils.hpp
/// \brief Aggregates the public utilities module.
///
/// This header is a self-contained entry point that exposes all utility components required
/// by the library: configuration macros, enums, formatting helpers, value wrappers and the
/// log record type. Include
/// it before any header inside `utils/` to satisfy the nearest-header requirement.

#include "config.hpp"
#include "enums.hpp"
#include "utils/format.hpp"
#include "utils/VariableValue.hpp"
#include "utils/argument_utils.hpp"
#include "utils/encoding_utils.hpp"
#include "utils/path_utils.hpp"
#include "utils/LogRecord.hpp"
#include "utils/tag_utils.hpp"

#endif // _LOGIT_UTILS_HPP_INCLUDED
