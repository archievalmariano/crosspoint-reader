// TEST ONLY: collection tables over the shipped catalogue, in place of OnPointRegionData.cpp.
//   ON_POINT_TEST_ONE_COLLECTION: every route in one collection (no SELECT COLLECTION row).
//   ON_POINT_TEST_PICKER_COLLECTIONS=N: N collections (1-9) for picker paging.
//   otherwise: a one-route collection and a six-route one (two list pages).
#include <cstdint>
#include <iterator>

#include "activities/on_point/OnPointRegions.h"

namespace on_point {
namespace {
#if defined(ON_POINT_TEST_ONE_COLLECTION)
constexpr uint8_t ALL[] = {0, 1, 2, 3, 4, 5, 6};
constexpr Region REGIONS[] = {{"everything", ALL, std::size(ALL), "p2p-balagtas-trinoma"}};
constexpr size_t REGION_COUNT = std::size(REGIONS);
#elif defined(ON_POINT_TEST_PICKER_COLLECTIONS)
// Collection cN lists the first N routes, so each picker row shows a different route count.
constexpr uint8_t FIRST[] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
constexpr Region REGIONS[] = {{"c1", FIRST, 1, "p2p-balagtas-trinoma"}, {"c2", FIRST, 2, "p2p-balagtas-trinoma"},
                              {"c3", FIRST, 3, "p2p-balagtas-trinoma"}, {"c4", FIRST, 4, "p2p-balagtas-trinoma"},
                              {"c5", FIRST, 5, "p2p-balagtas-trinoma"}, {"c6", FIRST, 6, "p2p-balagtas-trinoma"},
                              {"c7", FIRST, 7, "p2p-balagtas-trinoma"}, {"c8", FIRST, 8, "p2p-balagtas-trinoma"},
                              {"c9", FIRST, 9, "p2p-balagtas-trinoma"}};
constexpr size_t REGION_COUNT = ON_POINT_TEST_PICKER_COLLECTIONS;
static_assert(REGION_COUNT >= 1 && REGION_COUNT <= std::size(REGIONS));
#else
constexpr uint8_t ONE[] = {3};
constexpr uint8_t REST[] = {0, 1, 2, 4, 5, 6};
constexpr Region REGIONS[] = {{"one-route", ONE, std::size(ONE), "p2p-clark-airport-naia-t3"},
                              {"rest", REST, std::size(REST), "p2p-balagtas-trinoma"}};
constexpr size_t REGION_COUNT = std::size(REGIONS);
#endif
}  // namespace
const Region* regionTable() { return REGIONS; }
size_t regionTableSize() { return REGION_COUNT; }
}  // namespace on_point
