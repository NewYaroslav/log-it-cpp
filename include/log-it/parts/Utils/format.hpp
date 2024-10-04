#pragma once
#ifndef _LOGIT_FORMAT_HPP_INCLUDED
#define _LOGIT_FORMAT_HPP_INCLUDED
/// \file format.hpp
/// \brief Function for formatting strings according to a specified format.

#include <cstdarg>
#include <vector>
#include <string>

#ifdef LOGIT_USE_FMT_LIB
#include <fmt/core.h>
#endif

namespace logit {

    /// \brief Formats a string according to the specified format.
    ///
    /// This function formats a string using either the custom implementation
    /// based on `vsnprintf`) or the `fmt` library, depending on whether
    /// the macro `LOGIT_USE_FMT_LIB` is defined.
    ///
    /// \param fmt The format string (similar to printf format).
    /// \param ... A variable number of arguments matching the format string.
    /// \see https://habr.com/ru/articles/318962/
    /// \return A formatted std::string.
    inline std::string format(const char *fmt, ...) {
#       ifdef LOGIT_USE_FMT_LIB
        va_list args;
        va_start(args, fmt);
        std::string result = fmt::vformat(fmt, fmt::make_format_args(args));
        va_end(args);
        return result;
#       else
        va_list args;
        va_start(args, fmt);
        std::vector<char> buffer(1024);
        for (;;) {
            va_list args_copy;
            va_copy(args_copy, args); // Copy args to prevent modifying the original list.
            int res = vsnprintf(buffer.data(), buffer.size(), fmt, args_copy);
            va_end(args_copy); // Clean up the copied argument list.

            if ((res >= 0) && (res < static_cast<int>(buffer.size()))) {
                va_end(args); // Clean up the original argument list.
                return std::string(buffer.data()); // Return the formatted string.
            }

            // If the buffer was too small, resize it.
            const size_t size = res < 0 ? buffer.size() * 2 : static_cast<size_t>(res) + 1;
            buffer.clear();
            buffer.resize(size);
        }
#       endif
    }

#   ifndef LOGIT_USE_FMT_LIB
    /// \brief Formats a string according to the specified format using std::string.
    ///
    /// This function formats a string using either the custom implementation
    /// based on `vsnprintf`) or the `fmt` library, depending on whether
    /// the macro `LOGIT_USE_FMT_LIB` is defined.
    ///
    /// \param fmt The format string (similar to printf format) provided as std::string.
    /// \param ... A variable number of arguments matching the format string.
    /// \return A formatted std::string.
    inline std::string format(const std::string& fmt, ...) {
        va_list args;
        va_start(args, fmt);
        std::vector<char> buffer(1024);
        for (;;) {
            va_list args_copy;
            va_copy(args_copy, args); // Copy args to prevent modifying the original list.
            int res = vsnprintf(buffer.data(), buffer.size(), fmt.c_str(), args_copy);
            va_end(args_copy); // Clean up the copied argument list.

            if ((res >= 0) && (res < static_cast<int>(buffer.size()))) {
                va_end(args); // Clean up the original argument list.
                return std::string(buffer.data()); // Return the formatted string.
            }

            // If the buffer was too small, resize it.
            const size_t size = res < 0 ? buffer.size() * 2 : static_cast<size_t>(res) + 1;
            buffer.clear();
            buffer.resize(size);
        }
    }
#   else
    /// \brief Formats a string according to the specified format using std::string.
    ///
    /// This version of the function uses the `fmt` library to format the string.
    ///
    /// \param fmt The format string (similar to printf format) provided as std::string.
    /// \return A formatted std::string.
    inline std::string format_string(const std::string& fmt) {
        return fmt;
    }

    /// \brief Formats a string according to the specified format using std::string.
    ///
    /// This templated version of the function accepts a format string and a variable number
    /// of arguments to format the string using the `fmt` library.
    ///
    /// \param fmt The format string (similar to printf format) provided as std::string.
    /// \param ... A variable number of arguments matching the format string.
    /// \return A formatted std::string.
    template <typename... Args>
    inline std::string format_string(const std::string& fmt, Args&&... args) {
        return fmt::format(fmt, std::forward<Args>(args)...);
    }
#   endif

}; // namespace logit

#endif // _LOGIT_FORMAT_HPP_INCLUDED
