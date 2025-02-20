#ifndef _LOGIT_ENCODING_UTILS_HPP_INCLUDED
#define _LOGIT_ENCODING_UTILS_HPP_INCLUDED
/// \file encoding_utils.hpp
/// \brief Utilities for working with character encodings and string transformations.

#if defined(_WIN32)
#include <windows.h>

namespace logit {

    /// \brief Converts a UTF-8 string to an ANSI string (Windows-specific).
    /// \param utf8 The UTF-8 encoded string.
    /// \return The converted ANSI string.
    std::string utf8_to_ansi(const std::string& utf8) {
        int n_len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, NULL, 0);
        if (n_len == 0) return {};

        std::wstring wide_string(n_len + 1, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1,  &wide_string[0], n_len);

        n_len = WideCharToMultiByte(CP_ACP, 0, wide_string.c_str(), -1, NULL, 0, NULL, NULL);
        if (n_len == 0) return {};

        std::string ansi_string(n_len - 1, '\0');
        WideCharToMultiByte(CP_ACP, 0, wide_string.c_str(), -1, &ansi_string[0], n_len, NULL, NULL);
        return ansi_string;
    }

};
#endif

#endif // _LOGIT_ENCODING_UTILS_HPP_INCLUDED
