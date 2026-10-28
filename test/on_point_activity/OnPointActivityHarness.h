#pragma once

// The production OnPointActivity on the host: stand-ins in stubs/ for the renderer, input,
// settings, translations (tr(STR_X) draws "STR_X") and the clock; FakeStorage
// (test/on_point_store) as the SD card behind OnPointStore.

#include <gtest/gtest.h>

#include <memory>
#include <string>

#include "../on_point_store/FakeStorage.h"
#include "CrossPointSettings.h"
#include "activities/on_point/OnPointActivity.h"
#include "activities/on_point/OnPointScheduleData.h"

namespace harness {

extern unsigned long nowMs;
extern bool clockReady;
extern on_point::CivilDateTime clockUtc;
extern FakeStorage storage;

using Button = MappedInputManager::Button;
using Event = MappedInputManager::Event;
using SwipeDir = MappedInputManager::SwipeDir;

// Geometry (OnPointActivity.cpp / OnPointRouteList.h).
constexpr int HEADING_Y = 76;
constexpr int PINNED_ORIGIN_Y = 106;
constexpr int FOOTER_Y = 744;
constexpr int ROUTE_ORIGIN_Y = 72;
constexpr int ROW_Y[] = {316, 442, 568};
constexpr int COLLECTION_ROW_Y = 678;
constexpr int COLLECTION_TEXT_Y = 692;
constexpr int PICKER_ROW_Y[] = {102, 228, 354, 480};

class ActivityTest : public ::testing::Test {
 protected:
  GfxRenderer renderer;
  MappedInputManager input;
  std::unique_ptr<OnPointActivity> activity;

  void SetUp() override;

  void enter();
  void draw() { activity->render(RenderLock{}); }
  void send(Event event, Button button = Button::Back);
  void touch(Event event, int x, int y);
  void swipe(SwipeDir dir);

  std::string textAt(int y) const;
  bool drew(const std::string& text) const;
  bool drewAt(const std::string& text, int y) const;
  bool pinnedSelected() const;
  int selectedRow() const;  // route row 0..2 with the selected bar, or -1
  bool collectionRowShown() const { return drew("STR_ON_POINT_COLLECTION"); }
  bool collectionRowSelected() const;
  int selectedPickerRow() const;
  bool inDepartureView() const { return !textAt(ROUTE_ORIGIN_Y).empty(); }
  bool inPicker() const { return drew("STR_ON_POINT_COLLECTIONS"); }
};

}  // namespace harness
