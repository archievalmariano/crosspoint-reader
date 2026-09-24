"""Repo-controlled repair/validation of pioarduino's tool-scons (no network).

Problem: the C3 custom-core rebuild (firmware_tuned_c3, used by gh_release)
reinstalls ``scons-local-4.11.1`` mid-build and relinks ``firmware.elf`` against a
copy missing ``SCons.Tool.FortranCommon`` — imported by
``SCons.Tool.linkCommon.smart_link`` — so the link dies with
``ModuleNotFoundError: No module named 'SCons.Tool.FortranCommon'``. The X4 Pro
production profile uses ``scons-local-4.8.1``, which pioarduino ships complete.

Fix: a narrow, deterministic, offline repair driven by ``scons_repair/manifest.json``.
For the ACTIVE SCons version the manifest records the sha256 of exactly the two
modules the link needs (``SCons/Tool/FortranCommon.py`` and
``SCons/Scanner/Fortran.py``):

  * "completeness" means every allowlisted file EXISTS AND its sha256 MATCHES the
    manifest — so corrupt, mixed-version, or arbitrary same-named files are never
    accepted;
  * an active SCons version absent from the manifest is a HARD FAILURE (existence
    of the files never bypasses the version check);
  * a ``repairable`` version (4.11.1) whose files are missing/mismatched is fixed
    from the vendored, hash-pinned payload; a validate-only version (4.8.1) that
    does not match its recorded hashes is a HARD FAILURE (we do not overwrite what
    pioarduino ships complete);
  * repair mutates only a PlatformIO-managed ``.../packages/tool-scons/
    scons-local-<version>/SCons`` (exact adjacent components required; system,
    venv and site-packages engines are refused);
  * every payload source file is hash-verified before use, every staged file
    before ``os.replace``, and the FINAL installed state is re-validated by hash;
  * installs are atomic (same-dir stage + ``os.replace``) under an exclusive
    ``fcntl`` lock; an interrupted or concurrent repair never leaves a state that
    validates as complete;
  * any validation/lock/install failure is FATAL at the repair boundary (raises);
    a second run after a successful repair is a clean no-op.

Tests: ``scripts/scons_repair/test_repair.py``.
"""

import hashlib
import json
import os
import sys
import tempfile
from pathlib import Path

try:
    _SCRIPTS_DIR = Path(__file__).resolve().parent
except NameError:  # PlatformIO execs this without a module __file__
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


def _check_files_list(files, where: str) -> None:
    if not isinstance(files, list) or not files:
        raise SconsRepairError("invalid manifest: no files for %s" % where)
    for entry in files:
        if not entry.get("path") or not entry.get("sha256"):
            raise SconsRepairError("invalid manifest file entry in %s: %r" % (where, entry))
        rel = Path(entry["path"])
        if rel.is_absolute() or ".." in rel.parts or not entry["path"].startswith("SCons/"):
            raise SconsRepairError("unsafe manifest path in %s: %s" % (where, entry["path"]))


def load_manifest(path: Path = MANIFEST_PATH) -> dict:
    data = json.loads(Path(path).read_text(encoding="utf-8"))
    versions = data.get("supported_versions")
    if not isinstance(versions, dict) or not versions:
        raise SconsRepairError("invalid manifest: missing supported_versions in %s" % path)
    for ver, entry in versions.items():
        if not isinstance(entry, dict) or "repairable" not in entry:
            raise SconsRepairError("invalid manifest entry for %s" % ver)
        _check_files_list(entry.get("files"), ver)
    return data


def resolve_version(version: str, manifest: dict) -> dict:
    """Return the manifest entry for an active SCons version, or HARD FAIL."""
    entry = manifest.get("supported_versions", {}).get(version)
    if entry is None:
        raise SconsRepairError(
            "SCons version %r is not in scripts/scons_repair/manifest.json; refusing "
            "to touch an unsupported toolchain — add its recorded module hashes there"
            % version
        )
    return entry


def _dest_for(scons_pkg: Path, rel_path: str) -> Path:
    """Resolve a manifest path ("SCons/<...>") to its file inside the engine.

    ``scons_pkg`` is the ``SCons/`` directory, so "SCons/Tool/x.py" lands at
    ``scons_pkg/Tool/x.py``.
    """
    return scons_pkg / rel_path[len("SCons/"):]


def _file_valid(dest: Path, expected_sha: str) -> bool:
    return dest.is_file() and sha256_file(dest) == expected_sha


def allowlist_valid(scons_pkg: Path, entry: dict) -> bool:
    """Complete iff EVERY allowlisted file exists AND its sha256 matches."""
    return all(_file_valid(_dest_for(scons_pkg, f["path"]), f["sha256"])
               for f in entry["files"])


def verify_payload(entry: dict, payload_root: Path = PAYLOAD_ROOT) -> None:
    """Each allowlisted file must exist in the vendored payload and match its hash."""
    for f in entry["files"]:
        src = payload_root / f["path"]
        if not src.is_file():
            raise SconsRepairError("repair payload missing file: %s" % src)
        actual = sha256_file(src)
        if actual != f["sha256"]:
            raise SconsRepairError(
                "repair payload hash mismatch for %s: expected %s got %s"
                % (f["path"], f["sha256"], actual)
            )


def is_pio_managed_tool_scons(scons_pkg: Path, version: str) -> bool:
    """True only for an EXACT PlatformIO-managed engine path:
    ``.../packages/tool-scons/scons-local-<version>/SCons``.

    Requires those four components adjacent and in order (rejecting lookalikes
    such as ``packages/tool-scons-not-platformio/...``) and refuses site-packages
    / dist-packages engines.
    """
    parts = scons_pkg.resolve().parts
    if "site-packages" in parts or "dist-packages" in parts:
        return False
    if len(parts) < 4:
        return False
    tail = parts[-4:]
    return (tail[0] == "packages"
            and tail[1] == "tool-scons"
            and tail[2] == "scons-local-%s" % version
            and tail[3] == "SCons")


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
    """Validate/repair the active SCons. Returns "noop" or "repaired".
    Raises SconsRepairError on any failure (fatal)."""
    entry = resolve_version(version, manifest)          # HARD FAIL: unknown version
    if allowlist_valid(scons_pkg, entry):               # hash-based completeness
        return "noop"
    # A supported version whose files are missing or do not match their hashes.
    if not entry.get("repairable"):
        raise SconsRepairError(
            "SCons %s modules are missing or do not match the recorded hashes, and "
            "this version is validate-only (no repair payload). Refusing to guess."
            % version
        )
    if not is_pio_managed_tool_scons(scons_pkg, version):
        raise SconsRepairError(
            "refusing to modify SCons outside an exact PlatformIO-managed "
            "packages/tool-scons/scons-local-%s/SCons path: %s" % (version, scons_pkg)
        )
    verify_payload(entry, payload_root)
    lock_path = scons_pkg.parent.parent / LOCK_NAME     # the tool-scons package dir
    with _FileLock(lock_path):
        if allowlist_valid(scons_pkg, entry):           # another process repaired
            return "noop"
        for f in entry["files"]:
            _atomic_install(payload_root / f["path"],
                            _dest_for(scons_pkg, f["path"]), f["sha256"])
    if not allowlist_valid(scons_pkg, entry):           # HASH-based post-install check
        raise SconsRepairError("repair did not produce a valid engine at %s" % scons_pkg)
    log("[scons-repair] restored %d allowlisted SCons module(s) into %s"
        % (len(entry["files"]), scons_pkg))
    return "repaired"


def repair_active_scons(repair_dir=None, log=print) -> str:
    """Validate/repair the SCons engine currently executing this build."""
    import SCons  # noqa: PLC0415 -- available: this runs inside SCons

    rd = Path(repair_dir) if repair_dir else REPAIR_DIR
    scons_pkg = Path(SCons.__file__).resolve().parent
    version = getattr(SCons, "__version__", "") or scons_pkg.parent.name.replace(
        "scons-local-", "", 1
    )
    return repair(scons_pkg, version, load_manifest(rd / "manifest.json"),
                  rd / "payload", log=log)


# --- self-test (manifest + vendored-payload integrity) ------------------------
if __name__ == "__main__" and "--self-test" in sys.argv:
    m = load_manifest()
    n = 0
    for ver, entry in m["supported_versions"].items():
        if entry.get("repairable"):
            verify_payload(entry)
            n += len(entry["files"])
    print("patch_scons_tools self-test OK (%d supported version(s), %d vendored "
          "payload file(s) verified)" % (len(m["supported_versions"]), n))
    raise SystemExit(0)

# --- PlatformIO pre-build entry point (runs only under SCons) ------------------
try:
    Import  # type: ignore # noqa: F821 -- injected by SCons only
    _UNDER_SCONS = True
except NameError:
    _UNDER_SCONS = False

if _UNDER_SCONS:
    Import("env")  # noqa: F821
    _proj = Path(env.subst("$PROJECT_DIR"))  # noqa: F821 -- repo root
    # Fatal on failure: a broken repair must stop the build at this boundary,
    # never print-and-continue into a mislinked firmware.
    repair_active_scons(repair_dir=_proj / "scripts" / "scons_repair")
