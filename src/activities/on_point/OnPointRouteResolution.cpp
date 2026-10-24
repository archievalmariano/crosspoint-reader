#include "OnPointRouteResolution.h"

#include <cstring>

#include "OnPointScheduleData.h"

namespace on_point {
namespace {

// Route pair whose ID equals id exactly, or routeCount() when there is none.
size_t findRoute(const char* id) {
  for (size_t index = 0; index < routeCount(); ++index) {
    const char* routeId = routeAt(index).routeId;
    if (routeId != nullptr && strcmp(routeId, id) == 0) return index;
  }
  return routeCount();
}

}  // namespace

AliasTable productionAliases() { return {}; }

RouteResolution resolveSavedRoute(const char* saved, const size_t bufSize, const AliasTable aliases) {
  RouteResolution result;
  if (saved == nullptr || bufSize == 0 || saved[0] == '\0') {
    result.state = RouteResolutionState::Unset;
    return result;
  }
  // No terminator inside the buffer: never read past it.
  if (strnlen(saved, bufSize) == bufSize) {
    result.state = RouteResolutionState::Unknown;
    return result;
  }

  const size_t direct = findRoute(saved);
  if (direct < routeCount()) {
    result.state = RouteResolutionState::Canonical;
    result.index = direct;
    return result;
  }

  for (size_t i = 0; i < aliases.count; ++i) {
    const RouteAlias& alias = aliases.entries[i];
    if (alias.from == nullptr || alias.to == nullptr || strcmp(alias.from, saved) != 0) continue;
    const size_t target = findRoute(alias.to);
    if (target < routeCount()) {
      result.state = RouteResolutionState::Aliased;
      result.index = target;
      return result;
    }
    break;  // an alias to a route this build no longer has stays Unknown
  }

  result.state = RouteResolutionState::Unknown;
  return result;
}

}  // namespace on_point
