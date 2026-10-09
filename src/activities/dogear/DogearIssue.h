#pragma once

#include <string>
#include <vector>

// One DOGEAR weekly issue, as the selector writes it (issue schema v2, or v3 for a
// quiet week). Every
// string is display-ready DATA prepared upstream (typeset quotes, uppercase
// kickers, "1 Jan" item dates, "Monday 4 January 2027"); the device lays it out
// and never formats, selects or edits.

// A secondary reading view: READ ONLINE / FIND THE BOOK, opened from its item.
struct DogearCta {
  std::string label;  // "READ ONLINE"
  std::string title;  // the work that opens
  std::string author;
  std::string edition;  // optional: "The 1818 text"
  std::string note;     // "Free on Project Gutenberg"
  std::string url;
  bool representative = false;  // an editor-chosen representative work
};

struct DogearEntry {
  std::string id;
  bool featured = false;      // the lead item
  std::string dateLabel;      // "30 Dec" (no weekday)
  std::string kicker;         // "DIED 1896"
  std::string contentsLabel;  // "DIED", "OBSERVANCE · PH" (cover rows; falls back to kicker)
  std::string title;          // contents title
  std::string headline;
  std::string body;
  bool secondPage = false;  // the editor allows a second screen
  bool hasCta = false;
  DogearCta cta;
};

struct DogearIssue {
  std::string issueId;   // "dogear-2026-11-23"
  bool preview = false;  // a proof built from records awaiting approval
  std::string label;     // "DOGEAR"
  std::string strapline;
  std::string dateline;       // "23–29 NOVEMBER 2026"
  std::string datelineShort;  // "23–29 NOV 2026"
  std::string nextIssue;      // "Monday 30 November 2026"
  std::string editionUrl;     // the final page's QR
  // A quiet week (schema 3): an empty week's issue of earlier "Also this week"
  // mentions. The cover shows this heading and note as data, never device copy.
  bool quiet = false;
  std::string quietHeading;  // "SOME QUIET THIS WEEK"
  std::string quietNote;     // one sentence or two, wrapped on the cover
  std::vector<DogearEntry> entries;
};

// Parse an issue payload. True only for a complete, in-bounds issue with at
// least one item; anything over a DogearLimits cap is rejected, not truncated.
bool parseDogearIssue(const char* json, DogearIssue& out);

// Parse bytes whose SHA-256 the caller has already checked, and require the
// issue to be the one its manifest names.
bool parseVerifiedIssue(const std::string& verifiedBytes, const std::string& expectedIssueId, DogearIssue& out);

// The compiled-in sample issue (used only with no network and no SD cache).
bool loadBuiltinDogearIssue(DogearIssue& out);
