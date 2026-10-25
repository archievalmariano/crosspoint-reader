#include "OnPointActivityHarness.h"

ActivityManager activityManager;
bool RenderLock::peekResult = false;

namespace harness {
unsigned long nowMs = 0;
bool clockReady = true;
on_point::CivilDateTime clockUtc{2026, 10, 21, 0, 30, 0};  // Wed 08:30 Manila
FakeStorage storage;
}  // namespace harness

unsigned long millis() { return harness::nowMs; }

bool on_point::HalClockSource::nowUtc(CivilDateTime& value) const {
  if (!harness::clockReady) return false;
  value = harness::clockUtc;
  return true;
}

on_point::OnPointStorage& on_point::onPointStorage() { return harness::storage; }

namespace harness {

void ActivityTest::SetUp() {
  SETTINGS.reset();
  activityManager = ActivityManager{};
  RenderLock::peekResult = false;
  clockReady = true;
  nowMs = 1000;
  storage = FakeStorage{};
}

void ActivityTest::enter() {
  activity = std::make_unique<OnPointActivity>(renderer, input);
  activity->onEnter();
  draw();
}

void ActivityTest::send(const Event event, const Button button) {
  input.event = event;
  input.button = button;
  activity->loop();
  input.clear();
  draw();
}

void ActivityTest::touch(const Event event, const int x, const int y) {
  input.event = event;
  input.x = x;
  input.y = y;
  activity->loop();
  input.clear();
  draw();
}

void ActivityTest::swipe(const SwipeDir dir) {
  input.event = Event::Swipe;
  input.swipe = dir;
  activity->loop();
  input.clear();
  draw();
}

std::string ActivityTest::textAt(const int y) const {
  for (const auto& t : renderer.texts)
    if (t.y == y) return t.text;
  return "";
}

bool ActivityTest::drew(const std::string& text) const {
  for (const auto& t : renderer.texts)
    if (t.text == text) return true;
  return false;
}

bool ActivityTest::drewAt(const std::string& text, const int y) const {
  for (const auto& t : renderer.texts)
    if (t.text == text && t.y == y) return true;
  return false;
}

// Selection bars: pinned card 10 (selected) / 4; route and picker rows 8 / 3; collection row 8 / 3.
bool ActivityTest::pinnedSelected() const {
  for (const auto& r : renderer.rects)
    if (r.y == 102 && r.height == 150) return r.width == 10;
  return false;
}

int ActivityTest::selectedRow() const {
  for (const auto& r : renderer.rects)
    for (int row = 0; row < 3; ++row)
      if (r.y == ROW_Y[row] && r.height == 104 && r.width == 8) return row;
  return -1;
}

bool ActivityTest::collectionRowSelected() const {
  for (const auto& r : renderer.rects)
    if (r.y == COLLECTION_ROW_Y + 6) return r.width == 8;
  return false;
}

int ActivityTest::selectedPickerRow() const {
  for (const auto& r : renderer.rects)
    for (int row = 0; row < 4; ++row)
      if (r.y == PICKER_ROW_Y[row] && r.height == 104 && r.width == 8) return row;
  return -1;
}

}  // namespace harness
