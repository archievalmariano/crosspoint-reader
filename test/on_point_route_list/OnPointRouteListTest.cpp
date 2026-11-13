#include <gtest/gtest.h>

#include <cstring>

#include "activities/on_point/OnPointRouteList.h"
#include "activities/on_point/OnPointScheduleData.h"
#include "test/fixtures/on_point/LegacyRouteList.h"

// Golden route-list behaviour for the shipped routes, produced by the v1.2.0
// implementation (test/fixtures/on_point/LegacyRouteList.h) and kept as
// literal tables: captured for the seven v1.2.0 routes before the refactor,
// regenerated from the same reference when the BULACAN, M2, M3-A and M4 routes were added.

namespace {

using on_point::RouteListModel;
using on_point::RouteResolution;
using on_point::RouteResolutionState;

constexpr size_t N = 21;
constexpr int SCREEN_WIDTH = 480;

// Comparator order (ATC ALABANG > GREENBELT, SM MEGAMALL, BALAGTAS, CAYPOMBO,
// CLARK AIRPORT > BAGUIO, BALANGA, NAIA T3, CUBAO > BAGUIO, CLARK AIRPORT, GUIGUINTO,
// MALOLOS, NAIA T3, NUVALI, PANDI, PASAY, PITX, PLARIDEL, TRINOMA, VTX ALABANG,
// WM CABUYAO > MAKATI, MARKET MARKET).
constexpr size_t SORTED[N] = {15, 17, 0, 1, 5, 14, 3, 12, 2, 10, 9, 6, 18, 7, 13, 11, 8, 4, 16, 19, 20};

struct Golden {
  size_t display[N];     // route at each display position (0 = pinned)
  size_t positionOf[N];  // display position of each route
  size_t pageStart[N];   // page start with each route selected
  size_t visible[N];     // visible rows with each route selected
  size_t next[N];        // NavNext position from each route
  size_t previous[N];    // NavPrevious position from each route
  int swipeUp[N];        // route selected by swiping to the later page (-1: none)
  int swipeDown[N];      // route selected by swiping to the earlier page (-1: none)
};

// One entry per main route index 0..20.
constexpr Golden GOLDEN[N] = {
    {
        /* main 0 p2p-balagtas-trinoma */
        {0, 15, 17, 1, 5, 14, 3, 12, 2, 10, 9, 6, 18, 7, 13, 11, 8, 4, 16, 19, 20},   // display order
        {0, 3, 8, 6, 17, 4, 11, 13, 16, 10, 9, 15, 7, 14, 5, 1, 18, 2, 12, 19, 20},   // position of route
        {0, 0, 6, 3, 15, 3, 9, 12, 15, 9, 6, 12, 6, 12, 3, 0, 15, 0, 9, 18, 18},      // page start by route
        {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2},              // visible rows by route
        {1, 4, 9, 7, 18, 5, 12, 14, 17, 11, 10, 16, 8, 15, 6, 2, 19, 3, 13, 20, 1},   // NavNext position by route
        {20, 2, 7, 5, 16, 3, 10, 12, 15, 9, 8, 14, 6, 13, 4, 20, 17, 1, 11, 18, 19},  // NavPrevious position by route
        {5, 5, 9, 12, 19, 12, 7, 8, 19, 7, 9, 8, 9, 8, 12, 5, 19, 5, 7, -1, -1},      // swipe up route (-1 none)
        {-1, -1, 5, 15, 7, 15, 12, 9, 7, 12, 5, 9, 5, 9, 15, -1, 7, -1, 12, 8, 8},    // swipe down route (-1 none)
    },
    {
        /* main 1 p2p-caypombo-sm-north-edsa */
        {1, 15, 17, 0, 5, 14, 3, 12, 2, 10, 9, 6, 18, 7, 13, 11, 8, 4, 16, 19, 20},   // display order
        {3, 0, 8, 6, 17, 4, 11, 13, 16, 10, 9, 15, 7, 14, 5, 1, 18, 2, 12, 19, 20},   // position of route
        {0, 0, 6, 3, 15, 3, 9, 12, 15, 9, 6, 12, 6, 12, 3, 0, 15, 0, 9, 18, 18},      // page start by route
        {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2},              // visible rows by route
        {4, 1, 9, 7, 18, 5, 12, 14, 17, 11, 10, 16, 8, 15, 6, 2, 19, 3, 13, 20, 1},   // NavNext position by route
        {2, 20, 7, 5, 16, 3, 10, 12, 15, 9, 8, 14, 6, 13, 4, 20, 17, 1, 11, 18, 19},  // NavPrevious position by route
        {5, 5, 9, 12, 19, 12, 7, 8, 19, 7, 9, 8, 9, 8, 12, 5, 19, 5, 7, -1, -1},      // swipe up route (-1 none)
        {-1, -1, 5, 15, 7, 15, 12, 9, 7, 12, 5, 9, 5, 9, 15, -1, 7, -1, 12, 8, 8},    // swipe down route (-1 none)
    },
    {
        /* main 2 p2p-cubao-clark-airport */
        {2, 15, 17, 0, 1, 5, 14, 3, 12, 10, 9, 6, 18, 7, 13, 11, 8, 4, 16, 19, 20},   // display order
        {3, 4, 0, 7, 17, 5, 11, 13, 16, 10, 9, 15, 8, 14, 6, 1, 18, 2, 12, 19, 20},   // position of route
        {0, 3, 0, 6, 15, 3, 9, 12, 15, 9, 6, 12, 6, 12, 3, 0, 15, 0, 9, 18, 18},      // page start by route
        {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2},              // visible rows by route
        {4, 5, 1, 8, 18, 6, 12, 14, 17, 11, 10, 16, 9, 15, 7, 2, 19, 3, 13, 20, 1},   // NavNext position by route
        {2, 3, 20, 6, 16, 4, 10, 12, 15, 9, 8, 14, 7, 13, 5, 20, 17, 1, 11, 18, 19},  // NavPrevious position by route
        {1, 3, 1, 9, 19, 3, 7, 8, 19, 7, 9, 8, 9, 8, 3, 1, 19, 1, 7, -1, -1},         // swipe up route (-1 none)
        {-1, 15, -1, 1, 7, 15, 3, 9, 7, 3, 1, 9, 1, 9, 15, -1, 7, -1, 3, 8, 8},       // swipe down route (-1 none)
    },
    {
        /* main 3 p2p-clark-airport-naia-t3 */
        {3, 15, 17, 0, 1, 5, 14, 12, 2, 10, 9, 6, 18, 7, 13, 11, 8, 4, 16, 19, 20},   // display order
        {3, 4, 8, 0, 17, 5, 11, 13, 16, 10, 9, 15, 7, 14, 6, 1, 18, 2, 12, 19, 20},   // position of route
        {0, 3, 6, 0, 15, 3, 9, 12, 15, 9, 6, 12, 6, 12, 3, 0, 15, 0, 9, 18, 18},      // page start by route
        {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2},              // visible rows by route
        {4, 5, 9, 1, 18, 6, 12, 14, 17, 11, 10, 16, 8, 15, 7, 2, 19, 3, 13, 20, 1},   // NavNext position by route
        {2, 3, 7, 20, 16, 4, 10, 12, 15, 9, 8, 14, 6, 13, 5, 20, 17, 1, 11, 18, 19},  // NavPrevious position by route
        {1, 12, 9, 1, 19, 12, 7, 8, 19, 7, 9, 8, 9, 8, 12, 1, 19, 1, 7, -1, -1},      // swipe up route (-1 none)
        {-1, 15, 1, -1, 7, 15, 12, 9, 7, 12, 1, 9, 1, 9, 15, -1, 7, -1, 12, 8, 8},    // swipe down route (-1 none)
    },
    {
        /* main 4 p2p-trinoma-clark-airport */
        {4, 15, 17, 0, 1, 5, 14, 3, 12, 2, 10, 9, 6, 18, 7, 13, 11, 8, 16, 19, 20},     // display order
        {3, 4, 9, 7, 0, 5, 12, 14, 17, 11, 10, 16, 8, 15, 6, 1, 18, 2, 13, 19, 20},     // position of route
        {0, 3, 6, 6, 0, 3, 9, 12, 15, 9, 9, 15, 6, 12, 3, 0, 15, 0, 12, 18, 18},        // page start by route
        {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2},                // visible rows by route
        {4, 5, 10, 8, 1, 6, 13, 15, 18, 12, 11, 17, 9, 16, 7, 2, 19, 3, 14, 20, 1},     // NavNext position by route
        {2, 3, 8, 6, 20, 4, 11, 13, 16, 10, 9, 15, 7, 14, 5, 20, 17, 1, 12, 18, 19},    // NavPrevious position by route
        {1, 3, 10, 10, 1, 3, 18, 11, 19, 18, 18, 19, 10, 11, 3, 1, 19, 1, 11, -1, -1},  // swipe up route (-1 none)
        {-1, 15, 1, 1, -1, 15, 3, 10, 18, 3, 3, 18, 1, 10, 15, -1, 18, -1, 10, 11, 11},  // swipe down route (-1 none)
    },
    {
        /* main 5 p2p-clark-airport-baguio */
        {5, 15, 17, 0, 1, 14, 3, 12, 2, 10, 9, 6, 18, 7, 13, 11, 8, 4, 16, 19, 20},   // display order
        {3, 4, 8, 6, 17, 0, 11, 13, 16, 10, 9, 15, 7, 14, 5, 1, 18, 2, 12, 19, 20},   // position of route
        {0, 3, 6, 3, 15, 0, 9, 12, 15, 9, 6, 12, 6, 12, 3, 0, 15, 0, 9, 18, 18},      // page start by route
        {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2},              // visible rows by route
        {4, 5, 9, 7, 18, 1, 12, 14, 17, 11, 10, 16, 8, 15, 6, 2, 19, 3, 13, 20, 1},   // NavNext position by route
        {2, 3, 7, 5, 16, 20, 10, 12, 15, 9, 8, 14, 6, 13, 4, 20, 17, 1, 11, 18, 19},  // NavPrevious position by route
        {1, 12, 9, 12, 19, 1, 7, 8, 19, 7, 9, 8, 9, 8, 12, 1, 19, 1, 7, -1, -1},      // swipe up route (-1 none)
        {-1, 15, 1, 15, 7, -1, 12, 9, 7, 12, 1, 9, 1, 9, 15, -1, 7, -1, 12, 8, 8},    // swipe down route (-1 none)
    },
    {
        /* main 6 p2p-naia-pitx */
        {6, 15, 17, 0, 1, 5, 14, 3, 12, 2, 10, 9, 18, 7, 13, 11, 8, 4, 16, 19, 20},   // display order
        {3, 4, 9, 7, 17, 5, 0, 13, 16, 11, 10, 15, 8, 14, 6, 1, 18, 2, 12, 19, 20},   // position of route
        {0, 3, 6, 6, 15, 3, 0, 12, 15, 9, 9, 12, 6, 12, 3, 0, 15, 0, 9, 18, 18},      // page start by route
        {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2},              // visible rows by route
        {4, 5, 10, 8, 18, 6, 1, 14, 17, 12, 11, 16, 9, 15, 7, 2, 19, 3, 13, 20, 1},   // NavNext position by route
        {2, 3, 8, 6, 16, 4, 20, 12, 15, 10, 9, 14, 7, 13, 5, 20, 17, 1, 11, 18, 19},  // NavPrevious position by route
        {1, 3, 10, 10, 19, 3, 1, 8, 19, 7, 7, 8, 10, 8, 3, 1, 19, 1, 7, -1, -1},      // swipe up route (-1 none)
        {-1, 15, 1, 1, 7, 15, -1, 10, 7, 3, 3, 10, 1, 10, 15, -1, 7, -1, 3, 8, 8},    // swipe down route (-1 none)
    },
    {
        /* main 7 p2p-pandi-trinoma */
        {7, 15, 17, 0, 1, 5, 14, 3, 12, 2, 10, 9, 6, 18, 13, 11, 8, 4, 16, 19, 20},    // display order
        {3, 4, 9, 7, 17, 5, 12, 0, 16, 11, 10, 15, 8, 14, 6, 1, 18, 2, 13, 19, 20},    // position of route
        {0, 3, 6, 6, 15, 3, 9, 0, 15, 9, 9, 12, 6, 12, 3, 0, 15, 0, 12, 18, 18},       // page start by route
        {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2},               // visible rows by route
        {4, 5, 10, 8, 18, 6, 13, 1, 17, 12, 11, 16, 9, 15, 7, 2, 19, 3, 14, 20, 1},    // NavNext position by route
        {2, 3, 8, 6, 16, 4, 11, 20, 15, 10, 9, 14, 7, 13, 5, 20, 17, 1, 12, 18, 19},   // NavPrevious position by route
        {1, 3, 10, 10, 19, 3, 18, 1, 19, 18, 18, 8, 10, 8, 3, 1, 19, 1, 8, -1, -1},    // swipe up route (-1 none)
        {-1, 15, 1, 1, 18, 15, 3, -1, 18, 3, 3, 10, 1, 10, 15, -1, 18, -1, 10, 8, 8},  // swipe down route (-1 none)
    },
    {
        /* main 8 p2p-plaridel-trinoma */
        {8, 15, 17, 0, 1, 5, 14, 3, 12, 2, 10, 9, 6, 18, 7, 13, 11, 4, 16, 19, 20},     // display order
        {3, 4, 9, 7, 17, 5, 12, 14, 0, 11, 10, 16, 8, 15, 6, 1, 18, 2, 13, 19, 20},     // position of route
        {0, 3, 6, 6, 15, 3, 9, 12, 0, 9, 9, 15, 6, 12, 3, 0, 15, 0, 12, 18, 18},        // page start by route
        {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2},                // visible rows by route
        {4, 5, 10, 8, 18, 6, 13, 15, 1, 12, 11, 17, 9, 16, 7, 2, 19, 3, 14, 20, 1},     // NavNext position by route
        {2, 3, 8, 6, 16, 4, 11, 13, 20, 10, 9, 15, 7, 14, 5, 20, 17, 1, 12, 18, 19},    // NavPrevious position by route
        {1, 3, 10, 10, 19, 3, 18, 11, 1, 18, 18, 19, 10, 11, 3, 1, 19, 1, 11, -1, -1},  // swipe up route (-1 none)
        {-1, 15, 1, 1, 18, 15, 3, 10, -1, 3, 3, 18, 1, 10, 15, -1, 18, -1, 10, 11, 11},  // swipe down route (-1 none)
    },
    {
        /* main 9 p2p-malolos-trinoma */
        {9, 15, 17, 0, 1, 5, 14, 3, 12, 2, 10, 6, 18, 7, 13, 11, 8, 4, 16, 19, 20},   // display order
        {3, 4, 9, 7, 17, 5, 11, 13, 16, 0, 10, 15, 8, 14, 6, 1, 18, 2, 12, 19, 20},   // position of route
        {0, 3, 6, 6, 15, 3, 9, 12, 15, 0, 9, 12, 6, 12, 3, 0, 15, 0, 9, 18, 18},      // page start by route
        {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2},              // visible rows by route
        {4, 5, 10, 8, 18, 6, 12, 14, 17, 1, 11, 16, 9, 15, 7, 2, 19, 3, 13, 20, 1},   // NavNext position by route
        {2, 3, 8, 6, 16, 4, 10, 12, 15, 20, 9, 14, 7, 13, 5, 20, 17, 1, 11, 18, 19},  // NavPrevious position by route
        {1, 3, 10, 10, 19, 3, 7, 8, 19, 1, 7, 8, 10, 8, 3, 1, 19, 1, 7, -1, -1},      // swipe up route (-1 none)
        {-1, 15, 1, 1, 7, 15, 3, 10, 7, -1, 3, 10, 1, 10, 15, -1, 7, -1, 3, 8, 8},    // swipe down route (-1 none)
    },
    {
        /* main 10 p2p-guiguinto-trinoma */
        {10, 15, 17, 0, 1, 5, 14, 3, 12, 2, 9, 6, 18, 7, 13, 11, 8, 4, 16, 19, 20},   // display order
        {3, 4, 9, 7, 17, 5, 11, 13, 16, 10, 0, 15, 8, 14, 6, 1, 18, 2, 12, 19, 20},   // position of route
        {0, 3, 6, 6, 15, 3, 9, 12, 15, 9, 0, 12, 6, 12, 3, 0, 15, 0, 9, 18, 18},      // page start by route
        {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2},              // visible rows by route
        {4, 5, 10, 8, 18, 6, 12, 14, 17, 11, 1, 16, 9, 15, 7, 2, 19, 3, 13, 20, 1},   // NavNext position by route
        {2, 3, 8, 6, 16, 4, 10, 12, 15, 9, 20, 14, 7, 13, 5, 20, 17, 1, 11, 18, 19},  // NavPrevious position by route
        {1, 3, 9, 9, 19, 3, 7, 8, 19, 7, 1, 8, 9, 8, 3, 1, 19, 1, 7, -1, -1},         // swipe up route (-1 none)
        {-1, 15, 1, 1, 7, 15, 3, 9, 7, 3, -1, 9, 1, 9, 15, -1, 7, -1, 3, 8, 8},       // swipe down route (-1 none)
    },
    {
        /* main 11 p2p-pitx-clark-airport */
        {11, 15, 17, 0, 1, 5, 14, 3, 12, 2, 10, 9, 6, 18, 7, 13, 8, 4, 16, 19, 20},    // display order
        {3, 4, 9, 7, 17, 5, 12, 14, 16, 11, 10, 0, 8, 15, 6, 1, 18, 2, 13, 19, 20},    // position of route
        {0, 3, 6, 6, 15, 3, 9, 12, 15, 9, 9, 0, 6, 12, 3, 0, 15, 0, 12, 18, 18},       // page start by route
        {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2},               // visible rows by route
        {4, 5, 10, 8, 18, 6, 13, 15, 17, 12, 11, 1, 9, 16, 7, 2, 19, 3, 14, 20, 1},    // NavNext position by route
        {2, 3, 8, 6, 16, 4, 11, 13, 15, 10, 9, 20, 7, 14, 5, 20, 17, 1, 12, 18, 19},   // NavPrevious position by route
        {1, 3, 10, 10, 19, 3, 18, 8, 19, 18, 18, 1, 10, 8, 3, 1, 19, 1, 8, -1, -1},    // swipe up route (-1 none)
        {-1, 15, 1, 1, 18, 15, 3, 10, 18, 3, 3, -1, 1, 10, 15, -1, 18, -1, 10, 8, 8},  // swipe down route (-1 none)
    },
    {
        /* main 12 genesis-cubao-baguio */
        {12, 15, 17, 0, 1, 5, 14, 3, 2, 10, 9, 6, 18, 7, 13, 11, 8, 4, 16, 19, 20},   // display order
        {3, 4, 8, 7, 17, 5, 11, 13, 16, 10, 9, 15, 0, 14, 6, 1, 18, 2, 12, 19, 20},   // position of route
        {0, 3, 6, 6, 15, 3, 9, 12, 15, 9, 6, 12, 0, 12, 3, 0, 15, 0, 9, 18, 18},      // page start by route
        {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2},              // visible rows by route
        {4, 5, 9, 8, 18, 6, 12, 14, 17, 11, 10, 16, 1, 15, 7, 2, 19, 3, 13, 20, 1},   // NavNext position by route
        {2, 3, 7, 6, 16, 4, 10, 12, 15, 9, 8, 14, 20, 13, 5, 20, 17, 1, 11, 18, 19},  // NavPrevious position by route
        {1, 3, 9, 9, 19, 3, 7, 8, 19, 7, 9, 8, 1, 8, 3, 1, 19, 1, 7, -1, -1},         // swipe up route (-1 none)
        {-1, 15, 1, 1, 7, 15, 3, 9, 7, 3, 1, 9, -1, 9, 15, -1, 7, -1, 3, 8, 8},       // swipe down route (-1 none)
    },
    {
        /* main 13 genesis-pasay-baguio */
        {13, 15, 17, 0, 1, 5, 14, 3, 12, 2, 10, 9, 6, 18, 7, 11, 8, 4, 16, 19, 20},    // display order
        {3, 4, 9, 7, 17, 5, 12, 14, 16, 11, 10, 15, 8, 0, 6, 1, 18, 2, 13, 19, 20},    // position of route
        {0, 3, 6, 6, 15, 3, 9, 12, 15, 9, 9, 12, 6, 0, 3, 0, 15, 0, 12, 18, 18},       // page start by route
        {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2},               // visible rows by route
        {4, 5, 10, 8, 18, 6, 13, 15, 17, 12, 11, 16, 9, 1, 7, 2, 19, 3, 14, 20, 1},    // NavNext position by route
        {2, 3, 8, 6, 16, 4, 11, 13, 15, 10, 9, 14, 7, 20, 5, 20, 17, 1, 12, 18, 19},   // NavPrevious position by route
        {1, 3, 10, 10, 19, 3, 18, 8, 19, 18, 18, 8, 10, 1, 3, 1, 19, 1, 8, -1, -1},    // swipe up route (-1 none)
        {-1, 15, 1, 1, 18, 15, 3, 10, 18, 3, 3, 10, 1, -1, 15, -1, 18, -1, 10, 8, 8},  // swipe down route (-1 none)
    },
    {
        /* main 14 p2p-clark-airport-balanga */
        {14, 15, 17, 0, 1, 5, 3, 12, 2, 10, 9, 6, 18, 7, 13, 11, 8, 4, 16, 19, 20},   // display order
        {3, 4, 8, 6, 17, 5, 11, 13, 16, 10, 9, 15, 7, 14, 0, 1, 18, 2, 12, 19, 20},   // position of route
        {0, 3, 6, 3, 15, 3, 9, 12, 15, 9, 6, 12, 6, 12, 0, 0, 15, 0, 9, 18, 18},      // page start by route
        {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2},              // visible rows by route
        {4, 5, 9, 7, 18, 6, 12, 14, 17, 11, 10, 16, 8, 15, 1, 2, 19, 3, 13, 20, 1},   // NavNext position by route
        {2, 3, 7, 5, 16, 4, 10, 12, 15, 9, 8, 14, 6, 13, 20, 20, 17, 1, 11, 18, 19},  // NavPrevious position by route
        {1, 12, 9, 12, 19, 12, 7, 8, 19, 7, 9, 8, 9, 8, 1, 1, 19, 1, 7, -1, -1},      // swipe up route (-1 none)
        {-1, 15, 1, 15, 7, 15, 12, 9, 7, 12, 1, 9, 1, 9, -1, -1, 7, -1, 12, 8, 8},    // swipe down route (-1 none)
    },
    {
        /* main 15 p2p-atc-greenbelt */
        {15, 17, 0, 1, 5, 14, 3, 12, 2, 10, 9, 6, 18, 7, 13, 11, 8, 4, 16, 19, 20},   // display order
        {2, 3, 8, 6, 17, 4, 11, 13, 16, 10, 9, 15, 7, 14, 5, 0, 18, 1, 12, 19, 20},   // position of route
        {0, 0, 6, 3, 15, 3, 9, 12, 15, 9, 6, 12, 6, 12, 3, 0, 15, 0, 9, 18, 18},      // page start by route
        {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2},              // visible rows by route
        {3, 4, 9, 7, 18, 5, 12, 14, 17, 11, 10, 16, 8, 15, 6, 1, 19, 2, 13, 20, 1},   // NavNext position by route
        {1, 2, 7, 5, 16, 3, 10, 12, 15, 9, 8, 14, 6, 13, 4, 20, 17, 20, 11, 18, 19},  // NavPrevious position by route
        {5, 5, 9, 12, 19, 12, 7, 8, 19, 7, 9, 8, 9, 8, 12, 5, 19, 5, 7, -1, -1},      // swipe up route (-1 none)
        {-1, -1, 5, 17, 7, 17, 12, 9, 7, 12, 5, 9, 5, 9, 17, -1, 7, -1, 12, 8, 8},    // swipe down route (-1 none)
    },
    {
        /* main 16 p2p-starmall-alabang-starmall-shaw */
        {16, 15, 17, 0, 1, 5, 14, 3, 12, 2, 10, 9, 6, 18, 7, 13, 11, 8, 4, 19, 20},     // display order
        {3, 4, 9, 7, 18, 5, 12, 14, 17, 11, 10, 16, 8, 15, 6, 1, 0, 2, 13, 19, 20},     // position of route
        {0, 3, 6, 6, 15, 3, 9, 12, 15, 9, 9, 15, 6, 12, 3, 0, 0, 0, 12, 18, 18},        // page start by route
        {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2},                // visible rows by route
        {4, 5, 10, 8, 19, 6, 13, 15, 18, 12, 11, 17, 9, 16, 7, 2, 1, 3, 14, 20, 1},     // NavNext position by route
        {2, 3, 8, 6, 17, 4, 11, 13, 16, 10, 9, 15, 7, 14, 5, 20, 20, 1, 12, 18, 19},    // NavPrevious position by route
        {1, 3, 10, 10, 19, 3, 18, 11, 19, 18, 18, 19, 10, 11, 3, 1, 1, 1, 11, -1, -1},  // swipe up route (-1 none)
        {-1, 15, 1, 1, 18, 15, 3, 10, 18, 3, 3, 18, 1, 10, 15, -1, -1, -1, 10, 11, 11},  // swipe down route (-1 none)
    },
    {
        /* main 17 p2p-atc-sm-megamall */
        {17, 15, 0, 1, 5, 14, 3, 12, 2, 10, 9, 6, 18, 7, 13, 11, 8, 4, 16, 19, 20},   // display order
        {2, 3, 8, 6, 17, 4, 11, 13, 16, 10, 9, 15, 7, 14, 5, 1, 18, 0, 12, 19, 20},   // position of route
        {0, 0, 6, 3, 15, 3, 9, 12, 15, 9, 6, 12, 6, 12, 3, 0, 15, 0, 9, 18, 18},      // page start by route
        {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2},              // visible rows by route
        {3, 4, 9, 7, 18, 5, 12, 14, 17, 11, 10, 16, 8, 15, 6, 2, 19, 1, 13, 20, 1},   // NavNext position by route
        {1, 2, 7, 5, 16, 3, 10, 12, 15, 9, 8, 14, 6, 13, 4, 20, 17, 20, 11, 18, 19},  // NavPrevious position by route
        {5, 5, 9, 12, 19, 12, 7, 8, 19, 7, 9, 8, 9, 8, 12, 5, 19, 5, 7, -1, -1},      // swipe up route (-1 none)
        {-1, -1, 5, 15, 7, 15, 12, 9, 7, 12, 5, 9, 5, 9, 15, -1, 7, -1, 12, 8, 8},    // swipe down route (-1 none)
    },
    {
        /* main 18 p2p-nuvali-one-ayala */
        {18, 15, 17, 0, 1, 5, 14, 3, 12, 2, 10, 9, 6, 7, 13, 11, 8, 4, 16, 19, 20},   // display order
        {3, 4, 9, 7, 17, 5, 12, 13, 16, 11, 10, 15, 8, 14, 6, 1, 18, 2, 0, 19, 20},   // position of route
        {0, 3, 6, 6, 15, 3, 9, 12, 15, 9, 9, 12, 6, 12, 3, 0, 15, 0, 0, 18, 18},      // page start by route
        {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2},              // visible rows by route
        {4, 5, 10, 8, 18, 6, 13, 14, 17, 12, 11, 16, 9, 15, 7, 2, 19, 3, 1, 20, 1},   // NavNext position by route
        {2, 3, 8, 6, 16, 4, 11, 12, 15, 10, 9, 14, 7, 13, 5, 20, 17, 1, 20, 18, 19},  // NavPrevious position by route
        {1, 3, 10, 10, 19, 3, 7, 8, 19, 7, 7, 8, 10, 8, 3, 1, 19, 1, 1, -1, -1},      // swipe up route (-1 none)
        {-1, 15, 1, 1, 7, 15, 3, 10, 7, 3, 3, 10, 1, 10, 15, -1, 7, -1, -1, 8, 8},    // swipe down route (-1 none)
    },
    {
        /* main 19 p2p-cabuyao-makati */
        {19, 15, 17, 0, 1, 5, 14, 3, 12, 2, 10, 9, 6, 18, 7, 13, 11, 8, 4, 16, 20},     // display order
        {3, 4, 9, 7, 18, 5, 12, 14, 17, 11, 10, 16, 8, 15, 6, 1, 19, 2, 13, 0, 20},     // position of route
        {0, 3, 6, 6, 15, 3, 9, 12, 15, 9, 9, 15, 6, 12, 3, 0, 18, 0, 12, 0, 18},        // page start by route
        {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3, 2},                // visible rows by route
        {4, 5, 10, 8, 19, 6, 13, 15, 18, 12, 11, 17, 9, 16, 7, 2, 20, 3, 14, 1, 1},     // NavNext position by route
        {2, 3, 8, 6, 17, 4, 11, 13, 16, 10, 9, 15, 7, 14, 5, 20, 18, 1, 12, 20, 19},    // NavPrevious position by route
        {1, 3, 10, 10, 16, 3, 18, 11, 16, 18, 18, 16, 10, 11, 3, 1, -1, 1, 11, 1, -1},  // swipe up route (-1 none)
        {-1, 15, 1, 1, 18, 15, 3, 10, 18, 3, 3, 18, 1, 10, 15, -1, 11, -1, 10, -1, 11},  // swipe down route (-1 none)
    },
    {
        /* main 20 p2p-cabuyao-bgc */
        {20, 15, 17, 0, 1, 5, 14, 3, 12, 2, 10, 9, 6, 18, 7, 13, 11, 8, 4, 16, 19},     // display order
        {3, 4, 9, 7, 18, 5, 12, 14, 17, 11, 10, 16, 8, 15, 6, 1, 19, 2, 13, 20, 0},     // position of route
        {0, 3, 6, 6, 15, 3, 9, 12, 15, 9, 9, 15, 6, 12, 3, 0, 18, 0, 12, 18, 0},        // page start by route
        {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 2, 3},                // visible rows by route
        {4, 5, 10, 8, 19, 6, 13, 15, 18, 12, 11, 17, 9, 16, 7, 2, 20, 3, 14, 1, 1},     // NavNext position by route
        {2, 3, 8, 6, 17, 4, 11, 13, 16, 10, 9, 15, 7, 14, 5, 20, 18, 1, 12, 19, 20},    // NavPrevious position by route
        {1, 3, 10, 10, 16, 3, 18, 11, 16, 18, 18, 16, 10, 11, 3, 1, -1, 1, 11, -1, 1},  // swipe up route (-1 none)
        {-1, 15, 1, 1, 18, 15, 3, 10, 18, 3, 3, 18, 1, 10, 15, -1, 11, -1, 10, 11, -1},  // swipe down route (-1 none)
    },
};

RouteResolution canonical(const size_t index) { return {RouteResolutionState::Canonical, index}; }

void expectMatchesGolden(const RouteListModel& model, const Golden& golden) {
  ASSERT_EQ(model.count(), N);
  for (size_t position = 0; position < N; ++position) {
    EXPECT_EQ(model.routeAtPosition(position), golden.display[position]) << "position " << position;
  }
  for (size_t route = 0; route < N; ++route) {
    const size_t position = model.positionOfRoute(route);
    EXPECT_EQ(position, golden.positionOf[route]) << "route " << route;
    const size_t start = model.pageStart(position);
    EXPECT_EQ(start, golden.pageStart[route]) << "route " << route;
    EXPECT_EQ(model.visibleRows(start), golden.visible[route]) << "route " << route;
    EXPECT_EQ(model.nextPosition(position), golden.next[route]) << "route " << route;
    EXPECT_EQ(model.previousPosition(position), golden.previous[route]) << "route " << route;
    size_t target = 0;
    const bool up = model.swipeTarget(position, true, target);
    EXPECT_EQ(up ? static_cast<int>(model.routeAtPosition(target)) : -1, golden.swipeUp[route]) << "route " << route;
    const bool down = model.swipeTarget(position, false, target);
    EXPECT_EQ(down ? static_cast<int>(model.routeAtPosition(target)) : -1, golden.swipeDown[route])
        << "route " << route;
  }
}

TEST(OnPointRouteList, ComparatorKeepsTheShippedAlphabeticalOrder) {
  for (size_t position = 0; position < N; ++position) {
    EXPECT_EQ(on_point::routeIndexInAlphabeticalOrder(position), SORTED[position]);
    EXPECT_EQ(legacy_route_list::routeIndexInAlphabeticalOrder(position), SORTED[position]);
  }
  for (size_t i = 0; i + 1 < N; ++i) EXPECT_LT(on_point::compareRoutes(SORTED[i], SORTED[i + 1]), 0);
  EXPECT_EQ(on_point::compareRoutes(3, 3), 0);
}

TEST(OnPointRouteList, RouteIdsAreUniqueSoTheOrderIsTotal) {
  for (size_t a = 0; a < N; ++a)
    for (size_t b = a + 1; b < N; ++b) EXPECT_STRNE(on_point::routeAt(a).routeId, on_point::routeAt(b).routeId);
}

TEST(OnPointRouteList, ShippedCatalogueHoldsNoCapacityFixtureData) {
  ASSERT_EQ(on_point::routeCount(), N);
  for (size_t index = 0; index < N; ++index) {
    EXPECT_NE(strncmp(on_point::routeAt(index).routeId, "fixture-", 8), 0);
    EXPECT_EQ(strstr(on_point::scheduleForRoute(index, false).origin, "FIXTURE"), nullptr);
  }
}

TEST(OnPointRouteList, EveryMainRouteMatchesTheV120Golden) {
  for (size_t main = 0; main < N; ++main) {
    SCOPED_TRACE(main);
    RouteListModel model;
    model.build(canonical(main));
    EXPECT_TRUE(model.hasMainRoute());
    EXPECT_EQ(model.pinnedRoute(), main);
    expectMatchesGolden(model, GOLDEN[main]);
    for (size_t route = 0; route < N; ++route) EXPECT_EQ(model.isMainRoute(route), route == main);
  }
}

TEST(OnPointRouteList, AliasedMainRouteBehavesAsItsTarget) {
  RouteListModel model;
  model.build({RouteResolutionState::Aliased, 6});
  EXPECT_TRUE(model.hasMainRoute());
  EXPECT_TRUE(model.isMainRoute(6));
  expectMatchesGolden(model, GOLDEN[6]);
  EXPECT_EQ(on_point::pinnedHeading(RouteResolutionState::Aliased), on_point::PinnedHeading::MainRoute);
}

// No main route: ATC ALABANG > GREENBELT (first in sort order) fills the pinned
// slot, the list keeps the same order and paging, and no route is the main.
TEST(OnPointRouteList, NoMainRouteStatePinsTheFirstSortedRouteWithoutMakingItMain) {
  for (const auto state : {RouteResolutionState::Unknown, RouteResolutionState::Unset}) {
    SCOPED_TRACE(static_cast<int>(state));
    RouteListModel model;
    model.build({state, 4});  // a stale index must be ignored
    EXPECT_FALSE(model.hasMainRoute());
    EXPECT_EQ(model.pinnedRoute(), SORTED[0]);
    EXPECT_EQ(model.pinnedRoute(), on_point::routeIndexForId("p2p-atc-greenbelt"));
    expectMatchesGolden(model, GOLDEN[SORTED[0]]);
    for (size_t route = 0; route < N; ++route) EXPECT_FALSE(model.isMainRoute(route)) << route;
    EXPECT_EQ(on_point::pinnedHeading(state), on_point::PinnedHeading::NoMainRoute);
  }
}

TEST(OnPointRouteList, NoMainRouteHeadingAppearsOnlyWithoutAMainRoute) {
  size_t noMain = 0;
  for (const auto state : {RouteResolutionState::Canonical, RouteResolutionState::Aliased, RouteResolutionState::Unset,
                           RouteResolutionState::Unknown}) {
    if (on_point::pinnedHeading(state) == on_point::PinnedHeading::NoMainRoute) ++noMain;
  }
  EXPECT_EQ(noMain, 2u);
  EXPECT_EQ(on_point::pinnedHeading(RouteResolutionState::Canonical), on_point::PinnedHeading::MainRoute);
}

TEST(OnPointRouteList, OutOfRangeResolvedIndexFallsToNoMainRoute) {
  RouteListModel model;
  model.build(canonical(N + 5));
  EXPECT_FALSE(model.hasMainRoute());
  EXPECT_EQ(model.resolution().state, RouteResolutionState::Unknown);
  EXPECT_EQ(model.pinnedRoute(), SORTED[0]);
}

TEST(OnPointRouteList, MatchesLegacyFunctionsForEveryMainAndSelection) {
  for (size_t main = 0; main < N; ++main) {
    RouteListModel model;
    model.build(canonical(main));
    for (size_t d = 0; d < N; ++d) EXPECT_EQ(model.routeAtPosition(d), legacy_route_list::displayRouteIndex(main, d));
    for (size_t route = 0; route < N; ++route) {
      const size_t position = model.positionOfRoute(route);
      EXPECT_EQ(position, legacy_route_list::selectedDisplayIndex(main, route));
      EXPECT_EQ(model.pageStart(position), legacy_route_list::routeListFirstPosition(main, route));
      EXPECT_EQ(model.visibleRows(model.pageStart(position)), legacy_route_list::routeListVisibleCount(main, route));
      EXPECT_EQ(model.nextPosition(position), legacy_route_list::navNext(main, route));
      EXPECT_EQ(model.previousPosition(position), legacy_route_list::navPrevious(main, route));
    }
  }
}

// --- touch ---------------------------------------------------------------

struct Probe {
  int x;
  int y;
};

TEST(OnPointRouteList, TouchMatchesLegacyAcrossEdgesGapsAndOffScreenPoints) {
  const Probe probes[] = {
      {0, 102},   {479, 102}, {240, 101}, {240, 251},  {240, 252},   {240, 280}, {240, 315},
      {240, 316}, {240, 419}, {240, 420}, {240, 441},  {240, 442},   {240, 545}, {240, 546},
      {240, 567}, {240, 568}, {240, 671}, {240, 672},  {240, 700},   {240, 799}, {-1, 316},
      {480, 316}, {240, -1},  {240, 800}, {240, 5000}, {-500, -500}, {0, 0},     {479, 799},
  };
  for (size_t main = 0; main < N; ++main) {
    RouteListModel model;
    model.build(canonical(main));
    for (size_t selected = 0; selected < N; ++selected) {
      for (const Probe& p : probes) {
        size_t expectedRoute = 999;
        size_t route = 999;
        const bool expected = legacy_route_list::routeAtPoint(main, selected, p.x, p.y, SCREEN_WIDTH, expectedRoute);
        const bool hit = model.routeAtPoint(p.x, p.y, SCREEN_WIDTH, model.positionOfRoute(selected), route);
        EXPECT_EQ(hit, expected) << "main " << main << " sel " << selected << " (" << p.x << "," << p.y << ")";
        if (hit && expected) EXPECT_EQ(route, expectedRoute);
      }
    }
  }
}

TEST(OnPointRouteList, TouchRowEdgesOnTheFirstPage) {
  RouteListModel model;
  model.build(canonical(0));
  const size_t pinned = 0;
  size_t route = 0;
  // Pinned card: rows 102..251.
  ASSERT_TRUE(model.routeAtPoint(10, 102, SCREEN_WIDTH, pinned, route));
  EXPECT_EQ(route, 0u);
  ASSERT_TRUE(model.routeAtPoint(10, 251, SCREEN_WIDTH, pinned, route));
  EXPECT_FALSE(model.routeAtPoint(10, 252, SCREEN_WIDTH, pinned, route));
  EXPECT_FALSE(model.routeAtPoint(10, 315, SCREEN_WIDTH, pinned, route));
  // Rows: 316..419, gap 420..441, 442..545, gap 546..567, 568..671.
  ASSERT_TRUE(model.routeAtPoint(10, 316, SCREEN_WIDTH, pinned, route));
  EXPECT_EQ(route, GOLDEN[0].display[1]);
  ASSERT_TRUE(model.routeAtPoint(10, 419, SCREEN_WIDTH, pinned, route));
  EXPECT_EQ(route, GOLDEN[0].display[1]);
  EXPECT_FALSE(model.routeAtPoint(10, 420, SCREEN_WIDTH, pinned, route));
  EXPECT_FALSE(model.routeAtPoint(10, 441, SCREEN_WIDTH, pinned, route));
  ASSERT_TRUE(model.routeAtPoint(10, 442, SCREEN_WIDTH, pinned, route));
  EXPECT_EQ(route, GOLDEN[0].display[2]);
  ASSERT_TRUE(model.routeAtPoint(10, 671, SCREEN_WIDTH, pinned, route));
  EXPECT_EQ(route, GOLDEN[0].display[3]);
  EXPECT_FALSE(model.routeAtPoint(10, 672, SCREEN_WIDTH, pinned, route));  // below the third row
  // Off screen.
  EXPECT_FALSE(model.routeAtPoint(-1, 120, SCREEN_WIDTH, pinned, route));
  EXPECT_FALSE(model.routeAtPoint(SCREEN_WIDTH, 120, SCREEN_WIDTH, pinned, route));
  EXPECT_FALSE(model.routeAtPoint(10, -1, SCREEN_WIDTH, pinned, route));
}

// --- departures (pinned so Stage 1 provably leaves schedules alone) --------

enum Instant { WEEKDAY, WEEKEND, HOLIDAY, WEEKDAY_EVENING };

// UTC instants: Wed 2026-10-21 04:20, Sun 2026-10-25 04:20, Fri 2026-12-25
// 04:20 (Christmas, non-working) and Wed 2026-10-21 23:05:30, all Manila time.
constexpr on_point::CivilDateTime INSTANTS[] = {
    {2026, 10, 20, 20, 20, 0}, {2026, 10, 24, 20, 20, 0}, {2026, 12, 24, 20, 20, 0}, {2026, 10, 21, 15, 5, 30}};

struct DepartureGolden {
  size_t schedule;
  Instant instant;
  int status;  // on_point::ServiceStatus
  const char* next;
  const char* last;
  int minutesToNext;
  size_t followingCount;
};

constexpr DepartureGolden DEPARTURES[] = {
    {0, WEEKDAY, 2, "04:30", "20:00", 10, 3},   {0, WEEKEND, 2, "05:00", "20:00", 40, 3},
    {0, HOLIDAY, 2, "05:00", "20:00", 40, 3},   {0, WEEKDAY_EVENING, 4, "04:30", "20:00", 325, 3},
    {1, WEEKDAY, 2, "06:00", "22:30", 100, 3},  {1, WEEKEND, 2, "06:30", "22:30", 130, 3},
    {1, HOLIDAY, 2, "06:30", "22:30", 130, 3},  {1, WEEKDAY_EVENING, 4, "06:00", "22:30", 415, 3},
    {2, WEEKDAY, 2, "05:00", "20:30", 40, 3},   {2, WEEKEND, 2, "05:00", "20:30", 40, 3},
    {2, HOLIDAY, 2, "05:00", "20:30", 40, 3},   {2, WEEKDAY_EVENING, 4, "05:00", "20:30", 355, 3},
    {3, WEEKDAY, 2, "06:00", "22:30", 100, 3},  {3, WEEKEND, 2, "06:00", "22:30", 100, 3},
    {3, HOLIDAY, 2, "06:00", "22:30", 100, 3},  {3, WEEKDAY_EVENING, 4, "06:00", "22:30", 415, 3},
    {4, WEEKDAY, 3, "05:10", "21:10", 50, 3},   {4, WEEKEND, 3, "05:10", "21:10", 50, 3},
    {4, HOLIDAY, 3, "05:10", "21:10", 50, 3},   {4, WEEKDAY_EVENING, 4, "03:30", "21:10", 265, 3},
    {5, WEEKDAY, 2, "04:30", "20:30", 10, 3},   {5, WEEKEND, 2, "04:30", "20:30", 10, 3},
    {5, HOLIDAY, 2, "04:30", "20:30", 10, 3},   {5, WEEKDAY_EVENING, 4, "04:30", "20:30", 325, 3},
    {6, WEEKDAY, 3, "04:30", "22:30", 10, 3},   {6, WEEKEND, 3, "04:30", "22:30", 10, 3},
    {6, HOLIDAY, 3, "04:30", "22:30", 10, 3},   {6, WEEKDAY_EVENING, 4, "00:30", "22:30", 85, 3},
    {7, WEEKDAY, 3, "04:30", "22:30", 10, 3},   {7, WEEKEND, 3, "04:30", "22:30", 10, 3},
    {7, HOLIDAY, 3, "04:30", "22:30", 10, 3},   {7, WEEKDAY_EVENING, 4, "00:00", "22:30", 55, 3},
    {8, WEEKDAY, 2, "04:30", "23:00", 10, 3},   {8, WEEKEND, 2, "04:30", "23:00", 10, 3},
    {8, HOLIDAY, 2, "04:30", "23:00", 10, 3},   {8, WEEKDAY_EVENING, 4, "04:30", "23:00", 325, 3},
    {9, WEEKDAY, 3, "04:30", "20:30", 10, 3},   {9, WEEKEND, 3, "04:30", "20:30", 10, 3},
    {9, HOLIDAY, 3, "04:30", "20:30", 10, 3},   {9, WEEKDAY_EVENING, 4, "03:30", "20:30", 265, 3},
    {10, WEEKDAY, 3, "08:00", "19:30", 220, 3}, {10, WEEKEND, 3, "08:00", "19:30", 220, 3},
    {10, HOLIDAY, 3, "08:00", "19:30", 220, 3}, {10, WEEKDAY_EVENING, 4, "02:00", "19:30", 175, 3},
    {11, WEEKDAY, 3, "06:00", "18:00", 100, 3}, {11, WEEKEND, 3, "06:00", "18:00", 100, 3},
    {11, HOLIDAY, 3, "06:00", "18:00", 100, 3}, {11, WEEKDAY_EVENING, 4, "03:00", "18:00", 235, 3},
    {12, WEEKDAY, 2, "04:30", "18:45", 10, 3},  {12, WEEKEND, 2, "04:30", "18:45", 10, 3},
    {12, HOLIDAY, 2, "04:30", "18:45", 10, 3},  {12, WEEKDAY_EVENING, 4, "04:30", "18:45", 325, 3},
    {13, WEEKDAY, 2, "04:45", "18:30", 25, 3},  {13, WEEKEND, 2, "04:45", "18:30", 25, 3},
    {13, HOLIDAY, 2, "04:45", "18:30", 25, 3},  {13, WEEKDAY_EVENING, 4, "04:45", "18:30", 340, 3},
};

TEST(OnPointRouteList, ShippedDeparturesAreUnchanged) {
  for (const DepartureGolden& g : DEPARTURES) {
    SCOPED_TRACE(g.schedule);
    SCOPED_TRACE(static_cast<int>(g.instant));
    const auto state = on_point::getDepartureState(on_point::scheduleAt(g.schedule), INSTANTS[g.instant]);
    EXPECT_EQ(static_cast<int>(state.serviceStatus), g.status);
    char next[6];
    char last[6];
    snprintf(next, sizeof(next), "%02u:%02u", state.next.local.hour, state.next.local.minute);
    snprintf(last, sizeof(last), "%02u:%02u", state.last.local.hour, state.last.local.minute);
    EXPECT_STREQ(next, g.next);
    EXPECT_STREQ(last, g.last);
    EXPECT_EQ(state.minutesToNext, g.minutesToNext);
    EXPECT_EQ(state.followingCount, g.followingCount);
  }
}

}  // namespace
