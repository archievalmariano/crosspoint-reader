"""Repo-controlled repair of pioarduino's incomplete tool-scons (no network).

Problem: the C3 custom-core rebuild (firmware_tuned_c3, used by gh_release)
reinstalls ``scons-local-4.11.1`` mid-build and relinks ``firmware.elf`` against a
copy that is missing ``SCons.Tool.FortranCommon`` — imported by
``SCons.Tool.linkCommon.smart_link`` — so the link dies with
``ModuleNotFoundError: No module named 'SCons.Tool.FortranCommon'``. (The X4 Pro
production profile uses scons-local-4.8.1, which is complete, and is unaffected.)

Fix: a narrow, deterministic, offline repair. The exact modules that import chain
needs are VENDORED under ``scripts/scons_repair/payload`` (copied verbatim from the
pinned pioarduino bundle, see ``manifest.json``); this pre-build hook restores only
those allowlisted files, and only when the ACTIVE scons engine is missing them.

Guarantees (see ``scons_repair/test_repair.py``):
  * allowlist-based completeness (never "FortranCommon alone");
  * repairs only a PlatformIO-managed tool-scons (never a system/venv SCons);
  * SCons version must equal the payload's version, else HARD FAIL;
  * every payload source file is hash-verified before use, and every restored
    file is hash-verified after install;
  * atomic per-file install (os.replace) under an exclusive lock, so an
    interrupted or concurrent repair never leaves a state that looks complete;
  * any validation/lock/install failure is FATAL at the repair boundary (raises);
  * a second run after a successful repair is a clean no-op.
"""

import hashlib
import json
import os
import sys
import tempfile
from pathlib import Path

# PlatformIO execs this pre-script without a module __file__, so fall back to the
# build CWD (always the project root under pio) — the scons entry point below
# passes an explicit $PROJECT_DIR-derived path regardless.
try:
    _SCRIPTS_DIR = Path(__file__).resolve().parent
except NameError:
    _SCRIPTS_DIR = Path.cwd() / "scripts"
REPAIR_DIR = _SCRIPTS_DIR / "scons_repair"
MANIFEST_PATH = REPAIR_DIR / "manifest.json"
PAYLOAD_ROOT = REPAIR_DIR / "payload"
LOCK_NAME = ".crosspoint-scons-repair.lock"
LOCK_TIMEOUT_S = 120


class SconsRepairError(RuntimeError):
    """Raised for any repair validation/lock/install failure (fatal)."""


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with open(path, "rb") as fh:
        for chunk in iter(lambda: fh.read(65536), b""):
            h.update(chunk)
    return h.hexdigest()


def load_manifest(path: Path = MANIFEST_PATH) -> dict:
    data = json.loads(Path(path).read_text(encoding="utf-8"))
    if not data.get("scons_version") or not isinstance(data.get("files"), list) or not data["files"]:
        raise SconsRepairError("invalid repair manifest: %s" % path)
    for entry in data["files"]:
        if not entry.get("path") or not entry.get("sha256"):
            raise SconsRepairError("invalid manifest file entry: %r" % entry)
        # Guard against path escapes in the manifest itself.
        rel = Path(entry["path"])
        if rel.is_absolute() or ".." in rel.parts:
            raise SconsRepairError("unsafe manifest path: %s" % entry["path"])
    return data


def verify_payload(manifest: dict, payload_root: Path = PAYLOAD_ROOT) -> None:
    """Every allowlisted file must exist in the payload and match its sha256."""
    for entry in manifest["files"]:
        src = payload_root / entry["path"]
        if not src.is_file():
            raise SconsRepairError("repair payload missing file: %s" % src)
        actual = sha256_file(src)
        if actual != entry["sha256"]:
            raise SconsRepairError(
                "repair payload hash mismatch for %s: expected %s got %s"
                % (entry["path"], entry["sha256"], actual)
            )


def is_pio_managed_tool_scons(scons_pkg: Path) -> bool:
    """True only if scons_pkg is the SCons/ dir of a PlatformIO-managed tool-scons.

    Refuses to touch a system, venv, or site-packages SCons: the path must run
    through ``.platformio`` / ``packages`` / a ``tool-scons`` component and a
    ``scons-local-*`` component, and must not be inside a site-packages tree.
    """
    parts = scons_pkg.resolve().parts
    if "site-packages" in parts or "dist-packages" in parts:
        return False
    joined = "/".join(parts)
    if "/packages/tool-scons" not in joined:
        return False
    if not any(p.startswith("scons-local-") for p in parts):
        return False
    return parts[-1] == "SCons"


def _dest_for(scons_pkg: Path, rel_path: str) -> Path:
    """Resolve a manifest path ("SCons/<...>") to its file inside the engine.

    ``scons_pkg`` is the ``SCons/`` directory itself, so a "SCons/Tool/x.py"
    manifest entry lands at ``scons_pkg/Tool/x.py``.
    """
    if rel_path.startswith("SCons/"):
        return scons_pkg / rel_path[len("SCons/"):]
    return scons_pkg.parent / rel_path


def allowlist_present(scons_pkg: Path, manifest: dict) -> bool:
    """True only if EVERY allowlisted module is present (existence, full list)."""
    return all(_dest_for(scons_pkg, entry["path"]).is_file() for entry in manifest["files"])


class _FileLock:
    def __init__(self, path: Path, timeout_s: int = LOCK_TIMEOUT_S):
        self._path = path
        self._timeout = timeout_s
        self._fd = None

    def __enter__(self):
        import fcntl
        import time

        self._path.parent.mkdir(parents=True, exist_ok=True)
        self._fd = os.open(str(self._path), os.O_CREAT | os.O_RDWR, 0o644)
        deadline = time.monotonic() + self._timeout
        while True:
            try:
                fcntl.flock(self._fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
                return self
            except OSError:
                if time.monotonic() >= deadline:
                    os.close(self._fd)
                    self._fd = None
                    raise SconsRepairError("timed out acquiring scons repair lock")
                time.sleep(0.2)

    def __exit__(self, *exc):
        import fcntl

        if self._fd is not None:
            try:
                fcntl.flock(self._fd, fcntl.LOCK_UN)
            finally:
                os.close(self._fd)
                self._fd = None


def _atomic_install(src: Path, dest: Path, expected_sha: str) -> None:
    dest.parent.mkdir(parents=True, exist_ok=True)
    fd, tmp = tempfile.mkstemp(dir=str(dest.parent), prefix=".scons-repair-")
    try:
        with os.fdopen(fd, "wb") as out:
            out.write(src.read_bytes())
            out.flush()
            os.fsync(out.fileno())
        if sha256_file(Path(tmp)) != expected_sha:
            raise SconsRepairError("staged file hash mismatch for %s" % dest)
        os.replace(tmp, dest)  # atomic within the same filesystem
        tmp = None
    finally:
        if tmp is not None and os.path.exists(tmp):
            os.unlink(tmp)


def repair(scons_pkg: Path, version: str, manifest: dict,
           payload_root: Path = PAYLOAD_ROOT, log=print) -> str:
    """Ensure the allowlisted modules exist in the active SCons. Returns
    "noop" or "repaired". Raises SconsRepairError on any failure (fatal)."""
    if allowlist_present(scons_pkg, manifest):
        return "noop"  # complete for whatever version is installed
    # Repair is required from here on: everything must validate or we fail hard.
    if not version or version != manifest["scons_version"]:
        raise SconsRepairError(
            "SCons %r present but repair payload targets %r; refusing to repair a "
            "mismatched version — update scripts/scons_repair to this SCons version"
            % (version, manifest["scons_version"])
        )
    if not is_pio_managed_tool_scons(scons_pkg):
        raise SconsRepairError(
            "refusing to modify SCons outside a PlatformIO-managed tool-scons: %s"
            % scons_pkg
        )
    verify_payload(manifest, payload_root)
    lock_path = scons_pkg.parent.parent / LOCK_NAME  # in the tool-scons package dir
    with _FileLock(lock_path):
        if allowlist_present(scons_pkg, manifest):
            return "noop"  # another process repaired while we waited
        for entry in manifest["files"]:
            _atomic_install(payload_root / entry["path"],
                            _dest_for(scons_pkg, entry["path"]), entry["sha256"])
    if not allowlist_present(scons_pkg, manifest):
        raise SconsRepairError("repair incomplete after install into %s" % scons_pkg)
    log("[scons-repair] restored %d allowlisted SCons module(s) into %s"
        % (len(manifest["files"]), scons_pkg))
    return "repaired"


def repair_active_scons(repair_dir=None, log=print) -> str:
    """Repair the SCons engine currently executing this build."""
    import SCons  # noqa: PLC0415 -- available: this runs inside SCons

    rd = Path(repair_dir) if repair_dir else REPAIR_DIR
    scons_pkg = Path(SCons.__file__).resolve().parent
    version = getattr(SCons, "__version__", "") or scons_pkg.parent.name.replace(
        "scons-local-", "", 1
    )
    manifest = load_manifest(rd / "manifest.json")
    return repair(scons_pkg, version, manifest, rd / "payload", log=log)


# --- self-test (payload/manifest integrity; full suite in scons_repair/) ------
if __name__ == "__main__" and "--self-test" in sys.argv:
    m = load_manifest()
    verify_payload(m)
    print("patch_scons_tools self-test OK (manifest %s, %d payload file(s) verified)"
          % (m["scons_version"], len(m["files"])))
    raise SystemExit(0)

# --- PlatformIO pre-build entry point (runs only under SCons) ------------------
try:
    Import  # type: ignore # noqa: F821 -- injected by SCons only
    _UNDER_SCONS = True
except NameError:
    _UNDER_SCONS = False

if _UNDER_SCONS:
    Import("env")  # noqa: F821
    # $PROJECT_DIR is the repo root; locate the repair payload explicitly rather
    # than via __file__ (undefined in pio's exec context).
    _proj = Path(env.subst("$PROJECT_DIR"))  # noqa: F821
    # Fatal on failure: a broken repair must stop the build at this boundary,
    # never print-and-continue into a mislinked firmware.
    repair_active_scons(repair_dir=_proj / "scripts" / "scons_repair")
