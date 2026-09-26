#include <gtest/gtest.h>

#include <memory>

#include "activities/apps/AppRegistry.h"

namespace {

using app_registry::AppKey;

// The firmware's sort keys (src/activities/apps/AppsActivity.cpp).
constexpr AppKey kGoto{AppId::Goto, "GOTO"};
constexpr AppKey kOnPoint{AppId::OnPoint, "ON POINT"};
constexpr AppKey kGate{AppId::Gate, "THE GATE IS OPEN!"};

template <size_t N>
void expectOrder(const AppKey (&keys)[N], std::initializer_list<AppId> expected) {
  uint8_t order[N]{};
  app_registry::sortedOrder(keys, N, order);
  ASSERT_EQ(expected.size(), N);
  size_t row = 0;
  for (const AppId id : expected) {
    EXPECT_EQ(keys[order[row]].id, id) << "row " << row;
    ++row;
  }
}

TEST(AppRegistry, SortsByKeyWithGateCompiledIn) {
  const AppKey keys[] = {kGoto, kOnPoint, kGate};
  expectOrder(keys, {AppId::Goto, AppId::OnPoint, AppId::Gate});
}

TEST(AppRegistry, SortsByKeyWithGateCompiledOut) {
  const AppKey keys[] = {kGoto, kOnPoint};
  expectOrder(keys, {AppId::Goto, AppId::OnPoint});
}

TEST(AppRegistry, OrderIsIndependentOfDeclarationOrder) {
  const AppKey keys[] = {kGate, kOnPoint, kGoto};
  expectOrder(keys, {AppId::Goto, AppId::OnPoint, AppId::Gate});
}

TEST(AppRegistry, SortsOnKeyNotDisplayLabel) {
  // GOTO renders as "TOGO" in the evening; its key stays "GOTO", so it must
  // not jump past ON POINT (which a label sort on "TOGO" would do).
  const AppKey keys[] = {kOnPoint, kGoto};
  expectOrder(keys, {AppId::Goto, AppId::OnPoint});
}

TEST(AppRegistry, CompareIsAsciiCaseInsensitive) {
  EXPECT_EQ(app_registry::compareSortKeys("goto", "GOTO"), 0);
  EXPECT_LT(app_registry::compareSortKeys("goto", "ON POINT"), 0);
  EXPECT_GT(app_registry::compareSortKeys("on point", "GOTO"), 0);
  EXPECT_LT(app_registry::compareSortKeys("GO", "GOTO"), 0);  // prefix sorts first
}

TEST(AppRegistry, EqualKeysTieBreakOnAppId) {
  const AppKey keys[] = {{AppId::Gate, "SAME"}, {AppId::Goto, "same"}};
  expectOrder(keys, {AppId::Goto, AppId::Gate});
}

TEST(AppRegistry, FocusMapsToSortedRowNotTableIndex) {
  const AppKey keys[] = {kGate, kOnPoint, kGoto};  // table index != sorted row
  uint8_t order[3]{};
  app_registry::sortedOrder(keys, 3, order);
  EXPECT_EQ(app_registry::rowForApp(keys, order, 3, AppId::Goto), 0);
  EXPECT_EQ(app_registry::rowForApp(keys, order, 3, AppId::OnPoint), 1);
  EXPECT_EQ(app_registry::rowForApp(keys, order, 3, AppId::Gate), 2);
}

TEST(AppRegistry, AbsentOrNoFocusFallsBackToFirstRow) {
  const AppKey keys[] = {kOnPoint, kGoto};  // release build: no Gate
  uint8_t order[2]{};
  app_registry::sortedOrder(keys, 2, order);
  EXPECT_EQ(app_registry::rowForApp(keys, order, 2, AppId::Gate), 0);
  EXPECT_EQ(app_registry::rowForApp(keys, order, 2, AppId::None), 0);
}

TEST(AppRegistry, EmptyRegistryIsSafe) {
  uint8_t order[1]{};
  app_registry::sortedOrder(nullptr, 0, order);
  EXPECT_EQ(app_registry::rowForApp(nullptr, order, 0, AppId::Goto), 0);
}

// --- goToApps allocation fallback (fault-injected factories) ------------------

struct FakeActivity {
  virtual ~FakeActivity() = default;
  virtual const char* kind() const = 0;
};
struct FakeApps : FakeActivity {
  const char* kind() const override { return "apps"; }
};
struct FakeHome : FakeActivity {
  const char* kind() const override { return "home"; }
};

struct Factories {
  bool appsFails = false;
  bool homeFails = false;
  int appsCalls = 0;
  int homeCalls = 0;

  std::unique_ptr<FakeActivity> launch(app_registry::LaunchOutcome& outcome) {
    return app_registry::launchWithFallback<FakeActivity>(
        [this]() -> std::unique_ptr<FakeApps> {
          ++appsCalls;
          return appsFails ? nullptr : std::make_unique<FakeApps>();
        },
        [this]() -> std::unique_ptr<FakeHome> {
          ++homeCalls;
          return homeFails ? nullptr : std::make_unique<FakeHome>();
        },
        outcome);
  }
};

TEST(AppsLaunch, AppsAllocatedDoesNotTouchFallback) {
  Factories f;
  app_registry::LaunchOutcome outcome{};
  const auto activity = f.launch(outcome);
  ASSERT_NE(activity, nullptr);
  EXPECT_STREQ(activity->kind(), "apps");
  EXPECT_EQ(outcome, app_registry::LaunchOutcome::Primary);
  EXPECT_EQ(f.homeCalls, 0);  // Home is never allocated alongside Apps
}

TEST(AppsLaunch, AppsOomFallsBackToHome) {
  Factories f;
  f.appsFails = true;
  app_registry::LaunchOutcome outcome{};
  const auto activity = f.launch(outcome);
  ASSERT_NE(activity, nullptr);
  EXPECT_STREQ(activity->kind(), "home");
  EXPECT_EQ(outcome, app_registry::LaunchOutcome::Fallback);
  EXPECT_EQ(f.appsCalls, 1);
  EXPECT_EQ(f.homeCalls, 1);
}

TEST(AppsLaunch, BothOomReturnsNullSoCallerStaysPut) {
  Factories f;
  f.appsFails = true;
  f.homeFails = true;
  app_registry::LaunchOutcome outcome{};
  const auto activity = f.launch(outcome);
  EXPECT_EQ(activity, nullptr);
  EXPECT_EQ(outcome, app_registry::LaunchOutcome::None);
  EXPECT_EQ(f.appsCalls, 1);
  EXPECT_EQ(f.homeCalls, 1);
}

}  // namespace
