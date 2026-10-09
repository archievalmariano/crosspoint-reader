#pragma once

#include <cstddef>
#include <cstdio>

// Release asset the update check downloads (pure, host-testable).
//
// Suite builds carry a package (e.g. "g", "gp") and download
// firmware-<board>-<package>.bin, so a device only ever updates to the package
// it is running. Builds without a package keep the upstream names:
// firmware.bin for the combined X3/X4 binary ("x4"), firmware-<board>.bin
// otherwise. Returns false if the name does not fit in `out`.
namespace ota_asset {

inline bool name(char* out, size_t outSize, const char* board, size_t boardLen, const char* package) {
  const int boardChars = static_cast<int>(boardLen);
  int written;
  if (package != nullptr && package[0] != '\0') {
    written = std::snprintf(out, outSize, "firmware-%.*s-%s.bin", boardChars, board, package);
  } else if (boardLen == 2 && board[0] == 'x' && board[1] == '4') {
    written = std::snprintf(out, outSize, "firmware.bin");
  } else {
    written = std::snprintf(out, outSize, "firmware-%.*s.bin", boardChars, board);
  }
  return written > 0 && static_cast<size_t>(written) < outSize;
}

}  // namespace ota_asset
