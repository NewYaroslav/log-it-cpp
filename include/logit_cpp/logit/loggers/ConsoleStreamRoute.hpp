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
        std::ostream* custom_stream = nullptr; ///< Used when kind == Custom; null is ignored and the route falls back.

        /// \brief Builds a route that targets std::cout for the given level range.
        static ConsoleStreamRoute to_cout(LogLevel min_level, LogLevel max_level) {
            ConsoleStreamRoute route;
            route.min_level = min_level;
            route.max_level = max_level;
            route.kind = ConsoleStreamKind::Cout;
            return route;
        }

        /// \brief Builds a route that targets std::cerr for the given level range.
        static ConsoleStreamRoute to_cerr(LogLevel min_level, LogLevel max_level) {
            ConsoleStreamRoute route;
            route.min_level = min_level;
            route.max_level = max_level;
            route.kind = ConsoleStreamKind::Cerr;
            return route;
        }

        /// \brief Builds a route that targets a caller-provided std::ostream for the given level range.
        static ConsoleStreamRoute to_stream(
                LogLevel min_level,
                LogLevel max_level,
                std::ostream& stream) {
            ConsoleStreamRoute route;
            route.min_level = min_level;
            route.max_level = max_level;
            route.kind = ConsoleStreamKind::Custom;
            route.custom_stream = &stream;
            return route;
        }
    };

} // namespace logit

#endif // _LOGIT_CONSOLE_STREAM_ROUTE_HPP_INCLUDED
