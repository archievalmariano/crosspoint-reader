#pragma once

#include <cstdio>
#include <cstring>

// Pure OTA version comparison (no device deps, host-testable).
//
// Versions are compared on their leading MAJOR.MINOR.PATCH, with an optional
// leading 'v' (release tags are often "v1.2.3"). A string that does not start
// with three numeric segments is unparseable, and an unparseable version never
// counts as an update: offering a download on garbage input could replace this
// firmware with an unrelated build.
namespace ota_version {

inline bool parse(const char* version, int& major, int& minor, int& patch) {
  if (version == nullptr) return false;
  if (*version == 'v' || *version == 'V') ++version;
  return std::sscanf(version, "%d.%d.%d", &major, &minor, &patch) == 3;
}

// True when `latest` is strictly newer than `current`. When the three
// segments are equal, a current release candidate ("-rc") is treated as older
// than the final release.
inline bool isNewer(const char* latest, const char* current) {
  int latestMajor = 0, latestMinor = 0, latestPatch = 0;
  int currentMajor = 0, currentMinor = 0, currentPatch = 0;
  if (!parse(latest, latestMajor, latestMinor, latestPatch)) return false;
  if (!parse(current, currentMajor, currentMinor, currentPatch)) return false;

  if (latestMajor != currentMajor) return latestMajor > currentMajor;
  if (latestMinor != currentMinor) return latestMinor > currentMinor;
  if (latestPatch != currentPatch) return latestPatch > currentPatch;
  return std::strstr(current, "-rc") != nullptr;
}

}  // namespace ota_version
