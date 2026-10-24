#include "OnPointStore.h"

#include <ArduinoJson.h>
#include <Logging.h>
#include <Memory.h>

#include <cstring>

#include "OnPointRouteResolution.h"
#include "OnPointScheduleData.h"

namespace on_point {
namespace {

// Copies a NUL-terminated ID that fits; false (destination untouched) otherwise.
bool copyId(char (&destination)[ON_POINT_STORED_ID_CAPACITY], const char* source) {
  if (source == nullptr) return false;
  const size_t length = strnlen(source, ON_POINT_STORED_ID_CAPACITY);
  if (length >= ON_POINT_STORED_ID_CAPACITY) return false;
  memcpy(destination, source, length);
  destination[length] = '\0';
  return true;
}

// The first collection (table order) that lists a catalogue route.
size_t firstRegionWithRoute(const size_t route) {
  for (size_t r = 0; r < regionCount(); ++r) {
    const Region& region = *regionAt(r);
    for (size_t i = 0; i < region.routeCount; ++i)
      if (region.routes[i] == route) return r;
  }
  return NO_REGION;
}

// A legacy main-route value resolved to a catalogue route; false for Unset or Unknown.
bool resolveLegacy(const char* legacy, const size_t capacity, size_t& route) {
  if (legacy == nullptr) return false;
  const RouteResolution resolution = resolveSavedRoute(legacy, capacity, productionAliases());
  if (!resolution.hasRoute()) return false;
  route = resolution.index;
  return true;
}

// The schema version, or 0 when the document isn't a store file.
int schemaVersion(const JsonDocument& doc) {
  if (!doc.is<JsonObjectConst>() || !doc["version"].is<int>()) return 0;
  const int version = doc["version"].as<int>();
  return version >= 1 ? version : 0;
}

}  // namespace

OnPointStore::FileState OnPointStore::readFile(const char* path, const bool applyFields) {
  // Transient: freed on return. Files over the bound are never parsed.
  auto buffer = makeUniqueNoThrow<char[]>(ON_POINT_STORE_MAX_BYTES);
  if (!buffer) {
    LOG_ERR("ONPT", "OOM: %u bytes to read %s", static_cast<unsigned>(ON_POINT_STORE_MAX_BYTES), path);
    return storage.exists(path) ? FileState::Unreadable : FileState::Missing;
  }
  size_t length = 0;
  switch (storage.read(path, buffer.get(), ON_POINT_STORE_MAX_BYTES, length)) {
    case OnPointStorage::ReadResult::Missing:
      return FileState::Missing;
    case OnPointStorage::ReadResult::TooLarge:
    case OnPointStorage::ReadResult::Error:
      return FileState::Unreadable;
    case OnPointStorage::ReadResult::Ok:
      break;
  }

  // const input: ArduinoJson copies strings instead of parsing in place.
  JsonDocument doc;
  if (deserializeJson(doc, static_cast<const char*>(buffer.get()), length)) return FileState::Invalid;
  const int version = schemaVersion(doc);
  if (version == 0) return FileState::Invalid;

  if (applyFields) {
    // A newer schema is read only where its fields keep their schema-1 types.
    if (doc["activeRegion"].is<const char*>() && !copyId(activeRegion, doc["activeRegion"].as<const char*>()))
      LOG_INF("ONPT", "%s: activeRegion too long; ignored", path);
    if (doc["defaults"].is<JsonObjectConst>()) {
      for (const JsonPairConst entry : doc["defaults"].as<JsonObjectConst>()) {
        const size_t region = regionIndexForId(entry.key().c_str());
        if (region == NO_REGION || region >= ON_POINT_STORE_MAX_DEFAULTS || !entry.value().is<const char*>()) continue;
        hasDefault[region] = copyId(defaults[region], entry.value().as<const char*>());
      }
    }
    if (doc["legacyBaseline"].is<const char*>())
      hasBaseline = copyId(legacyBaseline, doc["legacyBaseline"].as<const char*>());
  }
  return version > ON_POINT_STORE_VERSION ? FileState::Newer : FileState::Current;
}

void OnPointStore::reset() {
  activeRegion[0] = '\0';
  for (size_t r = 0; r < ON_POINT_STORE_MAX_DEFAULTS; ++r) {
    defaults[r][0] = '\0';
    hasDefault[r] = false;
  }
  legacyBaseline[0] = '\0';
  hasBaseline = false;
  loadedFrom = Source::Migrated;
  readOnlyFile = false;
  unsaved = false;
  reconciled = false;
}

void OnPointStore::load(const char* legacyMainRouteId, const size_t legacyCapacity) {
  reset();
  // Never read past the caller's buffer.
  char legacy[ON_POINT_STORED_ID_CAPACITY] = "";
  if (legacyMainRouteId != nullptr) {
    const size_t limit = legacyCapacity < ON_POINT_STORED_ID_CAPACITY ? legacyCapacity : ON_POINT_STORED_ID_CAPACITY;
    const size_t length = strnlen(legacyMainRouteId, limit);
    if (length < limit) memcpy(legacy, legacyMainRouteId, length + 1);
  }

  // Classify first; fields are applied only from the copy chosen.
  const FileState live = readFile(ON_POINT_STORE_PATH, false);
  if (live == FileState::Unreadable) {
    LOG_INF("ONPT", "%s is unreadable or over %u bytes: read-only", ON_POINT_STORE_PATH,
            static_cast<unsigned>(ON_POINT_STORE_MAX_BYTES));
    readOnlyFile = true;
  }
  const char* chosenPath = nullptr;
  FileState chosen = FileState::Missing;
  if (live == FileState::Current || live == FileState::Newer) {
    chosenPath = ON_POINT_STORE_PATH;
    chosen = live;
    loadedFrom = Source::Live;
  } else {
    if (live == FileState::Missing || live == FileState::Invalid) {
      chosen = readFile(ON_POINT_STORE_TMP_PATH, false);
      chosenPath = ON_POINT_STORE_TMP_PATH;
      loadedFrom = Source::Temporary;
    }
    if (chosen != FileState::Current && chosen != FileState::Newer) {
      chosen = readFile(ON_POINT_STORE_BAK_PATH, false);
      chosenPath = ON_POINT_STORE_BAK_PATH;
      loadedFrom = Source::Backup;
    }
  }
  if (chosen != FileState::Current && chosen != FileState::Newer) {
    migrate(legacy);
    return;
  }
  const Source source = loadedFrom;
  if (chosen == FileState::Newer) {
    // Start from migration, then use every schema-1 field the newer file carries with its
    // schema-1 type; nothing is ever written back to it.
    LOG_INF("ONPT", "%s uses a newer schema: read-only", chosenPath);
    readOnlyFile = true;
    migrate(legacy);
  }
  if (readFile(chosenPath, true) != chosen) {  // changed between reads: start over from migration
    migrate(legacy);
    return;
  }
  loadedFrom = source;
  if (source != Source::Live) LOG_INF("ONPT", "Recovered preferences from %s", chosenPath);
  reconcile(legacy);
}

void OnPointStore::migrate(const char* legacy) {
  loadedFrom = Source::Migrated;
  for (size_t r = 0; r < ON_POINT_STORE_MAX_DEFAULTS; ++r) hasDefault[r] = false;
  hasBaseline = copyId(legacyBaseline, legacy);
  if (regionCount() == 0) return;

  size_t route = 0;
  const size_t region =
      resolveLegacy(legacy, ON_POINT_STORED_ID_CAPACITY, route) ? firstRegionWithRoute(route) : NO_REGION;
  if (region != NO_REGION && region < ON_POINT_STORE_MAX_DEFAULTS) {
    copyId(activeRegion, regionAt(region)->regionId);
    hasDefault[region] = copyId(defaults[region], routeAt(route).routeId);
    return;
  }
  // Empty or unknown: the first collection, keeping the value (the no-main state).
  copyId(activeRegion, regionAt(0)->regionId);
  hasDefault[0] = copyId(defaults[0], legacy);
}

void OnPointStore::reconcile(const char* legacy) {
  if (!hasBaseline || strncmp(legacy, legacyBaseline, ON_POINT_STORED_ID_CAPACITY) == 0) return;
  size_t route = 0;
  if (!resolveLegacy(legacy, ON_POINT_STORED_ID_CAPACITY, route)) return;
  const size_t region = firstRegionWithRoute(route);
  if (region == NO_REGION || region >= ON_POINT_STORE_MAX_DEFAULTS) return;
  // Persisted with the next user change, which also records the new baseline.
  copyId(activeRegion, regionAt(region)->regionId);
  hasDefault[region] = copyId(defaults[region], routeAt(route).routeId);
  copyId(legacyBaseline, legacy);
  reconciled = true;
  LOG_INF("ONPT", "Main route changed under older firmware: now the %s default", regionAt(region)->regionId);
}

size_t OnPointStore::activeRegionIndex() const {
  const size_t region = regionIndexForId(activeRegion);
  return region == NO_REGION ? 0 : region;
}

const char* OnPointStore::defaultRouteId(const size_t regionIndex) const {
  const Region* region = regionAt(regionIndex);
  if (region == nullptr) return nullptr;
  if (regionIndex < ON_POINT_STORE_MAX_DEFAULTS && hasDefault[regionIndex]) return defaults[regionIndex];
  return region->defaultRouteId;
}

OnPointStore::SaveOutcome OnPointStore::setActiveRegion(const size_t regionIndex) {
  const Region* region = regionAt(regionIndex);
  if (region == nullptr) return SaveOutcome::Failed;
  copyId(activeRegion, region->regionId);
  return save();
}

OnPointStore::SaveOutcome OnPointStore::setDefaultRoute(const size_t regionIndex, const char* routeId) {
  if (regionAt(regionIndex) == nullptr || regionIndex >= ON_POINT_STORE_MAX_DEFAULTS) return SaveOutcome::Failed;
  char value[ON_POINT_STORED_ID_CAPACITY];
  if (!copyId(value, routeId)) return SaveOutcome::Failed;
  memcpy(defaults[regionIndex], value, sizeof(value));
  hasDefault[regionIndex] = true;
  return save();
}

size_t OnPointStore::serialize(char* out, const size_t capacity, const char* base, const size_t baseLength) const {
  JsonDocument doc;
  // Keep the live file's unknown keys and other collections' entries (schema 1 only).
  const bool merged = base != nullptr && !deserializeJson(doc, base, baseLength) && schemaVersion(doc) == 1;
  if (!merged) doc.to<JsonObject>();

  for (int pass = 0; pass < 2; ++pass) {
    doc["version"] = ON_POINT_STORE_VERSION;
    doc["activeRegion"] = activeRegion;
    if (hasBaseline) doc["legacyBaseline"] = legacyBaseline;
    JsonObject entries =
        doc["defaults"].is<JsonObject>() ? doc["defaults"].as<JsonObject>() : doc["defaults"].to<JsonObject>();
    for (size_t r = 0; r < regionCount() && r < ON_POINT_STORE_MAX_DEFAULTS; ++r)
      if (hasDefault[r]) entries[regionAt(r)->regionId] = defaults[r];

    // Over the entry bound: drop unknown collections' entries, last first.
    while (entries.size() > ON_POINT_STORE_MAX_DEFAULTS) {
      JsonObject::iterator dropped = entries.end();
      for (JsonObject::iterator it = entries.begin(); it != entries.end(); ++it)
        if (regionIndexForId(it->key().c_str()) == NO_REGION) dropped = it;
      if (dropped == entries.end()) break;
      LOG_INF("ONPT", "Dropping a stored default for an unknown collection (over %u entries)",
              static_cast<unsigned>(ON_POINT_STORE_MAX_DEFAULTS));
      entries.remove(dropped);
    }

    if (measureJson(doc) < capacity) return serializeJson(doc, out, capacity);
    // Too large with what was kept: write this firmware's fields only.
    LOG_INF("ONPT", "Preferences over %u bytes: unknown content not kept", static_cast<unsigned>(capacity));
    doc.clear();
    doc.to<JsonObject>();
  }
  return 0;
}

bool OnPointStore::matchesMemory(const char* data, const size_t length) const {
  JsonDocument doc;
  if (deserializeJson(doc, data, length) || schemaVersion(doc) != ON_POINT_STORE_VERSION) return false;
  const char* active = doc["activeRegion"].as<const char*>();
  if (active == nullptr || strcmp(active, activeRegion) != 0) return false;
  if (hasBaseline) {
    const char* baseline = doc["legacyBaseline"].as<const char*>();
    if (baseline == nullptr || strcmp(baseline, legacyBaseline) != 0) return false;
  }
  for (size_t r = 0; r < regionCount() && r < ON_POINT_STORE_MAX_DEFAULTS; ++r) {
    if (!hasDefault[r]) continue;
    const char* stored = doc["defaults"][regionAt(r)->regionId].as<const char*>();
    if (stored == nullptr || strcmp(stored, defaults[r]) != 0) return false;
  }
  return true;
}

OnPointStore::SaveOutcome OnPointStore::save() {
  if (readOnlyFile) {
    unsaved = true;
    return SaveOutcome::ReadOnly;
  }
  const auto fail = [this]([[maybe_unused]] const char* step) {
    LOG_ERR("ONPT", "Saving preferences failed at %s; kept for this session", step);
    unsaved = true;
    return SaveOutcome::Failed;
  };

  // Preflight every copy: a newer one is never touched; an unreadable one can't be judged.
  FileState live = readFile(ON_POINT_STORE_PATH, false);
  FileState tmp = readFile(ON_POINT_STORE_TMP_PATH, false);
  const FileState bak = readFile(ON_POINT_STORE_BAK_PATH, false);
  if (live == FileState::Newer || tmp == FileState::Newer || bak == FileState::Newer) {
    readOnlyFile = true;
    unsaved = true;
    return SaveOutcome::ReadOnly;
  }
  if (live == FileState::Unreadable || tmp == FileState::Unreadable || bak == FileState::Unreadable)
    return fail("preflight (unreadable copy)");

  // A recovered tmp may be the only copy: make it live before tmp is reused.
  if (tmp == FileState::Current && live != FileState::Current) {
    if (live == FileState::Invalid && !storage.remove(ON_POINT_STORE_PATH)) return fail("remove invalid live file");
    if (!storage.rename(ON_POINT_STORE_TMP_PATH, ON_POINT_STORE_PATH)) return fail("promote recovered tmp");
    live = FileState::Current;
    tmp = FileState::Missing;
  }

  // Transient: one buffer for the live file (the merge base), the new content and the read-back.
  auto buffer = makeUniqueNoThrow<char[]>(ON_POINT_STORE_MAX_BYTES);
  if (!buffer) return fail("allocation");
  size_t liveLength = 0;
  const bool merge = live == FileState::Current &&
                     storage.read(ON_POINT_STORE_PATH, buffer.get(), ON_POINT_STORE_MAX_BYTES, liveLength) ==
                         OnPointStorage::ReadResult::Ok;
  const size_t length =
      serialize(buffer.get(), ON_POINT_STORE_MAX_BYTES, merge ? buffer.get() : nullptr, merge ? liveLength : 0);
  if (length == 0) return fail("serialise");

  // tmp is now Missing or Invalid, and never the only copy.
  if (!storage.write(ON_POINT_STORE_TMP_PATH, buffer.get(), length)) {
    storage.remove(ON_POINT_STORE_TMP_PATH);
    return fail("write");
  }
  size_t readBack = 0;
  if (storage.read(ON_POINT_STORE_TMP_PATH, buffer.get(), ON_POINT_STORE_MAX_BYTES, readBack) !=
          OnPointStorage::ReadResult::Ok ||
      !matchesMemory(buffer.get(), readBack)) {
    storage.remove(ON_POINT_STORE_TMP_PATH);
    return fail("read-back");
  }

  // tmp is a validated copy from here on, so the live file may be moved or removed.
  if (live == FileState::Current) {
    if (bak == FileState::Missing || bak == FileState::Current) {
      if (bak == FileState::Current && !storage.remove(ON_POINT_STORE_BAK_PATH)) return fail("remove backup");
      if (!storage.rename(ON_POINT_STORE_PATH, ON_POINT_STORE_BAK_PATH)) return fail("back up live file");
    } else if (!storage.remove(ON_POINT_STORE_PATH)) {  // an Invalid backup is kept, unreplaced
      return fail("remove live file");
    }
  } else if (live == FileState::Invalid && !storage.remove(ON_POINT_STORE_PATH)) {
    return fail("remove invalid live file");
  }
  if (!storage.rename(ON_POINT_STORE_TMP_PATH, ON_POINT_STORE_PATH)) return fail("install");
  unsaved = false;
  return SaveOutcome::Saved;
}

}  // namespace on_point
