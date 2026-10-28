#pragma once

// In-memory OnPointStorage with injected faults, FAT-like rename (never replaces), and a count of
// mutating calls, so tests can fail or "lose power" before any single write, remove or rename.

#include <map>
#include <set>
#include <string>

#include "activities/on_point/OnPointStore.h"

class FakeStorage : public on_point::OnPointStorage {
 public:
  std::map<std::string, std::string> files;
  int mutations = 0;                 // write, remove and rename calls made (including failed ones)
  int failAt = -1;                   // that mutation fails with no effect
  int powerLossAt = -1;              // that mutation and every later one have no effect (power lost)
  bool tornWrites = false;           // a failed or lost write leaves the first half of its data (a short write)
  bool closeFailures = false;        // a failed write leaves all its data but still reports failure
  std::set<std::string> unreadable;  // paths whose reads fail (present but unreadable)
  // From its Nth read of a path (0-based) on: return `result`, or, with Ok, `replacement`.
  struct ReadFault {
    int fromRead = 0;
    ReadResult result = ReadResult::Error;
    std::string replacement;
  };
  std::map<std::string, ReadFault> readFaults;
  std::map<std::string, int> reads;

  bool exists(const char* path) override { return files.count(path) != 0; }

  ReadResult read(const char* path, char* buffer, const size_t capacity, size_t& length) override {
    length = 0;
    const auto it = files.find(path);
    if (it == files.end()) return ReadResult::Missing;
    if (unreadable.count(path) != 0) return ReadResult::Error;
    const int n = reads[path]++;
    const auto fault = readFaults.find(path);
    if (fault != readFaults.end() && n >= fault->second.fromRead) {
      if (fault->second.result != ReadResult::Ok) return fault->second.result;
      const std::string& text = fault->second.replacement;
      if (text.size() > capacity) return ReadResult::TooLarge;
      text.copy(buffer, text.size());
      length = text.size();
      return ReadResult::Ok;
    }
    if (it->second.size() > capacity) return ReadResult::TooLarge;
    it->second.copy(buffer, it->second.size());
    length = it->second.size();
    return ReadResult::Ok;
  }

  bool write(const char* path, const char* data, const size_t length) override {
    if (!allow()) {
      if (closeFailures)
        files[path] = std::string(data, length);
      else if (tornWrites)
        files[path] = std::string(data, length / 2);
      return false;
    }
    files[path] = std::string(data, length);
    return true;
  }

  bool remove(const char* path) override { return allow() && files.erase(path) == 1; }

  bool rename(const char* oldPath, const char* newPath) override {
    if (!allow() || files.count(oldPath) == 0 || files.count(newPath) != 0) return false;
    files[newPath] = files[oldPath];
    files.erase(oldPath);
    return true;
  }

  void clearFaults() {
    failAt = -1;
    powerLossAt = -1;
    tornWrites = false;
    closeFailures = false;
  }

 private:
  bool allow() {
    const int index = mutations++;
    if (powerLossAt >= 0 && index >= powerLossAt) return false;
    return index != failAt;
  }
};
