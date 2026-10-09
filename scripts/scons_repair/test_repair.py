"""Tests for the scons repair mechanism (scripts/patch_scons_tools.py).

Covers required scenarios:
  * complete payload detected correctly
  * missing module -> repair; second run is a clean no-op
  * corrupt-but-present files -> not complete -> repaired (4.11.1)
  * mixed-version files (4.8.1 content in a 4.11.1 engine) -> repaired
  * unsupported SCons version (even with both files present) -> hard fail
  * validate-only 4.8.1: correct hashes -> no-op; wrong content -> hard fail
  * wrong / absent payload hash -> hard fail
  * partial state -> not complete
  * destination outside an exact PlatformIO tool-scons (venv/site-packages and a
    lookalike ``tool-scons-not-platformio`` path) -> hard fail
  * concurrent repair attempts do not corrupt state

Run: python3 scripts/scons_repair/test_repair.py
"""

import hashlib
import sys
import threading
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))  # scripts/
import patch_scons_tools as psr  # noqa: E402

REL_TOOL = "SCons/Tool/FortranCommon.py"
REL_SCAN = "SCons/Scanner/Fortran.py"


def _sha(text: str) -> str:
    return hashlib.sha256(text.encode()).hexdigest()


class Harness:
    """Builds a version-keyed manifest, a vendored 4.11.1 payload, and helpers to
    construct fake PlatformIO-managed engines with arbitrary file contents."""

    def __init__(self, tmp: Path):
        self.tmp = tmp
        self.c411 = {REL_TOOL: "# 4.11.1 FortranCommon\n", REL_SCAN: "# 4.11.1 Scanner\n"}
        self.c481 = {REL_TOOL: "# 4.8.1 FortranCommon\n", REL_SCAN: "# 4.8.1 Scanner\n"}
        self.manifest = {
            "supported_versions": {
                "4.11.1": {"repairable": True,
                           "files": [{"path": r, "sha256": _sha(t)} for r, t in self.c411.items()]},
                "4.8.1": {"repairable": False,
                          "files": [{"path": r, "sha256": _sha(t)} for r, t in self.c481.items()]},
            }
        }
        # vendored payload holds the 4.11.1 files
        self.payload = tmp / "payload"
        for rel, text in self.c411.items():
            p = self.payload / rel
            p.parent.mkdir(parents=True, exist_ok=True)
            p.write_text(text)

    def engine(self, version: str, files: dict | None = None,
               packages_name: str = "packages", tool_name: str = "tool-scons") -> Path:
        scons_pkg = (self.tmp / "eng" / version / ".platformio" / packages_name
                     / tool_name / ("scons-local-%s" % version) / "SCons")
        (scons_pkg / "Tool").mkdir(parents=True)
        (scons_pkg / "Scanner").mkdir(parents=True)
        if files:
            for rel, text in files.items():
                psr._dest_for(scons_pkg, rel).write_text(text)
        return scons_pkg

    def repair(self, scons_pkg: Path, version: str):
        return psr.repair(scons_pkg, version, self.manifest, self.payload,
                          log=lambda *a: None)


class SconsRepairTests(unittest.TestCase):
    def setUp(self):
        import tempfile
        self._td = tempfile.TemporaryDirectory()
        self.h = Harness(Path(self._td.name))

    def tearDown(self):
        self._td.cleanup()

    def test_verify_payload_ok(self):
        psr.verify_payload(self.h.manifest["supported_versions"]["4.11.1"], self.h.payload)

    def test_missing_module_then_noop(self):
        pkg = self.h.engine("4.11.1")  # no files
        self.assertFalse(psr.allowlist_valid(pkg, self.h.manifest["supported_versions"]["4.11.1"]))
        self.assertEqual(self.h.repair(pkg, "4.11.1"), "repaired")
        for rel, text in self.h.c411.items():
            self.assertEqual(psr._dest_for(pkg, rel).read_text(), text)
        self.assertEqual(self.h.repair(pkg, "4.11.1"), "noop")

    def test_corrupt_but_present_is_repaired(self):
        pkg = self.h.engine("4.11.1", {REL_TOOL: "CORRUPT\n", REL_SCAN: "CORRUPT\n"})
        self.assertFalse(psr.allowlist_valid(pkg, self.h.manifest["supported_versions"]["4.11.1"]))
        self.assertEqual(self.h.repair(pkg, "4.11.1"), "repaired")
        self.assertEqual(psr._dest_for(pkg, REL_TOOL).read_text(), self.h.c411[REL_TOOL])

    def test_mixed_version_content_is_repaired(self):
        # 4.8.1 content sitting inside a 4.11.1 engine must be detected + replaced.
        pkg = self.h.engine("4.11.1", dict(self.h.c481))
        self.assertFalse(psr.allowlist_valid(pkg, self.h.manifest["supported_versions"]["4.11.1"]))
        self.assertEqual(self.h.repair(pkg, "4.11.1"), "repaired")
        self.assertEqual(psr._dest_for(pkg, REL_TOOL).read_text(), self.h.c411[REL_TOOL])

    def test_unsupported_version_hard_fail_even_if_files_present(self):
        pkg = self.h.engine("5.0.0", {REL_TOOL: "x\n", REL_SCAN: "y\n"})
        with self.assertRaises(psr.SconsRepairError):
            self.h.repair(pkg, "5.0.0")

    def test_validate_only_481_complete_is_noop(self):
        pkg = self.h.engine("4.8.1", dict(self.h.c481))
        self.assertEqual(self.h.repair(pkg, "4.8.1"), "noop")

    def test_validate_only_481_corrupt_hard_fail(self):
        pkg = self.h.engine("4.8.1", {REL_TOOL: "BAD\n", REL_SCAN: self.h.c481[REL_SCAN]})
        with self.assertRaises(psr.SconsRepairError):
            self.h.repair(pkg, "4.8.1")

    def test_wrong_payload_hash_hard_fail(self):
        (self.h.payload / REL_TOOL).write_text("# tampered payload\n")
        pkg = self.h.engine("4.11.1")
        with self.assertRaises(psr.SconsRepairError):
            self.h.repair(pkg, "4.11.1")

    def test_absent_payload_file_hard_fail(self):
        (self.h.payload / REL_SCAN).unlink()
        pkg = self.h.engine("4.11.1")
        with self.assertRaises(psr.SconsRepairError):
            self.h.repair(pkg, "4.11.1")

    def test_partial_not_complete(self):
        pkg = self.h.engine("4.11.1", {REL_TOOL: self.h.c411[REL_TOOL]})  # only one, valid
        self.assertFalse(psr.allowlist_valid(pkg, self.h.manifest["supported_versions"]["4.11.1"]))
        self.assertEqual(self.h.repair(pkg, "4.11.1"), "repaired")
        self.assertTrue(psr.allowlist_valid(pkg, self.h.manifest["supported_versions"]["4.11.1"]))

    def test_destination_site_packages_hard_fail(self):
        import tempfile
        with tempfile.TemporaryDirectory() as d2:
            bad = Path(d2) / "venv" / "lib" / "site-packages" / "SCons"
            (bad / "Tool").mkdir(parents=True)
            self.assertFalse(psr.is_pio_managed_tool_scons(bad, "4.11.1"))
            with self.assertRaises(psr.SconsRepairError):
                psr.repair(bad, "4.11.1", self.h.manifest, self.h.payload, log=lambda *a: None)

    def test_destination_lookalike_package_hard_fail(self):
        pkg = self.h.engine("4.11.1", tool_name="tool-scons-not-platformio")
        self.assertFalse(psr.is_pio_managed_tool_scons(pkg, "4.11.1"))
        with self.assertRaises(psr.SconsRepairError):
            self.h.repair(pkg, "4.11.1")

    def test_is_pio_managed_exact_and_version_bound(self):
        pkg = self.h.engine("4.11.1")
        self.assertTrue(psr.is_pio_managed_tool_scons(pkg, "4.11.1"))
        # a 4.11.1 engine path must not validate as the 4.8.1 destination
        self.assertFalse(psr.is_pio_managed_tool_scons(pkg, "4.8.1"))

    def test_concurrent_repairs_no_corruption(self):
        pkg = self.h.engine("4.11.1")
        results, errors = [], []

        def worker():
            try:
                results.append(self.h.repair(pkg, "4.11.1"))
            except Exception as exc:  # noqa: BLE001
                errors.append(exc)

        threads = [threading.Thread(target=worker) for _ in range(6)]
        for t in threads:
            t.start()
        for t in threads:
            t.join()
        self.assertEqual(errors, [])
        self.assertTrue(psr.allowlist_valid(pkg, self.h.manifest["supported_versions"]["4.11.1"]))
        self.assertEqual(results.count("repaired"), 1, results)
        self.assertEqual(results.count("noop"), 5, results)


class RealManifestTests(unittest.TestCase):
    """The committed manifest + vendored payload must be self-consistent."""

    def test_committed_payload_matches_manifest(self):
        m = psr.load_manifest()
        self.assertIn("4.11.1", m["supported_versions"])
        self.assertIn("4.8.1", m["supported_versions"])
        self.assertTrue(m["supported_versions"]["4.11.1"]["repairable"])
        self.assertFalse(m["supported_versions"]["4.8.1"]["repairable"])
        # vendored payload for every repairable version must verify
        for entry in m["supported_versions"].values():
            if entry.get("repairable"):
                psr.verify_payload(entry)


if __name__ == "__main__":
    unittest.main(verbosity=2)
