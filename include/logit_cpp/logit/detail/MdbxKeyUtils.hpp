#pragma once

/// \file MdbxKeyUtils.hpp
/// \brief Key encoding helpers for MdbxLogger record ordering.

#include <cstdint>
#include <string>
#include <limits>

namespace logit {
namespace detail {

inline void mdbx_write_record_key_be(std::string& key, uint64_t value, size_t offset) {
    for (int shift = 56; shift >= 0; shift -= 8) {
        key[offset++] = static_cast<char>((value >> shift) & 0xFFu);
    }
}

inline void mdbx_write_record_sequence_be(std::string& key, uint32_t value, size_t offset) {
    for (int shift = 24; shift >= 0; shift -= 8) {
        key[offset++] = static_cast<char>((value >> shift) & 0xFFu);
    }
}

inline std::string make_mdbx_record_key(int64_t timestamp_ms, uint32_t sequence) {
    std::string key(12, '\0');
    const uint64_t sortable_ts = static_cast<uint64_t>(timestamp_ms) ^ 0x8000000000000000ULL;
    mdbx_write_record_key_be(key, sortable_ts, 0);
    mdbx_write_record_sequence_be(key, sequence, 8);
    return key;
}

} // namespace detail
} // namespace logit
