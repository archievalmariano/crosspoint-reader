#pragma once

#include <cstddef>
#include <cstdint>

namespace on_point {

// "Not found" for collection lookups; never a valid index.
constexpr size_t NO_REGION = SIZE_MAX;
// Stored IDs: lower-case letters, digits and single hyphens, at most this long.
constexpr size_t MAX_REGION_ID_LENGTH = 31;

// A route collection: an index layer over the route catalogue, so a route may belong to
// several collections without duplicating its timetables. regionId is stable and
// independent of the display name and of table order.
struct Region {
  const char* regionId = nullptr;
  const uint8_t* routes = nullptr;  // catalogue route indices (routeAt)
  size_t routeCount = 0;
  const char* defaultRouteId = nullptr;  // shipped default; one of the members
};

// The collection table, in display order (OnPointRegionData.cpp, or the capacity fixture).
const Region* regionTable();
size_t regionTableSize();

size_t regionCount();
// nullptr when index >= regionCount().
const Region* regionAt(size_t index);
// NO_REGION for a null, empty or unknown ID.
size_t regionIndexForId(const char* regionId);

enum class RegionTableError : uint8_t {
  None,
  NoRegions,
  InvalidId,  // null, empty, too long or not lower-kebab
  DuplicateId,
  NoRoutes,           // a collection with no members (or a null member list)
  TooManyRoutes,      // more members than the catalogue can hold
  RouteOutOfRange,    // a member index >= catalogueRoutes
  DuplicateRoute,     // the same route twice in one collection
  DefaultNotMember,   // defaultRouteId null, unknown, or not one of its members
  RouteNotCovered,    // a catalogue route in no collection
  CatalogueTooLarge,  // more catalogue routes than MAX_CATALOGUE_ROUTES (the route list's bound)
};

struct RegionTableCheck {
  RegionTableError error = RegionTableError::None;
  size_t region = 0;  // offending collection (or catalogue route for RouteNotCovered)
  bool ok() const { return error == RegionTableError::None; }
};

bool isValidRegionId(const char* regionId);
// Checks a table against the linked route catalogue's first catalogueRoutes routes.
RegionTableCheck validateRegionTable(const Region* regions, size_t count, size_t catalogueRoutes);

}  // namespace on_point
