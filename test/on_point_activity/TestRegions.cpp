// TEST ONLY: collection tables over the shipped catalogue, in place of OnPointRegionData.cpp.
//   ON_POINT_TEST_ONE_COLLECTION: every route in one collection (no SELECT COLLECTION row).
//   otherwise: a one-route collection and a six-route one (two list pages).
#include <cstdint>
#include <iterator>

#include "activities/on_point/OnPointRegions.h"

namespace on_point {
namespace {
#ifdef ON_POINT_TEST_ONE_COLLECTION
constexpr uint8_t ALL[] = {0, 1, 2, 3, 4, 5, 6};
constexpr Region REGIONS[] = {{"everything", ALL, std::size(ALL), "p2p-balagtas-trinoma"}};
#else
constexpr uint8_t ONE[] = {3};
constexpr uint8_t REST[] = {0, 1, 2, 4, 5, 6};
constexpr Region REGIONS[] = {{"one-route", ONE, std::size(ONE), "p2p-clark-airport-naia-t3"},
                              {"rest", REST, std::size(REST), "p2p-balagtas-trinoma"}};
#endif
}  // namespace
const Region* regionTable() { return REGIONS; }
size_t regionTableSize() { return std::size(REGIONS); }
}  // namespace on_point
