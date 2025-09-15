#pragma once
#ifndef _LOGIT_TAG_UTILS_HPP_INCLUDED
#define _LOGIT_TAG_UTILS_HPP_INCLUDED

#include <sstream>
#include <string>

namespace logit {
namespace detail {

    /// \brief Formats tags as key=value pairs separated by spaces.
    /// \tparam Tags Container of pairs convertible to output stream.
    /// \param tags Collection of tag key-value pairs.
    /// \return String in format " key=value ..." or empty string when no tags.
    template <typename Tags>
    inline std::string format_tags(const Tags& tags) {
        std::ostringstream oss;
        for (const auto& kv : tags) {
            oss << ' ' << kv.first << '=' << kv.second;
        }
        return oss.str();
    }

    /// \brief Helper to allow passing initializer lists to macros.
    template <typename K, typename V>
    inline std::initializer_list<std::pair<K, V>> make_tags(std::initializer_list<std::pair<K, V>> tags) {
        return tags;
    }

} // namespace detail
} // namespace logit

#endif // _LOGIT_TAG_UTILS_HPP_INCLUDED
