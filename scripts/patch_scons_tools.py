"""Restore SCons Tool modules that pioarduino's trimmed tool-scons omits.

The espressif32 (pioarduino) platform ships a trimmed ``tool-scons`` whose
``SCons/Tool`` directory is missing modules (notably ``FortranCommon``). SCons'
link-tool discovery (``Tool/linkCommon.smart_link``) imports ``FortranCommon``
while assembling ``firmware.elf``, so every firmware link fails with::

    *** [.pio/build/<env>/firmware.elf] ModuleNotFoundError:
        No module named 'SCons.Tool.FortranCommon'

Historically this was worked around per-developer by pointing ``tool-scons`` at a
manually restored complete SCons (``~/goto-tools/tool-scons``). That is not
reproducible: a fresh macOS/Linux/CI checkout has no such copy and cannot build.

This pre-build hook makes the fix repo-controlled. It restores ONLY the Tool
modules that are missing from the active (trimmed) tool-scons, copying them from
the official ``scons-local`` bundle of the SAME SCons version (behaviour is
therefore identical to a complete install — no version drift, nothing existing is
overwritten). The bundle is cached inside the tool-scons package dir so a build
downloads it at most once, and the whole step is a no-op once FortranCommon is
present.
"""

import io
import sys
import tarfile
import urllib.request
from pathlib import Path

# Sentinel module: if this is present the tool-scons is complete and we do nothing.
SENTINEL = "SCons/Tool/FortranCommon.py"
SF_URL = "https://downloads.sourceforge.net/project/scons/scons-local/{v}/scons-local-{v}.tar.gz"


def find_scons_root(tool_scons_dir: Path) -> "tuple[Path, str] | None":
    """Return (dir_containing_SCons, version) for the active scons-local, or None."""
    # Layout A: <tool-scons>/scons-local-<version>/SCons/...
    for child in sorted(tool_scons_dir.glob("scons-local-*")):
        if (child / "SCons").is_dir():
            return child, child.name.replace("scons-local-", "", 1)
    # Layout B: <tool-scons>/SCons/... (version from package manifest)
    if (tool_scons_dir / "SCons").is_dir():
        version = ""
        init = tool_scons_dir / "SCons" / "__init__.py"
        if init.is_file():
            for line in init.read_text(encoding="utf-8", errors="ignore").splitlines():
                if line.strip().startswith("__version__"):
                    version = line.split("=", 1)[1].strip().strip("\"'")
                    break
        return tool_scons_dir, version
    return None


def missing_files(official_scons: Path, live_scons: Path) -> "list[Path]":
    """Files present in the official SCons/ tree but absent from the live one."""
    out = []
    for src in official_scons.rglob("*"):
        if src.is_file():
            rel = src.relative_to(official_scons)
            if not (live_scons / rel).exists():
                out.append(rel)
    return out


def restore(tool_scons_dir: Path, log=print) -> bool:
    """Ensure the active tool-scons has a complete Tool set. Returns True if OK."""
    found = find_scons_root(tool_scons_dir)
    if not found:
        log("[scons-tools] no scons-local found under %s; skipping" % tool_scons_dir)
        return False
    scons_root, version = found
    if (scons_root / SENTINEL).is_file():
        return True  # already complete
    if not version:
        log("[scons-tools] cannot determine SCons version; skipping restore")
        return False

    cache = tool_scons_dir / ".crosspoint-scons-restore" / version
    official_scons = cache / ("scons-local-%s" % version) / "SCons"
    if not (official_scons / "Tool" / "FortranCommon.py").is_file():
        url = SF_URL.format(v=version)
        log("[scons-tools] trimmed tool-scons detected; fetching official "
            "scons-local-%s to restore missing Tool modules" % version)
        cache.mkdir(parents=True, exist_ok=True)
        with urllib.request.urlopen(url, timeout=90) as resp:  # noqa: S310 (pinned host)
            data = resp.read()
        with tarfile.open(fileobj=io.BytesIO(data), mode="r:gz") as tf:
            tf.extractall(cache)  # noqa: S202 (trusted, pinned SCons bundle)
    if not (official_scons / "Tool" / "FortranCommon.py").is_file():
        log("[scons-tools] official bundle missing FortranCommon; cannot restore")
        return False

    added = missing_files(official_scons, scons_root / "SCons")
    for rel in added:
        dst = scons_root / "SCons" / rel
        dst.parent.mkdir(parents=True, exist_ok=True)
        dst.write_bytes((official_scons / rel).read_bytes())
    log("[scons-tools] restored %d missing SCons module(s) (incl. FortranCommon)"
        % len(added))
    return (scons_root / SENTINEL).is_file()


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

_platform = env.PioPlatform()  # noqa: F821
try:
    _tool_scons = Path(_platform.get_package_dir("tool-scons"))
except Exception as exc:  # noqa: BLE001
    print("[scons-tools] tool-scons package dir unavailable: %s" % exc)
else:
    if _tool_scons and _tool_scons.is_dir():
        restore(_tool_scons)
