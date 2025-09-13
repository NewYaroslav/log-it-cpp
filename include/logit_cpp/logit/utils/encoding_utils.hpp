#pragma once

#ifndef _LOGIT_ENCODING_UTILS_HPP_INCLUDED
#define _LOGIT_ENCODING_UTILS_HPP_INCLUDED

/// \file encoding_utils.hpp
/// \brief Utilities for working with character encodings and string transformations.

#include <string>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <stringapiset.h>

namespace logit {

    /// \brief Converts a UTF-8 string to an ANSI string (Windows-specific).
    /// \param utf8 The UTF-8 encoded string.
    /// \return The converted ANSI string.
    inline std::string utf8_to_ansi(const std::string& utf8) {
        int n_len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, NULL, 0);
        if (n_len == 0) return {};

        std::wstring wide_string(n_len + 1, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wide_string[0], n_len);

        n_len = WideCharToMultiByte(CP_ACP, 0, wide_string.c_str(), -1, NULL, 0, NULL, NULL);
        if (n_len == 0) return {};

        std::string ansi_string(n_len - 1, '\0');
        WideCharToMultiByte(CP_ACP, 0, wide_string.c_str(), -1, &ansi_string[0], n_len, NULL, NULL);
        return ansi_string;
    }

    /// \brief Converts a UTF-8 string to a wide UTF-16 string (Windows-specific).
    /// \param utf8 The UTF-8 encoded string.
    /// \return The converted wide string.
    inline std::wstring utf8_to_wstring(const std::string& utf8) {
        int n_len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, NULL, 0);
        if (n_len == 0) return {};
        std::wstring wide_string(n_len - 1, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wide_string[0], n_len);
        return wide_string;
    }

    /// \brief Converts a wide UTF-16 string to a UTF-8 string (Windows-specific).
    /// \param wide The UTF-16 encoded string.
    /// \return The converted UTF-8 string.
    inline std::string wstring_to_utf8(const std::wstring& wide) {
        int n_len = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, NULL, 0, NULL, NULL);
        if (n_len == 0) return {};
        std::string utf8(n_len - 1, '\0');
        WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, &utf8[0], n_len, NULL, NULL);
        return utf8;
    }

} // namespace logit
#endif // defined(_WIN32)

#endif // _LOGIT_ENCODING_UTILS_HPP_INCLUDED
