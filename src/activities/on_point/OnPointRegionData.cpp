// Shipped route collections over the catalogue in OnPointScheduleData.cpp (indices into its
// ROUTES; test/on_point_regions pins each index to its route ID). Only populated collections
// ship.

#include <cstdint>
#include <iterator>

#include "OnPointRegions.h"

namespace on_point {
namespace {

constexpr uint8_t BULACAN[] = {0 /* p2p-balagtas-trinoma */, 1 /* p2p-caypombo-sm-north-edsa */,
                               7 /* p2p-pandi-trinoma */,    8 /* p2p-plaridel-trinoma */,
                               9 /* p2p-malolos-trinoma */,  10 /* p2p-guiguinto-trinoma */};
constexpr uint8_t NORTH_LUZON[] = {2 /* p2p-cubao-clark-airport */,  4 /* p2p-trinoma-clark-airport */,
                                   5 /* p2p-clark-airport-baguio */, 12 /* genesis-cubao-baguio */,
                                   13 /* genesis-pasay-baguio */,    14 /* p2p-clark-airport-balanga */};
constexpr uint8_t AIRPORT_TRANSFERS[] = {3 /* p2p-clark-airport-naia-t3 */, 6 /* p2p-naia-pitx */,
                                         11 /* p2p-pitx-clark-airport */};
constexpr uint8_t METRO_MANILA[] = {15 /* p2p-atc-greenbelt */, 16 /* p2p-starmall-alabang-starmall-shaw */,
                                    17 /* p2p-atc-sm-megamall */};
constexpr uint8_t LAGUNA[] = {18 /* p2p-nuvali-one-ayala */, 19 /* p2p-cabuyao-makati */, 20 /* p2p-cabuyao-bgc */};
constexpr uint8_t SOUTH_LUZON[] = {21 /* p2p-batangas-grand-newport */, 22 /* p2p-sm-lipa-newport */,
                                   23 /* p2p-sm-masinag-one-ayala */, 24 /* p2p-sierra-valley-one-ayala */};

constexpr Region REGIONS[] = {
    {"bulacan", BULACAN, std::size(BULACAN), "p2p-balagtas-trinoma"},
    {"north-luzon", NORTH_LUZON, std::size(NORTH_LUZON), "p2p-cubao-clark-airport"},
    {"airport-transfers", AIRPORT_TRANSFERS, std::size(AIRPORT_TRANSFERS), "p2p-clark-airport-naia-t3"},
    {"metro-manila", METRO_MANILA, std::size(METRO_MANILA), "p2p-atc-greenbelt"},
    {"laguna", LAGUNA, std::size(LAGUNA), "p2p-nuvali-one-ayala"},
    {"south-luzon", SOUTH_LUZON, std::size(SOUTH_LUZON), "p2p-batangas-grand-newport"},
};

}  // namespace

const Region* regionTable() { return REGIONS; }
size_t regionTableSize() { return std::size(REGIONS); }

}  // namespace on_point
