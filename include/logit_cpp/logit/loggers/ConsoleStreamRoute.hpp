#pragma once
#ifndef _LOGIT_CONSOLE_STREAM_ROUTE_HPP_INCLUDED
#define _LOGIT_CONSOLE_STREAM_ROUTE_HPP_INCLUDED

/// \file ConsoleStreamRoute.hpp
/// \brief Level-based output stream routing for ConsoleLogger.

#include "../enums.hpp"
#include <ostream>

namespace logit {

    /// \enum ConsoleStreamKind
    /// \brief Identifies a target output stream for ConsoleLogger routes.
    enum class ConsoleStreamKind {
        Cout,   ///< Routes records to std::cout.
        Cerr,   ///< Routes records to std::cerr.
        Custom  ///< Routes records to a caller-provided std::ostream pointer.
    };

    /// \struct ConsoleStreamRoute
    /// \brief Maps a log-level range to a target output stream.
    struct ConsoleStreamRoute {
        LogLevel min_level = LogLevel::LOG_LVL_TRACE; ///< Inclusive lower bound.
        LogLevel max_level = LogLevel::LOG_LVL_FATAL; ///< Inclusive upper bound.
        ConsoleStreamKind kind = ConsoleStreamKind::Cout; ///< Stream identifier.
        std::ostream* custom_stream = nullptr; ///< Used when kind == Custom; null is ignored.
    };

} // namespace logit

#endif // _LOGIT_CONSOLE_STREAM_ROUTE_HPP_INCLUDED
