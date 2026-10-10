"""Append CrossPoint's wolfSSL overrides to Arduino-wolfSSL's user_settings.h.

A PlatformIO pre-build script (every env's copy under .pio/libdeps). Run
directly, it patches the files it is given: the DOGEAR TLS host tests apply
it to the pinned wolfSSL they fetch, so they build the same configuration.
    python3 scripts/patch_wolfssl.py <user_settings.h>...
"""
import sys
from pathlib import Path

MARKER = "/* CrossPoint wolfSSL compatibility overrides */"
OVERRIDES = f"""

{MARKER}
#undef NO_DH
#ifndef HAVE_FFDHE_2048
#define HAVE_FFDHE_2048
#endif
/* MEMFIX-PORT: 8192 handles up to RSA-4096 keys (the public-CA maximum,
   ISRG Root X1 included) with half the per-bignum heap of 16384: with
   WOLFSSL_SMALL_STACK each fast-math temp is FP_MAX_BITS/8 * 2 bytes on the
   heap, and TLS cert verification allocates dozens at once. */
#undef FP_MAX_BITS
#define FP_MAX_BITS 8192
"""


def patch_user_settings(path: Path, shown: Path) -> None:
    text = path.read_text().split(MARKER, 1)[0].rstrip()
    path.write_text(text + OVERRIDES + "\n")
    print(f"Patched wolfSSL settings: {shown}")


try:
    Import("env")  # noqa: F821  # type: ignore[name-defined]  (SCons)
except NameError:
    for arg in sys.argv[1:]:
        patch_user_settings(Path(arg), Path(arg))
else:
    PROJECT_DIR = Path(env.subst("$PROJECT_DIR"))  # noqa: F821  # type: ignore[name-defined]
    for settings in PROJECT_DIR.glob(".pio/libdeps/*/Arduino-wolfSSL/src/user_settings.h"):
        patch_user_settings(settings, settings.relative_to(PROJECT_DIR))
