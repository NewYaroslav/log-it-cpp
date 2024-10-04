#pragma once
#ifndef _LOGIT_CONFIG_HPP_INCLUDED
#define _LOGIT_CONFIG_HPP_INCLUDED
/// \file LogItConfig.hpp
/// \brief Configuration macros for the LogIt logging system.

/// \brief Defines the base path used for log file paths.
/// If LOGIT_BASE_PATH is not defined, defaults to an empty string.
#ifndef LOGIT_BASE_PATH
    #define LOGIT_BASE_PATH std::string()
#endif

/// \brief Defines the default color for console output.
/// If LOGIT_DEFAULT_COLOR is not defined, defaults to `TextColor::LightGray`.
#ifndef LOGIT_DEFAULT_COLOR
    #define LOGIT_DEFAULT_COLOR TextColor::LightGray
#endif

/// \brief Macro to get the current timestamp in milliseconds.
/// If LOGIT_CURRENT_TIMESTAMP_MS is not defined, it uses `std::chrono` to return the current time in milliseconds.
#ifndef LOGIT_CURRENT_TIMESTAMP_MS
    #define LOGIT_CURRENT_TIMESTAMP_MS() \
        (std::chrono::duration_cast<std::chrono::milliseconds>( \
        std::chrono::system_clock::now().time_since_epoch()).count())
#endif

#endif // _LOGIT_CONFIG_HPP_INCLUDED
