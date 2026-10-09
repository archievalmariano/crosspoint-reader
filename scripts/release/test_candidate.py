"""Tests for scripts/release/candidate.py: python3 -m unittest discover -s scripts/release"""

import copy
import hashlib
import io
import json
import sys
import tempfile
import unittest
from contextlib import redirect_stderr, redirect_stdout
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import candidate as c  # noqa: E402

LAYOUT = c.partition_layout()
NVS_OFF, NVS_SIZE = LAYOUT["nvs"]
OTA_OFF, OTA_SIZE = LAYOUT["otadata"]
APP_OFF, _ = LAYOUT["app0"]
DISPLAY = "1.6.5-A1.1.1"


def run(*argv):
    out, err = io.StringIO(), io.StringIO()
    with redirect_stdout(out), redirect_stderr(err):
        code = c.main([str(a) for a in argv])
    return code, out.getvalue(), err.getvalue()


def pieces(seed: int) -> dict:
    return {
        "bootloader.bin": bytes([0xE9, seed]) * 900,
        "partitions.bin": bytes([0xAA, 0x50, seed]) * 64,
        "boot_app0.bin": b"\x01\x00\x00\x00" + b"\xff" * (OTA_SIZE - 4),
    }


def factory(p: dict, app: bytes) -> bytes:
    image = bytearray(b"\xff" * (APP_OFF + len(app)))
    image[0:len(p["bootloader.bin"])] = p["bootloader.bin"]
    image[c.PARTITION_TABLE_OFFSET:c.PARTITION_TABLE_OFFSET + len(p["partitions.bin"])] = p["partitions.bin"]
    image[OTA_OFF:OTA_OFF + OTA_SIZE] = p["boot_app0.bin"]
    image[APP_OFF:] = app
    return bytes(image)


def make_builds(root: Path) -> Path:
    builds = root / "builds"
    for i, (env, pkg, _stem, has_factory, _legacy) in enumerate(c.RELEASE):
        d = builds / f"build-{env}"
        d.mkdir(parents=True)
        app = bytes([0xE9, i]) + bytes(range(256)) * (40 + i)
        (d / "firmware.bin").write_bytes(app)
        (d / "version.txt").write_text(f"{DISPLAY}-{pkg}\n")
        if has_factory:
            p = pieces(i)
            for name, data in p.items():
                (d / name).write_bytes(data)
            (d / "firmware.factory.bin").write_bytes(factory(p, app))
    return builds


class Base(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        self.builds = make_builds(self.root)
        self.out, self.support = self.root / "candidate", self.root / "support"

    def tearDown(self):
        self.tmp.cleanup()

    def assemble(self, builds=None):
        return run("assemble", "--builds", builds or self.builds, "--out", self.out, "--support", self.support,
                   "--display", DISPLAY, "--commit", "b" * 40, "--ref", "release/v1.1.1-clean", "--run-id", "42",
                   "--run-attempt", "1", "--workflow-ref", "o/r/.github/workflows/release_candidate.yml@refs/x",
                   "--repository", "o/r")

    def sums_sha(self):
        return hashlib.sha256((self.out / c.SUMS).read_bytes()).hexdigest()


class AssembleTests(Base):
    def test_assembles_exactly_the_release(self):
        code, out, err = self.assemble()
        self.assertEqual(code, 0, err)
        self.assertEqual(sorted(p.name for p in self.out.iterdir()), sorted(c.release_files() + [c.SUMS, c.PROVENANCE]))
        self.assertEqual(len(c.release_files()), 9)
        self.assertEqual((self.out / "firmware.bin").read_bytes(), (self.out / "firmware-x4-g.bin").read_bytes())
        self.assertEqual((self.out / "firmware-x4pro.bin").read_bytes(),
                         (self.out / "firmware-x4pro-gp.bin").read_bytes())
        prov = json.loads((self.out / c.PROVENANCE).read_text())
        self.assertEqual((prov["commit"], prov["runId"], prov["sumsSha256"]), ("b" * 40, 42, self.sums_sha()))
        for env in ("x4pro-goto-gh_release", "x4pro-onpoint-gh_release", "x4pro-gh_release"):
            self.assertEqual(sorted(p.name for p in (self.support / env).iterdir()), sorted(c.FLASH_PIECES))
        self.assertFalse((self.support / "gh_release").exists())
        self.assertEqual(run("verify-dir", self.out, "--support", self.support, "--sums-sha256", self.sums_sha(),
                             "--commit", "b" * 40, "--run-id", "42")[0], 0)

    def test_refuses_a_wrong_version_string(self):
        (self.builds / "build-x4pro-gh_release" / "version.txt").write_text(f"{DISPLAY}-G\n")
        code, _, err = self.assemble()
        self.assertEqual(code, 1)
        self.assertIn("not 1.6.5-A1.1.1-GP", err)

    def test_refuses_a_missing_or_extra_env(self):
        (self.builds / "build-x4pro-onpoint-gh_release").rename(self.builds / "build-x4pro-gh_release_rc")
        self.assertIn("not exactly the release envs", self.assemble()[2])

    def test_refuses_a_factory_image_that_is_not_its_pieces(self):
        d = self.builds / "build-x4pro-goto-gh_release"
        for name, offset in (("bootloader", 5), ("partition table", c.PARTITION_TABLE_OFFSET + 3),
                             ("boot_app0", OTA_OFF), ("app", APP_OFF + 7)):
            image = bytearray((d / "firmware.factory.bin").read_bytes())
            original = bytes(image)
            image[offset] ^= 0x01
            (d / "firmware.factory.bin").write_bytes(bytes(image))
            code, _, err = self.assemble()
            self.assertEqual(code, 1, name)
            self.assertIn(f"{name} differs", err)
            (d / "firmware.factory.bin").write_bytes(original)

    def test_refuses_a_factory_image_that_writes_nvs_or_gaps(self):
        d = self.builds / "build-x4pro-gh_release"
        for offset in (NVS_OFF + 10, c.PARTITION_TABLE_OFFSET - 1):
            image = bytearray((d / "firmware.factory.bin").read_bytes())
            original = bytes(image)
            image[offset] = 0x00
            (d / "firmware.factory.bin").write_bytes(bytes(image))
            self.assertIn("outside its pieces", self.assemble()[2])
            (d / "firmware.factory.bin").write_bytes(original)


class VerifyDirTests(Base):
    def setUp(self):
        super().setUp()
        self.assertEqual(self.assemble()[0], 0)

    def test_a_changed_file_fails(self):
        p = self.out / "firmware-x4pro-p.bin"
        p.write_bytes(p.read_bytes() + b"\0")
        self.assertIn("does not match", run("verify-dir", self.out)[2])

    def test_an_extra_or_missing_file_fails(self):
        (self.out / "extra.bin").write_bytes(b"x")
        self.assertIn("not exactly the release files", run("verify-dir", self.out)[2])
        (self.out / "extra.bin").unlink()
        (self.out / "firmware.bin").unlink()
        self.assertEqual(run("verify-dir", self.out)[0], 1)

    def test_the_accepted_digest_commit_and_run_must_match(self):
        self.assertIn("not the accepted", run("verify-dir", self.out, "--sums-sha256", "0" * 64)[2])
        self.assertIn("not " + "c" * 40, run("verify-dir", self.out, "--commit", "c" * 40)[2])
        self.assertIn("came from run 42", run("verify-dir", self.out, "--run-id", "43")[2])

    def test_rewritten_sums_do_not_pass_against_provenance(self):
        p = self.out / "firmware-x4-g.bin"
        p.write_bytes(b"other")
        (self.out / "firmware.bin").write_bytes(b"other")
        files = {n: (self.out / n).read_bytes() for n in c.release_files()}
        c.write_sums(files, self.out / c.SUMS)
        self.assertIn("provenance does not bind", run("verify-dir", self.out)[2])

    def test_a_flash_piece_that_is_not_the_candidates_fails(self):
        p = self.support / "x4pro-gh_release" / "boot_app0.bin"
        p.write_bytes(b"\xff" * OTA_SIZE)
        self.assertIn("is not the candidate's piece", run("verify-dir", self.out, "--support", self.support)[2])


class TagTests(unittest.TestCase):
    GOOD = ("v1.1.1\n\ncandidate-run: 123456\ncandidate-sums-sha256: " + "a" * 64 +
            "\nphysically-accepted: x4-g x4pro-g x4pro-p x4pro-gp\n")

    def test_reads_the_fields(self):
        f = c.parse_tag_message(self.GOOD)
        self.assertEqual((f["candidate-run"], f["candidate-sums-sha256"]), ("123456", "a" * 64))

    def test_every_package_must_be_physically_accepted(self):
        for accepted in ("x4-g x4pro-gp", "x4-g x4pro-g x4pro-p", "x4-g x4pro-g x4pro-p x4pro-gp x4pro-gpd",
                         "x4-g x4pro-g x4pro-p x4pro-gp x4pro-gp", ""):
            text = self.GOOD.replace("x4-g x4pro-g x4pro-p x4pro-gp", accepted)
            with self.assertRaises(c.Problem, msg=accepted):
                c.parse_tag_message(text)

    def test_missing_repeated_or_malformed_fields_fail(self):
        for text in (self.GOOD.replace("candidate-run: 123456\n", ""),
                     self.GOOD + "candidate-run: 7\n",
                     self.GOOD.replace("123456", "12ab"),
                     self.GOOD.replace("a" * 64, "A" * 64),
                     self.GOOD.replace("a" * 64, "a" * 63)):
            with self.assertRaises(c.Problem):
                c.parse_tag_message(text)


RUN = {
    "id": 9, "run_attempt": 1, "path": ".github/workflows/release_candidate.yml", "workflow_id": 77,
    "repository": {"full_name": "o/r"}, "head_repository": {"full_name": "o/r"}, "event": "workflow_dispatch",
    "status": "completed", "conclusion": "success", "head_branch": "release/v1.1.1-clean", "head_sha": "b" * 40,
}


class RunTests(unittest.TestCase):
    def check(self, run_json):
        c.check_run_json(run_json, repository="o/r", workflow_id=77, commit="b" * 40, branch_prefix="release/v1.1.1")

    def test_the_candidate_run_passes_with_or_without_a_ref_suffix(self):
        self.check(RUN)
        self.check(dict(RUN, path=".github/workflows/release_candidate.yml@refs/heads/release/v1.1.1-clean"))

    def test_anything_else_fails(self):
        for change in ({"path": ".github/workflows/release.yml"},
                       {"path": ".github/workflows/release_candidate.yml.evil@x"},
                       {"path": "evil/.github/workflows/release_candidate.yml"},
                       {"workflow_id": 78},
                       {"repository": {"full_name": "x/r"}},
                       {"head_repository": {"full_name": "x/r"}},
                       {"event": "push"}, {"event": "pull_request"},
                       {"conclusion": "failure"}, {"status": "in_progress", "conclusion": None},
                       {"head_branch": "develop"}, {"head_branch": "release/v1.1.0-clean"},
                       {"head_sha": "c" * 40}):
            with self.assertRaises(c.Problem, msg=str(change)):
                self.check(dict(RUN, **change))


class ReleaseTests(Base):
    def setUp(self):
        super().setUp()
        self.assertEqual(self.assemble()[0], 0)
        self.sums = (self.out / c.SUMS).read_bytes()
        self.release = {
            "tag_name": "v1.1.1", "draft": True, "prerelease": False, "immutable": False,
            "assets": [{"name": n, "state": "uploaded", "digest": "sha256:" + hashlib.sha256(
                (self.out / n).read_bytes()).hexdigest()} for n in c.release_files() + [c.SUMS]],
        }

    def check(self, release, state="draft", immutable=False):
        c.check_release_json(release, self.sums, tag="v1.1.1", state=state, require_immutable=immutable)

    def test_the_accepted_assets_pass(self):
        self.check(self.release)
        self.check(dict(self.release, draft=False, immutable=True), state="published", immutable=True)

    def test_a_replaced_missing_extra_or_undigested_asset_fails(self):
        cases = []
        r = copy.deepcopy(self.release)
        r["assets"][0]["digest"] = "sha256:" + "0" * 64
        cases.append(r)
        r = copy.deepcopy(self.release)
        del r["assets"][3]
        cases.append(r)
        r = copy.deepcopy(self.release)
        r["assets"].append({"name": "provenance.json", "state": "uploaded", "digest": "sha256:" + "1" * 64})
        cases.append(r)
        r = copy.deepcopy(self.release)
        r["assets"][2]["digest"] = None
        cases.append(r)
        r = copy.deepcopy(self.release)
        r["assets"][1]["state"] = "starter"
        cases.append(r)
        r = copy.deepcopy(self.release)
        r["assets"].append(dict(r["assets"][0]))
        cases.append(r)
        for release in cases:
            with self.assertRaises(c.Problem):
                self.check(release)

    def test_state_tag_prerelease_and_immutability_are_checked(self):
        for release, state, immutable in ((self.release, "published", False),
                                          (dict(self.release, draft=False), "draft", False),
                                          (dict(self.release, tag_name="v1.1.0"), "draft", False),
                                          (dict(self.release, prerelease=True), "draft", False),
                                          (dict(self.release, draft=False), "published", True)):
            with self.assertRaises(c.Problem):
                self.check(release, state, immutable)

    def test_a_different_sums_file_fails(self):
        with self.assertRaises(c.Problem):
            c.check_release_json(self.release, self.sums + b"\n", tag="v1.1.1", state="draft", require_immutable=False)


class SliceTests(Base):
    def test_slices_rebuild_the_factory_image_and_skip_nvs(self):
        self.assertEqual(self.assemble()[0], 0)
        image = (self.out / "firmware-x4pro-gp.factory.bin").read_bytes()
        dest = self.root / "slices"
        code, out, err = run("slice-factory", self.out / "firmware-x4pro-gp.factory.bin", "--out", dest,
                             "--app", self.out / "firmware-x4pro-gp.bin")
        self.assertEqual(code, 0, err)
        rebuilt = bytearray(b"\xff" * len(image))
        for p in dest.iterdir():
            offset = int(p.name.split("-", 1)[0], 16)
            data = p.read_bytes()
            self.assertFalse(offset < NVS_OFF + NVS_SIZE and offset + len(data) > NVS_OFF, p.name)
            rebuilt[offset:offset + len(data)] = data
        self.assertEqual(bytes(rebuilt), image)
        self.assertEqual((dest / f"0x{OTA_OFF:x}-boot_app0.bin").read_bytes(),
                         (self.support / "x4pro-gh_release" / "boot_app0.bin").read_bytes())

    def test_a_factory_image_that_writes_nvs_is_refused(self):
        self.assertEqual(self.assemble()[0], 0)
        image = bytearray((self.out / "firmware-x4pro-g.factory.bin").read_bytes())
        image[NVS_OFF] = 0
        bad = self.root / "bad.factory.bin"
        bad.write_bytes(bytes(image))
        self.assertIn("writes NVS", run("slice-factory", bad, "--out", self.root / "s")[2])


if __name__ == "__main__":
    unittest.main()
