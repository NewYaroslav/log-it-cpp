#pragma once
#ifndef _LOGIT_OTLP_COMPRESSION_HPP_INCLUDED
#define _LOGIT_OTLP_COMPRESSION_HPP_INCLUDED

#include <string>

#if defined(LOGIT_HAS_ZLIB)
#   include <zlib.h>
#endif

#if defined(LOGIT_HAS_ZSTD)
#   include <zstd.h>
#endif

namespace logit {

enum class OtlpCompression { None, Gzip, Zstd };

inline bool compress_string_gzip(const std::string& input, std::string& output, int level) {
#if defined(LOGIT_HAS_ZLIB)
    z_stream zs;
    zs.zalloc = Z_NULL;
    zs.zfree = Z_NULL;
    zs.opaque = Z_NULL;
    zs.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(input.data()));
    zs.avail_in = static_cast<uInt>(input.size());
    zs.next_out = Z_NULL;
    zs.avail_out = 0;

    if (level < 1) level = 1;
    if (level > 9) level = 9;

    int window_bits = 15 + 16;
    if (deflateInit2(&zs, level, Z_DEFLATED, window_bits, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        return false;
    }

    output.clear();

    int ret = Z_OK;
    std::size_t offset = 0;
    const std::size_t buf_size = 32768;

    do {
        output.resize(offset + buf_size);
        zs.next_out = reinterpret_cast<Bytef*>(&output[offset]);
        zs.avail_out = static_cast<uInt>(buf_size);

        ret = deflate(&zs, Z_FINISH);
        if (ret == Z_STREAM_ERROR) {
            deflateEnd(&zs);
            return false;
        }

        offset = zs.total_out;
    } while (ret != Z_STREAM_END);

    output.resize(zs.total_out);
    deflateEnd(&zs);
    return true;
#else
    (void)input; (void)output; (void)level;
    return false;
#endif
}

inline bool compress_string_zstd(const std::string& input, std::string& output, int level) {
#if defined(LOGIT_HAS_ZSTD)
    if (level < 1) level = 1;
    if (level > 19) level = 19;

    std::size_t bound = ZSTD_compressBound(input.size());
    output.resize(bound);

    std::size_t result = ZSTD_compress(
        &output[0], output.size(),
        input.data(), input.size(),
        level);

    if (ZSTD_isError(result)) {
        output.clear();
        return false;
    }

    output.resize(result);
    return true;
#else
    (void)input; (void)output; (void)level;
    return false;
#endif
}

} // namespace logit

#endif // _LOGIT_OTLP_COMPRESSION_HPP_INCLUDED
