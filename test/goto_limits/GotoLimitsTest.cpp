#include <gtest/gtest.h>

#include "activities/goto/GotoLimits.h"

namespace {

// Valid production editionIds must pass (they build the SD cache path).
TEST(GotoLimits, AcceptsValidProductionEditionIds) {
  EXPECT_TRUE(goto_limits::isSafeEditionId("goto-2026-09-14"));
  EXPECT_TRUE(goto_limits::isSafeEditionId("togo-2026-09-14"));
  EXPECT_TRUE(goto_limits::isSafeEditionId("goto-2026-12-31"));
}

// Anything path-shaped / traversal-shaped / out-of-grammar is rejected, so a
// remote editionId can never escape /goto/editions/<id>.json.
TEST(GotoLimits, RejectsUnsafeEditionIds) {
  const char* bad[] = {
      "",
      "goto",                                  // too short / no date
      "../../etc/passwd",                      // traversal
      "goto-../../../secret",                  // traversal with prefix
      "goto/2026",                             // slash
      "goto-2026-09-14/..",                    // trailing traversal
      "GOTO-2026-09-14",                       // uppercase (grammar)
      "news-2026-09-14",                       // wrong prefix
      "goto-2026_09_14.json",                  // dot / underscore
      "goto-2026-09-14\n",                     // control char
      "goto-2026-09-14 ",                      // space
      "goto-this-id-is-way-too-long-to-be-valid-and-exceeds-the-cap",  // over length cap
  };
  for (const char* id : bad) EXPECT_FALSE(goto_limits::isSafeEditionId(id)) << "should reject: " << id;
}

// Limits are ordered/sane and comfortably above realistic production payloads
// while below the C3 heap ceiling.
TEST(GotoLimits, LimitsAreSaneAndDefensive) {
  EXPECT_GT(goto_limits::kMaxManifestBytes, 2u * 1024);      // >> ~0.6 KB current.json
  EXPECT_LT(goto_limits::kMaxManifestBytes, 64u * 1024);
  EXPECT_GT(goto_limits::kMaxEditionBytes, 16u * 1024);      // >> ~7 KB edition
  EXPECT_LT(goto_limits::kMaxEditionBytes, 128u * 1024);     // safely below heap
  EXPECT_GE(goto_limits::kMaxStories, 10u);                  // product ships up to 10
  EXPECT_LE(goto_limits::kMaxStories, 32u);                  // modest defensive ceiling
  EXPECT_EQ(goto_limits::kMaxUrlLen, 2953u);                 // == QR ECC_LOW v40 capacity
}

}  // namespace
