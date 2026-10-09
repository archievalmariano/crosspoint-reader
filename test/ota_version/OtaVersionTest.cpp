// OTA version comparison (src/network/OtaVersion.h): which release tags count as
// an update for the running firmware's plain semver.
#include <gtest/gtest.h>

#include <cstring>
#include <string>

#include "network/OtaAsset.h"
#include "network/OtaVersion.h"

namespace {

TEST(OtaVersion, ParsesPlainAndVPrefixed) {
  int a = 0, b = 0, c = 0;
  ASSERT_TRUE(ota_version::parse("1.2.3", a, b, c));
  EXPECT_EQ(a, 1);
  EXPECT_EQ(b, 2);
  EXPECT_EQ(c, 3);
  ASSERT_TRUE(ota_version::parse("v10.0.7", a, b, c));
  EXPECT_EQ(a, 10);
  EXPECT_EQ(c, 7);
}

TEST(OtaVersion, RejectsDisplayStringsAndGarbage) {
  int a = 0, b = 0, c = 0;
  EXPECT_FALSE(ota_version::parse("A1.0.0", a, b, c));
  EXPECT_FALSE(ota_version::parse("1.2", a, b, c));
  EXPECT_FALSE(ota_version::parse("", a, b, c));
  EXPECT_FALSE(ota_version::parse(nullptr, a, b, c));
}

TEST(OtaVersion, NewerBySegment) {
  EXPECT_TRUE(ota_version::isNewer("2.0.0", "1.9.9"));
  EXPECT_TRUE(ota_version::isNewer("1.1.0", "1.0.9"));
  EXPECT_TRUE(ota_version::isNewer("v1.0.1", "1.0.0"));
  EXPECT_FALSE(ota_version::isNewer("1.0.0", "1.0.1"));
  EXPECT_FALSE(ota_version::isNewer("1.0.0", "1.0.0"));
}

TEST(OtaVersion, ReleaseCandidateIsOlderThanItsFinal) {
  EXPECT_TRUE(ota_version::isNewer("1.0.0", "1.0.0-rc+abc123"));
  EXPECT_FALSE(ota_version::isNewer("1.0.0-rc+abc", "1.0.0"));
}

TEST(OtaVersion, UnparseableNeverOffersUpdate) {
  // An upstream CrossPoint tag must not replace a build whose version cannot be read.
  EXPECT_FALSE(ota_version::isNewer("9.9.9", "APPS 1.0.0"));
  EXPECT_FALSE(ota_version::isNewer("nightly", "1.0.0"));
}

std::string assetName(const char* board, const char* package) {
  char out[48];
  if (!ota_asset::name(out, sizeof(out), board, std::strlen(board), package)) return "<overflow>";
  return out;
}

TEST(OtaAsset, SuitePackagesNameBoardAndPackage) {
  EXPECT_EQ(assetName("x4", "g"), "firmware-x4-g.bin");
  EXPECT_EQ(assetName("x4pro", "g"), "firmware-x4pro-g.bin");
  EXPECT_EQ(assetName("x4pro", "p"), "firmware-x4pro-p.bin");
  EXPECT_EQ(assetName("x4pro", "gp"), "firmware-x4pro-gp.bin");
}

TEST(OtaAsset, BuildsWithoutPackageKeepUpstreamNames) {
  EXPECT_EQ(assetName("x4", ""), "firmware.bin");
  EXPECT_EQ(assetName("x4", nullptr), "firmware.bin");
  EXPECT_EQ(assetName("x4pro", ""), "firmware-x4pro.bin");
  EXPECT_EQ(assetName("x4c", nullptr), "firmware-x4c.bin");
}

TEST(OtaAsset, ReportsOverflow) {
  char tiny[8];
  EXPECT_FALSE(ota_asset::name(tiny, sizeof(tiny), "x4pro", 5, "gp"));
}

}  // namespace
