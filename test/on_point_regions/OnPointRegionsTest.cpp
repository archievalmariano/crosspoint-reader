// Route collections over the shipped catalogue: the table, its validation, lookups, and
// RouteListModel built from a collection's members (Stage 2, P1).

#include <gtest/gtest.h>

#include <algorithm>
#include <cstring>
#include <iterator>
#include <string>
#include <vector>

#include "activities/on_point/OnPointRegions.h"
#include "activities/on_point/OnPointRouteList.h"
#include "activities/on_point/OnPointScheduleData.h"

namespace {

using on_point::NO_REGION;
using on_point::Region;
using on_point::RegionTableError;
using on_point::RouteListModel;
using on_point::RouteResolution;
using on_point::RouteResolutionState;

// Stage 1 golden order of the shipped catalogue (test/on_point_route_list).
constexpr size_t SORTED[] = {0, 1, 5, 3, 2, 6, 4};

const RouteResolution UNSET{RouteResolutionState::Unset, 0};
RouteResolution canonical(const size_t route) { return {RouteResolutionState::Canonical, route}; }

std::vector<std::string> memberIds(const Region& region) {
  std::vector<std::string> ids;
  for (size_t i = 0; i < region.routeCount; ++i) ids.emplace_back(on_point::routeAt(region.routes[i]).routeId);
  return ids;
}

std::vector<size_t> listOrder(const RouteListModel& model) {
  std::vector<size_t> order;
  for (size_t position = 0; position < model.count(); ++position) order.push_back(model.routeAtPosition(position));
  return order;
}

// The golden order restricted to a collection's members.
std::vector<size_t> goldenOrderOf(const Region& region) {
  std::vector<size_t> order;
  for (const size_t route : SORTED)
    for (size_t i = 0; i < region.routeCount; ++i)
      if (region.routes[i] == route) order.push_back(route);
  return order;
}

// ---- Shipped table ----

TEST(OnPointRegions, ShippedTableValidates) {
  const auto check =
      on_point::validateRegionTable(on_point::regionTable(), on_point::regionTableSize(), on_point::routeCount());
  EXPECT_TRUE(check.ok()) << static_cast<int>(check.error) << " at " << check.region;
}

TEST(OnPointRegions, ShippedCollectionsAreExactlyTheApprovedThree) {
  ASSERT_EQ(on_point::regionCount(), 3u);
  const Region* bulacan = on_point::regionAt(0);
  const Region* northLuzon = on_point::regionAt(1);
  const Region* airports = on_point::regionAt(2);
  ASSERT_NE(bulacan, nullptr);
  ASSERT_NE(northLuzon, nullptr);
  ASSERT_NE(airports, nullptr);
  EXPECT_STREQ(bulacan->regionId, "bulacan");
  EXPECT_STREQ(northLuzon->regionId, "north-luzon");
  EXPECT_STREQ(airports->regionId, "airport-transfers");
  // Pins each member index to its route ID, so a reordered catalogue fails here.
  EXPECT_EQ(memberIds(*bulacan), (std::vector<std::string>{"p2p-balagtas-trinoma", "p2p-caypombo-sm-north-edsa"}));
  EXPECT_EQ(memberIds(*northLuzon), (std::vector<std::string>{"p2p-cubao-clark-airport", "p2p-trinoma-clark-airport",
                                                              "p2p-clark-airport-baguio"}));
  EXPECT_EQ(memberIds(*airports), (std::vector<std::string>{"p2p-clark-airport-naia-t3", "p2p-naia-pitx"}));
  EXPECT_STREQ(bulacan->defaultRouteId, "p2p-balagtas-trinoma");
  EXPECT_STREQ(northLuzon->defaultRouteId, "p2p-cubao-clark-airport");
  EXPECT_STREQ(airports->defaultRouteId, "p2p-clark-airport-naia-t3");
}

TEST(OnPointRegions, EveryShippedRouteIsInExactlyOneCollection) {
  for (size_t route = 0; route < on_point::routeCount(); ++route) {
    size_t memberships = 0;
    for (size_t r = 0; r < on_point::regionCount(); ++r) {
      const Region& region = *on_point::regionAt(r);
      for (size_t i = 0; i < region.routeCount; ++i) memberships += region.routes[i] == route;
    }
    EXPECT_EQ(memberships, 1u) << on_point::routeAt(route).routeId;
  }
}

TEST(OnPointRegions, FutureCategoriesAreNotShipped) {
  for (const char* id : {"rizal", "cavite", "laguna", "south-luzon"})
    EXPECT_EQ(on_point::regionIndexForId(id), NO_REGION);
}

TEST(OnPointRegions, LookupsNeverFallBackToIndexZero) {
  EXPECT_EQ(on_point::regionIndexForId("bulacan"), 0u);
  EXPECT_EQ(on_point::regionIndexForId("north-luzon"), 1u);
  EXPECT_EQ(on_point::regionIndexForId("airport-transfers"), 2u);
  for (const char* id : {"", "BULACAN", "Bulacan", "bulacan ", "north luzon", "airport", "x"})
    EXPECT_EQ(on_point::regionIndexForId(id), NO_REGION) << id;
  EXPECT_EQ(on_point::regionIndexForId(nullptr), NO_REGION);
  EXPECT_EQ(on_point::regionAt(3), nullptr);
  EXPECT_EQ(on_point::regionAt(NO_REGION), nullptr);
}

TEST(OnPointRegions, RegionIdFormat) {
  for (const char* id :
       {"bulacan", "north-luzon", "airport-transfers", "a", "a1", "x-2-y", "abcdefghijklmnopqrstuvwxyz01234"})
    EXPECT_TRUE(on_point::isValidRegionId(id)) << id;
  for (const char* id : {"", "-a", "a-", "a--b", "A", "north_luzon", "north luzon", "é",
                         "abcdefghijklmnopqrstuvwxyz012345"})  // 32 characters
    EXPECT_FALSE(on_point::isValidRegionId(id)) << id;
  EXPECT_FALSE(on_point::isValidRegionId(nullptr));
}

// ---- Validation over synthetic tables (the shipped catalogue's 7 routes) ----

constexpr uint8_t A[] = {0, 1, 2, 3};
constexpr uint8_t B[] = {3, 4, 5, 6};
constexpr uint8_t ALL[] = {0, 1, 2, 3, 4, 5, 6};

RegionTableError errorOf(const std::vector<Region>& table) {
  return on_point::validateRegionTable(table.data(), table.size(), on_point::routeCount()).error;
}

TEST(OnPointRegions, OverlappingMembershipValidates) {
  // Route 3 in all three, collection "a" equal to a subset of "everything".
  EXPECT_EQ(errorOf({{"a", A, 4, "p2p-clark-airport-naia-t3"},
                     {"b", B, 4, "p2p-clark-airport-naia-t3"},
                     {"everything", ALL, 7, "p2p-balagtas-trinoma"}}),
            RegionTableError::None);
}

TEST(OnPointRegions, ValidationRejectsEachDefect) {
  constexpr uint8_t dup[] = {0, 1, 1, 2, 3, 4, 5, 6};
  constexpr uint8_t outOfRange[] = {0, 1, 2, 3, 4, 5, 6, 7};
  constexpr uint8_t partial[] = {0, 1, 2, 3, 4, 5};
  EXPECT_EQ(on_point::validateRegionTable(nullptr, 0, 7).error, RegionTableError::NoRegions);
  EXPECT_EQ(errorOf({}), RegionTableError::NoRegions);
  EXPECT_EQ(errorOf({{"Bad", ALL, 7, "p2p-naia-pitx"}}), RegionTableError::InvalidId);
  EXPECT_EQ(errorOf({{nullptr, ALL, 7, "p2p-naia-pitx"}}), RegionTableError::InvalidId);
  EXPECT_EQ(errorOf({{"x", ALL, 7, "p2p-naia-pitx"}, {"x", A, 4, "p2p-balagtas-trinoma"}}),
            RegionTableError::DuplicateId);
  EXPECT_EQ(errorOf({{"x", ALL, 7, "p2p-naia-pitx"}, {"empty", A, 0, "p2p-balagtas-trinoma"}}),
            RegionTableError::NoRoutes);
  EXPECT_EQ(errorOf({{"x", nullptr, 3, "p2p-naia-pitx"}}), RegionTableError::NoRoutes);
  EXPECT_EQ(errorOf({{"x", ALL, 129, "p2p-naia-pitx"}}), RegionTableError::TooManyRoutes);
  EXPECT_EQ(errorOf({{"x", outOfRange, 8, "p2p-naia-pitx"}}), RegionTableError::RouteOutOfRange);
  EXPECT_EQ(errorOf({{"x", dup, 8, "p2p-naia-pitx"}}), RegionTableError::DuplicateRoute);
  EXPECT_EQ(errorOf({{"x", A, 4, "p2p-naia-pitx"}, {"y", ALL, 7, "p2p-naia-pitx"}}),
            RegionTableError::DefaultNotMember);  // p2p-naia-pitx is route 6, not in A
  EXPECT_EQ(errorOf({{"x", ALL, 7, nullptr}}), RegionTableError::DefaultNotMember);
  EXPECT_EQ(errorOf({{"x", ALL, 7, "p2p-unknown"}}), RegionTableError::DefaultNotMember);
  const auto uncovered = on_point::validateRegionTable(
      std::vector<Region>{{"x", partial, 6, "p2p-balagtas-trinoma"}}.data(), 1, on_point::routeCount());
  EXPECT_EQ(uncovered.error, RegionTableError::RouteNotCovered);
  EXPECT_EQ(uncovered.region, 6u);  // the uncovered catalogue route
}

// ---- RouteListModel over a collection ----

TEST(OnPointRegions, AllRoutesBuildKeepsStage1Order) {
  RouteListModel model;
  model.build(UNSET);
  EXPECT_EQ(listOrder(model), (std::vector<size_t>(std::begin(SORTED), std::end(SORTED))));
  model.build(canonical(4));
  EXPECT_EQ(model.pinnedRoute(), 4u);
  EXPECT_EQ(listOrder(model), (std::vector<size_t>{4, 0, 1, 5, 3, 2, 6}));
}

TEST(OnPointRegions, EachShippedCollectionListsItsMembersInGoldenOrder) {
  for (size_t r = 0; r < on_point::regionCount(); ++r) {
    const Region& region = *on_point::regionAt(r);
    RouteListModel model;
    model.build(region.routes, region.routeCount, UNSET);
    EXPECT_EQ(model.count(), region.routeCount);
    EXPECT_EQ(listOrder(model), goldenOrderOf(region)) << region.regionId;
    EXPECT_FALSE(model.hasMainRoute());
    for (size_t route = 0; route < on_point::routeCount(); ++route) {
      const bool member =
          std::find(region.routes, region.routes + region.routeCount, route) != region.routes + region.routeCount;
      EXPECT_EQ(model.contains(route), member) << region.regionId << " route " << route;
    }
  }
}

TEST(OnPointRegions, PinnedMainRouteWithinACollection) {
  const Region& northLuzon = *on_point::regionAt(1);  // members 2, 4, 5; golden order 5, 2, 4
  RouteListModel model;
  model.build(northLuzon.routes, northLuzon.routeCount, canonical(4));
  EXPECT_TRUE(model.hasMainRoute());
  EXPECT_TRUE(model.isMainRoute(4));
  EXPECT_EQ(listOrder(model), (std::vector<size_t>{4, 5, 2}));
  EXPECT_EQ(model.positionOfRoute(5), 1u);
  EXPECT_EQ(model.positionOfRoute(2), 2u);

  // A resolved main route outside the collection is Unknown: the no-main state.
  model.build(northLuzon.routes, northLuzon.routeCount, canonical(0));
  EXPECT_FALSE(model.hasMainRoute());
  EXPECT_EQ(model.resolution().state, RouteResolutionState::Unknown);
  EXPECT_EQ(model.pinnedRoute(), 5u);
  EXPECT_EQ(on_point::pinnedHeading(model.resolution().state), on_point::PinnedHeading::NoMainRoute);
}

TEST(OnPointRegions, RebuildingForAnotherCollectionLeavesNothingStale) {
  RouteListModel model;
  model.build(canonical(4));                // every route, TRINOMA pinned
  ASSERT_EQ(model.positionOfRoute(6), 6u);  // last row, page 2
  ASSERT_EQ(RouteListModel::pageStart(model.positionOfRoute(6)), 3u);

  const Region& bulacan = *on_point::regionAt(0);  // members 0, 1
  model.build(bulacan.routes, bulacan.routeCount, canonical(0));
  EXPECT_EQ(model.count(), 2u);
  EXPECT_EQ(model.pinnedRoute(), 0u);
  EXPECT_EQ(listOrder(model), (std::vector<size_t>{0, 1}));
  for (const size_t stale : {2u, 3u, 4u, 5u, 6u}) {
    EXPECT_FALSE(model.contains(stale)) << stale;
    EXPECT_EQ(model.positionOfRoute(stale), 0u) << stale;  // not listed: the pinned slot
  }
  EXPECT_EQ(model.positionOfRoute(1), 1u);
  EXPECT_EQ(model.visibleRows(0), 1u);
  EXPECT_EQ(model.visibleRows(3), 0u);
  EXPECT_EQ(model.nextPosition(1), 1u);  // one other route: Next stays on it
  size_t target = 0;
  EXPECT_FALSE(model.swipeTarget(1, true, target));
  EXPECT_FALSE(model.swipeTarget(1, false, target));
}

TEST(OnPointRegions, SharedRouteUsesTheSameTimetableInEveryCollection) {
  // Route 3 (CLARK -> NAIA T3) in two synthetic collections.
  RouteListModel first, second;
  first.build(A, 4, canonical(3));
  second.build(B, 4, canonical(3));
  EXPECT_EQ(first.pinnedRoute(), second.pinnedRoute());
  EXPECT_EQ(&on_point::scheduleForRoute(first.pinnedRoute(), false),
            &on_point::scheduleForRoute(second.pinnedRoute(), false));
  EXPECT_EQ(&on_point::scheduleForRoute(first.pinnedRoute(), true),
            &on_point::scheduleForRoute(second.pinnedRoute(), true));
}

TEST(OnPointRegions, MalformedSpansAreSkipped) {
  constexpr uint8_t messy[] = {6, 6, 200, 3, 127, 3};
  RouteListModel model;
  model.build(messy, std::size(messy), UNSET);
  EXPECT_EQ(listOrder(model), (std::vector<size_t>{3, 6}));
  model.build(nullptr, 5, UNSET);
  EXPECT_EQ(model.count(), 0u);
  EXPECT_FALSE(model.contains(0));
  model.build(A, 0, canonical(0));
  EXPECT_EQ(model.count(), 0u);
  EXPECT_FALSE(model.hasMainRoute());
}

TEST(OnPointRegions, TouchGeometryIsUnchangedForACollection) {
  using namespace on_point::list_geometry;
  const Region& northLuzon = *on_point::regionAt(1);
  RouteListModel model;
  model.build(northLuzon.routes, northLuzon.routeCount, canonical(4));  // 4 pinned; rows 5, 2
  size_t route = 99;
  EXPECT_TRUE(model.routeAtPoint(100, MAIN_ROUTE_TOP + 10, 480, 0, route));
  EXPECT_EQ(route, 4u);
  EXPECT_TRUE(model.routeAtPoint(100, ROW_TOP + 10, 480, 0, route));
  EXPECT_EQ(route, 5u);
  EXPECT_TRUE(model.routeAtPoint(100, ROW_TOP + ROW_HEIGHT + ROW_GAP + 10, 480, 0, route));
  EXPECT_EQ(route, 2u);
  EXPECT_FALSE(model.routeAtPoint(100, ROW_TOP + ROW_HEIGHT + 5, 480, 0, route));                   // gap
  EXPECT_FALSE(model.routeAtPoint(100, ROW_TOP + 2 * (ROW_HEIGHT + ROW_GAP) + 10, 480, 0, route));  // no third row
}

}  // namespace
