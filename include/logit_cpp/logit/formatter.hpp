#pragma once
#ifndef _LOGIT_FORMATTER_HPP_INCLUDED
#define _LOGIT_FORMATTER_HPP_INCLUDED

/// \file formatter.hpp
/// \brief Aggregates the formatter subsystem for convenient inclusion.
///
/// This header is self-contained and pulls in the generic formatter interface alongside the
/// default implementations and pattern compiler support utilities.

#include "utils.hpp"
#include "formatter/ILogFormatter.hpp"
#include "formatter/SimpleLogFormatter.hpp"
#include "formatter/compiler/PatternCompiler.hpp"

#endif // _LOGIT_FORMATTER_HPP_INCLUDED
