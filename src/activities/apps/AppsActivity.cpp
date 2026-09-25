#include "AppsActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>

#include "MappedInputManager.h"
#include "activities/goto/GotoEditionSource.h"
#include "components/UITheme.h"

namespace fui = freeink::ui;

namespace {

struct AppDescriptor {
  app_registry::AppKey key;
  const char* (*label)();
  void (*launch)();
};

// GOTO's label follows the cached current edition (no network). Its sort key
// stays "GOTO" so the row never moves when the label reads TOGO.
const char* gotoLabel() { return cachedCurrentIsTogo() ? tr(STR_TOGO) : tr(STR_GOTO); }

// Declaration order is irrelevant: rows are sorted by key in onEnter().
constexpr AppDescriptor APP_TABLE[] = {
    {{AppId::Goto, "GOTO"}, gotoLabel, [] { activityManager.goToGoto(); }},
    {{AppId::OnPoint, "ON POINT"}, [] { return tr(STR_ON_POINT); }, [] { activityManager.goToOnPoint(); }},
#ifdef GATE_ENABLED
    {{AppId::Gate, "THE GATE IS OPEN!"}, [] { return tr(STR_GATE_IS_OPEN); }, [] { activityManager.goToGate(); }},
#endif
};
constexpr int APP_COUNT = sizeof(APP_TABLE) / sizeof(APP_TABLE[0]);
static_assert(APP_COUNT <= AppsActivity::MAX_APPS, "raise AppsActivity::MAX_APPS");

}  // namespace

AppsActivity::AppsActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, const AppId focus)
    : UiListActivity("Apps", renderer, mappedInput), focus(focus) {}

void AppsActivity::onEnter() {
  UiListActivity::onEnter();

  app_registry::AppKey keys[APP_COUNT];
  for (int i = 0; i < APP_COUNT; ++i) keys[i] = APP_TABLE[i].key;
  app_registry::sortedOrder(keys, APP_COUNT, order);
  appCount = APP_COUNT;

  for (int row = 0; row < appCount; ++row) {
    fui::ListItem item;
    item.label = APP_TABLE[order[row]].label();
    item.actionValue = static_cast<int16_t>(row);
    rowItems[row] = item;
  }

  // The first screen build pulls the viewport to this row (ListNav follow-on-build).
  nav.selected = app_registry::rowForApp(keys, order, APP_COUNT, focus);
}

const char* AppsActivity::headerTitle() const { return tr(STR_APPS); }

void AppsActivity::activateIndex(const int index) {
  if (index < 0 || index >= appCount) return;
  // The activated row leaves this screen; a lingering flash would gray an
  // unrelated element on the next render.
  app.clearTapFlash();
  nav.selected = index;
  APP_TABLE[order[index]].launch();
}

void AppsActivity::onBackButton() { activityManager.goHome(HomeMenuItem::APPS); }

void AppsActivity::buildScreen(UiScreen& screen) {
  const auto& metrics = UITheme::getInstance().getMetrics();
  const Rect safe = UITheme::getInstance().getScreenSafeArea(renderer, true, false);
  // Content: the safe area minus the header band GUI.drawHeader paints.
  screen.setContentMarginFromScreen(fui::Insets{
      static_cast<int16_t>(safe.y + metrics.topPadding + metrics.headerHeight),
      static_cast<int16_t>(renderer.getScreenWidth() - (safe.x + safe.width)),
      static_cast<int16_t>(renderer.getScreenHeight() - (safe.y + safe.height)), static_cast<int16_t>(safe.x)});
  screen.spacer(static_cast<int16_t>(metrics.verticalSpacing));

  fui::ListProps props;
  props.items = rowItems;
  props.count = static_cast<uint16_t>(appCount);
  props.action = ACTION_ROW;
  props.inputMask = fui::InputTouch;  // physical buttons stay in loop()
  syncListViewport(screen, props);
  screen.list(props);
}
