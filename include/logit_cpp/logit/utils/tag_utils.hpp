#pragma once
#ifndef _LOGIT_TAG_UTILS_HPP_INCLUDED
#define _LOGIT_TAG_UTILS_HPP_INCLUDED

#include <sstream>
#include <string>
#include <initializer_list>
#include <utility>

namespace logit { namespace detail {

    template <typename T>
    inline std::string to_string_any(const T& v) {
        std::ostringstream oss;
        oss << v;                // работает для int, std::string, const char*, и т.д.
        return oss.str();
    }

    inline std::pair<std::string,std::string> make_tag(std::string key, std::string value) {
        return { std::move(key), std::move(value) };
    }

    template <typename V>
    inline std::pair<std::string,std::string> make_tag(const char* key, const V& v) {
        return { std::string(key), to_string_any(v) };
    }

    inline std::string format_tags(std::initializer_list<std::pair<std::string,std::string>> tags) {
        if (tags.size() == 0) return {};
        std::ostringstream oss;
        oss << LOGIT_TAGS_JOIN;

        bool first = true;
        for (const auto& kv : tags) {
            if (!first) oss << LOGIT_TAG_PAIR_SEP;
            first = false;

            const std::string& k = kv.first;
            const std::string& v = kv.second;

            auto need_quote = [] (const std::string& s) {
                return s.find(' ') != std::string::npos ||
                       s.find('=') != std::string::npos;
            };

            oss << k << LOGIT_TAG_KV_SEP;
#           if LOGIT_TAG_QUOTE_VALUES
            if (need_quote(v)) {
                oss << '"';
                for (char c : v) { if (c == '"') oss << '\\'; oss << c; }
                oss << '"';
            } else {
                oss << v;
            }
#           else
            oss << v;
#           endif
        }
        return oss.str();
    }

}} // namespace logit::detail

#endif

