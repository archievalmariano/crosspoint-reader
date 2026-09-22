#pragma once

// The Gate Is Open! -- CrossPoint Activity host.
//
// This is a THIN adapter. It owns a gate::Game and drives it exactly as the
// Phase 1.1 hardening documented for CrossPoint:
//   * CrossPoint owns the loop; we poll input in loop() and requestUpdate().
//   * render() reads Game::buildScreen() and paints it with GfxRenderer.
//   * Game::run() and the blocking Input::next() are NOT used.
// No narrative/game logic lives here -- it is all in the portable engine
// (the-gate-is-open repo), compiled via the gate_tu_*.cpp forwarding TUs.

#include <memory>
#include <string>
#include <vector>

// CrossPoint's lib/hal/HalStorage.h defines a convenience macro
// `Storage` (=> HalStorage::getInstance()) that would clobber the
// gate::Storage platform interface. Suspend it only while the portable Gate
// headers are parsed, then restore it -- so consumers that already pulled in
// HalStorage.h (e.g. HomeActivity) keep the macro, and the portable core is
// left untouched. These headers are self-contained (STL + gate only).
#pragma push_macro("Storage")
#undef Storage
#include "gate/Content.h"
#include "gate/Screen.h"
#include "gate/Story.h"
#include "gate/app/Game.h"
#include "gate/platform/Platform.h"
#pragma pop_macro("Storage")

#include "activities/Activity.h"
#include "activities/gate/GateDeviceProfile.h"

class GateActivity : public Activity {
 public:
  GateActivity(GfxRenderer& renderer, MappedInputManager& mappedInput);
  ~GateActivity() override;

  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  void drawScreen(const gate::Screen& s);
  void drawError();
  int drawMenu(const gate::Screen& s, int top, int bottomLimit);  // returns y below last row
  int menuBlockHeight(const gate::Screen& s);                     // wrapped total height of the menu
  void drawHints(const gate::Screen& s);                          // device-correct button-hint bar

  const gate_device::GateDeviceProfile profile_ = gate_device::activeProfile();
  gate::Platform platform_{};  // all-null: display/input/sleep unused on device
  gate::Cast cast_;
  std::vector<gate::Story> stories_;
  std::unique_ptr<gate::Game> game_;

  bool loadedOk_ = false;
  std::string loadError_;
  std::string lastSceneKey_;  // full refresh on scene change, fast on cursor move
  bool firstRender_ = true;
};
