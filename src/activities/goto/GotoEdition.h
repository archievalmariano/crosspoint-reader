#pragma once

#include <string>
#include <vector>

// A single glanceable story = one 800x480 page in the GOTO reader.
struct GotoStory {
  std::string section;   // kicker, e.g. "TOP STORY"
  std::string headline;  // one-line-ish headline (wrapped at render time)
  std::string dek;       // one-sentence explainer
};

// A finite edition (GOTO = morning). label/dateline are display-ready DATA
// prepared upstream; the device only renders them, it does not format dates.
struct GotoEdition {
  std::string label;     // e.g. "GOTO"
  std::string dateIso;   // machine date, e.g. "2026-09-11"
  std::string dateline;  // human dateline, e.g. "Friday · 11 September"
  std::vector<GotoStory> stories;
};

// Parse an edition JSON payload into `out`. Returns true only when at least one
// story was parsed. Uses ArduinoJson; safe on malformed input (logs + false).
bool parseGotoEdition(const char* json, GotoEdition& out);

// Load the compiled-in fixture edition (E0 source of truth; no SD/network).
bool loadBuiltinGotoEdition(GotoEdition& out);
