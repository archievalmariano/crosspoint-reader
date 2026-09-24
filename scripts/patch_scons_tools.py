"""Restore SCons Tool modules missing from the SCons that runs the firmware link.

The espressif32 (pioarduino) platform installs/reinstalls its ``tool-scons`` at
several points during a build (notably the C3 custom-core rebuild reinstalls it
mid-build). In some of those states the active ``scons-local`` is left without
``SCons/Tool/FortranCommon.py``, so SCons' link-tool discovery
(``Tool/linkCommon.smart_link``) fails while assembling ``firmware.elf``::

    *** [.pio/build/<env>/firmware.elf] ModuleNotFoundError:
        No module named 'SCons.Tool.FortranCommon'

Historically this was worked around per-developer with a hand-restored complete
SCons at ``~/goto-tools/tool-scons`` — not reproducible on a fresh macOS/Linux/CI
checkout. This pre-build hook makes the fix repo-controlled: it patches the SCons
that is ACTUALLY running (``SCons.__file__``, i.e. the engine the link will use),
restoring only the Tool modules it is missing from the official ``scons-local``
bundle of the SAME version. Behaviour is identical to a complete install (no
version drift, nothing existing overwritten); it is a no-op once complete.
"""

import io
import sys
import tarfile
import urllib.request
from pathlib import Path

SENTINEL = "Tool/FortranCommon.py"  # relative to a SCons/ dir
SF_URL = "https://downloads.sourceforge.net/project/scons/scons-local/{v}/scons-local-{v}.tar.gz"


def missing_files(official_scons: Path, live_scons: Path) -> "list[Path]":
    """Files present under the official SCons/ tree but absent from the live one."""
    out = []
    for src in official_scons.rglob("*"):
        if src.is_file():
            rel = src.relative_to(official_scons)
            if not (live_scons / rel).exists():
                out.append(rel)
    return out


def fetch_official_scons(version: str, cache_dir: Path, log=print) -> "Path | None":
    """Download+extract the official scons-local-<version>; return its SCons/ dir."""
    official = cache_dir / ("scons-local-%s" % version) / "SCons"
    if (official / SENTINEL).is_file():
        return official
    url = SF_URL.format(v=version)
    log("[scons-tools] fetching official scons-local-%s to restore missing Tool "
        "modules" % version)
    cache_dir.mkdir(parents=True, exist_ok=True)
    with urllib.request.urlopen(url, timeout=90) as resp:  # noqa: S310 (pinned host)
        data = resp.read()
    with tarfile.open(fileobj=io.BytesIO(data), mode="r:gz") as tf:
        tf.extractall(cache_dir)  # noqa: S202 (trusted, pinned SCons bundle)
    return official if (official / SENTINEL).is_file() else None


def restore_into(scons_pkg: Path, version: str, cache_dir: Path, log=print) -> bool:
    """Ensure the given SCons/ dir has a complete Tool set. Returns True if OK."""
    if (scons_pkg / SENTINEL).is_file():
        return True
    if not version:
        log("[scons-tools] unknown SCons version; cannot restore")
        return False
    official = fetch_official_scons(version, cache_dir, log=log)
    if not official:
        log("[scons-tools] official scons-local-%s unavailable; cannot restore" % version)
        return False
    added = missing_files(official, scons_pkg)
    for rel in added:
        dst = scons_pkg / rel
        dst.parent.mkdir(parents=True, exist_ok=True)
        dst.write_bytes((official / rel).read_bytes())
    log("[scons-tools] restored %d missing SCons module(s) incl. FortranCommon "
        "into %s" % (len(added), scons_pkg))
    return (scons_pkg / SENTINEL).is_file()


def restore_active_scons(log=print) -> bool:
    """Patch the SCons engine that is currently running this build."""
    import SCons  # noqa: PLC0415 -- provided: this executes inside SCons

    scons_pkg = Path(SCons.__file__).resolve().parent        # .../scons-local-<ver>/SCons
    scons_root = scons_pkg.parent                            # .../scons-local-<ver>
    version = getattr(SCons, "__version__", "") or scons_root.name.replace(
        "scons-local-", "", 1
    )
    cache = scons_root.parent / ".crosspoint-scons-restore"
    return restore_into(scons_pkg, version, cache, log=log)


def _self_test() -> None:
    import tempfile

    with tempfile.TemporaryDirectory() as d:
        root = Path(d)
        official = root / "off" / "SCons"
        (official / "Tool").mkdir(parents=True)
        (official / "Tool" / "FortranCommon.py").write_text("x = 1\n")
        (official / "Tool" / "cc.py").write_text("y = 1\n")
        live = root / "live" / "SCons"
        (live / "Tool").mkdir(parents=True)
        (live / "Tool" / "cc.py").write_text("y = 1\n")  # cc present, Fortran missing
        miss = missing_files(official, live)
        assert Path("Tool/FortranCommon.py") in miss, miss
        assert Path("Tool/cc.py") not in miss, miss
    print("patch_scons_tools self-test OK")


if __name__ == "__main__" and "--self-test" in sys.argv:
    _self_test()
    raise SystemExit(0)

# --- PlatformIO pre-build entry point ---------------------------------------
Import("env")  # noqa: F821 -- provided by PlatformIO

try:
    restore_active_scons()
except Exception as exc:  # noqa: BLE001
    print("[scons-tools] could not restore active SCons: %s" % exc)
