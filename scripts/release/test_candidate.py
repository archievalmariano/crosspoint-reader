"""Tests for scripts/release/candidate.py: python3 -m unittest discover -s scripts/release"""

import argparse
import copy
import hashlib
import io
import json
import subprocess
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


class FakeGitHub:
    """GitHub as publish sees it: canned GETs, and a recorded PATCH that publishes."""

    def __init__(self, gets: dict, immutable_after=True, tamper_after=None, raise_on=None, after_patch=None,
                 patch_error=None):
        self.gets, self.patches = gets, []
        self.immutable_after, self.tamper_after = immutable_after, tamper_after
        self.raise_on = raise_on or {}        # path -> exception raised by GET (from the start)
        self.after_patch = after_patch or {}  # path -> exception or value, once the PATCH succeeded
        self.patch_error = patch_error

    def get(self, path):
        if self.patches and path in self.after_patch:
            outcome = self.after_patch[path]
            if isinstance(outcome, BaseException):
                raise outcome
            return copy.deepcopy(outcome)
        if path in self.raise_on:
            raise self.raise_on[path]
        if path not in self.gets:
            raise c.Problem(f"unexpected GET {path}")
        return copy.deepcopy(self.gets[path])

    def publish_latest(self, repository, release_id):
        self.patches.append((repository, release_id))
        if self.patch_error:
            raise self.patch_error
        self.apply_publication(release_id)

    def apply_publication(self, release_id):
        release = copy.deepcopy(self.gets["repos/o/r/releases?per_page=100"][0])
        release.update(draft=False, immutable=self.immutable_after)
        if self.tamper_after:
            self.tamper_after(release)
        self.gets[f"repos/o/r/releases/{release_id}"] = release
        self.gets["repos/o/r/releases/latest"] = release


class PublishBase(Base):
    TAG, COMMIT, RUN_ID = "v1.1.1", "b" * 40, 42

    def setUp(self):
        super().setUp()
        self.assertEqual(self.assemble()[0], 0)
        sums = self.sums_sha()
        draft = {"id": 7, "tag_name": self.TAG, "draft": True, "prerelease": False, "immutable": False,
                 "assets": [{"name": n, "state": "uploaded", "digest": "sha256:" + hashlib.sha256(
                     (self.out / n).read_bytes()).hexdigest()} for n in c.release_files() + [c.SUMS]]}
        message = (f"{self.TAG}\n\ncandidate-run: {self.RUN_ID}\ncandidate-sums-sha256: {sums}\n"
                   "physically-accepted: x4-g x4pro-g x4pro-p x4pro-gp\n")
        self.gets = {
            "repos/o/r/immutable-releases": {"enabled": True, "enforced_by_owner": False},
            "repos/o/r/releases?per_page=100": [draft, {"id": 6, "tag_name": "v1.1.0", "draft": False}],
            f"repos/o/r/git/ref/tags/{self.TAG}": {"object": {"type": "tag", "sha": "t" * 40}},
            f"repos/o/r/git/tags/{'t' * 40}": {"object": {"type": "commit", "sha": self.COMMIT}, "message": message},
            "repos/o/r/actions/workflows/release_candidate.yml": {"id": 77},
            f"repos/o/r/actions/runs/{self.RUN_ID}": dict(RUN, id=self.RUN_ID, head_sha=self.COMMIT),
        }

    def publish(self, gh, publish=True, **override):
        args = dict(repository="o/r", tag=self.TAG, commit=self.COMMIT, run_id=str(self.RUN_ID),
                    candidate=str(self.out), sums_sha256=self.sums_sha(), publish=publish)
        args.update(override)
        out = io.StringIO()
        with redirect_stdout(out):
            c.publish(argparse.Namespace(**args), gh)
        return out.getvalue()

    def refused(self, gh=None, **override):
        gh = gh or FakeGitHub(self.gets)
        with self.assertRaises(c.Problem) as ctx:
            self.publish(gh, **override)
        self.assertNotIsInstance(ctx.exception, c.PostPublishProblem)
        self.assertEqual(gh.patches, [], "no publication action may happen")
        return str(ctx.exception)

    def draft(self):
        return self.gets["repos/o/r/releases?per_page=100"][0]

    def main(self, gh):
        """The command as the owner runs it, with `gh` as GitHub; (exit code, stderr)."""
        original = c.GitHub
        c.GitHub = lambda: gh
        try:
            code, _, err = run("publish", "--repository", "o/r", "--tag", self.TAG, "--commit", self.COMMIT,
                               "--run-id", self.RUN_ID, "--candidate", self.out, "--sums-sha256", self.sums_sha(),
                               "--publish")
        except BaseException as escaped:  # noqa: BLE001 -- an escape is itself the failure under test
            return -1, f"escaped main(): {type(escaped).__name__}"
        finally:
            c.GitHub = original
        return code, err

    def assert_uncertain(self, gh):
        code, err = self.main(gh)
        self.assertEqual((code, len(gh.patches)), (2, 1), err)
        self.assertIn("MAY BE PUBLISHED", err)

    def assert_not_published(self, gh, code_expected):
        code, err = self.main(gh)
        self.assertEqual(code, code_expected, err)
        self.assertNotIn("MAY BE PUBLISHED", err)
        return err


class PublishTests(PublishBase):
    def test_a_clean_preflight_publishes_once_and_verifies(self):
        gh = FakeGitHub(self.gets)
        out = self.publish(gh)
        self.assertEqual(gh.patches, [("o/r", 7)])
        self.assertIn("published v1.1.1 as latest: immutable", out)

    def test_a_dry_run_changes_nothing(self):
        gh = FakeGitHub(self.gets)
        self.assertIn("dry run", self.publish(gh, publish=False))
        self.assertEqual(gh.patches, [])

    def test_immutability_disabled_or_unknown_refuses(self):
        for setting in ({"enabled": False, "enforced_by_owner": False}, {}, {"enabled": "true"}):
            self.gets["repos/o/r/immutable-releases"] = setting
            self.assertIn("immutability", self.refused())

    def test_draft_asset_tampering_refuses(self):
        self.draft()["assets"][4]["digest"] = "sha256:" + "0" * 64
        self.assertIn("not the accepted sha256", self.refused())

    def test_missing_extra_duplicate_or_undigested_assets_refuse(self):
        original = copy.deepcopy(self.draft()["assets"])
        for mutate in (lambda a: a.pop(0),
                       lambda a: a.append({"name": "provenance.json", "state": "uploaded", "digest": "sha256:" + "1" * 64}),
                       lambda a: a.append(dict(a[0])),
                       lambda a: a[1].update(digest=None)):
            self.draft()["assets"] = copy.deepcopy(original)
            mutate(self.draft()["assets"])
            self.refused()

    def test_wrong_tag_commit_or_run_refuses(self):
        tag_obj = f"repos/o/r/git/tags/{'t' * 40}"
        run = f"repos/o/r/actions/runs/{self.RUN_ID}"
        cases = [
            (f"repos/o/r/git/ref/tags/{self.TAG}", {"object": {"type": "commit", "sha": self.COMMIT}}),  # lightweight
            (tag_obj, dict(self.gets[tag_obj], object={"type": "commit", "sha": "c" * 40})),
            (tag_obj, dict(self.gets[tag_obj], message=self.gets[tag_obj]["message"].replace("run: 42", "run: 43"))),
            (tag_obj, dict(self.gets[tag_obj], message=self.gets[tag_obj]["message"].replace(" x4pro-p", ""))),
            (tag_obj, dict(self.gets[tag_obj], message=self.gets[tag_obj]["message"].replace(self.sums_sha(), "e" * 64))),
            (run, dict(self.gets[run], head_sha="c" * 40)),
            (run, dict(self.gets[run], path=".github/workflows/release.yml")),
            (run, dict(self.gets[run], head_branch="release/v1.1.0-clean")),
            (run, dict(self.gets[run], conclusion="failure")),
        ]
        for path, value in cases:
            saved = self.gets[path]
            self.gets[path] = value
            self.refused()
            self.gets[path] = saved

    def test_a_local_candidate_that_is_not_the_accepted_one_refuses(self):
        self.refused(commit="c" * 40)
        self.refused(run_id="43")
        self.refused(sums_sha256="e" * 64)
        p = self.out / "firmware-x4pro-gp.bin"
        p.write_bytes(p.read_bytes() + b"\0")
        self.refused()

    def test_an_already_published_missing_or_duplicated_release_refuses(self):
        self.draft()["draft"] = False
        self.assertIn("already published", self.refused())
        self.draft()["draft"] = True
        self.draft()["prerelease"] = True
        self.assertIn("prerelease", self.refused())
        self.draft()["prerelease"] = False
        releases = self.gets["repos/o/r/releases?per_page=100"]
        self.gets["repos/o/r/releases?per_page=100"] = releases[1:]
        self.assertIn("0 releases", self.refused())
        self.gets["repos/o/r/releases?per_page=100"] = releases + [copy.deepcopy(releases[0])]
        self.assertIn("2 releases", self.refused())

    def test_post_publication_requires_immutable_and_the_accepted_digests(self):
        for gh in (FakeGitHub(self.gets, immutable_after=False),
                   FakeGitHub(self.gets, tamper_after=lambda r: r["assets"][0].update(digest="sha256:" + "0" * 64)),
                   FakeGitHub(self.gets, tamper_after=lambda r: r.update(prerelease=True))):
            with self.assertRaises(c.PostPublishProblem):
                self.publish(gh)
            self.assertEqual(len(gh.patches), 1)

    def test_post_publication_requires_it_to_be_latest(self):
        gh = FakeGitHub(self.gets)
        real = gh.publish_latest

        def publish_but_not_latest(repository, release_id):
            real(repository, release_id)
            gh.gets["repos/o/r/releases/latest"] = {"id": 6, "tag_name": "v1.1.0"}
        gh.publish_latest = publish_but_not_latest
        with self.assertRaises(c.PostPublishProblem):
            self.publish(gh)

    def test_a_clean_run_exits_0(self):
        gh = FakeGitHub(self.gets)
        self.assertEqual(self.main(gh), (0, ""))
        self.assertEqual(len(gh.patches), 1)

    def test_post_publication_validation_failures_exit_2(self):
        self.assert_uncertain(FakeGitHub(self.gets, immutable_after=False))
        self.assert_uncertain(FakeGitHub(self.gets, tamper_after=lambda r: r["assets"].pop()))

    def test_post_publication_oserror_on_the_first_refetch_exits_2(self):
        self.assert_uncertain(FakeGitHub(self.gets, after_patch={"repos/o/r/releases/7": OSError("connection reset")}))

    def test_post_publication_malformed_responses_exit_2(self):
        for path, value in (("repos/o/r/releases/7", ["not", "a", "release"]),   # AttributeError
                            ("repos/o/r/releases/7", "text"),                     # AttributeError
                            ("repos/o/r/releases/7", {"tag_name": "v1.1.1", "draft": False, "prerelease": False,
                                                      "immutable": True, "assets": [{"name": None}]}),
                            ("repos/o/r/releases/latest", None),                   # AttributeError
                            ("repos/o/r/releases/latest", ValueError("Expecting value: line 1 column 1")),
                            ("repos/o/r/releases/latest", KeyError("id")),
                            (f"repos/o/r/git/tags/{'t' * 40}", RuntimeError("gh crashed")),
                            (f"repos/o/r/git/tags/{'t' * 40}", subprocess.CalledProcessError(1, ["gh"], "", "HTTP 502")),
                            ("repos/o/r/releases/7", KeyboardInterrupt())):
            with self.subTest(path=path, value=type(value).__name__):
                self.assert_uncertain(FakeGitHub(self.gets, after_patch={path: value}))

    def test_preflight_errors_exit_1_and_never_say_may_be_published(self):
        for path, value in (("repos/o/r/immutable-releases", OSError("network down")),
                            ("repos/o/r/immutable-releases", ValueError("Expecting value")),
                            ("repos/o/r/releases?per_page=100", subprocess.CalledProcessError(1, ["gh"], "", "HTTP 502")),
                            ("repos/o/r/actions/workflows/release_candidate.yml", KeyError("id"))):
            with self.subTest(path=path, value=type(value).__name__):
                gh = FakeGitHub(self.gets, raise_on={path: value})
                self.assert_not_published(gh, 1)
                self.assertEqual(gh.patches, [])
        for path, value in (("repos/o/r/immutable-releases", ["enabled"]),          # AttributeError
                            (f"repos/o/r/git/tags/{'t' * 40}", {"object": None})):  # TypeError
            with self.subTest(path=path, value=repr(value)):
                saved = self.gets[path]
                self.gets[path] = value
                gh = FakeGitHub(self.gets)
                self.assert_not_published(gh, 1)
                self.assertEqual(gh.patches, [])
                self.gets[path] = saved

    def test_a_patch_certainly_not_applied_exits_3(self):
        gh = FakeGitHub(self.gets, patch_error=c.PublishNotApplied("could not start gh: [Errno 2] gh"))
        self.assertIn("not published", self.assert_not_published(gh, 3))
        self.assertEqual(len(gh.patches), 1)

    def test_a_patch_that_may_have_been_applied_exits_2(self):
        for error in (RuntimeError("gh api PATCH exited 1: (HTTP 502)"), TimeoutError(), KeyboardInterrupt()):
            with self.subTest(error=type(error).__name__):
                self.assert_uncertain(FakeGitHub(self.gets, patch_error=error))


def fake_popen(behaviour: dict, record: list):
    """A stand-in for subprocess.Popen: `start` raises on creation, `communicate`
    raises while talking to the child, else the child exits `rc` with `stderr`."""

    class FakeProcess:
        def __init__(self, argv, **kwargs):
            record.append(("start", argv))
            if "start" in behaviour:
                raise behaviour["start"]
            self.returncode = None

        def communicate(self, timeout=None):
            record.append(("communicate", timeout))
            if "communicate" in behaviour:
                raise behaviour["communicate"]
            self.returncode = behaviour.get("rc", 0)
            return "", behaviour.get("stderr", "")

        def kill(self):
            record.append(("kill",))

        def wait(self, timeout=None):
            record.append(("wait",))
            self.returncode = -9
            return -9

    return FakeProcess


class popen_as:  # noqa: N801 -- a context manager used like a function
    def __init__(self, behaviour):
        self.behaviour, self.record = behaviour, []

    def __enter__(self):
        self.original = c.subprocess.Popen
        c.subprocess.Popen = fake_popen(self.behaviour, self.record)
        return self.record

    def __exit__(self, *exc):
        c.subprocess.Popen = self.original
        return False


STARTUP_FAILURES = (FileNotFoundError("gh"), PermissionError("gh"), OSError("fork failed"), ValueError("bad argv"))
COMMUNICATION_FAILURES = (OSError("read failed"), BrokenPipeError(), ConnectionResetError(),
                          UnicodeDecodeError("utf-8", b"\xff", 0, 1, "invalid start byte"),
                          subprocess.TimeoutExpired(["gh"], 120), KeyboardInterrupt())
# Every non-zero exit of a started gh is uncertain, even a clean-looking 4xx:
# stderr cannot prove the PATCH was not applied.
NONZERO_EXITS = ((1, "gh: Validation Failed (HTTP 422)"), (1, "gh: Not Found (HTTP 404)"),
                 (1, "gh: Resource not accessible by integration (HTTP 403)"), (1, "gh: Server Error (HTTP 502)"),
                 (1, "gh: (HTTP 500)"), (1, "error connecting to api.github.com"), (1, ""),
                 (1, "timeout awaiting response headers (HTTP 422)"), (2, "gh: Validation Failed (HTTP 422)"))


class GitHubClientTests(unittest.TestCase):
    """publish_latest: only a creation failure is PublishNotApplied; once the
    process exists, every non-success outcome is PublishUncertain."""

    def call(self, behaviour):
        with popen_as(behaviour) as record:
            try:
                c.GitHub().publish_latest("o/r", 7)
                return None, record
            except BaseException as err:  # noqa: BLE001
                return err, record

    def test_success_sends_one_patch_with_a_timeout(self):
        err, record = self.call({"rc": 0})
        self.assertIsNone(err)
        self.assertEqual(record, [("start", ["gh", "api", "-X", "PATCH", "repos/o/r/releases/7", "-F", "draft=false",
                                             "-f", "make_latest=true"]), ("communicate", c.PUBLISH_TIMEOUT_S)])

    def test_a_process_that_was_never_created_is_not_applied(self):
        for failure in STARTUP_FAILURES:
            with self.subTest(failure=repr(failure)):
                err, record = self.call({"start": failure})
                self.assertIsInstance(err, c.PublishNotApplied)
                self.assertEqual([r[0] for r in record], ["start"])

    def test_any_failure_after_the_process_started_is_uncertain(self):
        for failure in COMMUNICATION_FAILURES:
            with self.subTest(failure=type(failure).__name__):
                err, record = self.call({"communicate": failure})
                self.assertIsInstance(err, c.PublishUncertain)
                self.assertNotIsInstance(err, c.PublishNotApplied)
                self.assertIn(("kill",), record)

    def test_every_nonzero_exit_of_a_started_process_is_uncertain(self):
        for rc, stderr in NONZERO_EXITS:
            with self.subTest(rc=rc, stderr=stderr):
                err, _ = self.call({"rc": rc, "stderr": stderr})
                self.assertIsInstance(err, c.PublishUncertain)
                self.assertNotIsInstance(err, c.PublishNotApplied)


REAL_GITHUB = c.GitHub  # PublishTests.main swaps c.GitHub out for the fake


class ClientGitHub(FakeGitHub):
    """Canned GETs, but the publication request goes through the real client
    (over a fake Popen), so its classification reaches the exit status."""

    def publish_latest(self, repository, release_id):
        self.patches.append((repository, release_id))
        REAL_GITHUB.publish_latest(self, repository, release_id)
        self.apply_publication(release_id)


class PublishRequestExitTests(PublishBase):
    def exit_for(self, behaviour):
        gh = ClientGitHub(self.gets)
        with popen_as(behaviour):
            code, err = self.main(gh)
        return code, err, gh

    def test_startup_failure_exits_3(self):
        for failure in STARTUP_FAILURES:
            with self.subTest(failure=repr(failure)):
                code, err, _ = self.exit_for({"start": failure})
                self.assertEqual(code, 3, err)
                self.assertIn("not published", err)
                self.assertNotIn("MAY BE PUBLISHED", err)

    def test_communication_failures_after_start_exit_2(self):
        for failure in COMMUNICATION_FAILURES:
            with self.subTest(failure=type(failure).__name__):
                code, err, _ = self.exit_for({"communicate": failure})
                self.assertEqual(code, 2, err)
                self.assertIn("MAY BE PUBLISHED", err)

    def test_any_nonzero_exit_even_a_clean_4xx_exits_2(self):
        for rc, stderr in NONZERO_EXITS:
            with self.subTest(rc=rc, stderr=stderr):
                code, err, _ = self.exit_for({"rc": rc, "stderr": stderr})
                self.assertEqual(code, 2, err)
                self.assertIn("MAY BE PUBLISHED", err)

    def test_a_successful_request_still_publishes_and_verifies(self):
        code, err, gh = self.exit_for({"rc": 0})
        self.assertEqual((code, err, len(gh.patches)), (0, "", 1))


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
