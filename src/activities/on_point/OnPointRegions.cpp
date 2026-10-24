#include "OnPointRegions.h"

#include <cstring>

#include "OnPointRouteList.h"
#include "OnPointScheduleData.h"

namespace on_point {
namespace {

// One bit per catalogue route.
struct RouteSet {
  uint8_t bits[MAX_CATALOGUE_ROUTES / 8] = {};
  bool has(const size_t route) const { return (bits[route / 8] >> (route % 8)) & 1u; }
  void add(const size_t route) { bits[route / 8] |= static_cast<uint8_t>(1u << (route % 8)); }
};

}  // namespace

size_t regionCount() { return regionTableSize(); }

const Region* regionAt(const size_t index) { return index < regionTableSize() ? &regionTable()[index] : nullptr; }

size_t regionIndexForId(const char* regionId) {
  if (regionId == nullptr || regionId[0] == '\0') return NO_REGION;
  for (size_t i = 0; i < regionTableSize(); ++i) {
    if (strcmp(regionTable()[i].regionId, regionId) == 0) return i;
  }
  return NO_REGION;
}

bool isValidRegionId(const char* regionId) {
  if (regionId == nullptr) return false;
  const size_t length = strnlen(regionId, MAX_REGION_ID_LENGTH + 1);
  if (length == 0 || length > MAX_REGION_ID_LENGTH || regionId[0] == '-' || regionId[length - 1] == '-') return false;
  for (size_t i = 0; i < length; ++i) {
    const char c = regionId[i];
    const bool allowed = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-';
    if (!allowed || (c == '-' && regionId[i + 1] == '-')) return false;
  }
  return true;
}

RegionTableCheck validateRegionTable(const Region* regions, const size_t count, const size_t catalogueRoutes) {
  if (regions == nullptr || count == 0) return {RegionTableError::NoRegions, 0};
  RouteSet covered;
  for (size_t r = 0; r < count; ++r) {
    const Region& region = regions[r];
    if (!isValidRegionId(region.regionId)) return {RegionTableError::InvalidId, r};
    for (size_t earlier = 0; earlier < r; ++earlier)
      if (strcmp(regions[earlier].regionId, region.regionId) == 0) return {RegionTableError::DuplicateId, r};
    if (region.routes == nullptr || region.routeCount == 0) return {RegionTableError::NoRoutes, r};
    if (region.routeCount > MAX_CATALOGUE_ROUTES) return {RegionTableError::TooManyRoutes, r};

    RouteSet member;
    bool defaultFound = false;
    for (size_t m = 0; m < region.routeCount; ++m) {
      const size_t route = region.routes[m];
      if (route >= catalogueRoutes || route >= MAX_CATALOGUE_ROUTES) return {RegionTableError::RouteOutOfRange, r};
      if (member.has(route)) return {RegionTableError::DuplicateRoute, r};
      member.add(route);
      covered.add(route);
      if (region.defaultRouteId != nullptr && strcmp(routeAt(route).routeId, region.defaultRouteId) == 0)
        defaultFound = true;
    }
    if (!defaultFound) return {RegionTableError::DefaultNotMember, r};
  }
  for (size_t route = 0; route < catalogueRoutes && route < MAX_CATALOGUE_ROUTES; ++route)
    if (!covered.has(route)) return {RegionTableError::RouteNotCovered, route};
  return {};
}

}  // namespace on_point
