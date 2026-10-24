#pragma once
// Host-test stand-in for the one setting ON POINT uses.
#include <cstring>

#include "activities/on_point/OnPointRouteResolution.h"

class CrossPointSettings {
 public:
  char onPointMainRouteId[48];
  int saves = 0;

  CrossPointSettings() { reset(); }
  void reset() {
    strncpy(onPointMainRouteId, on_point::DEFAULT_MAIN_ROUTE_ID, sizeof(onPointMainRouteId) - 1);
    onPointMainRouteId[sizeof(onPointMainRouteId) - 1] = '\0';
    saves = 0;
  }
  bool saveToFile() {
    ++saves;
    return true;
  }
  static CrossPointSettings& getInstance() {
    static CrossPointSettings instance;
    return instance;
  }
};

#define SETTINGS CrossPointSettings::getInstance()
