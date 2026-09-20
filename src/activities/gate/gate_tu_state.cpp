// Forwarding translation unit: compiles the portable Gate source "engine/src/State.cpp" into the
// CrossPoint firmware as its own TU. The canonical source lives in the
// the-gate-is-open repo (unchanged); the repo root is on the include path
// (see platformio.ini [env:default]). One TU per file avoids the anonymous-
// namespace symbol collisions a single unity build would cause.
#include "engine/src/State.cpp"
