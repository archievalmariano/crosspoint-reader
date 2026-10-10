#pragma once

#include <cstdint>

// Which DOGEAR screen to show (pure, host-tested). The rules, in order:
//   1. An issue confirmed current this session (downloaded, or the SD copy the
//      server's manifest names): shown normally.
//   2. Otherwise the newest readable SD generation: shown marked CACHED. This
//      includes a server that has published nothing yet: a readable issue
//      always wins over a status screen.
//   3. Otherwise, with no readable issue anywhere, one status screen:
//      NO ISSUE YET           the server answered "nothing published";
//      NO ISSUE ON THIS DEVICE Wi-Fi is not connected;
//      CAN'T LOAD AN ISSUE     anything else (outage, TLS, clock, a bad issue).
namespace dogear_state {

enum class Screen : uint8_t { Issue, Cached, NoIssueYet, NoIssueOnDevice, CantLoad };

// What this session's /current.json fetch gave.
enum class Manifest : uint8_t { NotTried, Ok, NothingPublished, Failed };

inline Screen choose(bool wifiUp, Manifest manifest, bool networkIssueOk, bool readableCache) {
  if (networkIssueOk) return Screen::Issue;
  if (readableCache) return Screen::Cached;
  if (!wifiUp) return Screen::NoIssueOnDevice;
  if (manifest == Manifest::NothingPublished) return Screen::NoIssueYet;
  return Screen::CantLoad;
}

inline bool isStatus(Screen s) { return s != Screen::Issue && s != Screen::Cached; }

// A status screen's heading and note as i18n ids (StrId on the device; any
// enum with these members in the host tests). False for an issue screen.
template <typename Id>
bool statusCopy(Screen s, Id& heading, Id& note) {
  switch (s) {
    case Screen::NoIssueYet:
      heading = Id::STR_DOGEAR_NO_ISSUE_YET;
      note = Id::STR_DOGEAR_NO_ISSUE_YET_NOTE;
      return true;
    case Screen::NoIssueOnDevice:
      heading = Id::STR_DOGEAR_NO_ISSUE_ON_DEVICE;
      note = Id::STR_DOGEAR_NO_ISSUE_ON_DEVICE_NOTE;
      return true;
    case Screen::CantLoad:
      heading = Id::STR_DOGEAR_CANT_LOAD;
      note = Id::STR_DOGEAR_CANT_LOAD_NOTE;
      return true;
    case Screen::Issue:
    case Screen::Cached:
      return false;
  }
  return false;
}

}  // namespace dogear_state
