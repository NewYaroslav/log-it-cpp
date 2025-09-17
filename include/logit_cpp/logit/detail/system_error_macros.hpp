#pragma once
#ifndef LOGIT_DETAIL_SYSTEM_ERROR_MACROS_HPP_INCLUDED
#define LOGIT_DETAIL_SYSTEM_ERROR_MACROS_HPP_INCLUDED

#include <errno.h>
#include <cstring>
#include <string>

/// \file system_error_macros.hpp
/// \brief Internal helpers shared by the system error logging macros.

#if defined(_WIN32)
#include <windows.h>
#endif

namespace logit {
namespace detail {

#if defined(_WIN32)
inline std::string _logit_format_winerr(DWORD code) {
    LPSTR buffer = nullptr;
    const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
    const DWORD length = ::FormatMessageA(flags,
                                          nullptr,
                                          code,
                                          MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                          reinterpret_cast<LPSTR>(&buffer),
                                          0,
                                          nullptr);

    if (length == 0 || buffer == nullptr) {
        return std::string();
    }

    std::string message(buffer, length);
    ::LocalFree(buffer);

    while (!message.empty() && (message.back() == '\r' || message.back() == '\n')) {
        message.pop_back();
    }

    return message;
}
#endif // defined(_WIN32)

} // namespace detail
} // namespace logit

#define LOGIT_DETAIL_PERROR(level_macro, message)                                                           \
    do {                                                                                                    \
        const int _logit_errno_code = (errno);                                                              \
        const char *_logit_errno_desc = ::strerror(_logit_errno_code);                                      \
        if (_logit_errno_desc == nullptr) {                                                                 \
            _logit_errno_desc = "";                                                                        \
        }                                                                                                   \
        const std::string _logit_errno_text(message);                                                       \
        LOGIT_PRINTF_##level_macro(LOGIT_POSIX_ERROR_PATTERN,                                              \
                                   _logit_errno_text.c_str(),                                              \
                                   _logit_errno_code,                                                      \
                                   _logit_errno_desc);                                                     \
    } while (0)

#if defined(_WIN32)

#define LOGIT_DETAIL_WINERR(level_macro, message)                                                           \
    do {                                                                                                    \
        const DWORD _logit_winerr_code = ::GetLastError();                                                  \
        const std::string _logit_winerr_text(message);                                                      \
        const std::string _logit_winerr_desc = ::logit::detail::_logit_format_winerr(_logit_winerr_code);   \
        LOGIT_PRINTF_##level_macro(LOGIT_WINDOWS_ERROR_PATTERN,                                            \
                                   _logit_winerr_text.c_str(),                                             \
                                   static_cast<unsigned long>(_logit_winerr_code),                         \
                                   _logit_winerr_desc.c_str());                                            \
    } while (0)

#else // defined(_WIN32)

#define LOGIT_DETAIL_WINERR(level_macro, message) do { } while (0)

#endif // defined(_WIN32)

#endif // LOGIT_DETAIL_SYSTEM_ERROR_MACROS_HPP_INCLUDED
