#pragma once
// Host-test stand-in for the Activity base, RenderLock and ActivityManager.
#include <string>

#include "GfxRenderer.h"
#include "MappedInputManager.h"
#include "activities/apps/AppRegistry.h"

unsigned long millis();  // defined by the test

class RenderLock {
 public:
  static bool peekResult;
  static bool peek() { return peekResult; }
};

class ActivityManager {
 public:
  int goToAppsCalls = 0;
  AppId lastFocus = AppId::None;
  void goToApps(AppId focus) {
    ++goToAppsCalls;
    lastFocus = focus;
  }
};
extern ActivityManager activityManager;

class Activity {
 protected:
  std::string name;
  GfxRenderer& renderer;
  MappedInputManager& mappedInput;

 public:
  int updateRequests = 0;

  Activity(std::string name, GfxRenderer& renderer, MappedInputManager& mappedInput)
      : name(std::move(name)), renderer(renderer), mappedInput(mappedInput) {}
  virtual ~Activity() = default;
  virtual void onEnter() {}
  virtual void onExit() {}
  virtual void loop() {}
  virtual void render(RenderLock&&) {}
  virtual void requestUpdate(bool = false) { ++updateRequests; }
};
