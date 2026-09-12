#include "QrVersionSelect.h"

namespace {

// ECC_LOW, byte-mode data capacity in bytes for QR versions 1..40 (ISO/IEC
// 18004). Derived directly from the bundled ricmoo/QRCode tables:
//   dataCodewords(v) = NUM_RAW_DATA_MODULES[v-1] / 8 - NUM_ERROR_CORRECTION_CODEWORDS[Low][v-1]
//   capacityBytes(v) = (dataCodewords(v) * 8 - 4 (mode) - charCountBits(v)) / 8
// where charCountBits is 8 for v1..9 and 16 for v10..40. These are the true
// capacities the encoder enforces implicitly; the previous coarse thresholds
// (v4 for <=114 B) overstated v4 (real capacity 78 B), routing 79..114 B
// payloads into a version that cannot hold them and silently corrupting them.
constexpr int kByteCapacityEccLow[40] = {
      17,   32,   53,   78,  106,  134,  154,  192,  230,  271,
     321,  367,  425,  458,  520,  586,  644,  718,  792,  858,
     929, 1003, 1091, 1171, 1273, 1367, 1465, 1528, 1628, 1732,
    1840, 1952, 2068, 2188, 2303, 2431, 2563, 2699, 2809, 2953,
};

}  // namespace

int QrUtils::selectQrVersionEccLow(size_t len) {
  for (int v = 1; v <= 40; ++v) {
    if (len <= static_cast<size_t>(kByteCapacityEccLow[v - 1])) return v;
  }
  return 0;  // exceeds version-40 ECC_LOW capacity: caller must not encode
}
