#pragma once

#include <string>
#include <vector>

// A single glanceable story = one 800x480 page in the GOTO reader.
//
// V1 shows source-derived excerpts, not an AI summary. excerptParagraphs holds
// the source article's leading body paragraphs as separate strings (boundaries
// are editorially meaningful); the E0.6 renderer draws only the FIRST one, but
// the array is kept for future compatibility. All fields are display-ready DATA
// prepared upstream; the device does not summarize, rank, or interpret.
struct GotoStory {
  std::string section;                         // placement label, e.g. "TOP STORY"
  std::string headline;                        // wrapped at render time
  std::vector<std::string> excerptParagraphs;  // 1-2 source paragraphs, in order
  std::string source;                          // e.g. "GMA News"
  std::string publishedAt;                     // display value, e.g. "8:30 AM"
  std::string url;                             // canonical article URL (future full-story handoff)
};

// A finite edition (GOTO = morning). label/dateline are display-ready DATA
// prepared upstream; the device only renders them, it does not format dates.
struct GotoEdition {
  std::string label;         // e.g. "GOTO"
  std::string dateIso;       // machine date, e.g. "2026-09-11"
  std::string dateline;      // human dateline, e.g. "Friday · 11 September"
  std::string companionUrl;  // hosted whole-edition companion page (from current.json; QR target)
  std::vector<GotoStory> stories;
};

// Parse an edition JSON payload into `out`. Returns true only when at least one
// story was parsed. Uses ArduinoJson; safe on malformed input (logs + false).
bool parseGotoEdition(const char* json, GotoEdition& out);

// Load the compiled-in fixture edition (E0.5 source of truth; no SD/network).
bool loadBuiltinGotoEdition(GotoEdition& out);
