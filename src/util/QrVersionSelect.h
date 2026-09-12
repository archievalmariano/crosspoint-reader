#pragma once

#include <cstddef>

namespace QrUtils {

// Largest byte-mode payload encodable at ECC_LOW (version 40). A payload longer
// than this cannot be represented as a QR code at this ECC level and must not be
// handed to the encoder.
inline constexpr size_t kQrMaxByteCapacityEccLow = 2953;

// Smallest QR version (1..40) whose ECC_LOW byte-mode data capacity is at least
// `len` bytes, or 0 when `len` exceeds the version-40 capacity (caller must then
// refuse to encode rather than overrun the codeword buffer / corrupt the ECC
// region). Capacity-aware selection: the returned version is guaranteed to hold
// the payload, and no larger, so modules stay as physically large as possible.
int selectQrVersionEccLow(size_t len);

}  // namespace QrUtils
