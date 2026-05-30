#pragma once
#ifndef _LOGIT_OTLP_COMPRESSION_HPP_INCLUDED
#define _LOGIT_OTLP_COMPRESSION_HPP_INCLUDED

/// \file OtlpCompression.hpp
/// \brief Backward-compatible forwarding header for shared compression helpers.
///
/// New code should include `<logit/detail/CompressionUtils.hpp>` directly.
/// This header is kept for existing OTLP headers that include it.

#include "../../detail/CompressionUtils.hpp"

namespace logit {

enum class OtlpCompression { None, Gzip, Zstd };

// Forward declarations: the actual implementations live in detail::
using detail::compress_string_gzip;
using detail::compress_string_zstd;

} // namespace logit

#endif // _LOGIT_OTLP_COMPRESSION_HPP_INCLUDED
