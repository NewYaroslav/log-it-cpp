#pragma once

/// \file MdbxByteIO.hpp
/// \brief Byte serialization helpers for MdbxLogger.

#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace logit {
namespace detail {

class MdbxByteWriter {
public:
    void write_u8(uint8_t value) {
        m_out.push_back(value);
    }

    void write_u32(uint32_t value) {
        for (int shift = 24; shift >= 0; shift -= 8) {
            m_out.push_back(static_cast<uint8_t>((value >> shift) & 0xFFu));
        }
    }

    void write_u64(uint64_t value) {
        for (int shift = 56; shift >= 0; shift -= 8) {
            m_out.push_back(static_cast<uint8_t>((value >> shift) & 0xFFu));
        }
    }

    void write_i64(int64_t value) {
        uint64_t bits = 0;
        std::memcpy(&bits, &value, sizeof(bits));
        write_u64(bits);
    }

    void write_string(const std::string& value) {
        if (value.size() > static_cast<size_t>((std::numeric_limits<uint32_t>::max)())) {
            throw std::length_error("MdbxLogger: string field is too large");
        }
        write_u32(static_cast<uint32_t>(value.size()));
        m_out.insert(m_out.end(), value.begin(), value.end());
    }

    const std::vector<uint8_t>& bytes() const {
        return m_out;
    }

private:
    std::vector<uint8_t> m_out;
};

class MdbxByteReader {
public:
    MdbxByteReader(const void* data, size_t size)
        : m_cur(static_cast<const uint8_t*>(data)),
          m_end(static_cast<const uint8_t*>(data)) {
        if (m_cur == nullptr && size != 0) {
            throw std::runtime_error("MdbxLogger: null serialized value");
        }
        m_end = m_cur == nullptr ? m_cur : m_cur + size;
    }

    uint8_t read_u8() {
        require(1);
        return *m_cur++;
    }

    uint32_t read_u32() {
        require(4);
        uint32_t value = 0;
        for (int i = 0; i < 4; ++i) {
            value = (value << 8) | static_cast<uint32_t>(*m_cur++);
        }
        return value;
    }

    uint64_t read_u64() {
        require(8);
        uint64_t value = 0;
        for (int i = 0; i < 8; ++i) {
            value = (value << 8) | static_cast<uint64_t>(*m_cur++);
        }
        return value;
    }

    int64_t read_i64() {
        const uint64_t bits = read_u64();
        int64_t value = 0;
        std::memcpy(&value, &bits, sizeof(value));
        return value;
    }

    std::string read_string() {
        const uint32_t size = read_u32();
        require(size);
        const char* begin = reinterpret_cast<const char*>(m_cur);
        m_cur += size;
        return std::string(begin, size);
    }

    void finish() const {
        if (m_cur != m_end) {
            throw std::runtime_error("MdbxLogger: trailing bytes in serialized value");
        }
    }

private:
    const uint8_t* m_cur;
    const uint8_t* m_end;

    void require(size_t size) const {
        if (static_cast<size_t>(m_end - m_cur) < size) {
            throw std::runtime_error("MdbxLogger: corrupted serialized value");
        }
    }
};

} // namespace detail
} // namespace logit
