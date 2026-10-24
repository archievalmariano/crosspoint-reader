#include "OnPointStoreHal.h"

#include <HalStorage.h>

namespace on_point {

bool HalOnPointStorage::exists(const char* path) { return Storage.exists(path); }

OnPointStorage::ReadResult HalOnPointStorage::read(const char* path, char* buffer, const size_t capacity,
                                                   size_t& length) {
  length = 0;
  if (!Storage.exists(path)) return ReadResult::Missing;
  HalFile file;
  if (!Storage.openFileForRead("ONPT", path, file)) return ReadResult::Error;
  const size_t size = file.fileSize();
  if (size > capacity) return ReadResult::TooLarge;
  if (size > 0 && file.read(buffer, size) != static_cast<int>(size)) return ReadResult::Error;
  length = size;
  return ReadResult::Ok;
}

bool HalOnPointStorage::write(const char* path, const char* data, const size_t length) {
  Storage.ensureDirectoryExists("/.crosspoint");
  HalFile file;
  if (!Storage.openFileForWrite("ONPT", path, file)) return false;
  const bool written = file.write(data, length) == length;
  // close() flushes; it must succeed for the data to be on the card.
  return file.close() && written;
}

bool HalOnPointStorage::remove(const char* path) { return Storage.remove(path); }

bool HalOnPointStorage::rename(const char* oldPath, const char* newPath) { return Storage.rename(oldPath, newPath); }

}  // namespace on_point
