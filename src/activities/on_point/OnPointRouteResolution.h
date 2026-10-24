#pragma once

#include <cstddef>
#include <cstdint>

namespace on_point {

// The compiled-in main route (CrossPointSettings::onPointMainRouteId's
// initializer). A missing settings file or key keeps it; tests pin the two
// together.
constexpr char DEFAULT_MAIN_ROUTE_ID[] = "p2p-balagtas-trinoma";

// How a saved main-route ID resolved against the route catalogue. Only
// Canonical and Aliased name a route; the others never substitute one.
enum class RouteResolutionState : uint8_t {
  Canonical,  // exact, case-sensitive match of a route pair's ID
  Aliased,    // an alias entry that maps to a route pair's ID
  Unset,      // null or explicitly empty
  Unknown,    // anything else: no match, case mismatch, unterminated
};

struct RouteResolution {
  RouteResolutionState state = RouteResolutionState::Unset;
  size_t index = 0;  // route pair index; meaningful only when hasRoute()

  bool hasRoute() const { return state == RouteResolutionState::Canonical || state == RouteResolutionState::Aliased; }
};

// A retired route ID and the current ID it now stands for.
struct RouteAlias {
  const char* from;
  const char* to;
};

struct AliasTable {
  const RouteAlias* entries = nullptr;
  size_t count = 0;
};

// Aliases shipped with this firmware (none yet).
AliasTable productionAliases();

// Resolves a saved route ID held in a buffer of bufSize bytes. Reads at most
// bufSize bytes, so an unterminated buffer is Unknown rather than an overread.
// Never writes and never picks a substitute route.
RouteResolution resolveSavedRoute(const char* saved, size_t bufSize, AliasTable aliases);

}  // namespace on_point
