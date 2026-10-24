#pragma once

#include <cstddef>
#include <cstdint>

#include "OnPointRegions.h"

namespace on_point {

// ON POINT's own preferences, /.crosspoint/onpoint.json (schema 1):
//   { "version": 1, "activeRegion": "<region ID>",
//     "defaults": { "<region ID>": "<route ID>", ... }, "legacyBaseline": "<route ID>" }
// IDs are stable strings, never catalogue positions. Stock settings.json is never written:
// SETTINGS.onPointMainRouteId is only read, for first-run migration and rollback reconciliation.
//
// Each file is Missing (confirmed absent), Unreadable (present but not readable: read error, over
// 4 KiB, or no memory to read it), Invalid (read, but not a store file), Current (schema 1) or
// Newer (a later schema). A save never deletes or overwrites a Newer or Unreadable file, and
// removes or replaces a file only while another validated copy exists:
//   1. Any Newer file: read-only. Any Unreadable file: the save is refused, nothing touched.
//   2. A Current tmp with no Current live file (a recovered copy) is promoted to live first.
//   3. The new content is written to tmp (now Missing or Invalid), read back and validated.
//   4. A Current live file becomes the backup (replacing only a Missing or Current backup; an
//      Invalid backup is kept and the live file removed instead); an Invalid live file is removed.
//   5. tmp becomes live.
// Loading uses live, then tmp (live Missing or Invalid), then the backup, else migrates, and
// never writes. Power-loss recovery is best-effort: FAT32 has no journal, so an interruption
// inside a filesystem operation, or media corruption, can still lose a copy.

constexpr char ON_POINT_STORE_PATH[] = "/.crosspoint/onpoint.json";
constexpr char ON_POINT_STORE_TMP_PATH[] = "/.crosspoint/onpoint.tmp";
constexpr char ON_POINT_STORE_BAK_PATH[] = "/.crosspoint/onpoint.bak";

constexpr int ON_POINT_STORE_VERSION = 1;
// Files larger than this are never parsed, merged or overwritten (the store goes read-only).
constexpr size_t ON_POINT_STORE_MAX_BYTES = 4096;
// The most "defaults" entries written: this firmware's collections first, then unknown ones as found.
constexpr size_t ON_POINT_STORE_MAX_DEFAULTS = 16;
// Stored IDs (route IDs fit CrossPointSettings::onPointMainRouteId, char[48]).
constexpr size_t ON_POINT_STORED_ID_CAPACITY = 48;

// The SD calls the store needs, so the protocol can be tested with injected faults.
class OnPointStorage {
 public:
  enum class ReadResult : uint8_t { Ok, Missing, TooLarge, Error };

  virtual ~OnPointStorage() = default;
  virtual bool exists(const char* path) = 0;
  // Reads the whole file into buffer (at most capacity bytes).
  virtual ReadResult read(const char* path, char* buffer, size_t capacity, size_t& length) = 0;
  // Creates or truncates path and writes data; false if any byte wasn't written.
  virtual bool write(const char* path, const char* data, size_t length) = 0;
  virtual bool remove(const char* path) = 0;
  // Fails if newPath exists (FAT rename never replaces).
  virtual bool rename(const char* oldPath, const char* newPath) = 0;
};

class OnPointStore {
 public:
  enum class Source : uint8_t { Live, Temporary, Backup, Migrated };
  enum class SaveOutcome : uint8_t { Saved, ReadOnly, Failed };

  explicit OnPointStore(OnPointStorage& storage) : storage(storage) {}

  // Loads preferences (recovery order: live, then tmp if live is missing, then backup; else
  // migration from the legacy main route), then applies rollback reconciliation. Never writes.
  void load(const char* legacyMainRouteId, size_t legacyCapacity);

  // Explicit user changes: applied in memory, then saved. A failed or read-only save keeps the
  // change for this session only (hasUnsavedChanges()).
  SaveOutcome setActiveRegion(size_t regionIndex);
  SaveOutcome setDefaultRoute(size_t regionIndex, const char* routeId);

  // The collection to show: the stored one, or the first when it's unknown (the stored ID is
  // kept until the user picks a collection).
  size_t activeRegionIndex() const;
  bool activeRegionKnown() const { return regionIndexForId(activeRegion) != NO_REGION; }
  // The stored default for a collection, or its shipped default; nullptr for a bad index.
  // May be empty or unknown (the no-main state); callers resolve it with resolveSavedRoute.
  const char* defaultRouteId(size_t regionIndex) const;

  Source source() const { return loadedFrom; }
  // The live file is a newer schema or unreadable (or a newer copy was found): nothing is written.
  bool readOnly() const { return readOnlyFile; }
  // Some change made this session isn't on the card (read-only store or a failed save).
  bool hasUnsavedChanges() const { return unsaved; }
  // A main route changed under older firmware was taken into its collection at load.
  bool reconciledLegacyRoute() const { return reconciled; }

 private:
  enum class FileState : uint8_t { Missing, Unreadable, Invalid, Current, Newer };

  void reset();
  FileState readFile(const char* path, bool applyFields);
  void migrate(const char* legacy);
  void reconcile(const char* legacy);
  SaveOutcome save();
  size_t serialize(char* out, size_t capacity, const char* base, size_t baseLength) const;
  bool matchesMemory(const char* data, size_t length) const;

  OnPointStorage& storage;
  char activeRegion[ON_POINT_STORED_ID_CAPACITY] = "";
  // By this firmware's collection index (regionAt).
  char defaults[ON_POINT_STORE_MAX_DEFAULTS][ON_POINT_STORED_ID_CAPACITY] = {};
  bool hasDefault[ON_POINT_STORE_MAX_DEFAULTS] = {};
  char legacyBaseline[ON_POINT_STORED_ID_CAPACITY] = "";
  bool hasBaseline = false;
  Source loadedFrom = Source::Migrated;
  bool readOnlyFile = false;
  bool unsaved = false;
  bool reconciled = false;
};

}  // namespace on_point
