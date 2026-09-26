#pragma once

#include <cstdint>

#include "activities/UiListActivity.h"
#include "activities/apps/AppRegistry.h"

/**
 * Launcher for custom firmware apps (GOTO/TOGO, ON POINT, The Gate Is Open!),
 * reached from the single Home "Apps" entry. Rows come from the static app
 * table in AppsActivity.cpp, ordered by each app's stable sort key.
 *
 * Every transition is a replace: an app's top-level Back returns here via
 * activityManager.goToApps(<its AppId>), and Back here returns to Home.
 */
class AppsActivity final : public UiListActivity {
 public:
  AppsActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, AppId focus);

  void onEnter() override;

  // Upper bound on registered apps; sizes the row storage below.
  static constexpr int MAX_APPS = 8;

 private:
  int listCount() const override { return appCount; }
  void buildScreen(UiScreen& screen) override;
  void activateIndex(int index) override;
  void onBackButton() override;
  const char* headerTitle() const override;

  const AppId focus;
  int appCount = 0;
  // Row -> index into the app table, in sort-key order.
  uint8_t order[MAX_APPS]{};
  // Built once in onEnter(): labels are snapshotted there (GOTO/TOGO reads the
  // cached edition manifest), so repaints never touch the SD card.
  freeink::ui::ListItem rowItems[MAX_APPS]{};
};
