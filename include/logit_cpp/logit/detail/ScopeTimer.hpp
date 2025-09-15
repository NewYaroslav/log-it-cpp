#pragma once
#ifndef _LOGIT_DETAIL_SCOPE_TIMER_HPP_INCLUDED
#define _LOGIT_DETAIL_SCOPE_TIMER_HPP_INCLUDED

/// \file ScopeTimer.hpp
/// \brief RAII timer that logs the duration of a scope.

#include "../config.hpp"
#include "../utils.hpp"
#include "../Logger.hpp"

#include <chrono>
#include <string>

namespace logit { namespace detail {

    class ScopeTimer {
    public:
        ScopeTimer(LogLevel level,
                   std::string phase,
                   const char* file,
                   int line,
                   const char* function,
                   int logger_index,
                   int64_t threshold_ms = 0)
            : level_(level)
            , phase_(std::move(phase))
            , file_(file)
            , line_(line)
            , function_(function)
            , logger_index_(logger_index)
            , threshold_ms_(threshold_ms)
            , t0_(std::chrono::steady_clock::now()) {}

        ~ScopeTimer() {
            const auto t1 = std::chrono::steady_clock::now();
            const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0_).count();
            if (ms < threshold_ms_) return;

            std::string msg = phase_;
            msg += " | duration_ms=";
            msg += std::to_string(ms);

            Logger::get_instance().log_and_return(LogRecord{
                level_,
                LOGIT_WALLCLOCK_MS(),
                logit::make_relative(file_, LOGIT_BASE_PATH),
                line_,
                function_,
                std::string(), std::string(),
                logger_index_,
                false
            }, msg);
        }

    private:
        LogLevel level_;
        std::string phase_;
        const char* file_;
        int line_;
        const char* function_;
        int logger_index_;
        int64_t threshold_ms_;
        std::chrono::steady_clock::time_point t0_;
    };

}} // namespace logit::detail

#endif // _LOGIT_DETAIL_SCOPE_TIMER_HPP_INCLUDED
