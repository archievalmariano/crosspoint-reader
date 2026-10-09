// Compiled only into builds that enable DOGEAR (see [dogear] in platformio.ini).
#ifdef DOGEAR_ENABLED

#include "DogearIssue.h"

#include <ArduinoJson.h>
#include <Logging.h>

#include <cstring>
#include <utility>

#include "DogearBuiltinIssue.generated.h"
#include "DogearLimits.h"

namespace {

// A required string within `cap` bytes; nullptr when missing-and-required or over cap.
const char* field(JsonVariantConst v, size_t cap, bool required) {
  const char* s = v | "";
  if (strlen(s) > cap || (required && s[0] == '\0')) return nullptr;
  return s;
}

bool parseCta(JsonObjectConst c, DogearCta& out) {
  const char* label = field(c["label"], dogear_limits::kMaxShortLen, true);
  const char* title = field(c["title"], dogear_limits::kMaxLineLen, true);
  const char* author = field(c["author"], dogear_limits::kMaxLineLen, false);
  const char* edition = field(c["edition"], dogear_limits::kMaxLineLen, false);
  const char* note = field(c["note"], dogear_limits::kMaxLineLen, false);
  const char* url = field(c["url"], dogear_limits::kMaxUrlLen, true);
  if (!label || !title || !author || !edition || !note || !url) return false;
  out.label = label;
  out.title = title;
  out.author = author;
  out.edition = edition;
  out.note = note;
  out.url = url;
  out.representative = c["representative"] | false;
  return dogear_limits::isQrUrl(out.url);
}

}  // namespace

bool parseDogearIssue(const char* json, DogearIssue& out) {
  JsonDocument doc;
  const DeserializationError err = deserializeJson(doc, json);
  if (err) {
    LOG_ERR("DOGEAR", "issue parse failed: %s", err.c_str());
    return false;
  }
  using namespace dogear_limits;
  const int schema = doc["schemaVersion"] | 0;
  if (schema < static_cast<int>(kIssueSchemaMin) || schema > static_cast<int>(kIssueSchemaMax)) {
    LOG_ERR("DOGEAR", "unsupported issue schema %d", schema);
    return false;
  }
  // Schema 3 is a quiet week and must carry exactly {heading, note}; schema 2 must
  // carry no quiet copy at all. Anything else is rejected, never repaired.
  JsonObjectConst quiet = doc["quiet"].as<JsonObjectConst>();
  const char* quietHeading = nullptr;
  const char* quietNote = nullptr;
  if (schema == 3) {
    if (quiet.isNull() || quiet.size() != 2) {
      LOG_ERR("DOGEAR", "quiet issue without exactly {heading, note}; rejecting");
      return false;
    }
    quietHeading = field(quiet["heading"], kMaxShortLen, true);
    quietNote = field(quiet["note"], kMaxLineLen, true);
    if (!quietHeading || !quietNote) {
      LOG_ERR("DOGEAR", "quiet heading or note missing or over cap; rejecting");
      return false;
    }
    // The approved copy exactly: the device shows it as data, but no other text.
    if (strcmp(quietHeading, kQuietHeading) != 0 || strcmp(quietNote, kQuietNote) != 0) {
      LOG_ERR("DOGEAR", "quiet copy is not the approved heading and note; rejecting");
      return false;
    }
  } else if (!doc["quiet"].isNull()) {
    LOG_ERR("DOGEAR", "a regular issue carries quiet copy; rejecting");
    return false;
  }

  const char* issueId = field(doc["issueId"], kMaxIssueIdLen, true);
  const char* label = field(doc["label"], kMaxShortLen, true);
  const char* strapline = field(doc["strapline"], kMaxLineLen, false);
  const char* dateline = field(doc["dateline"], kMaxShortLen, true);
  const char* datelineShort = field(doc["datelineShort"], kMaxShortLen, true);
  const char* nextIssue = field(doc["nextIssue"], kMaxLineLen, true);
  const char* editionUrl = field(doc["editionUrl"], kMaxUrlLen, false);
  if (!issueId || !label || !strapline || !dateline || !datelineShort || !nextIssue || !editionUrl ||
      !isSafeIssueId(issueId)) {
    LOG_ERR("DOGEAR", "issue metadata missing or over cap; rejecting");
    return false;
  }

  JsonArrayConst entries = doc["entries"].as<JsonArrayConst>();
  if (entries.isNull() || entries.size() == 0 || entries.size() > kMaxEntries) {
    LOG_ERR("DOGEAR", "issue has %u entries; rejecting", static_cast<unsigned>(entries.size()));
    return false;
  }
  if (schema == 3) {
    // The quiet shape: 3-5 items; exactly one featured item, first; every other item
    // standard. Any other or missing role is not a quiet issue.
    size_t featured = 0;
    bool seenStandard = false;
    bool shapeOk = entries.size() >= kQuietMinEntries && entries.size() <= kQuietMaxEntries;
    for (const JsonObjectConst e : entries) {
      const char* role = e["role"] | "";
      if (strcmp(role, "featured") == 0) {
        shapeOk = shapeOk && !seenStandard;
        ++featured;
      } else if (strcmp(role, "standard") == 0) {
        seenStandard = true;
      } else {
        shapeOk = false;
      }
    }
    if (!shapeOk || featured != kQuietFeatured) {
      LOG_ERR("DOGEAR", "quiet issue shape invalid (%u items, %u featured); rejecting",
              static_cast<unsigned>(entries.size()), static_cast<unsigned>(featured));
      return false;
    }
  }

  DogearIssue issue;
  issue.issueId = issueId;
  issue.preview = doc["preview"] | false;
  issue.label = label;
  issue.strapline = strapline;
  issue.dateline = dateline;
  issue.datelineShort = datelineShort;
  issue.nextIssue = nextIssue;
  issue.editionUrl = editionUrl;
  issue.quiet = schema == 3;
  if (issue.quiet) {
    issue.quietHeading = quietHeading;
    issue.quietNote = quietNote;
  }
  issue.entries.reserve(entries.size());  // bounded by kMaxEntries above
  for (const JsonObjectConst e : entries) {
    DogearEntry entry;
    const char* id = field(e["id"], kMaxShortLen, true);
    const char* dateLabel = field(e["dateLabel"], kMaxShortLen, true);
    const char* kicker = field(e["kicker"], kMaxShortLen, true);
    const char* contentsLabel = field(e["contentsLabel"], kMaxShortLen, false);
    const char* title = field(e["title"], kMaxLineLen, true);
    const char* headline = field(e["headline"], kMaxHeadlineLen, true);
    const char* body = field(e["body"], kMaxBodyLen, true);
    if (!id || !dateLabel || !kicker || !contentsLabel || !title || !headline || !body) {
      LOG_ERR("DOGEAR", "entry field missing or over cap; rejecting issue");
      return false;
    }
    entry.id = id;
    entry.featured = strcmp(e["role"] | "", "featured") == 0;
    entry.dateLabel = dateLabel;
    entry.kicker = kicker;
    entry.contentsLabel = contentsLabel;
    entry.title = title;
    entry.headline = headline;
    entry.body = body;
    entry.secondPage = e["secondPage"] | false;
    JsonObjectConst cta = e["cta"].as<JsonObjectConst>();
    if (!cta.isNull()) {
      if (!parseCta(cta, entry.cta)) {
        LOG_ERR("DOGEAR", "entry %s has an invalid reading link; rejecting issue", id);
        return false;
      }
      entry.hasCta = true;
    }
    issue.entries.push_back(std::move(entry));
  }
  out = std::move(issue);
  return true;
}

bool parseVerifiedIssue(const std::string& verifiedBytes, const std::string& expectedIssueId, DogearIssue& out) {
  DogearIssue issue;
  if (!parseDogearIssue(verifiedBytes.c_str(), issue)) return false;
  if (issue.issueId != expectedIssueId) {
    LOG_ERR("DOGEAR", "issue %s does not match its manifest (%s)", issue.issueId.c_str(), expectedIssueId.c_str());
    return false;
  }
  out = std::move(issue);
  return true;
}

bool loadBuiltinDogearIssue(DogearIssue& out) { return parseDogearIssue(kDogearBuiltinIssueJson, out); }

#endif  // DOGEAR_ENABLED
