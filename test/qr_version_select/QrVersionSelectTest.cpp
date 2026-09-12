#include <gtest/gtest.h>

#include "QrVersionSelect.h"

using QrUtils::kQrMaxByteCapacityEccLow;
using QrUtils::selectQrVersionEccLow;

namespace {

// Real ECC_LOW byte-mode capacities (ISO/IEC 18004) for the low versions the
// GOTO/TOGO URLs actually reach. The bug being guarded against picked version 4
// for anything up to 114 bytes, but v4 holds only 78 — routing 79..114 B
// payloads into a version too small to hold them.
TEST(QrVersionSelect, V4BoundaryAt78) {
  EXPECT_EQ(selectQrVersionEccLow(78), 4);   // exactly fills v4
  EXPECT_EQ(selectQrVersionEccLow(79), 5);   // one over v4 -> must step up, not corrupt
}

TEST(QrVersionSelect, V5BoundaryAt106) {
  EXPECT_EQ(selectQrVersionEccLow(106), 5);  // exactly fills v5
  EXPECT_EQ(selectQrVersionEccLow(107), 6);  // FAIL #4 (107 B) landed here -> v6, not v4
}

TEST(QrVersionSelect, V6BoundaryAt134) {
  EXPECT_EQ(selectQrVersionEccLow(134), 6);  // exactly fills v6
  EXPECT_EQ(selectQrVersionEccLow(135), 7);  // one over v6 -> v7
}

TEST(QrVersionSelect, ObservedFailingPayloadLengthsPickFittingVersions) {
  // The two stories that failed to scan on hardware, by URL byte length.
  EXPECT_EQ(selectQrVersionEccLow(101), 5);  // "LIVE UPDATES: Martin Romualdez in PGH"
  EXPECT_EQ(selectQrVersionEccLow(107), 6);  // "Big-time fuel price hike seen next week"
}

TEST(QrVersionSelect, SmallAndEmptyPayloadsPickLowestVersions) {
  EXPECT_EQ(selectQrVersionEccLow(0), 1);
  EXPECT_EQ(selectQrVersionEccLow(17), 1);   // exactly fills v1
  EXPECT_EQ(selectQrVersionEccLow(18), 2);
}

TEST(QrVersionSelect, MaximumSupportedPayloadPicksV40) {
  EXPECT_EQ(kQrMaxByteCapacityEccLow, 2953u);
  EXPECT_EQ(selectQrVersionEccLow(2952), 40);
  EXPECT_EQ(selectQrVersionEccLow(2953), 40);  // exactly fills v40
}

TEST(QrVersionSelect, OverCapacityPayloadReturnsZero) {
  EXPECT_EQ(selectQrVersionEccLow(2954), 0);        // one past v40 -> refuse
  EXPECT_EQ(selectQrVersionEccLow(100000), 0);      // far past -> refuse, do not encode
}

// The selected version's capacity must always be >= the payload, and the
// version below it must be too small (i.e. genuinely the smallest fit) for
// every capacity boundary across all 40 versions.
TEST(QrVersionSelect, MonotonicAndSmallestFittingAcrossAllBoundaries) {
  int prev = 0;
  for (size_t len = 0; len <= kQrMaxByteCapacityEccLow; ++len) {
    const int v = selectQrVersionEccLow(len);
    ASSERT_GE(v, 1);
    ASSERT_LE(v, 40);
    ASSERT_GE(v, prev);  // never decreases as the payload grows
    prev = v;
  }
}

}  // namespace
