"""Tests for the scons repair mechanism (scripts/patch_scons_tools.py).

Covers the scenarios required by review:
  1. complete payload detected correctly
  2. missing one required module -> repair occurs
  3. wrong SCons version -> hard fail
  4. wrong payload / hash -> hard fail
  5. partial / incomplete repair -> not treated as complete
  6. destination outside a PlatformIO-managed tool-scons -> hard fail
  7. concurrent repair attempts do not corrupt state
  8. second run after a successful repair is a clean no-op

Run: python3 scripts/scons_repair/test_repair.py
"""

import hashlib
import json
import sys
import threading
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))  # scripts/
import patch_scons_tools as psr  # noqa: E402


def _sha(text: str) -> str:
    return hashlib.sha256(text.encode()).hexdigest()


class RepairHarness:
    """Builds a fake pio-managed tool-scons + a matching vendored payload."""

    def __init__(self, tmp: Path, version: str = "4.11.1"):
        self.tmp = tmp
        self.version = version
        # allowlist content
        self.contents = {
            "SCons/Tool/FortranCommon.py": "# FortranCommon\nvalue = 1\n",
            "SCons/Scanner/Fortran.py": "# Scanner.Fortran\nvalue = 2\n",
        }
        # payload (vendored)
        self.payload = tmp / "payload"
        for rel, text in self.contents.items():
            p = self.payload / rel
            p.parent.mkdir(parents=True, exist_ok=True)
            p.write_text(text)
        self.manifest = {
            "scons_version": version,
            "files": [{"path": rel, "sha256": _sha(text)}
                      for rel, text in self.contents.items()],
        }
        # a realistic PlatformIO-managed engine path, missing the allowlist files
        self.scons_pkg = (tmp / ".platformio" / "packages" / "tool-scons"
                          / ("scons-local-%s" % version) / "SCons")
        self.scons_pkg.mkdir(parents=True)
        (self.scons_pkg / "__init__.py").write_text('__version__ = "%s"\n' % version)
        (self.scons_pkg / "Tool").mkdir()
        (self.scons_pkg / "Tool" / "__init__.py").write_text("# tool init\n")

    def make_complete(self):
        for rel, text in self.contents.items():
            d = psr._dest_for(self.scons_pkg, rel)
            d.parent.mkdir(parents=True, exist_ok=True)
            d.write_text(text)


class SconsRepairTests(unittest.TestCase):
    def setUp(self):
        import tempfile
        self._td = tempfile.TemporaryDirectory()
        self.h = RepairHarness(Path(self._td.name))

    def tearDown(self):
        self._td.cleanup()

    # 1
    def test_complete_payload_detected(self):
        psr.verify_payload(self.h.manifest, self.h.payload)  # must not raise

    # 2 + 8
    def test_missing_module_triggers_repair_then_noop(self):
        self.assertFalse(psr.allowlist_present(self.h.scons_pkg, self.h.manifest))
        r1 = psr.repair(self.h.scons_pkg, self.h.version, self.h.manifest,
                        self.h.payload, log=lambda *a: None)
        self.assertEqual(r1, "repaired")
        self.assertTrue(psr.allowlist_present(self.h.scons_pkg, self.h.manifest))
        for rel, text in self.h.contents.items():
            self.assertEqual(psr._dest_for(self.h.scons_pkg, rel).read_text(), text)
        # second run == clean no-op
        r2 = psr.repair(self.h.scons_pkg, self.h.version, self.h.manifest,
                        self.h.payload, log=lambda *a: None)
        self.assertEqual(r2, "noop")

    # 3
    def test_wrong_version_hard_fail(self):
        with self.assertRaises(psr.SconsRepairError):
            psr.repair(self.h.scons_pkg, "4.8.1", self.h.manifest,
                       self.h.payload, log=lambda *a: None)
        # nothing installed
        self.assertFalse(psr.allowlist_present(self.h.scons_pkg, self.h.manifest))

    def test_complete_foreign_version_is_noop_not_failure(self):
        # A complete engine of a different version must NOT hard-fail (x4pro case).
        self.h.make_complete()
        r = psr.repair(self.h.scons_pkg, "4.8.1", self.h.manifest,
                       self.h.payload, log=lambda *a: None)
        self.assertEqual(r, "noop")

    # 4
    def test_wrong_payload_hash_hard_fail(self):
        bad = self.h.payload / "SCons/Tool/FortranCommon.py"
        bad.write_text("# tampered\n")
        with self.assertRaises(psr.SconsRepairError):
            psr.repair(self.h.scons_pkg, self.h.version, self.h.manifest,
                       self.h.payload, log=lambda *a: None)
        with self.assertRaises(psr.SconsRepairError):
            psr.verify_payload(self.h.manifest, self.h.payload)

    def test_missing_payload_file_hard_fail(self):
        (self.h.payload / "SCons/Scanner/Fortran.py").unlink()
        with self.assertRaises(psr.SconsRepairError):
            psr.repair(self.h.scons_pkg, self.h.version, self.h.manifest,
                       self.h.payload, log=lambda *a: None)

    # 5
    def test_partial_state_not_treated_complete(self):
        # Only one of the two allowlisted files present -> not complete -> repairs.
        one = psr._dest_for(self.h.scons_pkg, "SCons/Tool/FortranCommon.py")
        one.parent.mkdir(parents=True, exist_ok=True)
        one.write_text(self.h.contents["SCons/Tool/FortranCommon.py"])
        self.assertFalse(psr.allowlist_present(self.h.scons_pkg, self.h.manifest))
        psr.repair(self.h.scons_pkg, self.h.version, self.h.manifest,
                   self.h.payload, log=lambda *a: None)
        self.assertTrue(psr.allowlist_present(self.h.scons_pkg, self.h.manifest))

    # 6
    def test_destination_outside_pio_hard_fail(self):
        import tempfile
        with tempfile.TemporaryDirectory() as d2:
            # a venv/site-packages-style SCons, NOT pio-managed
            bad_pkg = Path(d2) / "venv" / "lib" / "site-packages" / "SCons"
            (bad_pkg / "Tool").mkdir(parents=True)
            self.assertFalse(psr.is_pio_managed_tool_scons(bad_pkg))
            with self.assertRaises(psr.SconsRepairError):
                psr.repair(bad_pkg, self.h.version, self.h.manifest,
                           self.h.payload, log=lambda *a: None)

    def test_is_pio_managed_accepts_real_layout(self):
        self.assertTrue(psr.is_pio_managed_tool_scons(self.h.scons_pkg))

    # 7
    def test_concurrent_repairs_no_corruption(self):
        results, errors = [], []

        def worker():
            try:
                results.append(psr.repair(self.h.scons_pkg, self.h.version,
                                          self.h.manifest, self.h.payload,
                                          log=lambda *a: None))
            except Exception as exc:  # noqa: BLE001
                errors.append(exc)

        threads = [threading.Thread(target=worker) for _ in range(6)]
        for t in threads:
            t.start()
        for t in threads:
            t.join()
        self.assertEqual(errors, [], "no repair should error under contention")
        self.assertTrue(psr.allowlist_present(self.h.scons_pkg, self.h.manifest))
        for rel, text in self.h.contents.items():
            self.assertEqual(psr._dest_for(self.h.scons_pkg, rel).read_text(), text)
        # exactly one repaired; the rest are clean no-ops
        self.assertEqual(results.count("repaired"), 1, results)
        self.assertEqual(results.count("noop"), 5, results)


class RealManifestTests(unittest.TestCase):
    """The committed manifest + vendored payload must be self-consistent."""

    def test_committed_payload_matches_manifest(self):
        m = psr.load_manifest()
        psr.verify_payload(m)  # raises if any vendored file is missing/altered
        self.assertEqual(m["scons_version"], "4.11.1")


if __name__ == "__main__":
    unittest.main(verbosity=2)
