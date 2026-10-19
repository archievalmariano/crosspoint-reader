"""Tests for scripts/release/candidate.py: python3 -m unittest discover -s scripts/release"""

import argparse
import copy
import errno
import hashlib
import io
import json
import os
import shutil
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
DISPLAY = "1.6.5-A1.2.0"


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


def junit(statuses: dict = None, extra: str = "") -> bytes:
    """A ctest JUnit report: every required E3 test passed, unless `statuses`
    says otherwise ({name: "fail" | "skip" | "missing"})."""
    statuses = statuses or {}
    cases = []
    names = dict.fromkeys(c.E3_HOST_TESTS + c.POST_LAUNCH_TESTS + ("DogearPages.NavigationIsCircular",))
    for name in names:
        state = statuses.get(name, "pass")
        if state == "missing":
            continue
        if state == "fail":
            cases.append(f'<testcase name="{name}" classname="{name}" time="0.01" status="fail">'
                         '<failure message="Failed"/></testcase>')
        elif state == "skip":
            cases.append(f'<testcase name="{name}" classname="{name}" time="0" status="notrun"><skipped/></testcase>')
        else:
            cases.append(f'<testcase name="{name}" classname="{name}" time="0.01" status="run"/>')
    return (f'<?xml version="1.0"?><testsuite name="Linux" tests="{len(cases)}">' + "".join(cases) + extra
            + "</testsuite>").encode()


def evidence(commit="b" * 40, run=42, repository="o/r", report=None) -> bytes:
    return c.e3_evidence_from_junit(report or junit(), c.load_acceptance_schema(), repository=repository,
                                    commit=commit, run=run)


def post_launch(commit="b" * 40, run=42, repository="o/r", report=None) -> bytes:
    return c.post_launch_evidence_from_junit(report or junit(), c.load_maintenance_schema(), repository=repository,
                                             commit=commit, run=run)


class Base(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        self.builds = make_builds(self.root)
        self.out, self.support = self.root / "candidate", self.root / "support"
        self.e3 = self.root / "e3-host-evidence.json"
        self.e3.write_bytes(evidence())
        self.post_launch = self.root / "post-launch-host-evidence.json"
        self.post_launch.write_bytes(post_launch())

    def tearDown(self):
        self.tmp.cleanup()

    def assemble(self, builds=None):
        return run("assemble", "--builds", builds or self.builds, "--out", self.out, "--support", self.support,
                   "--display", DISPLAY, "--commit", "b" * 40, "--ref", "release/v1.2.0-dogear", "--run-id", "42",
                   "--run-attempt", "1", "--workflow-ref", "o/r/.github/workflows/release_candidate.yml@refs/x",
                   "--repository", "o/r", "--e3-evidence", self.e3, "--post-launch-evidence", self.post_launch)

    def sums_sha(self):
        return hashlib.sha256((self.out / c.SUMS).read_bytes()).hexdigest()


FIXTURES = {
    "generator": "d" * 40,
    "R": {"issueId": "dogear-2026-11-23", "schemaVersion": 2, "sha256": "1" * 64, "items": 4, "pages": 6},
    "Q1": {"issueId": "dogear-2026-11-16", "schemaVersion": 3, "sha256": "2" * 64, "items": 4, "coverRows": 3,
           "pages": 6},
    "Q2": {"issueId": "dogear-2026-12-21", "schemaVersion": 3, "sha256": "3" * 64, "items": 5, "coverRows": 3,
           "pages": 7},
}


def acceptance_record(candidate: Path) -> dict:
    """A complete, valid acceptance record of the candidate in `candidate`."""
    prov = json.loads((candidate / c.PROVENANCE).read_text())
    sums = c.read_sums((candidate / c.SUMS).read_bytes())
    schema = c.load_acceptance_schema()
    entries = []
    for env, _pkg, stem, has_factory, _legacy in c.RELEASE:
        name = c.package_name(stem)
        spec = schema["packages"][name]
        entry = {"package": name, "env": env, "board": spec["board"], "file": f"{stem}.bin",
                 "sha256": sums[f"{stem}.bin"], "install": spec["install"], "level": spec["level"],
                 "date": "2026-10-20", "checks": {check: "PASS" for check in spec["checks"]}}
        if has_factory:
            entry["pieces"] = {k: prov["flashSupport"][env][f"{k}.bin"] for k in c.PIECE_KEYS}
        entries.append(entry)
    return {"schema": "crosspoint-acceptance/1", "repository": prov["repository"], "tag": "v1.2.0",
            "commit": prov["commit"], "candidateRun": prov["runId"], "sumsSha256": prov["sumsSha256"],
            "fixtures": copy.deepcopy(FIXTURES), "packages": entries,
            "hostCovered": {c.E3_CHECK: json.loads((candidate / c.E3_EVIDENCE).read_bytes())},
            "approvedBy": "owner", "approvedOn": "2026-10-21"}


class AcceptanceTests(Base):
    def setUp(self):
        super().setUp()
        self.assertEqual(self.assemble()[0], 0)
        self.record = acceptance_record(self.out)
        self.path = self.root / "ACCEPTANCE.json"

    def write(self, record=None, text=None):
        self.path.write_text(text if text is not None else json.dumps(record or self.record, indent=2))
        return self.path

    def digest(self, record=None, text=None):
        return c.acceptance_digest(self.write(record, text), self.out)

    def refused(self, mutate=None, text=None):
        record = copy.deepcopy(self.record)
        if mutate:
            mutate(record)
        with self.assertRaises(c.Problem):
            self.digest(record, text)

    def test_the_complete_record_passes_and_its_digest_is_canonical(self):
        digest = self.digest()
        self.assertEqual(digest, hashlib.sha256(c.canonical(self.record)).hexdigest())
        # Layout and key order in the file do not change it; the content does.
        self.assertEqual(self.digest(text=json.dumps(self.record, sort_keys=True)), digest)
        self.assertEqual(self.digest(text=json.dumps(dict(reversed(list(self.record.items()))))), digest)
        code, out, err = run("acceptance", "--record", self.write(), "--candidate-dir", self.out,
                             "--flash-support", self.support)
        self.assertEqual(code, 0, err)
        self.assertIn(f"acceptance-sha256: {digest}", out)

    def test_exactly_the_eight_packages_once_each_in_order(self):
        pk = "packages"
        self.refused(lambda r: r[pk].pop(1))                                   # x4-gd missing
        self.refused(lambda r: r[pk].append(copy.deepcopy(r[pk][0])))          # duplicate
        self.refused(lambda r: r[pk].insert(2, copy.deepcopy(r[pk][1])))       # duplicate, in the middle
        self.refused(lambda r: r[pk].__setitem__(slice(0, 2), r[pk][1::-1]))   # order
        self.refused(lambda r: r[pk][1].update(package="x4-d"))                # an unknown package
        self.refused(lambda r: r.update(packages={}))
        self.refused(lambda r: r.update(packages=[]))

    def test_exactly_the_required_checks_each_pass(self):
        flagship = lambda r: r["packages"][7]["checks"]  # noqa: E731 -- x4pro-gpd
        for check in ("dogear-e1-no-issue-yet", "dogear-e2-no-issue-on-device", "dogear-v3-q1-cover",
                      "dogear-v3-q2-navigation", "dogear-v2-cache-503", "settings-kept", "install-bytes"):
            self.refused(lambda r, k=check: flagship(r).pop(k))                  # missing
            self.refused(lambda r, k=check: flagship(r).update({k: "FAIL"}))     # failed
            self.refused(lambda r, k=check: flagship(r).update({k: "pass"}))     # not exactly PASS
            self.refused(lambda r, k=check: flagship(r).update({k: True}))
        self.refused(lambda r: r["packages"][2]["checks"].update({"dogear-opens": "PASS"}))  # x4pro-g has no DOGEAR
        self.refused(lambda r: r["packages"][0]["checks"].update({"extra": "PASS"}))
        self.refused(lambda r: r["packages"][1]["checks"].pop("dogear-v3-q2-cover"))         # x4-gd: Q2 mandatory
        # Physical E3 is not a check: recording it is refused, on either flagship.
        self.refused(lambda r: r["packages"][1]["checks"].update({c.E3_CHECK: "PASS"}))
        self.refused(lambda r: r["packages"][7]["checks"].update({c.E3_CHECK: "PASS"}))
        self.refused(lambda r: r["packages"][3].update(checks=["install-bytes"]))

    def test_each_package_is_bound_to_its_candidate_file_and_pieces(self):
        self.refused(lambda r: r["packages"][1].update(sha256="0" * 64))
        self.refused(lambda r: r["packages"][1].update(sha256=r["packages"][0]["sha256"]))
        self.refused(lambda r: r["packages"][1].update(file="firmware-x4-g.bin", sha256=r["packages"][0]["sha256"]))
        self.refused(lambda r: r["packages"][3].update(env="x4pro-goto-gh_release"))
        self.refused(lambda r: r["packages"][7].update(level="full"))
        self.refused(lambda r: r["packages"][7].update(install="microSD"))
        self.refused(lambda r: r["packages"][7].update(board="X4"))
        self.refused(lambda r: r["packages"][7]["pieces"].update(boot_app0="0" * 64))
        self.refused(lambda r: r["packages"][7]["pieces"].pop("partitions"))
        self.refused(lambda r: r["packages"][7].pop("pieces"))
        self.refused(lambda r: r["packages"][1].update(pieces=r["packages"][3]["pieces"]))  # the X4 has none
        self.refused(lambda r: r["packages"][1].update(date="2026-13-01"))
        self.refused(lambda r: r["packages"][1].update(notes="looked fine"))  # no free text

    def test_the_record_is_bound_to_the_candidate(self):
        self.refused(lambda r: r.update(commit="c" * 40))
        self.refused(lambda r: r.update(candidateRun=43))
        self.refused(lambda r: r.update(candidateRun="42"))
        self.refused(lambda r: r.update(candidateRun=True))
        self.refused(lambda r: r.update(sumsSha256="e" * 64))
        self.refused(lambda r: r.update(tag="v1.2.1"))
        self.refused(lambda r: r.update(repository="x/r"))
        self.refused(lambda r: r.update(schema="crosspoint-acceptance/2"))
        self.refused(lambda r: r.update(approvedBy="claude"))
        self.refused(lambda r: r.update(approvedOn="tomorrow"))
        self.refused(lambda r: r.update(email="someone@example.com"))
        self.refused(lambda r: r.pop("fixtures"))

    def test_fixtures_are_hash_bound_issues_of_the_right_schema(self):
        fx = "fixtures"
        self.refused(lambda r: r[fx]["R"].update(schemaVersion=3))
        self.refused(lambda r: r[fx]["Q1"].update(schemaVersion=2))
        self.refused(lambda r: r[fx]["Q2"].update(schemaVersion=True))
        self.refused(lambda r: r[fx]["Q1"].update(sha256="2" * 63))
        self.refused(lambda r: r[fx]["Q2"].update(sha256=r[fx]["Q1"]["sha256"]))
        self.refused(lambda r: r[fx]["Q2"].update(issueId=r[fx]["Q1"]["issueId"]))
        self.refused(lambda r: r[fx]["Q1"].update(items=6, pages=8))
        self.refused(lambda r: r[fx]["Q1"].update(pages=7))
        self.refused(lambda r: r[fx]["Q1"].update(coverRows=4))
        self.refused(lambda r: r[fx]["R"].update(coverRows=3))
        self.refused(lambda r: r[fx].pop("Q2"))
        self.refused(lambda r: r[fx].update(generator="main"))

    def test_duplicate_keys_and_non_json_are_refused(self):
        text = json.dumps(self.record)
        self.refused(text=text.replace('"approvedBy": "owner"', '"approvedBy": "owner", "approvedBy": "owner"'))
        self.refused(text=text.replace('"install-bytes": "PASS"', '"install-bytes": "PASS", "install-bytes": "PASS"', 1))
        self.refused(text=text.replace('"candidateRun": 42', '"candidateRun": NaN'))
        self.refused(text=text[:-1])
        self.refused(text="")

    def test_the_schema_file_matches_the_release(self):
        schema = c.load_acceptance_schema()
        self.assertEqual(schema["packageOrder"], c.packages())
        self.assertEqual(schema["dogearFlagships"], ["x4-gd", "x4pro-gpd"])
        for name in schema["dogearFlagships"]:
            checks = set(schema["packages"][name]["checks"])
            for required in schema["quietChecks"] + schema["regularCacheChecks"] + [
                    "dogear-e1-no-issue-yet", "dogear-e2-no-issue-on-device"]:
                self.assertIn(required, checks, name)
            self.assertNotIn(c.E3_CHECK, checks, name)  # host-covered, never physical
        definition = schema["hostCovered"][c.E3_CHECK]
        self.assertEqual(definition["policy"], "Physical E3 is not a release gate. Host E3 coverage is mandatory.")
        self.assertEqual(definition["tests"], list(c.E3_HOST_TESTS))
        self.assertEqual(definition["evidence"], c.E3_EVIDENCE)
        for required in ("DogearState.TheCasesThatMatter", "DogearState.EveryCombinationFollowsTheThreeRules",
                         "DogearState.StatusCopyIsExactlyTheApprovedCopy", "DogearTls.SelfSignedCertificateIsRefused",
                         "DogearTlsRead.AReadFailureAfterTheExactBodyIsNotNothingPublished",
                         "DogearBootstrap.OnlyExplicitFramingCounts"):
            self.assertIn(required, definition["tests"])
        for name in ("x4pro-g", "x4pro-gd", "x4pro-p", "x4pro-pd"):
            self.assertEqual(schema["packages"][name]["level"], "boot")
        for name in ("x4-g", "x4pro-gp"):
            self.assertEqual(schema["packages"][name]["level"], "full")


class E3HostEvidenceTests(Base):
    """Physical E3 is not a release gate; host E3 coverage is mandatory, on the
    exact candidate, and every reader fails closed without it."""

    def replace_evidence(self, data: bytes) -> None:
        """Put `data` in the assembled candidate as its evidence, with SHA256SUMS.txt
        and provenance rewritten to match: only the evidence rule can refuse it."""
        (self.out / c.E3_EVIDENCE).write_bytes(data)
        files = {p.name: p.read_bytes() for p in self.out.iterdir() if p.name not in (c.SUMS, c.PROVENANCE)}
        sums = c.write_sums(files, self.out / c.SUMS)
        prov = json.loads((self.out / c.PROVENANCE).read_text())
        prov["files"] = {n: {"sha256": c.sha256(d), "size": len(d)} for n, d in sorted(files.items())}
        prov["sumsSha256"] = c.sha256(sums)
        (self.out / c.PROVENANCE).write_text(json.dumps(prov, indent=2, sort_keys=True) + "\n")

    def schema_file(self, mutate) -> Path:
        schema = json.loads(c.ACCEPTANCE_SCHEMA.read_text())
        mutate(schema)
        path = self.root / "acceptance-v1.json"
        path.write_text(json.dumps(schema))
        return path

    def test_the_evidence_is_every_required_test_pass_and_deterministic(self):
        data = evidence()
        self.assertEqual(data, evidence())  # no times, no ordering: the same run gives the same bytes
        ev = json.loads(data)
        self.assertEqual(ev["tests"], {t: "PASS" for t in c.E3_HOST_TESTS})
        self.assertEqual((ev["skipped"], ev["commit"], ev["candidateRun"]), ([], "b" * 40, 42))
        self.assertEqual(ev["definitionSha256"], c.e3_definition_sha256(c.load_acceptance_schema()))
        self.assertEqual(len(c.E3_HOST_TESTS), 28)

    def test_a_missing_failed_skipped_or_repeated_test_has_no_evidence(self):
        t = "DogearState.StatusCopyIsExactlyTheApprovedCopy"
        for case, why in (({t: "missing"}, "missing"), ({t: "fail"}, "failed"), ({t: "skip"}, "skipped"),
                          ({"DogearTls.ExpiredCertificateIsRefused": "skip"}, "skipped"),
                          ({"DogearBootstrap.OnlyStatus503": "fail"}, "failed")):
            with self.subTest(case=case), self.assertRaisesRegex(c.Problem, why):
                evidence(report=junit(case))
        repeated = junit(extra=f'<testcase name="{t}" classname="{t}" status="run"/>')
        with self.assertRaises(c.Problem):
            evidence(report=repeated)
        with self.assertRaises(c.Problem):
            evidence(report=b"not xml")
        # An unrequired test skipped elsewhere does not matter (DogearTlsLive is network-only).
        self.assertTrue(evidence(report=junit(extra='<testcase name="DogearTlsLive.X" status="notrun"><skipped/>'
                                                    '</testcase>')))

    def test_the_cli_writes_it_and_never_overwrites(self):
        report = self.root / "ctest-junit.xml"
        report.write_bytes(junit())
        out = self.root / "e3" / c.E3_EVIDENCE
        out.parent.mkdir()
        args = ("e3-evidence", "--junit", report, "--repository", "o/r", "--commit", "b" * 40, "--run-id", "42",
                "--out", out)
        self.assertEqual(run(*args)[0], 0)
        self.assertEqual(out.read_bytes(), evidence())
        self.assertNotEqual(run(*args)[0], 0)  # exists already
        report.write_bytes(junit({"DogearTlsRead.ReadResultWaitsOnlyWhileTheTransportIsOpen": "skip"}))
        out.unlink()
        self.assertNotEqual(run(*args)[0], 0)
        self.assertFalse(out.exists())

    def test_assemble_refuses_evidence_from_another_commit_run_or_definition(self):
        cases = {
            "another commit": evidence(commit="c" * 40),
            "another run": evidence(run=43),
            "another repository": evidence(repository="x/r"),
            "not canonical": json.dumps(json.loads(evidence()), indent=2).encode(),
            "a test not PASS": c.canonical(dict(json.loads(evidence()), tests=dict(
                json.loads(evidence())["tests"], **{c.E3_HOST_TESTS[0]: "FAIL"}))),
            "a skipped test recorded": c.canonical(dict(json.loads(evidence()), skipped=[c.E3_HOST_TESTS[0]])),
            "another definition": c.canonical(dict(json.loads(evidence()), definitionSha256="0" * 64)),
            "a test missing": c.canonical(dict(json.loads(evidence()), tests={
                k: v for k, v in json.loads(evidence())["tests"].items() if k != c.E3_HOST_TESTS[-1]})),
        }
        for name, data in cases.items():
            with self.subTest(case=name):
                if self.out.exists():
                    shutil.rmtree(self.out)
                    shutil.rmtree(self.support)
                self.e3.write_bytes(data)
                self.assertNotEqual(self.assemble()[0], 0)
                self.assertFalse(self.out.exists())
        self.e3.unlink()  # no evidence at all
        self.assertNotEqual(self.assemble()[0], 0)
        self.assertFalse(self.out.exists())

    def test_verify_dir_refuses_a_candidate_without_its_evidence(self):
        self.assertEqual(self.assemble()[0], 0)
        self.assertEqual(run("verify-dir", self.out)[0], 0)
        good = (self.out / c.E3_EVIDENCE).read_bytes()
        failing = c.canonical(dict(json.loads(good), tests=dict(json.loads(good)["tests"],
                                                                **{c.E3_HOST_TESTS[1]: "FAIL"})))
        for name, data in (("a failed test, sums rewritten", failing), ("another run's", evidence(run=43)),
                           ("another commit's", evidence(commit="c" * 40))):
            with self.subTest(case=name):
                self.replace_evidence(data)
                self.assertNotEqual(run("verify-dir", self.out)[0], 0)
        self.replace_evidence(good)
        self.assertEqual(run("verify-dir", self.out)[0], 0)
        (self.out / c.E3_EVIDENCE).unlink()
        self.assertNotEqual(run("verify-dir", self.out)[0], 0)

    def test_the_schema_requires_hostCovered_exactly(self):
        e3 = c.E3_CHECK
        mutations = {
            "hostCovered deleted": lambda s: s.pop("hostCovered"),
            "E3 entry deleted": lambda s: s["hostCovered"].pop(e3),
            "a required test removed": lambda s: s["hostCovered"][e3]["tests"].pop(),
            "a test added": lambda s: s["hostCovered"][e3]["tests"].append("DogearPages.NavigationIsCircular"),
            "tests reordered": lambda s: s["hostCovered"][e3]["tests"].reverse(),
            "the policy reworded": lambda s: s["hostCovered"][e3].update(policy="Host E3 coverage is optional."),
            "another evidence file": lambda s: s["hostCovered"][e3].update(evidence="e3.json"),
            "physical E3 back on a flagship": lambda s: (s["checks"].update({e3: "physical"}),
                                                          s["packages"]["x4-gd"]["checks"].append(e3)),
        }
        for name, mutate in mutations.items():
            with self.subTest(case=name), self.assertRaises(c.Problem):
                c.load_acceptance_schema(self.schema_file(mutate))

    def test_the_record_requires_the_candidates_own_evidence(self):
        self.assertEqual(self.assemble()[0], 0)
        record = acceptance_record(self.out)
        path = self.root / "ACCEPTANCE.json"

        def refused(mutate):
            r = copy.deepcopy(record)
            mutate(r)
            path.write_text(json.dumps(r))
            with self.assertRaises(c.Problem):
                c.acceptance_digest(path, self.out)

        path.write_text(json.dumps(record))
        self.assertTrue(c.acceptance_digest(path, self.out))  # the complete record passes
        e3 = c.E3_CHECK
        refused(lambda r: r.pop("hostCovered"))                                       # deleted
        refused(lambda r: r.update(hostCovered={}))
        refused(lambda r: r["hostCovered"][e3]["tests"].update({c.E3_HOST_TESTS[3]: "FAIL"}))
        refused(lambda r: r["hostCovered"][e3]["tests"].pop(c.E3_HOST_TESTS[3]))     # a test missing
        refused(lambda r: r["hostCovered"][e3].update(skipped=[c.E3_HOST_TESTS[3]]))
        refused(lambda r: r["hostCovered"][e3].update(commit="c" * 40))            # another commit
        refused(lambda r: r["hostCovered"][e3].update(candidateRun=43))             # another run
        refused(lambda r: r["hostCovered"][e3].update(definitionSha256="0" * 64))
        # Consistent evidence for another run, not the candidate's file: refused by its digest.
        refused(lambda r: r.update(candidateRun=43) or r["hostCovered"][e3].update(candidateRun=43))
        # The candidate's own file is bad (its SUMS rewritten to list it): a record
        # carrying that same bad evidence is refused by the evidence rule itself...
        good = (self.out / c.E3_EVIDENCE).read_bytes()
        failing = c.canonical(dict(json.loads(good), tests=dict(json.loads(good)["tests"],
                                                                **{c.E3_HOST_TESTS[2]: "FAIL"})))
        self.replace_evidence(failing)
        bad = acceptance_record(self.out)
        self.assertEqual(bad["hostCovered"][e3]["tests"][c.E3_HOST_TESTS[2]], "FAIL")
        path.write_text(json.dumps(bad))
        with self.assertRaisesRegex(c.Problem, "not PASS"):
            c.acceptance_digest(path, self.out)
        # ... and a record carrying valid evidence is refused because it is not that file.
        valid = acceptance_record(self.out)
        valid["hostCovered"][e3] = json.loads(good)
        path.write_text(json.dumps(valid))
        with self.assertRaisesRegex(c.Problem, "is not the candidate's"):
            c.acceptance_digest(path, self.out)
        self.replace_evidence(good)
        record = acceptance_record(self.out)
        # Physical E3 substituted for the host evidence: refused both ways.
        refused(lambda r: r.pop("hostCovered") and r["packages"][1]["checks"].update({e3: "PASS"}))
        refused(lambda r: r["packages"][7]["checks"].update({e3: "PASS"}))


class AssembleTests(Base):
    def test_assembles_exactly_the_release(self):
        code, out, err = self.assemble()
        self.assertEqual(code, 0, err)
        self.assertEqual(sorted(p.name for p in self.out.iterdir()), sorted(c.release_files() + [c.SUMS, c.PROVENANCE]))
        # 16 firmware files + e3-host-evidence.json + post-launch-host-evidence.json
        self.assertEqual(len(c.release_files()), 18)
        self.assertEqual(len([n for n in c.release_files() if n.endswith(".bin")]), 16)
        self.assertEqual((self.out / c.E3_EVIDENCE).read_bytes(), self.e3.read_bytes())
        self.assertEqual((self.out / c.POST_LAUNCH_EVIDENCE).read_bytes(), self.post_launch.read_bytes())
        self.assertEqual(c.packages(), ["x4-g", "x4-gd", "x4pro-g", "x4pro-gd", "x4pro-p", "x4pro-pd", "x4pro-gp",
                                        "x4pro-gpd"])
        self.assertEqual((self.out / "firmware.bin").read_bytes(), (self.out / "firmware-x4-g.bin").read_bytes())
        self.assertEqual((self.out / "firmware-x4pro.bin").read_bytes(),
                         (self.out / "firmware-x4pro-gp.bin").read_bytes())
        prov = json.loads((self.out / c.PROVENANCE).read_text())
        self.assertEqual((prov["commit"], prov["runId"], prov["sumsSha256"]), ("b" * 40, 42, self.sums_sha()))
        for env in ("x4pro-goto-gh_release", "x4pro-goto-dogear-gh_release", "x4pro-onpoint-gh_release",
                    "x4pro-onpoint-dogear-gh_release", "x4pro-gh_release", "x4pro-all-gh_release"):
            self.assertEqual(sorted(p.name for p in (self.support / env).iterdir()), sorted(c.FLASH_PIECES))
        self.assertFalse((self.support / "gh_release").exists())
        self.assertFalse((self.support / "x4-goto-dogear-gh_release").exists())
        self.assertEqual(run("verify-dir", self.out, "--support", self.support, "--sums-sha256", self.sums_sha(),
                             "--commit", "b" * 40, "--run-id", "42")[0], 0)

    def test_refuses_a_wrong_version_string(self):
        (self.builds / "build-x4pro-gh_release" / "version.txt").write_text(f"{DISPLAY}-G\n")
        code, _, err = self.assemble()
        self.assertEqual(code, 1)
        self.assertIn("not 1.6.5-A1.2.0-GP", err)

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


ALL = "x4-g x4-gd x4pro-g x4pro-gd x4pro-p x4pro-pd x4pro-gp x4pro-gpd"


def tag_message(run_id=123456, commit="b" * 40, sums="a" * 64, accepted=ALL, acceptance="f" * 64, title="v1.2.0"):
    return (f"{title}\n\ncandidate-run: {run_id}\ncandidate-commit: {commit}\ncandidate-sums-sha256: {sums}\n"
            f"physically-accepted: {accepted}\nacceptance-sha256: {acceptance}\n")


class TagTests(unittest.TestCase):
    GOOD = tag_message()

    def test_reads_the_fields(self):
        f = c.parse_tag_message(self.GOOD)
        self.assertEqual((f["candidate-run"], f["candidate-commit"], f["candidate-sums-sha256"], f["acceptance-sha256"]),
                         ("123456", "b" * 40, "a" * 64, "f" * 64))

    def test_every_package_must_be_physically_accepted_in_order(self):
        for accepted in ("x4-g x4pro-gp", "x4-g x4pro-g x4pro-p x4pro-gp", ALL.replace(" x4pro-pd", ""),
                         ALL + " x4pro-d", ALL + " x4pro-gp", "x4-gd x4-g x4pro-g x4pro-gd x4pro-p x4pro-pd x4pro-gp x4pro-gpd",
                         ALL.replace("x4pro-gpd", "x4pro-dgp"), ""):
            with self.assertRaises(c.Problem, msg=accepted):
                c.parse_tag_message(tag_message(accepted=accepted))

    def test_missing_repeated_unknown_or_malformed_fields_fail(self):
        for text in (self.GOOD.replace("candidate-run: 123456\n", ""),
                     self.GOOD.replace("candidate-commit: " + "b" * 40 + "\n", ""),
                     self.GOOD.replace("acceptance-sha256: " + "f" * 64 + "\n", ""),
                     self.GOOD + "candidate-run: 7\n",
                     self.GOOD + "acceptance-sha256: " + "f" * 64 + "\n",
                     self.GOOD + "physically-tested: x4-g\n",
                     self.GOOD + "notes: also fine\n",
                     tag_message(run_id="12ab"),
                     tag_message(commit="B" * 40), tag_message(commit="b" * 39), tag_message(commit="b" * 64),
                     tag_message(sums="A" * 64), tag_message(sums="a" * 63),
                     tag_message(acceptance="F" * 64), tag_message(acceptance="f" * 63), tag_message(acceptance="")):
            with self.assertRaises(c.Problem, msg=text):
                c.parse_tag_message(text)

    def test_free_text_lines_are_not_fields(self):
        c.parse_tag_message(tag_message(title="CrossPoint Apps 1.2.0, physically accepted 2026-10-21."))

    def test_parse_tag_requires_the_candidate_commit_to_be_the_tagged_commit(self):
        with tempfile.TemporaryDirectory() as d:
            msg = Path(d) / "m.txt"
            msg.write_text(self.GOOD)
            code, out, err = run("parse-tag", msg, "--commit", "b" * 40)
            self.assertEqual(code, 0, err)
            self.assertIn("candidate_commit=" + "b" * 40, out)
            self.assertIn("acceptance_sha256=" + "f" * 64, out)
            code, _, err = run("parse-tag", msg, "--commit", "c" * 40)
            self.assertEqual(code, 1)
            self.assertIn("points at", err)


RUN = {
    "id": 9, "run_attempt": 1, "path": ".github/workflows/release_candidate.yml", "workflow_id": 77,
    "repository": {"full_name": "o/r"}, "head_repository": {"full_name": "o/r"}, "event": "workflow_dispatch",
    "status": "completed", "conclusion": "success", "head_branch": "release/v1.2.0-dogear", "head_sha": "b" * 40,
}


class RunTests(unittest.TestCase):
    def check(self, run_json):
        c.check_run_json(run_json, repository="o/r", workflow_id=77, commit="b" * 40, branch_prefix="release/v1.2.0")

    def test_the_candidate_run_passes_with_or_without_a_ref_suffix(self):
        self.check(RUN)
        self.check(dict(RUN, path=".github/workflows/release_candidate.yml@refs/heads/release/v1.2.0-dogear"))

    def test_anything_else_fails(self):
        for change in ({"path": ".github/workflows/release.yml"},
                       {"path": ".github/workflows/release_candidate.yml.evil@x"},
                       {"path": "evil/.github/workflows/release_candidate.yml"},
                       {"workflow_id": 78},
                       {"repository": {"full_name": "x/r"}},
                       {"head_repository": {"full_name": "x/r"}},
                       {"event": "push"}, {"event": "pull_request"},
                       {"conclusion": "failure"}, {"status": "in_progress", "conclusion": None},
                       {"head_branch": "develop"}, {"head_branch": "release/v1.1.1-clean"},
                       {"head_sha": "c" * 40}):
            with self.assertRaises(c.Problem, msg=str(change)):
                self.check(dict(RUN, **change))


class ReleaseTests(Base):
    def setUp(self):
        super().setUp()
        self.assertEqual(self.assemble()[0], 0)
        self.sums = (self.out / c.SUMS).read_bytes()
        self.release = {
            "tag_name": "v1.2.0", "draft": True, "prerelease": False, "immutable": False,
            "assets": [{"name": n, "state": "uploaded", "digest": "sha256:" + hashlib.sha256(
                (self.out / n).read_bytes()).hexdigest()} for n in c.release_files() + [c.SUMS]],
        }

    def check(self, release, state="draft", immutable=False):
        c.check_release_json(release, self.sums, tag="v1.2.0", state=state, require_immutable=immutable)

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
                                          (dict(self.release, tag_name="v1.1.1"), "draft", False),
                                          (dict(self.release, prerelease=True), "draft", False),
                                          (dict(self.release, draft=False), "published", True)):
            with self.assertRaises(c.Problem):
                self.check(release, state, immutable)

    def test_a_different_sums_file_fails(self):
        with self.assertRaises(c.Problem):
            c.check_release_json(self.release, self.sums + b"\n", tag="v1.2.0", state="draft", require_immutable=False)


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
    TAG, COMMIT, RUN_ID = "v1.2.0", "b" * 40, 42

    def setUp(self):
        super().setUp()
        self.assertEqual(self.assemble()[0], 0)
        sums = self.sums_sha()
        draft = {"id": 7, "tag_name": self.TAG, "draft": True, "prerelease": False, "immutable": False,
                 "assets": [{"name": n, "state": "uploaded", "digest": "sha256:" + hashlib.sha256(
                     (self.out / n).read_bytes()).hexdigest()} for n in c.release_files() + [c.SUMS]]}
        self.acceptance = self.root / "ACCEPTANCE.json"
        self.acceptance.write_text(json.dumps(acceptance_record(self.out), indent=2))
        self.acceptance_sha = c.acceptance_digest(self.acceptance, self.out)
        message = tag_message(run_id=self.RUN_ID, commit=self.COMMIT, sums=sums, acceptance=self.acceptance_sha,
                              title=self.TAG)
        self.gets = {
            "repos/o/r/immutable-releases": {"enabled": True, "enforced_by_owner": False},
            "repos/o/r/releases?per_page=100": [draft, {"id": 6, "tag_name": "v1.1.1", "draft": False}],
            f"repos/o/r/git/ref/tags/{self.TAG}": {"object": {"type": "tag", "sha": "t" * 40}},
            f"repos/o/r/git/tags/{'t' * 40}": {"object": {"type": "commit", "sha": self.COMMIT}, "message": message},
            "repos/o/r/actions/workflows/release_candidate.yml": {"id": 77},
            f"repos/o/r/actions/runs/{self.RUN_ID}": dict(RUN, id=self.RUN_ID, head_sha=self.COMMIT),
        }

    def publish(self, gh, publish=True, **override):
        args = dict(repository="o/r", tag=self.TAG, commit=self.COMMIT, run_id=str(self.RUN_ID),
                    candidate=str(self.out), sums_sha256=self.sums_sha(), acceptance=str(self.acceptance),
                    publish=publish)
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
                               "--acceptance", self.acceptance, "--publish")
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
        self.assertIn("published v1.2.0 as latest: immutable", out)

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
            (tag_obj, dict(self.gets[tag_obj], message=self.gets[tag_obj]["message"].replace(self.acceptance_sha, "e" * 64))),
            (tag_obj, dict(self.gets[tag_obj], message=self.gets[tag_obj]["message"].replace(
                "candidate-commit: " + self.COMMIT, "candidate-commit: " + "c" * 40))),
            (run, dict(self.gets[run], head_sha="c" * 40)),
            (run, dict(self.gets[run], path=".github/workflows/release.yml")),
            (run, dict(self.gets[run], head_branch="release/v1.1.1-clean")),
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

    def test_an_edited_missing_or_unbound_acceptance_record_refuses(self):
        record = json.loads(self.acceptance.read_text())
        record["packages"][7]["checks"]["dogear-v3-q2-pages"] = "FAIL"
        self.acceptance.write_text(json.dumps(record))
        self.assertIn("not PASS", self.refused())
        record["packages"][7]["checks"]["dogear-v3-q2-pages"] = "PASS"
        record["approvedOn"] = "2026-10-22"  # valid, but not the record the tag names
        self.acceptance.write_text(json.dumps(record))
        self.assertIn("different acceptance record", self.refused())
        self.acceptance.unlink()
        gh = FakeGitHub(self.gets)
        with self.assertRaises(FileNotFoundError):
            self.publish(gh)
        self.assertEqual(gh.patches, [])
        self.assert_not_published(FakeGitHub(self.gets), 1)  # the command exits 1, before any request

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
            gh.gets["repos/o/r/releases/latest"] = {"id": 6, "tag_name": "v1.1.1"}
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
                            ("repos/o/r/releases/7", {"tag_name": "v1.2.0", "draft": False, "prerelease": False,
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


GH = "/opt/test/bin/gh"  # where the fake PATH lookup finds gh


class popen_as:  # noqa: N801 -- a context manager used like a function
    """Fake Popen, plus the PATH lookup (`which`: path or None) and the
    executable check (`access`: bool) that run before it."""

    def __init__(self, behaviour):
        self.behaviour, self.record = behaviour, []

    def __enter__(self):
        self.saved = (c.subprocess.Popen, c.shutil.which, c.os.access)
        c.subprocess.Popen = fake_popen(self.behaviour, self.record)
        c.shutil.which = lambda name: self.behaviour.get("which", GH) if name == "gh" else None
        c.os.access = lambda path, mode: self.behaviour.get("access", True)
        return self.record

    def __exit__(self, *exc):
        c.subprocess.Popen, c.shutil.which, c.os.access = self.saved
        return False


# Provably never executed: exit 3.
NO_EXEC = ({"which": None},                                                     # not on PATH
           {"access": False},                                                   # not executable
           {"start": FileNotFoundError(errno.ENOENT, "No such file or directory", GH)},
           {"start": PermissionError(errno.EACCES, "Permission denied", GH)})
# Popen failures that do not prove gh never ran: exit 2.
AMBIGUOUS_STARTS = (FileNotFoundError(errno.ENOENT, "No such file or directory", "/some/cwd"),  # post-fork chdir
                    PermissionError(errno.EACCES, "Permission denied", "/other/gh"),
                    FileNotFoundError("gh"), PermissionError(errno.EACCES, "Permission denied"),
                    OSError(errno.ENOEXEC, "Exec format error", GH), OSError(errno.EMFILE, "Too many open files"),
                    OSError(errno.EAGAIN, "Resource temporarily unavailable"), OSError(errno.ENOMEM, "No memory"),
                    OSError(), BrokenPipeError(), ValueError("bad argv"), subprocess.SubprocessError("setup"),
                    KeyboardInterrupt())
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
        self.assertEqual(record, [("start", [GH, "api", "-X", "PATCH", "repos/o/r/releases/7", "-F", "draft=false",
                                             "-f", "make_latest=true"]), ("communicate", c.PUBLISH_TIMEOUT_S)])

    def test_gh_that_provably_never_executed_is_not_applied(self):
        for behaviour in NO_EXEC:
            with self.subTest(behaviour=repr(behaviour)):
                err, record = self.call(behaviour)
                self.assertIsInstance(err, c.PublishNotApplied)
                self.assertLessEqual(len(record), 1)  # at most the failed start
        self.assertEqual(self.call({"which": None})[1], [])   # nothing is started at all
        self.assertEqual(self.call({"access": False})[1], [])

    def test_an_ambiguous_start_failure_is_uncertain(self):
        for failure in AMBIGUOUS_STARTS:
            with self.subTest(failure=repr(failure)):
                err, _ = self.call({"start": failure})
                self.assertIsInstance(err, c.PublishUncertain)
                self.assertNotIsInstance(err, c.PublishNotApplied)

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


class ExecFailureSemanticsTests(unittest.TestCase):
    """exec_failed against real Popen errors on this platform."""

    def popen_error(self, argv, **kwargs):
        try:
            subprocess.Popen(argv, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, **kwargs).wait()
        except OSError as err:
            return err
        self.fail("Popen did not raise")

    def test_a_missing_or_non_executable_file_is_an_exec_failure(self):
        with tempfile.TemporaryDirectory() as d:
            missing = os.path.join(d, "gh")
            self.assertTrue(c.exec_failed(self.popen_error([missing]), missing))
            with open(missing, "w") as f:
                f.write("#!/bin/sh\nexit 0\n")
            os.chmod(missing, 0o644)
            self.assertTrue(c.exec_failed(self.popen_error([missing]), missing))

    def test_a_post_fork_setup_failure_is_not(self):
        true = shutil.which("true")
        with tempfile.TemporaryDirectory() as d:
            err = self.popen_error([true], cwd=os.path.join(d, "absent"))
        self.assertEqual(err.errno, errno.ENOENT)  # the same errno as a missing executable...
        self.assertFalse(c.exec_failed(err, true))  # ...but it does not name the executable



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

    def test_gh_missing_or_not_executable_exits_3(self):
        for behaviour in NO_EXEC:
            with self.subTest(behaviour=repr(behaviour)):
                code, err, _ = self.exit_for(behaviour)
                self.assertEqual(code, 3, err)
                self.assertIn("not published", err)
                self.assertNotIn("MAY BE PUBLISHED", err)

    def test_an_ambiguous_start_failure_exits_2(self):
        for failure in AMBIGUOUS_STARTS:
            with self.subTest(failure=repr(failure)):
                code, err, _ = self.exit_for({"start": failure})
                self.assertEqual(code, 2, err)
                self.assertIn("MAY BE PUBLISHED", err)

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



# ---- post-launch maintenance acceptance (docs/RELEASING.md 2a) ---------------

CI_ONLY = [p for p in c.packages() if p not in c.FLAGSHIPS]


def maintenance_record(candidate: Path) -> dict:
    """A complete, valid crosspoint-acceptance-maintenance/1 record of the candidate."""
    prov = json.loads((candidate / c.PROVENANCE).read_text())
    sums = c.read_sums((candidate / c.SUMS).read_bytes())
    m = c.load_maintenance_schema()
    entries = []
    for env, _pkg, stem, has_factory, _legacy in c.RELEASE:
        name = c.package_name(stem)
        spec = m["packages"][name]
        entry = {"package": name, "env": env, "board": spec["board"], "file": f"{stem}.bin",
                 "sha256": sums[f"{stem}.bin"], "verification": spec["verification"]}
        if spec["verification"] == "physical":
            entry.update(install=spec["install"], date="2026-10-22",
                         checks={check: "PASS" for check in spec["checks"]})
        if has_factory:
            entry["pieces"] = {k: prov["flashSupport"][env][f"{k}.bin"] for k in c.PIECE_KEYS}
        entries.append(entry)
    return {"schema": c.MAINTENANCE, "policy": c.MAINTENANCE_POLICY, "authorization": c.MAINTENANCE_AUTHORIZATION,
            "repository": prov["repository"], "tag": "v1.2.0", "commit": prov["commit"],
            "candidateRun": prov["runId"], "sumsSha256": prov["sumsSha256"], "fixtures": copy.deepcopy(FIXTURES),
            "packages": entries,
            "hostCovered": {c.E3_CHECK: json.loads((candidate / c.E3_EVIDENCE).read_bytes()),
                            c.POST_LAUNCH: json.loads((candidate / c.POST_LAUNCH_EVIDENCE).read_bytes())},
            "approvedBy": "owner", "approvedOn": "2026-10-22"}


def entry(record: dict, name: str) -> dict:
    return next(e for e in record["packages"] if e["package"] == name)


class MaintenanceAcceptanceTests(Base):
    def setUp(self):
        super().setUp()
        self.assertEqual(self.assemble()[0], 0)
        self.record = maintenance_record(self.out)
        self.path = self.root / "ACCEPTANCE.json"

    def write(self, record=None, text=None):
        self.path.write_text(text if text is not None else json.dumps(record or self.record, indent=2))
        return self.path

    def digest(self, record=None, text=None):
        return c.acceptance_digest(self.write(record, text), self.out)

    def refused(self, mutate=None, text=None):
        record = copy.deepcopy(self.record)
        if mutate:
            mutate(record)
        with self.assertRaises(c.Problem):
            self.digest(record, text)

    def test_the_complete_maintenance_record_passes_and_names_its_policy(self):
        digest = self.digest()
        self.assertEqual(digest, hashlib.sha256(c.canonical(self.record)).hexdigest())
        self.assertEqual(c.checked_acceptance(self.write(), self.out), (digest, c.MAINTENANCE_POLICY))
        code, out, err = run("acceptance", "--record", self.write(), "--candidate-dir", self.out,
                             "--flash-support", self.support)
        self.assertEqual(code, 0, err)
        self.assertIn(f"physically accepted {' '.join(c.FLAGSHIPS)}", out)
        self.assertIn(f"CI-verified only {' '.join(CI_ONLY)}", out)
        self.assertIn(f"acceptance-policy: {c.MAINTENANCE_POLICY}", out)
        self.assertIn(f"acceptance-sha256: {digest}", out)

    def test_fixtures_duplicate_keys_and_non_json_are_refused(self):
        self.refused(lambda r: r["fixtures"].pop("Q1"))
        self.refused(lambda r: r["fixtures"]["R"].update(schemaVersion=3))
        self.refused(lambda r: r["fixtures"]["Q2"].update(sha256=r["fixtures"]["Q1"]["sha256"]))
        text = json.dumps(self.record)
        self.refused(text=text.replace('"approvedBy": "owner"', '"approvedBy": "owner", "approvedBy": "owner"'))
        self.refused(text=text[:-1])
        self.refused(text="NaN")

    def test_all_eight_packages_once_each_in_order(self):
        for drop in c.packages():
            self.refused(lambda r, d=drop: r["packages"].remove(entry(r, d)))
        self.refused(lambda r: r["packages"].reverse())
        self.refused(lambda r: r["packages"].append(copy.deepcopy(r["packages"][0])))
        self.refused(lambda r: r.update(packages=[e for e in r["packages"] if e["package"] in c.FLAGSHIPS]))

    def test_ci_packages_claim_no_physical_acceptance(self):
        for name in CI_ONLY:
            for key, value in (("checks", {"boots": "PASS"}), ("checks", {}), ("install", "microSD"),
                               ("date", "2026-10-22"), ("level", "boot")):
                self.refused(lambda r, n=name, k=key, v=value: entry(r, n).update({k: copy.deepcopy(v)}))
            self.refused(lambda r, n=name: entry(r, n).update(verification="physical"))
            self.refused(lambda r, n=name: entry(r, n).pop("verification"))
        with self.assertRaisesRegex(c.Problem, "CI-verified only"):
            record = copy.deepcopy(self.record)
            entry(record, "x4-g")["checks"] = {"boots": "PASS"}
            self.digest(record)

    def test_flagships_need_exactly_their_maintenance_checks_each_pass(self):
        m = c.load_maintenance_schema()
        for name in c.FLAGSHIPS:
            for check in m["packages"][name]["checks"]:
                self.refused(lambda r, n=name, k=check: entry(r, n)["checks"].pop(k))
                for bad in ("FAIL", "SKIP", "pass", "", None, True):
                    self.refused(lambda r, n=name, k=check, b=bad: entry(r, n)["checks"].update({k: b}))
            self.refused(lambda r, n=name: entry(r, n)["checks"].update({"photo-taken": "PASS"}))
            self.refused(lambda r, n=name: entry(r, n).update(verification="ci"))
            self.refused(lambda r, n=name: entry(r, n).pop("date"))
            self.refused(lambda r, n=name: entry(r, n).update(install="USB 4-region" if n == "x4-gd" else "microSD"))
        self.assertIn("dogear-no-writes-offline", m["packages"]["x4-gd"]["checks"])

    def test_replaced_and_host_checks_cannot_be_claimed_physically(self):
        for name in c.FLAGSHIPS:
            for check in (*c.REPLACED_503, c.NO_WRITES, c.E3_CHECK):
                with self.assertRaisesRegex(c.Problem, "cannot be physical checks", msg=f"{name} {check}"):
                    record = copy.deepcopy(self.record)
                    entry(record, name)["checks"][check] = "PASS"
                    self.digest(record)

            def substitute(r, n=name):  # the v1 no-writes PASS in place of the offline one
                checks = entry(r, n)["checks"]
                checks[c.NO_WRITES] = checks.pop(c.NO_WRITES_OFFLINE)
            self.refused(substitute)

    def test_the_record_is_bound_to_the_candidate(self):
        self.refused(lambda r: r.update(commit="c" * 40))
        self.refused(lambda r: r.update(candidateRun=43))
        self.refused(lambda r: r.update(candidateRun="42"))
        self.refused(lambda r: r.update(sumsSha256="0" * 64))
        self.refused(lambda r: r.update(repository="o/other"))
        self.refused(lambda r: r.update(tag="v1.2.1"))
        for name in c.packages():
            self.refused(lambda r, n=name: entry(r, n).update(sha256="0" * 64))
            self.refused(lambda r, n=name: entry(r, n).update(env="gh_release" if n != "x4-g" else "x4pro-gh_release"))
        for name in ("x4pro-g", "x4pro-gpd"):
            self.refused(lambda r, n=name: entry(r, n)["pieces"].update(bootloader="0" * 64))
            self.refused(lambda r, n=name: entry(r, n).pop("pieces"))

    def test_policy_authorization_and_approval_are_fixed(self):
        self.refused(lambda r: r.update(policy="full"))
        self.refused(lambda r: r.update(policy="post-launch-maintenance-lite"))
        self.refused(lambda r: r.update(authorization=c.MAINTENANCE_AUTHORIZATION.replace("x4pro-gp ", "")))
        self.refused(lambda r: r.pop("authorization"))
        self.refused(lambda r: r.update(approvedBy="claude"))
        self.refused(lambda r: r.update(approvedOn="2026-13-01"))
        self.refused(lambda r: r.update(schema="crosspoint-acceptance-maintenance/2"))

    def test_host_evidence_is_the_candidates_own(self):
        self.refused(lambda r: r["hostCovered"].pop(c.POST_LAUNCH))
        self.refused(lambda r: r["hostCovered"].pop(c.E3_CHECK))
        self.refused(lambda r: r["hostCovered"].update({c.POST_LAUNCH: r["hostCovered"][c.E3_CHECK]}))
        other = json.loads(post_launch(commit="c" * 40))
        self.refused(lambda r: r["hostCovered"].update({c.POST_LAUNCH: other}))
        self.refused(lambda r: r["hostCovered"].update({c.POST_LAUNCH: json.loads(post_launch(run=43))}))
        test = c.POST_LAUNCH_TESTS[0]
        self.refused(lambda r: r["hostCovered"][c.POST_LAUNCH]["tests"].update({test: "FAIL"}))
        self.refused(lambda r: r["hostCovered"][c.POST_LAUNCH]["tests"].pop(test))
        self.refused(lambda r: r["hostCovered"][c.POST_LAUNCH].update(skipped=[test]))
        self.refused(lambda r: r["hostCovered"][c.POST_LAUNCH].update(definitionSha256="0" * 64))
        self.refused(lambda r: r["hostCovered"][c.POST_LAUNCH].update(checks=[c.REPLACED_503[0]]))
        self.refused(lambda r: r["hostCovered"].update({c.REPLACED_503[0]: r["hostCovered"][c.POST_LAUNCH]}))

    def test_a_full_record_is_still_checked_under_crosspoint_acceptance_1(self):
        full = acceptance_record(self.out)
        self.assertEqual(c.checked_acceptance(self.write(full), self.out)[1], c.FULL_POLICY)
        # Under v1, a record of only the flagships, or with CI-only entries, is refused as before.
        flagships_only = copy.deepcopy(full)
        flagships_only["packages"] = [e for e in full["packages"] if e["package"] in c.FLAGSHIPS]
        self.refused(text=json.dumps(flagships_only))
        mixed = copy.deepcopy(full)
        mixed["packages"] = copy.deepcopy(self.record["packages"])
        self.refused(text=json.dumps(mixed))
        # And a maintenance record cannot wear the v1 schema name, nor an unknown one.
        self.refused(lambda r: r.update(schema="crosspoint-acceptance/1"))
        self.refused(lambda r: r.update(schema="crosspoint-acceptance/2"))


class PostLaunchEvidenceTests(Base):
    def replace_post_launch(self, data: bytes) -> None:
        """As E3HostEvidenceTests.replace_evidence: only the evidence rule can refuse it."""
        (self.out / c.POST_LAUNCH_EVIDENCE).write_bytes(data)
        files = {p.name: p.read_bytes() for p in self.out.iterdir() if p.name not in (c.SUMS, c.PROVENANCE)}
        sums = c.write_sums(files, self.out / c.SUMS)
        prov = json.loads((self.out / c.PROVENANCE).read_text())
        prov["files"] = {n: {"sha256": c.sha256(d), "size": len(d)} for n, d in sorted(files.items())}
        prov["sumsSha256"] = c.sha256(sums)
        (self.out / c.PROVENANCE).write_text(json.dumps(prov, indent=2, sort_keys=True) + "\n")

    def test_the_rule_itself_refuses_failures_skips_and_other_definitions(self):
        m, good = c.load_maintenance_schema(), json.loads(post_launch())
        bind = dict(repository="o/r", commit="b" * 40, run=42)
        c.check_post_launch_evidence(good, m, **bind)
        test = c.POST_LAUNCH_TESTS[0]
        for name, bad in (("skipped", dict(good, skipped=[test])),
                          ("failed", dict(good, tests=dict(good["tests"], **{test: "FAIL"}))),
                          ("missing test", dict(good, tests={t: "PASS" for t in c.POST_LAUNCH_TESTS[1:]})),
                          ("extra test", dict(good, tests=dict(good["tests"], **{"DogearX.Y": "PASS"}))),
                          ("other definition", dict(good, definitionSha256="0" * 64)),
                          ("other checks", dict(good, checks=list(reversed(c.REPLACED_503)))),
                          ("other schema", dict(good, schema=c.E3_EVIDENCE_SCHEMA)),
                          ("run as a string", dict(good, candidateRun="42"))):
            with self.subTest(case=name), self.assertRaises(c.Problem):
                c.check_post_launch_evidence(bad, m, **bind)

    def test_verify_dir_refuses_a_candidate_whose_post_launch_evidence_is_not_its_own(self):
        self.assertEqual(self.assemble()[0], 0)
        self.assertEqual(run("verify-dir", self.out)[0], 0)
        good = (self.out / c.POST_LAUNCH_EVIDENCE).read_bytes()
        loaded = json.loads(good)
        for name, data in (("a failed test, sums rewritten",
                            c.canonical(dict(loaded, tests=dict(loaded["tests"], **{c.POST_LAUNCH_TESTS[1]: "FAIL"})))),
                           ("a skipped test, sums rewritten", c.canonical(dict(loaded, skipped=[c.POST_LAUNCH_TESTS[1]]))),
                           ("another run's", post_launch(run=43)), ("another commit's", post_launch(commit="c" * 40))):
            with self.subTest(case=name):
                self.replace_post_launch(data)
                self.assertNotEqual(run("verify-dir", self.out)[0], 0)
        self.replace_post_launch(good)
        self.assertEqual(run("verify-dir", self.out)[0], 0)

    def test_the_evidence_is_every_required_test_pass_and_deterministic(self):
        data = post_launch()
        self.assertEqual(data, post_launch())
        ev = json.loads(data)
        self.assertEqual(ev["checks"], list(c.REPLACED_503))
        self.assertEqual(sorted(ev["tests"]), list(c.POST_LAUNCH_TESTS))
        self.assertEqual(set(ev["tests"].values()), {"PASS"})
        self.assertEqual(ev["skipped"], [])
        self.assertIn("DogearPostLaunch.NothingPublishedWithNoReadableIssueIsNoIssueYet", ev["tests"])
        self.assertIn("DogearPostLaunch.CachedRegularIssueWithNothingPublishedIsCachedNeverNoIssueYet", ev["tests"])

    def test_a_missing_failed_skipped_or_repeated_test_has_no_evidence(self):
        for test in c.POST_LAUNCH_TESTS:
            for state in ("missing", "fail", "skip"):
                with self.assertRaises(c.Problem, msg=f"{test} {state}"):
                    post_launch(report=junit({test: state}))
        repeated = f'<testcase name="{c.POST_LAUNCH_TESTS[0]}" status="run"/>'
        with self.assertRaises(c.Problem):
            post_launch(report=junit(extra=repeated))

    def test_the_cli_writes_it_and_never_overwrites(self):
        report = self.root / "junit.xml"
        report.write_bytes(junit())
        out = self.root / "pl.json"
        args = ("post-launch-evidence", "--junit", report, "--repository", "o/r", "--commit", "b" * 40,
                "--run-id", "42", "--out", out)
        self.assertEqual(run(*args)[0], 0)
        self.assertEqual(out.read_bytes(), post_launch())
        self.assertEqual(run(*args)[0], 1)
        report.write_bytes(junit({c.POST_LAUNCH_TESTS[-1]: "skip"}))
        out.unlink()
        self.assertEqual(run(*args)[0], 1)
        self.assertFalse(out.exists())

    def test_assemble_and_verify_dir_require_the_candidates_own_evidence(self):
        for bad in (post_launch(commit="c" * 40), post_launch(run=43), post_launch(repository="o/x"),
                    post_launch() + b"\n", evidence()):
            self.post_launch.write_bytes(bad)
            self.assertEqual(self.assemble()[0], 1)
            self.assertFalse(self.out.exists())
        self.post_launch.write_bytes(post_launch())
        self.assertEqual(self.assemble()[0], 0)
        (self.out / c.POST_LAUNCH_EVIDENCE).unlink()
        self.assertEqual(run("verify-dir", self.out)[0], 1)


class MaintenanceSchemaTests(unittest.TestCase):
    def loosened(self, mutate):
        schema = json.loads(c.MAINTENANCE_SCHEMA.read_text())
        mutate(schema)
        with tempfile.TemporaryDirectory() as d:
            path = Path(d) / "m.json"
            path.write_text(json.dumps(schema))
            with self.assertRaises(c.Problem):
                c.load_maintenance_schema(path)

    def test_the_schema_file_is_exactly_the_fixed_policy(self):
        m = c.load_maintenance_schema()
        v1 = c.load_acceptance_schema()
        self.assertEqual([p for p in m["packageOrder"] if m["packages"][p]["verification"] == "physical"],
                         list(c.FLAGSHIPS))
        for name in c.FLAGSHIPS:
            self.assertEqual(m["packages"][name]["checks"], c.maintenance_checks(v1["packages"][name]["checks"]))
            self.assertFalse(set(m["packages"][name]["checks"]) & {*c.REPLACED_503, c.NO_WRITES, c.E3_CHECK})
        self.assertEqual(m["hostCovered"][c.E3_CHECK], v1["hostCovered"][c.E3_CHECK])

    def test_any_loosening_is_refused(self):
        self.loosened(lambda s: s["packages"]["x4-g"].update(verification="physical"))
        self.loosened(lambda s: s["packages"]["x4pro-gpd"].update(verification="ci"))
        self.loosened(lambda s: s["packages"]["x4-gd"]["checks"].remove("book-reads"))
        self.loosened(lambda s: s["packages"]["x4-gd"]["checks"].append("dogear-e1-no-issue-yet"))
        self.loosened(lambda s: s["packages"]["x4-g"].update(checks=["boots"]))
        self.loosened(lambda s: s["checks"].update({"book-reads": "A book exists."}))
        self.loosened(lambda s: s["checks"].update({"dogear-no-writes-offline": s["checks"]["dogear-no-writes-offline"]})
                      or s["checks"].pop("goto-edition"))
        self.loosened(lambda s: s["hostCovered"]["dogear-v2-cache-503"]["tests"].pop())
        self.loosened(lambda s: s["hostCovered"]["dogear-e1-no-issue-yet"].update(evidence="e3-host-evidence.json"))
        self.loosened(lambda s: s["hostCovered"][c.E3_CHECK]["tests"].pop())
        self.loosened(lambda s: s["hostCovered"].pop("dogear-v2-cache-503"))
        self.loosened(lambda s: s.update(authorization="The owner authorizes it."))
        self.loosened(lambda s: s.update(policy="full"))
        self.loosened(lambda s: s["fixtures"].pop("Q2"))
        self.loosened(lambda s: s["replaced"].pop("dogear-no-writes"))
        self.loosened(lambda s: s.update(extra=True))


MAINTENANCE_TAIL = (f"acceptance-policy: {c.MAINTENANCE_POLICY}\nci-verified: {' '.join(CI_ONLY)}\n")


def maintenance_tag(accepted=" ".join(c.FLAGSHIPS), ci=" ".join(CI_ONLY), policy=c.MAINTENANCE_POLICY, **kw):
    return (tag_message(accepted=accepted, **kw) + f"acceptance-policy: {policy}\nci-verified: {ci}\n")


class MaintenanceTagTests(unittest.TestCase):
    def test_a_maintenance_tag_names_its_policy_and_both_lists(self):
        f = c.parse_tag_message(maintenance_tag())
        self.assertEqual(c.tag_policy(f), c.MAINTENANCE_POLICY)
        self.assertEqual((f["physically-accepted"], f["ci-verified"]), (" ".join(c.FLAGSHIPS), " ".join(CI_ONLY)))
        self.assertEqual(c.tag_policy(c.parse_tag_message(tag_message())), c.FULL_POLICY)

    def test_the_lists_and_policy_are_exactly_the_fixed_ones(self):
        for text in (maintenance_tag(accepted=ALL), maintenance_tag(accepted="x4pro-gpd x4-gd"),
                     maintenance_tag(accepted="x4-gd"), maintenance_tag(accepted="x4-gd x4pro-gpd x4-g"),
                     maintenance_tag(ci=" ".join(CI_ONLY[:-1])), maintenance_tag(ci=" ".join(reversed(CI_ONLY))),
                     maintenance_tag(ci=" ".join(CI_ONLY) + " x4-gd"), maintenance_tag(ci=""),
                     maintenance_tag(policy="full"), maintenance_tag(policy="post-launch-maintenance-lite"),
                     tag_message() + f"ci-verified: {' '.join(CI_ONLY)}\n",
                     tag_message() + f"acceptance-policy: {c.MAINTENANCE_POLICY}\n",
                     maintenance_tag() + f"acceptance-policy: {c.MAINTENANCE_POLICY}\n",
                     maintenance_tag().replace("candidate-run: 123456\n", ""),
                     maintenance_tag(acceptance="f" * 63)):
            with self.assertRaises(c.Problem, msg=text):
                c.parse_tag_message(text)

    def test_parse_tag_reports_the_policy(self):
        with tempfile.TemporaryDirectory() as d:
            msg = Path(d) / "m.txt"
            msg.write_text(maintenance_tag())
            code, out, err = run("parse-tag", msg, "--commit", "b" * 40)
            self.assertEqual(code, 0, err)
            self.assertIn(f"acceptance_policy={c.MAINTENANCE_POLICY}", out)
            self.assertIn(f"ci_verified={' '.join(CI_ONLY)}", out)
            msg.write_text(tag_message())
            code, out, err = run("parse-tag", msg, "--commit", "b" * 40)
            self.assertEqual(code, 0, err)
            self.assertIn(f"acceptance_policy={c.FULL_POLICY}", out)
            self.assertNotIn("ci_verified=", out)


class MaintenancePublishTests(PublishBase):
    def setUp(self):
        super().setUp()
        self.acceptance.write_text(json.dumps(maintenance_record(self.out), indent=2))
        self.maintenance_sha = c.acceptance_digest(self.acceptance, self.out)
        self.tag_text(maintenance_tag(run_id=self.RUN_ID, commit=self.COMMIT, sums=self.sums_sha(),
                                      acceptance=self.maintenance_sha, title=self.TAG))

    def tag_text(self, message):
        self.gets[f"repos/o/r/git/tags/{'t' * 40}"]["message"] = message

    def test_a_maintenance_record_and_tag_preflight_and_publish(self):
        gh = FakeGitHub(self.gets)
        self.assertIn("dry run", self.publish(gh, publish=False))
        self.assertEqual(gh.patches, [])
        gh = FakeGitHub(self.gets)
        self.assertIn("published v1.2.0 as latest", self.publish(gh))
        self.assertEqual(len(gh.patches), 1)

    def test_the_tag_policy_must_be_the_records(self):
        # A full tag naming the maintenance record (all eight claimed physically accepted): refused.
        self.tag_text(tag_message(run_id=self.RUN_ID, commit=self.COMMIT, sums=self.sums_sha(),
                                  acceptance=self.maintenance_sha, title=self.TAG))
        self.assertIn("acceptance policy", self.refused())

    def test_a_maintenance_tag_cannot_carry_a_full_record(self):
        self.acceptance.write_text(json.dumps(acceptance_record(self.out), indent=2))
        full_sha = c.acceptance_digest(self.acceptance, self.out)
        self.tag_text(maintenance_tag(run_id=self.RUN_ID, commit=self.COMMIT, sums=self.sums_sha(),
                                      acceptance=full_sha, title=self.TAG))
        self.assertIn("acceptance policy", self.refused())

    def test_a_different_maintenance_record_than_the_tag_names_refuses(self):
        record = maintenance_record(self.out)
        record["approvedOn"] = "2026-10-23"
        self.acceptance.write_text(json.dumps(record, indent=2))
        self.assertIn("different acceptance record", self.refused())


if __name__ == "__main__":
    unittest.main()
