#pragma once

#include "OnPointStore.h"

namespace on_point {

// OnPointStorage over HalStorage (the SD card), for the firmware.
class HalOnPointStorage final : public OnPointStorage {
 public:
  bool exists(const char* path) override;
  ReadResult read(const char* path, char* buffer, size_t capacity, size_t& length) override;
  bool write(const char* path, const char* data, size_t length) override;
  bool remove(const char* path) override;
  bool rename(const char* oldPath, const char* newPath) override;
};

}  // namespace on_point
