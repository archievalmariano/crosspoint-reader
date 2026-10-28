// Route collections over the 60-route capacity fixture: 6 overlapping collections
// (test/fixtures/on_point/gen_capacity_fixture.py).

#include <gtest/gtest.h>

#include <vector>

#include "activities/on_point/OnPointRegions.h"
#include "activities/on_point/OnPointRouteList.h"
#include "activities/on_point/OnPointScheduleData.h"

namespace {

using on_point::Region;
using on_point::RouteListModel;
using on_point::RouteResolution;
using on_point::RouteResolutionState;

size_t membershipsOf(const size_t route) {
  size_t count = 0;
  for (size_t r = 0; r < on_point::regionCount(); ++r) {
    const Region& region = *on_point::regionAt(r);
    for (size_t i = 0; i < region.routeCount; ++i) count += region.routes[i] == route;
  }
  return count;
}

TEST(OnPointRegionsFixture, TableValidatesAndOverlaps) {
  ASSERT_EQ(on_point::routeCount(), 60u);
  ASSERT_EQ(on_point::regionCount(), 6u);
  const auto check =
      on_point::validateRegionTable(on_point::regionTable(), on_point::regionTableSize(), on_point::routeCount());
  EXPECT_TRUE(check.ok()) << static_cast<int>(check.error) << " at " << check.region;
  for (size_t route = 30; route < 35; ++route) EXPECT_EQ(membershipsOf(route), 3u) << route;
  EXPECT_EQ(membershipsOf(5), 1u);
  EXPECT_EQ(on_point::regionIndexForId("fixture-collection-5"), 5u);
  EXPECT_EQ(on_point::regionIndexForId("fixture-collection-6"), on_point::NO_REGION);
}

TEST(OnPointRegionsFixture, EachCollectionListsSortedMembersAndPages) {
  for (size_t r = 0; r < on_point::regionCount(); ++r) {
    const Region& region = *on_point::regionAt(r);
    RouteListModel model;
    model.build(region.routes, region.routeCount, {RouteResolutionState::Unset, 0});
    ASSERT_EQ(model.count(), region.routeCount);
    for (size_t p = 2; p < model.count(); ++p)
      EXPECT_LT(on_point::compareRoutes(model.routeAtPosition(p - 1), model.routeAtPosition(p)), 0);
    // Every member reachable by Next from position 1, and by paging.
    size_t position = 1, steps = 0;
    do {
      position = model.nextPosition(position);
      ++steps;
    } while (position != 1 && steps < 200);
    EXPECT_EQ(steps, model.count() - 1) << region.regionId;
    const size_t pages =
        (model.count() - 1 + on_point::ROUTE_LIST_VISIBLE_ROWS - 1) / on_point::ROUTE_LIST_VISIBLE_ROWS;
    size_t target = 1, swipes = 0;
    while (model.swipeTarget(target, true, target)) ++swipes;
    EXPECT_EQ(swipes + 1, pages) << region.regionId;
  }
}

TEST(OnPointRegionsFixture, SwitchingCollectionsResetsPositions) {
  const Region& first = *on_point::regionAt(0);   // 20 routes, includes 30..34
  const Region& second = *on_point::regionAt(3);  // 30..44
  RouteListModel model;
  model.build(first.routes, first.routeCount, {RouteResolutionState::Canonical, 12});
  ASSERT_TRUE(model.contains(12));
  model.build(second.routes, second.routeCount, {RouteResolutionState::Canonical, 32});
  EXPECT_FALSE(model.contains(12));
  EXPECT_EQ(model.positionOfRoute(12), 0u);
  EXPECT_EQ(model.pinnedRoute(), 32u);
  EXPECT_EQ(model.count(), 15u);
  EXPECT_EQ(&on_point::scheduleForRoute(32, false), &on_point::scheduleAt(on_point::routeAt(32).outboundScheduleIndex));
}

}  // namespace
