#include "QrUtils.h"

#include <qrcode.h>

#include <algorithm>
#include <memory>

#include "Logging.h"
#include "QrVersionSelect.h"

void QrUtils::drawQrCode(const GfxRenderer& renderer, const Rect& bounds, const std::string& textPayload) {
  const size_t len = textPayload.length();
  const char* payload = textPayload.c_str();

  // Capacity-aware version selection (ECC_LOW, byte mode): pick the smallest
  // version whose true data capacity holds the payload. This keeps modules as
  // large as possible and, unlike the old coarse thresholds, never routes a
  // payload into a version too small to hold it (the encoder does NOT check
  // capacity: over-capacity data overruns the codeword buffer and overwrites the
  // ECC region, producing a rendered-but-undecodable code). A payload that
  // exceeds even version 40 fails gracefully here, before qrcode_initBytes.
  const int version = selectQrVersionEccLow(len);
  if (version == 0) {
    LOG_ERR("QR", "Payload %u bytes exceeds max QR capacity (%u bytes, ECC_LOW)", static_cast<unsigned>(len),
            static_cast<unsigned>(kQrMaxByteCapacityEccLow));
    return;
  }

  // Make sure we have a large enough buffer on the heap to avoid blowing the stack
  uint32_t bufferSize = qrcode_getBufferSize(version);
  auto qrcodeBytes = std::make_unique<uint8_t[]>(bufferSize);

  QRCode qrcode;
  // Initialize the QR code. We use ECC_LOW for max capacity.
  int8_t res = qrcode_initText(&qrcode, qrcodeBytes.get(), version, ECC_LOW, payload);

  if (res == 0) {
    // Determine the optimal pixel size.
    const int maxDim = std::min(bounds.width, bounds.height);

    int px = maxDim / qrcode.size;
    if (px < 1) px = 1;

    // Calculate centering X and Y
    const int qrDisplaySize = qrcode.size * px;
    const int xOff = bounds.x + (bounds.width - qrDisplaySize) / 2;
    const int yOff = bounds.y + (bounds.height - qrDisplaySize) / 2;

    // Draw the QR Code
    for (uint8_t cy = 0; cy < qrcode.size; cy++) {
      for (uint8_t cx = 0; cx < qrcode.size; cx++) {
        if (qrcode_getModule(&qrcode, cx, cy)) {
          renderer.fillRect(xOff + px * cx, yOff + px * cy, px, px, true);
        }
      }
    }
  } else {
    // If it fails (e.g. text too large), log an error
    LOG_ERR("QR", "Text too large for QR Code version %d", version);
  }
}
