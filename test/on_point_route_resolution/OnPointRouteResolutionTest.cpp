#include <gtest/gtest.h>

#include <cstring>
#include <fstream>
#include <iterator>
#include <regex>
#include <string>

#include "activities/on_point/OnPointRouteResolution.h"
#include "activities/on_point/OnPointScheduleData.h"

namespace {

using on_point::AliasTable;
using on_point::RouteAlias;
using on_point::RouteResolutionState;

// The settings field is char[48] (CrossPointSettings::onPointMainRouteId).
constexpr size_t SETTINGS_BUFFER = 48;

on_point::RouteResolution resolve(const char* saved, const size_t bufSize = SETTINGS_BUFFER,
                                  const AliasTable aliases = on_point::productionAliases()) {
  return on_point::resolveSavedRoute(saved, bufSize, aliases);
}

TEST(OnPointRouteResolution, EveryShippedIdIsCanonical) {
  for (size_t index = 0; index < on_point::routeCount(); ++index) {
    char buffer[SETTINGS_BUFFER] = {};
    strncpy(buffer, on_point::routeAt(index).routeId, sizeof(buffer) - 1);
    const auto result = resolve(buffer);
    EXPECT_EQ(result.state, RouteResolutionState::Canonical) << buffer;
    EXPECT_EQ(result.index, index);
    EXPECT_TRUE(result.hasRoute());
  }
}

TEST(OnPointRouteResolution, ShippedDefaultResolvesToBalagtasTrinoma) {
  const auto result = resolve(on_point::DEFAULT_MAIN_ROUTE_ID);
  ASSERT_EQ(result.state, RouteResolutionState::Canonical);
  EXPECT_STREQ(on_point::routeAt(result.index).routeId, "p2p-balagtas-trinoma");
  EXPECT_STREQ(on_point::scheduleForRoute(result.index, false).origin, "BALAGTAS");
}

// A missing settings file or key keeps the struct initializer (see
// CrossPointSettings::fromJson), so that initializer must stay the default
// the resolver knows. Read from the header so the two can't drift apart.
TEST(OnPointRouteResolution, SettingsInitializerIsTheDefaultRoute) {
  std::ifstream header(std::string(REPO_ROOT_PATH) + "/src/CrossPointSettings.h");
  ASSERT_TRUE(header.good());
  const std::string text((std::istreambuf_iterator<char>(header)), std::istreambuf_iterator<char>());
  std::smatch match;
  ASSERT_TRUE(std::regex_search(text, match, std::regex(R"re(char onPointMainRouteId\[(\d+)\] = "([^"]*)";)re")));
  EXPECT_EQ(match[1].str(), std::to_string(SETTINGS_BUFFER));
  EXPECT_EQ(match[2].str(), on_point::DEFAULT_MAIN_ROUTE_ID);
}

TEST(OnPointRouteResolution, NullAndEmptyAreUnset) {
  EXPECT_EQ(resolve(nullptr).state, RouteResolutionState::Unset);
  EXPECT_EQ(resolve("").state, RouteResolutionState::Unset);
  const char empty[SETTINGS_BUFFER] = {};
  EXPECT_EQ(resolve(empty).state, RouteResolutionState::Unset);
  EXPECT_EQ(resolve("p2p-naia-pitx", 0).state, RouteResolutionState::Unset);
  EXPECT_FALSE(resolve(nullptr).hasRoute());
}

TEST(OnPointRouteResolution, NearMissesAreUnknownNeverSubstituted) {
  for (const char* saved : {"P2P-BALAGTAS-TRINOMA", "p2p-balagtas-trinoma ", " p2p-balagtas-trinoma",
                            "p2p-balagtas-trinom", "missing-route", "p2p-araneta-city-naia"}) {
    const auto result = resolve(saved);
    EXPECT_EQ(result.state, RouteResolutionState::Unknown) << saved;
    EXPECT_FALSE(result.hasRoute()) << saved;
  }
}

TEST(OnPointRouteResolution, LongestStorableUnknownIdIsUnknown) {
  char buffer[SETTINGS_BUFFER];
  memset(buffer, 'x', sizeof(buffer) - 1);
  buffer[sizeof(buffer) - 1] = '\0';
  ASSERT_EQ(strlen(buffer), SETTINGS_BUFFER - 1);
  EXPECT_EQ(resolve(buffer).state, RouteResolutionState::Unknown);
}

TEST(OnPointRouteResolution, UnterminatedBufferIsUnknownAndNotOverread) {
  // A shipped ID filling the whole buffer with no terminator: the resolver may
  // read only these bytes.
  char buffer[13];
  memcpy(buffer, "p2p-naia-pitx", sizeof(buffer));
  EXPECT_EQ(resolve(buffer, sizeof(buffer)).state, RouteResolutionState::Unknown);
  // The same bytes with room for the terminator resolve normally.
  EXPECT_EQ(resolve("p2p-naia-pitx", 14).state, RouteResolutionState::Canonical);
}

TEST(OnPointRouteResolution, ProductionAliasTableIsEmpty) { EXPECT_EQ(on_point::productionAliases().count, 0u); }

TEST(OnPointRouteResolution, AliasResolvesToItsTarget) {
  static constexpr RouteAlias ALIASES[] = {{"old-airport-link", "p2p-naia-pitx"}, {"gone", "no-such-route"}};
  const AliasTable table{ALIASES, 2};
  const auto aliased = resolve("old-airport-link", SETTINGS_BUFFER, table);
  EXPECT_EQ(aliased.state, RouteResolutionState::Aliased);
  EXPECT_EQ(aliased.index, on_point::routeIndexForId("p2p-naia-pitx"));
  EXPECT_TRUE(aliased.hasRoute());
  // An alias whose target this build lacks stays Unknown.
  EXPECT_EQ(resolve("gone", SETTINGS_BUFFER, table).state, RouteResolutionState::Unknown);
  // A canonical ID wins over any alias.
  EXPECT_EQ(resolve("p2p-naia-pitx", SETTINGS_BUFFER, table).state, RouteResolutionState::Canonical);
  // Aliases are case-sensitive too.
  EXPECT_EQ(resolve("OLD-AIRPORT-LINK", SETTINGS_BUFFER, table).state, RouteResolutionState::Unknown);
}

TEST(OnPointRouteResolution, NullAliasEntriesWithACountAreIgnored) {
  const AliasTable broken{nullptr, 3};
  EXPECT_EQ(resolve("old-airport-link", SETTINGS_BUFFER, broken).state, RouteResolutionState::Unknown);
  EXPECT_EQ(resolve("p2p-naia-pitx", SETTINGS_BUFFER, broken).state, RouteResolutionState::Canonical);
  EXPECT_EQ(resolve("", SETTINGS_BUFFER, broken).state, RouteResolutionState::Unset);
}

TEST(OnPointRouteResolution, ResolverNeverWritesTheSavedValue) {
  char buffer[SETTINGS_BUFFER] = "not-a-route";
  char before[SETTINGS_BUFFER];
  memcpy(before, buffer, sizeof(buffer));
  (void)resolve(buffer);
  EXPECT_EQ(memcmp(buffer, before, sizeof(buffer)), 0);
}

}  // namespace
