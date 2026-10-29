#!/usr/bin/env python3
"""Release candidates: assemble, verify and promote exactly what was accepted.

A suite release is built once, by .github/workflows/release_candidate.yml on a
`release/v<apps_version>[-...]` branch. Its `release-candidate` artifact is
installed and physically accepted, and .github/workflows/release.yml then
publishes THOSE bytes as a draft release; nothing is rebuilt. Builds embed
their build time, so a rebuild could never match the accepted bytes.

Standard library only; every subcommand exits non-zero on the first problem.

    assemble        build outputs -> candidate dir + X4 Pro flash pieces
    verify-dir      check a candidate dir (files, sums, legacy copies, provenance)
    e3-evidence     the candidate run's E3 host-test evidence, from ctest's JUnit report
    post-launch-evidence  its post-launch (production-503) host-test evidence, likewise
    acceptance      validate the acceptance record (full or post-launch maintenance); print its sha256
    parse-tag       read an annotated release tag's acceptance fields
    check-run       prove a workflow run is this repo's release_candidate.yml at a commit
    verify-release  compare a GitHub release's asset digests with accepted sums
    publish         the ONLY way to publish: full preflight, then draft -> published + latest
    slice-factory   cut a .factory.bin into the X4 Pro USB flash pieces (NVS untouched)

See docs/RELEASING.md.
"""

from __future__ import annotations

import argparse
import csv
import datetime as dt
import errno
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CANDIDATE_WORKFLOW = ".github/workflows/release_candidate.yml"
SUMS = "SHA256SUMS.txt"
PROVENANCE = "provenance.json"

# The release, one row per build, in the canonical package order: env, package
# suffix (as in the version string), asset stem, has a .factory.bin (X4 Pro),
# legacy copy for 1.0.1 devices. The `d` twins add DOGEAR to a package.
RELEASE = (
    ("gh_release", "G", "firmware-x4-g", False, "firmware.bin"),
    ("x4-goto-dogear-gh_release", "GD", "firmware-x4-gd", False, None),
    ("x4pro-goto-gh_release", "G", "firmware-x4pro-g", True, None),
    ("x4pro-goto-dogear-gh_release", "GD", "firmware-x4pro-gd", True, None),
    ("x4pro-onpoint-gh_release", "P", "firmware-x4pro-p", True, None),
    ("x4pro-onpoint-dogear-gh_release", "PD", "firmware-x4pro-pd", True, None),
    ("x4pro-gh_release", "GP", "firmware-x4pro-gp", True, "firmware-x4pro.bin"),
    ("x4pro-all-gh_release", "GPD", "firmware-x4pro-gpd", True, None),
)
# Which checks each package's physical acceptance must record (docs/RELEASING.md).
ACCEPTANCE_SCHEMA = ROOT / "scripts" / "release" / "acceptance-v1.json"
# The X4 Pro USB pieces, written at their offsets; NVS is never written.
FLASH_PIECES = ("bootloader.bin", "partitions.bin", "boot_app0.bin")

# E3 (CAN'T LOAD AN ISSUE: Wi-Fi associated, no internet) is host-covered, not a
# physical check (owner amendment, 2026-10-11): "Physical E3 is not a release
# gate. Host E3 coverage is mandatory." The candidate run itself runs these
# exact host tests on its own commit; a test missing, failed or skipped fails
# the candidate. Their PASS is e3-host-evidence.json, a release file listed in
# SHA256SUMS.txt (so the tag's candidate-sums-sha256 binds it), and the
# acceptance record carries the same evidence (so acceptance-sha256 binds it).
E3_CHECK = "dogear-e3-cant-load"
E3_POLICY = "Physical E3 is not a release gate. Host E3 coverage is mandatory."
E3_EVIDENCE = "e3-host-evidence.json"
E3_EVIDENCE_SCHEMA = "crosspoint-e3-host-evidence/1"
E3_DEFINITION_KEYS = ("policy", "scenario", "reason", "evidence", "tests")
E3_EVIDENCE_KEYS = ("schema", "check", "repository", "commit", "candidateRun", "definitionSha256", "tests", "skipped")
E3_HOST_TESTS = (
    "DogearBootstrap.ATlsReadFailureAfterTheExactBodyIsNeverNothingPublished",
    "DogearBootstrap.BytesReadOneAtATimeClassifyTheSame",
    "DogearBootstrap.ChunkedFraming",
    "DogearBootstrap.ContentLengthMustMatchTheBody",
    "DogearBootstrap.EveryTruncationIsAnError",
    "DogearBootstrap.NonOkBodiesAreCappedAt64Bytes",
    "DogearBootstrap.OnlyExplicitFramingCounts",
    "DogearBootstrap.OnlyStatus503",
    "DogearBootstrap.OnlyTheExactBodyCounts",
    "DogearBootstrap.OnlyTheFirstManifestResponse",
    "DogearBootstrap.RouterVectorsClassifyExactlyAsTheServerMeansThem",
    "DogearBootstrap.TheReaderStopsAtCompletionAndReportsHowItEnded",
    "DogearState.EachStatusScreenHasItsOwnHeadingAndNote",
    "DogearState.EveryCombinationFollowsTheThreeRules",
    "DogearState.StatusCopyIsExactlyTheApprovedCopy",
    "DogearState.TheCasesThatMatter",
    "DogearTls.CertificateForAnotherHostIsRefused",
    "DogearTls.ExpiredCertificateIsRefused",
    "DogearTls.FailsClosedWhenTrustCannotBeSetUp",
    "DogearTls.ProductionRootsDoNotTrustTheTestRoot",
    "DogearTls.SelfSignedCertificateIsRefused",
    "DogearTls.TrustedCertificateForTheRightHostConnects",
    "DogearTls.ValidCertificateDoesNotCoverADifferentRequestedHost",
    "DogearTlsRead.ACorruptRecordAfterTheExactBodyIsNotNothingPublished",
    "DogearTlsRead.AReadFailureAfterTheExactBodyIsNotNothingPublished",
    "DogearTlsRead.EvenACleanCloseCannotFrameNothingPublished",
    "DogearTlsRead.ExplicitFramingIsNothingPublishedAndACutBeforeItsEndIsNot",
    "DogearTlsRead.ReadResultWaitsOnlyWhileTheTransportIsOpen",
)

# Post-launch maintenance acceptance (docs/RELEASING.md 2a): one fixed
# alternative to crosspoint-acceptance/1, which stays exactly as it is. The two
# DOGEAR flagships are physically accepted; the other six packages are
# CI-verified only and claim no physical check. Production can no longer answer
# 503 "nothing published" (DOGEAR has launched) and a candidate build cannot be
# pointed elsewhere, so the two v1 checks that needed it are host-covered by the
# candidate run's own post-launch-host-evidence.json (a release file in
# SHA256SUMS.txt, built like e3-host-evidence.json), and dogear-no-writes, which
# ran after one of them, becomes dogear-no-writes-offline.
MAINTENANCE_SCHEMA = ROOT / "scripts" / "release" / "acceptance-maintenance-v1.json"
MAINTENANCE = "crosspoint-acceptance-maintenance/1"
MAINTENANCE_POLICY = "post-launch-maintenance"
FULL_POLICY = "full"  # crosspoint-acceptance/1: every package physically accepted
MAINTENANCE_AUTHORIZATION = (
    "The owner authorizes post-launch maintenance acceptance for this release: x4-gd and x4pro-gpd are physically "
    "accepted; x4-g, x4pro-g, x4pro-gd, x4pro-p, x4pro-pd and x4pro-gp are CI-verified only and not physically "
    "accepted.")
FLAGSHIPS = ("x4-gd", "x4pro-gpd")
REPLACED_503 = ("dogear-e1-no-issue-yet", "dogear-v2-cache-503")
NO_WRITES, NO_WRITES_OFFLINE = "dogear-no-writes", "dogear-no-writes-offline"
ONLINE_CURRENT = "dogear-online-current"
# The two maintenance-only physical checks, pinned word for word; every other
# check keeps its acceptance-v1 definition exactly.
MAINTENANCE_DEFINITIONS = {
    NO_WRITES_OFFLINE: (
        "After dogear-v2-cache-offline and the Q1 and Q2 checks (all with Wi-Fi off), the test /dogear files are "
        "byte-identical to what was placed. (Post-launch, an online DOGEAR run may rightly replace the card's "
        "generation with the published issue, so only the offline checks can be no-write.)"),
    ONLINE_CURRENT: (
        "On the candidate bytes, with the device's own /dogear renamed aside (no /dogear on the card), online against "
        "real production: DOGEAR downloads the currently published issue and shows it with no CACHED marker; the "
        "issueId and issueSha256 named by the /dogear/active.txt it writes equal production's /current.json at the "
        "time and are recorded as this package's onlineIssue; then, with Wi-Fi off, DOGEAR opens that same "
        "downloaded issue, marked CACHED."),
}
POST_LAUNCH = "dogear-post-launch-503"  # the maintenance record's hostCovered key for the evidence below
POST_LAUNCH_EVIDENCE = "post-launch-host-evidence.json"
POST_LAUNCH_EVIDENCE_SCHEMA = "crosspoint-post-launch-host-evidence/1"
POST_LAUNCH_POLICY = ("Post-launch, production never again answers 503 'nothing published', and a candidate build "
                      "cannot be pointed at another host. This check is host-covered: physical acceptance cannot "
                      "reproduce it.")
POST_LAUNCH_EVIDENCE_KEYS = ("schema", "checks", "repository", "commit", "candidateRun", "definitionSha256", "tests",
                             "skipped")
POST_LAUNCH_HOST_TESTS = {
    "dogear-e1-no-issue-yet": (
        "DogearBootstrap.OnlyExplicitFramingCounts",
        "DogearBootstrap.OnlyStatus503",
        "DogearBootstrap.OnlyTheExactBodyCounts",
        "DogearBootstrap.RouterVectorsClassifyExactlyAsTheServerMeansThem",
        "DogearPostLaunch.NothingPublishedWithNoReadableIssueIsNoIssueYet",
        "DogearState.EachStatusScreenHasItsOwnHeadingAndNote",
        "DogearState.StatusCopyIsExactlyTheApprovedCopy",
        "DogearState.TheCasesThatMatter",
    ),
    "dogear-v2-cache-503": (
        "DogearBootstrap.RouterVectorsClassifyExactlyAsTheServerMeansThem",
        "DogearCache.CommitsAndLoads",
        "DogearCache.RefusesBytesThatDoNotMatchTheirHash",
        "DogearPostLaunch.CachedRegularIssueWithNothingPublishedIsCachedNeverNoIssueYet",
        "DogearState.EveryCombinationFollowsTheThreeRules",
        "DogearState.TheCasesThatMatter",
    ),
}
POST_LAUNCH_TESTS = tuple(sorted({t for tests in POST_LAUNCH_HOST_TESTS.values() for t in tests}))

# The v1.3.0 acceptance amendment (docs/RELEASING.md 2c): the owner's bounded
# decision for exactly one candidate, never a general exception. Its record is a
# post-launch maintenance record, except that each flagship lists the DOGEAR
# fixture checks under notPerformed instead of checks, and it names no fixtures.
# Any candidate other than AMENDED_CANDIDATE refuses it.
AMENDED = "crosspoint-acceptance-maintenance-v1.3.0/1"
AMENDED_CANDIDATE = {
    "repository": "archievalmariano/crosspoint-reader",
    "tag": "v1.3.0",
    "commit": "ff8df71461839e5b9c6ed00904fc4004f145f746",
    "candidateRun": 40715904821,
    "sumsSha256": "05cf4b40553a568e2d2519d7c02bee8eb9f51417a8b2cee58ddc409f1869180c",
}
NOT_PERFORMED = ("dogear-v2-cache-offline", "dogear-v3-q1-cover", "dogear-v3-q1-pages", "dogear-v3-q1-navigation",
                 "dogear-v3-q2-cover", "dogear-v3-q2-pages", "dogear-v3-q2-navigation", NO_WRITES_OFFLINE)
# Pinned word for word: the owner's decision, recorded in the amended record.
AMENDMENT = (
    "The owner amends post-launch maintenance acceptance for v1.3.0 only: commit "
    "ff8df71461839e5b9c6ed00904fc4004f145f746, candidate run 40715904821, and the candidate artifacts listed in its "
    "SHA256SUMS.txt (sha256 05cf4b40553a568e2d2519d7c02bee8eb9f51417a8b2cee58ddc409f1869180c). The eight DOGEAR "
    "fixture checks listed under each flagship's notPerformed were NOT PERFORMED on v1.3.0; they are recorded neither "
    "as PASS nor as CARRIED. Historical acceptance and successful current smoke tests cannot prove that those checks "
    "would pass on the candidate's bytes. The owner accepts the residual DOGEAR visual and offline no-writes "
    "regression risk for this exact release, based on the completed v1.2.0 acceptance (acceptance-sha256 "
    "3922e7e807261bff060f0176aa45e137b32aefb8e31e5c0c81bed6e6612642ab), the v1.2.1 reviews, existing source "
    "comparisons, successful v1.3.0 hardware tests and the lack of an identified DOGEAR rendering regression. No "
    "DOGEAR publication-contract eligibility is authorized.")


class Problem(Exception):
    pass


def require(ok: bool, message: str) -> None:
    if not ok:
        raise Problem(message)


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def package_name(stem: str) -> str:
    return stem.removeprefix("firmware-")  # "x4pro-gp": the name physical acceptance uses


def release_files() -> list[str]:
    names = [E3_EVIDENCE, POST_LAUNCH_EVIDENCE]
    for _env, _pkg, stem, factory, legacy in RELEASE:
        names.append(f"{stem}.bin")
        if factory:
            names.append(f"{stem}.factory.bin")
        if legacy:
            names.append(legacy)
    return sorted(names)


def packages() -> list[str]:
    return [package_name(stem) for _e, _p, stem, _f, _l in RELEASE]


# ---- flash layout ----------------------------------------------------------


def partition_layout(path: Path = ROOT / "partitions.csv") -> dict:
    """{name: (offset, size)} from the partition table every board uses."""
    table = {}
    with open(path, newline="", encoding="utf-8") as f:
        for row in csv.reader(line for line in f if not line.lstrip().startswith("#")):
            cells = [c.strip() for c in row]
            if len(cells) >= 5 and cells[0]:
                table[cells[0]] = (int(cells[3], 0), int(cells[4], 0))
    for name in ("nvs", "otadata", "app0"):
        require(name in table, f"partitions.csv has no {name}")
    return table


PARTITION_TABLE_OFFSET = 0x8000  # ESP-IDF default; the bootloader sits at 0x0 on the ESP32-S3


def check_factory(factory: bytes, pieces: dict, app: bytes, layout: dict) -> None:
    """A factory image is exactly bootloader, partition table, boot_app0 and app at
    their offsets, with 0xFF everywhere else -- including all of NVS, which is why a
    0x0 factory install erases Wi-Fi and settings, and why the USB procedure writes
    the four pieces instead."""
    nvs_off, nvs_size = layout["nvs"]
    ota_off, ota_size = layout["otadata"]
    app_off, _ = layout["app0"]
    regions = [
        (0x0, pieces["bootloader.bin"], "bootloader"),
        (PARTITION_TABLE_OFFSET, pieces["partitions.bin"], "partition table"),
        (ota_off, pieces["boot_app0.bin"], "boot_app0"),
        (app_off, app, "app"),
    ]
    require(len(pieces["boot_app0.bin"]) == ota_size, "boot_app0.bin is not the size of otadata")
    require(len(factory) == app_off + len(app), "factory image length is not app offset + app length")
    covered = bytearray(len(factory))
    for offset, data, name in regions:
        require(factory[offset:offset + len(data)] == data, f"factory image: {name} differs at 0x{offset:x}")
        covered[offset:offset + len(data)] = b"\x01" * len(data)
    gaps = [i for i, c in enumerate(covered) if not c and factory[i] != 0xFF]
    require(not gaps, f"factory image has data outside its pieces (first at 0x{gaps[0]:x})" if gaps else "")
    require(all(b == 0xFF for b in factory[nvs_off:nvs_off + nvs_size]), "factory image writes NVS")


# ---- assemble / verify -----------------------------------------------------


def write_sums(files: dict, dest: Path) -> bytes:
    text = "".join(f"{sha256(files[name])}  {name}\n" for name in sorted(files))
    data = text.encode("ascii")
    dest.write_bytes(data)
    return data


def read_sums(data: bytes) -> dict:
    sums = {}
    for line in data.decode("ascii").splitlines():
        m = re.fullmatch(r"([0-9a-f]{64})  ([A-Za-z0-9._-]+)", line)
        require(m is not None, f"malformed {SUMS} line: {line!r}")
        require(m.group(2) not in sums, f"{SUMS} names {m.group(2)} twice")
        sums[m.group(2)] = m.group(1)
    return sums


def assemble(args) -> None:
    builds, out, support = Path(args.builds), Path(args.out), Path(args.support)
    require(not out.exists() and not support.exists(), "output directories must not exist yet")
    found = sorted(p.name.removeprefix("build-") for p in builds.iterdir() if p.is_dir())
    expected = sorted(env for env, *_ in RELEASE)
    require(found == expected, f"builds {found} are not exactly the release envs {expected}")
    layout = partition_layout()
    files, flash, support_files = {}, {}, {}  # everything is checked before anything is written
    for env, pkg, stem, factory, legacy in RELEASE:
        d = builds / f"build-{env}"
        version = (d / "version.txt").read_text(encoding="ascii").strip()
        require(version == f"{args.display}-{pkg}", f"{env} built {version!r}, not {args.display}-{pkg}")
        app = (d / "firmware.bin").read_bytes()
        files[f"{stem}.bin"] = app
        if legacy:
            files[legacy] = app
        if factory:
            image = (d / "firmware.factory.bin").read_bytes()
            pieces = {name: (d / name).read_bytes() for name in FLASH_PIECES}
            check_factory(image, pieces, app, layout)
            files[f"{stem}.factory.bin"] = image
            flash[env] = {name: sha256(data) for name, data in pieces.items()}
            support_files.update({f"{env}/{name}": data for name, data in pieces.items()})
        else:
            require(not (d / "firmware.factory.bin").exists(), f"{env} unexpectedly has a factory image")
    evidence = Path(args.e3_evidence).read_bytes()
    check_e3_evidence_bytes(evidence, load_acceptance_schema(), repository=args.repository, commit=args.commit,
                            run=int(args.run_id))
    files[E3_EVIDENCE] = evidence
    post_launch = Path(args.post_launch_evidence).read_bytes()
    check_post_launch_evidence_bytes(post_launch, load_maintenance_schema(), repository=args.repository,
                                     commit=args.commit, run=int(args.run_id))
    files[POST_LAUNCH_EVIDENCE] = post_launch
    require(sorted(files) == release_files(), "assembled files are not exactly the release files")
    out.mkdir(parents=True)
    for name, data in support_files.items():
        (support / name).parent.mkdir(parents=True, exist_ok=True)
        (support / name).write_bytes(data)
    for name, data in files.items():
        (out / name).write_bytes(data)
    sums = write_sums(files, out / SUMS)
    provenance = {
        "schema": 1,
        "repository": args.repository,
        "commit": args.commit,
        "ref": args.ref,
        "runId": int(args.run_id),
        "runAttempt": int(args.run_attempt),
        "workflowRef": args.workflow_ref,
        "version": args.display,
        "packages": packages(),
        "files": {name: {"sha256": sha256(data), "size": len(data)} for name, data in sorted(files.items())},
        "sumsSha256": sha256(sums),
        "flashSupport": flash,
    }
    (out / PROVENANCE).write_text(json.dumps(provenance, indent=2, sort_keys=True) + "\n", encoding="ascii")
    write_sums(support_files, support / SUMS)
    print(f"candidate {args.display} at {args.commit}: {len(files)} files, {SUMS} sha256 {sha256(sums)}")


def verify_dir(args) -> None:
    d = Path(args.dir)
    names = sorted(p.name for p in d.iterdir())
    require(names == sorted(release_files() + [SUMS, PROVENANCE]),
            f"candidate holds {names}, not exactly the release files, {SUMS} and {PROVENANCE}")
    sums_bytes = (d / SUMS).read_bytes()
    sums = read_sums(sums_bytes)
    require(sorted(sums) == release_files(), f"{SUMS} does not name exactly the release files")
    for name, digest in sums.items():
        require(sha256((d / name).read_bytes()) == digest, f"{name} does not match {SUMS}")
    for _env, _pkg, stem, _factory, legacy in RELEASE:
        if legacy:
            require(sums[legacy] == sums[f"{stem}.bin"], f"{legacy} is not a byte copy of {stem}.bin")
    prov = json.loads((d / PROVENANCE).read_text(encoding="ascii"))
    require(prov.get("schema") == 1, "unknown provenance schema")
    require(prov.get("sumsSha256") == sha256(sums_bytes), f"provenance does not bind this {SUMS}")
    require({n: v["sha256"] for n, v in prov.get("files", {}).items()} == sums, "provenance files differ from sums")
    require(prov.get("packages") == packages(), "provenance packages differ from the release")
    check_e3_evidence_bytes((d / E3_EVIDENCE).read_bytes(), load_acceptance_schema(),
                            repository=prov.get("repository"), commit=prov.get("commit"), run=prov.get("runId"))
    check_post_launch_evidence_bytes((d / POST_LAUNCH_EVIDENCE).read_bytes(), load_maintenance_schema(),
                                     repository=prov.get("repository"), commit=prov.get("commit"),
                                     run=prov.get("runId"))
    if args.sums_sha256:
        require(sha256(sums_bytes) == args.sums_sha256, f"{SUMS} sha256 is not the accepted {args.sums_sha256}")
    if args.commit:
        require(prov.get("commit") == args.commit, f"candidate was built from {prov.get('commit')}, not {args.commit}")
    if args.run_id:
        require(prov.get("runId") == int(args.run_id), f"candidate came from run {prov.get('runId')}")
    if args.support:
        support = Path(args.support)
        layout = partition_layout()
        for _env, _pkg, stem, factory, _legacy in RELEASE:
            if not factory:
                continue
            env = _env
            pieces = {n: (support / env / n).read_bytes() for n in FLASH_PIECES}
            for n, data in pieces.items():
                require(sha256(data) == prov["flashSupport"][env][n], f"{env}/{n} is not the candidate's piece")
            check_factory((d / f"{stem}.factory.bin").read_bytes(), pieces, (d / f"{stem}.bin").read_bytes(), layout)
    print(f"candidate OK: {SUMS} sha256 {sha256(sums_bytes)}, commit {prov.get('commit')}, run {prov.get('runId')}")


# ---- physical acceptance ---------------------------------------------------
#
# One record per release candidate, filled in on hardware and approved by the
# owner before the tag. It must hold exactly the release packages, in order,
# each with exactly the checks acceptance-v1.json requires of it, each exactly
# "PASS", and bind the candidate's commit, run, SHA256SUMS.txt and per-file
# digests. The tag carries the sha256 of its canonical bytes.

HEX40 = re.compile(r"[0-9a-f]{40}")
HEX64 = re.compile(r"[0-9a-f]{64}")
ISSUE_ID = re.compile(r"dogear-[0-9]{4}-[0-9]{2}-[0-9]{2}")
RECORD_KEYS = ("schema", "repository", "tag", "commit", "candidateRun", "sumsSha256", "fixtures", "packages",
               "hostCovered", "approvedBy", "approvedOn")
PACKAGE_KEYS = ("package", "env", "board", "file", "sha256", "install", "level", "date", "checks")
PIECE_KEYS = ("bootloader", "partitions", "boot_app0")


def strict_json(data: bytes):
    """JSON with no duplicate key anywhere and no NaN/Infinity."""

    def pairs(items):
        keys = [k for k, _ in items]
        dupes = sorted({k for k in keys if keys.count(k) > 1})
        require(not dupes, f"duplicate key {dupes[0]!r}" if dupes else "")
        return dict(items)

    def constant(name):
        raise Problem(f"{name} is not JSON")

    try:
        return json.loads(data.decode("utf-8"), object_pairs_hook=pairs, parse_constant=constant)
    except (UnicodeDecodeError, json.JSONDecodeError) as err:
        raise Problem(f"not JSON: {err}") from None


def canonical(obj) -> bytes:
    return json.dumps(obj, sort_keys=True, separators=(",", ":"), ensure_ascii=True).encode("ascii")


def exact_keys(obj, keys, where: str) -> None:
    require(isinstance(obj, dict), f"{where} is not an object")
    missing, extra = sorted(set(keys) - set(obj)), sorted(set(obj) - set(keys))
    require(not missing and not extra, f"{where}: missing {missing}, unexpected {extra}")


def is_int(value) -> bool:
    return type(value) is int  # not bool


def check_date(value, where: str) -> None:
    ok = isinstance(value, str) and re.fullmatch(r"[0-9]{4}-[0-9]{2}-[0-9]{2}", value) is not None
    if ok:
        try:
            dt.date.fromisoformat(value)
        except ValueError:
            ok = False
    require(ok, f"{where} is not a YYYY-MM-DD date")


def load_acceptance_schema(path: Path = ACCEPTANCE_SCHEMA) -> dict:
    schema = strict_json(path.read_bytes())
    require(schema.get("schema") == "crosspoint-acceptance/1", "unknown acceptance schema")
    require(schema.get("packageOrder") == packages(), "acceptance-v1.json packages differ from the release")
    require(sorted(schema.get("packages", {})) == sorted(packages()), "acceptance-v1.json packages are not the release")
    for name, spec in schema["packages"].items():
        checks = spec.get("checks")
        require(isinstance(checks, list) and checks and len(checks) == len(set(checks)), f"{name}: bad check list")
        require(all(c in schema.get("checks", {}) for c in checks), f"{name} names an undefined check")
    check_e3_definition(schema)
    return schema


def check_e3_definition(schema: dict) -> dict:
    """hostCovered is required: exactly E3, with the policy, the evidence file and
    exactly E3_HOST_TESTS; and E3 is not a physical check anywhere."""
    host = schema.get("hostCovered")
    require(isinstance(host, dict), "acceptance schema: hostCovered is missing; host E3 coverage is mandatory")
    exact_keys(host, [E3_CHECK], "hostCovered")
    definition = host[E3_CHECK]
    exact_keys(definition, E3_DEFINITION_KEYS, f"hostCovered.{E3_CHECK}")
    require(definition["policy"] == E3_POLICY, f"hostCovered.{E3_CHECK}: the policy must read {E3_POLICY!r}")
    require(definition["evidence"] == E3_EVIDENCE, f"hostCovered.{E3_CHECK}: evidence must be {E3_EVIDENCE}")
    require(definition["tests"] == list(E3_HOST_TESTS),
            f"hostCovered.{E3_CHECK}: tests are not exactly the required E3 host tests")
    require(E3_CHECK not in schema.get("checks", {}), f"{E3_CHECK} must not be a physical check")
    for name, spec in schema["packages"].items():
        require(E3_CHECK not in spec["checks"], f"{name}: physical E3 cannot stand in for host E3 evidence")
    return definition


def e3_definition_sha256(schema: dict) -> str:
    return sha256(canonical(schema["hostCovered"][E3_CHECK]))


def check_e3_evidence(evidence, schema: dict, *, repository, commit, run) -> None:
    """The candidate run's E3 host-test PASS: this commit and run, this definition,
    exactly the required tests, each exactly PASS, none skipped."""
    exact_keys(evidence, E3_EVIDENCE_KEYS, "E3 host evidence")
    require(evidence["schema"] == E3_EVIDENCE_SCHEMA, f"E3 host evidence schema is not {E3_EVIDENCE_SCHEMA}")
    require(evidence["check"] == E3_CHECK, f"E3 host evidence is not for {E3_CHECK}")
    require(evidence["repository"] == repository, "E3 host evidence names another repository")
    require(isinstance(evidence["commit"], str) and evidence["commit"] == commit,
            "E3 host evidence belongs to another commit")
    require(is_int(evidence["candidateRun"]) and evidence["candidateRun"] == run,
            "E3 host evidence belongs to another candidate run")
    require(evidence["definitionSha256"] == e3_definition_sha256(schema),
            "E3 host evidence is for another host-test definition")
    exact_keys(evidence["tests"], E3_HOST_TESTS, "E3 host evidence tests")
    failed = sorted(t for t, result in evidence["tests"].items() if result != "PASS")
    require(not failed, f"E3 host tests not PASS: {failed}")
    require(evidence["skipped"] == [], "E3 host evidence records skipped tests")


def check_e3_evidence_bytes(data: bytes, schema: dict, *, repository, commit, run) -> dict:
    evidence = strict_json(data)
    check_e3_evidence(evidence, schema, repository=repository, commit=commit, run=run)
    require(data == canonical(evidence), f"{E3_EVIDENCE} is not in canonical form")
    return evidence


def require_junit_pass(junit: bytes, tests, label: str) -> None:
    """Fails closed unless each of `tests` ran exactly once in ctest's JUnit
    report and passed (none missing, failed, skipped or repeated)."""
    try:
        root = ET.fromstring(junit)
    except ET.ParseError as err:
        raise Problem(f"the JUnit report is not XML: {err}") from None
    cases = {}
    for case in root.iter("testcase"):
        name = case.get("name")
        require(name not in cases, f"the JUnit report names {name} twice")
        cases[name] = case
    missing = [t for t in tests if t not in cases]
    require(not missing, f"{label} host tests missing from the run: {missing}")
    skipped = [t for t in tests if cases[t].find("skipped") is not None or cases[t].get("status") in
               ("notrun", "disabled")]
    require(not skipped, f"{label} host tests skipped: {skipped}")
    failed = [t for t in tests if cases[t].get("status") != "run" or
              cases[t].find("failure") is not None or cases[t].find("error") is not None]
    require(not failed, f"{label} host tests failed: {failed}")


def e3_evidence_from_junit(junit: bytes, schema: dict, *, repository: str, commit: str, run: int) -> bytes:
    """Canonical evidence from ctest's JUnit report; fails closed unless every
    required test ran once and passed (none missing, failed, skipped or repeated)."""
    require_junit_pass(junit, E3_HOST_TESTS, "E3")
    evidence = {"schema": E3_EVIDENCE_SCHEMA, "check": E3_CHECK, "repository": repository, "commit": commit,
                "candidateRun": run, "definitionSha256": e3_definition_sha256(schema),
                "tests": {t: "PASS" for t in E3_HOST_TESTS}, "skipped": []}
    return canonical(evidence)


def e3_evidence(args) -> None:
    require(HEX40.fullmatch(args.commit or "") is not None, "--commit is not a 40-hex commit")
    data = e3_evidence_from_junit(Path(args.junit).read_bytes(), load_acceptance_schema(),
                                  repository=args.repository, commit=args.commit, run=int(args.run_id))
    out = Path(args.out)
    require(not out.exists(), f"{out} already exists")
    out.write_bytes(data)
    print(f"E3 host evidence: {len(E3_HOST_TESTS)} tests PASS, none skipped; {E3_EVIDENCE} sha256 {sha256(data)}")


def maintenance_checks(v1_checks: list) -> list:
    """A flagship's physical maintenance checks: its acceptance-v1 checks, in
    order, without the two production-503 checks, with dogear-no-writes renamed,
    and dogear-online-current right after E2 (both start with no /dogear)."""
    checks = [NO_WRITES_OFFLINE if c == NO_WRITES else c for c in v1_checks if c not in REPLACED_503]
    at = checks.index("dogear-e2-no-issue-on-device") + 1
    return checks[:at] + [ONLINE_CURRENT] + checks[at:]


def load_maintenance_schema(path: Path = MAINTENANCE_SCHEMA, v1: dict = None) -> dict:
    """acceptance-maintenance-v1.json, required to be exactly the fixed policy:
    every part is derived from acceptance-v1.json or pinned here, so the file
    cannot loosen it (another package, check, test or wording fails)."""
    v1 = v1 or load_acceptance_schema()
    m = strict_json(path.read_bytes())
    exact_keys(m, ("schema", "policy", "comment", "authorization", "packageOrder", "packages", "checks", "fixtures",
                   "replaced", "hostCovered"), "maintenance schema")
    require(m["schema"] == MAINTENANCE, f"maintenance schema is not {MAINTENANCE}")
    require(m["policy"] == MAINTENANCE_POLICY, f"maintenance policy is not {MAINTENANCE_POLICY}")
    require(m["authorization"] == MAINTENANCE_AUTHORIZATION, "maintenance authorization is not the fixed wording")
    require(m["packageOrder"] == packages(), "maintenance packageOrder differs from the release")
    exact_keys(m["packages"], packages(), "maintenance packages")
    used = []
    for name in packages():
        spec, base = m["packages"][name], v1["packages"][name]
        if name in FLAGSHIPS:
            exact_keys(spec, ("board", "verification", "install", "checks"), f"maintenance {name}")
            require(spec["verification"] == "physical", f"{name} must be physically accepted")
            require(spec["install"] == base["install"], f"{name}: install must be {base['install']!r}")
            require(spec["checks"] == maintenance_checks(base["checks"]),
                    f"{name}: checks are not its acceptance-v1 checks without the production-503 ones")
            used += [c for c in spec["checks"] if c not in used]
        else:
            exact_keys(spec, ("board", "verification"), f"maintenance {name}")
            require(spec["verification"] == "ci", f"{name} must be CI-verified only")
        require(spec["board"] == base["board"], f"{name}: board must be {base['board']!r}")
    exact_keys(m["checks"], used, "maintenance checks")
    for check, text in m["checks"].items():
        if check in MAINTENANCE_DEFINITIONS:
            require(text == MAINTENANCE_DEFINITIONS[check], f"{check} must keep its approved maintenance definition")
        else:
            require(text == v1["checks"][check], f"{check} must keep its acceptance-v1 definition")
    require(m["fixtures"] == v1["fixtures"], "maintenance fixtures differ from acceptance-v1's")
    require(m["replaced"] == {REPLACED_503[0]: "hostCovered", REPLACED_503[1]: "hostCovered",
                              NO_WRITES: NO_WRITES_OFFLINE}, "maintenance replaced is not the fixed replacement")
    host = m["hostCovered"]
    exact_keys(host, (E3_CHECK, *REPLACED_503), "maintenance hostCovered")
    require(host[E3_CHECK] == v1["hostCovered"][E3_CHECK], "maintenance E3 must be acceptance-v1's E3 exactly")
    for check in REPLACED_503:
        definition = host[check]
        exact_keys(definition, E3_DEFINITION_KEYS, f"maintenance hostCovered.{check}")
        require(definition["policy"] == POST_LAUNCH_POLICY, f"hostCovered.{check}: not the post-launch policy")
        require(definition["evidence"] == POST_LAUNCH_EVIDENCE, f"hostCovered.{check}: evidence must be "
                                                                f"{POST_LAUNCH_EVIDENCE}")
        require(definition["tests"] == list(POST_LAUNCH_HOST_TESTS[check]),
                f"hostCovered.{check}: tests are not exactly its post-launch host tests")
    return m


def post_launch_definition_sha256(m: dict) -> str:
    return sha256(canonical({check: m["hostCovered"][check] for check in REPLACED_503}))


def check_post_launch_evidence(evidence, m: dict, *, repository, commit, run) -> None:
    """The candidate run's post-launch host-test PASS: this commit and run, this
    definition, exactly the required tests, each exactly PASS, none skipped."""
    exact_keys(evidence, POST_LAUNCH_EVIDENCE_KEYS, "post-launch host evidence")
    require(evidence["schema"] == POST_LAUNCH_EVIDENCE_SCHEMA,
            f"post-launch host evidence schema is not {POST_LAUNCH_EVIDENCE_SCHEMA}")
    require(evidence["checks"] == list(REPLACED_503), f"post-launch host evidence is not for {list(REPLACED_503)}")
    require(evidence["repository"] == repository, "post-launch host evidence names another repository")
    require(isinstance(evidence["commit"], str) and evidence["commit"] == commit,
            "post-launch host evidence belongs to another commit")
    require(is_int(evidence["candidateRun"]) and evidence["candidateRun"] == run,
            "post-launch host evidence belongs to another candidate run")
    require(evidence["definitionSha256"] == post_launch_definition_sha256(m),
            "post-launch host evidence is for another host-test definition")
    exact_keys(evidence["tests"], POST_LAUNCH_TESTS, "post-launch host evidence tests")
    failed = sorted(t for t, result in evidence["tests"].items() if result != "PASS")
    require(not failed, f"post-launch host tests not PASS: {failed}")
    require(evidence["skipped"] == [], "post-launch host evidence records skipped tests")


def check_post_launch_evidence_bytes(data: bytes, m: dict, *, repository, commit, run) -> dict:
    evidence = strict_json(data)
    check_post_launch_evidence(evidence, m, repository=repository, commit=commit, run=run)
    require(data == canonical(evidence), f"{POST_LAUNCH_EVIDENCE} is not in canonical form")
    return evidence


def post_launch_evidence_from_junit(junit: bytes, m: dict, *, repository: str, commit: str, run: int) -> bytes:
    require_junit_pass(junit, POST_LAUNCH_TESTS, "post-launch")
    evidence = {"schema": POST_LAUNCH_EVIDENCE_SCHEMA, "checks": list(REPLACED_503), "repository": repository,
                "commit": commit, "candidateRun": run, "definitionSha256": post_launch_definition_sha256(m),
                "tests": {t: "PASS" for t in POST_LAUNCH_TESTS}, "skipped": []}
    return canonical(evidence)


def post_launch_evidence(args) -> None:
    require(HEX40.fullmatch(args.commit or "") is not None, "--commit is not a 40-hex commit")
    data = post_launch_evidence_from_junit(Path(args.junit).read_bytes(), load_maintenance_schema(),
                                           repository=args.repository, commit=args.commit, run=int(args.run_id))
    out = Path(args.out)
    require(not out.exists(), f"{out} already exists")
    out.write_bytes(data)
    print(f"post-launch host evidence: {len(POST_LAUNCH_TESTS)} tests PASS, none skipped; "
          f"{POST_LAUNCH_EVIDENCE} sha256 {sha256(data)}")


def check_fixtures(fixtures, schema: dict) -> None:
    specs = schema["fixtures"]
    exact_keys(fixtures, ["generator", *specs], "fixtures")
    require(isinstance(fixtures["generator"], str) and HEX40.fullmatch(fixtures["generator"]) is not None,
            "fixtures.generator is not a 40-hex commit")
    ids, digests = set(), set()
    for name, spec in specs.items():
        f, where = fixtures[name], f"fixture {name}"
        exact_keys(f, spec["keys"], where)
        require(is_int(f["schemaVersion"]) and f["schemaVersion"] == spec["schemaVersion"],
                f"{where} must be schema {spec['schemaVersion']}")
        require(isinstance(f["issueId"], str) and ISSUE_ID.fullmatch(f["issueId"]) is not None, f"{where}: bad issueId")
        require(isinstance(f["sha256"], str) and HEX64.fullmatch(f["sha256"]) is not None, f"{where}: bad sha256")
        for key in ("items", "pages", "coverRows"):
            if key in f:
                require(is_int(f[key]) and f[key] > 0, f"{where}: {key} is not a positive integer")
        require(f["pages"] == f["items"] + 2, f"{where}: pages must be the cover, one per item and the final page")
        if f["schemaVersion"] == 3:
            require(3 <= f["items"] <= 5, f"{where}: a quiet issue has 3 to 5 items")
            require(f["coverRows"] <= f["items"] - 1, f"{where}: more cover rows than supporting items")
        ids.add(f["issueId"])
        digests.add(f["sha256"])
    require(len(ids) == len(specs) and len(digests) == len(specs), "fixtures repeat an issue")


def check_acceptance(record, schema: dict, sums_bytes: bytes, prov: dict) -> None:
    exact_keys(record, RECORD_KEYS, "acceptance record")
    require(record["schema"] == schema["schema"], f"record schema is not {schema['schema']}")
    require(record["repository"] == prov.get("repository"), "record names another repository")
    tag = record["tag"]
    require(isinstance(tag, str) and re.fullmatch(r"v[0-9]+\.[0-9]+\.[0-9]+", tag) is not None
            and str(prov.get("version", "")).endswith(f"-A{tag[1:]}"), f"record tag {tag!r} is not this candidate's")
    require(isinstance(record["commit"], str) and HEX40.fullmatch(record["commit"]) is not None
            and record["commit"] == prov.get("commit"), "record commit is not the candidate's")
    require(is_int(record["candidateRun"]) and record["candidateRun"] == prov.get("runId"),
            "record candidateRun is not the candidate's run")
    require(record["sumsSha256"] == sha256(sums_bytes), f"record sumsSha256 is not this {SUMS}")
    check_fixtures(record["fixtures"], schema)
    sums = read_sums(sums_bytes)
    host = record["hostCovered"]
    require(isinstance(host, dict), "record hostCovered is missing; host E3 coverage is mandatory")
    exact_keys(host, [E3_CHECK], "record hostCovered")
    check_e3_evidence(host[E3_CHECK], schema, repository=record["repository"], commit=record["commit"],
                      run=record["candidateRun"])
    require(sha256(canonical(host[E3_CHECK])) == sums[E3_EVIDENCE],
            f"the record's E3 host evidence is not the candidate's {E3_EVIDENCE}")
    entries = record["packages"]
    require(isinstance(entries, list) and all(isinstance(e, dict) for e in entries), "packages is not a list of objects")
    names = [e.get("package") for e in entries]
    require(len(names) == len(set(names)), "record repeats a package")
    require(names == packages(), f"record packages {names} are not exactly {packages()} in that order")
    for (env, _pkg, stem, factory, _legacy), entry in zip(RELEASE, entries):
        name, spec = entry["package"], schema["packages"][entry["package"]]
        exact_keys(entry, PACKAGE_KEYS + (("pieces",) if factory else ()), name)
        require(entry["env"] == env, f"{name} was not built by {env}")
        require(entry["file"] == f"{stem}.bin" and entry["sha256"] == sums[f"{stem}.bin"],
                f"{name}: its file or digest is not the candidate's {stem}.bin")
        for key in ("board", "install", "level"):
            require(entry[key] == spec[key], f"{name}: {key} must be {spec[key]!r}")
        check_date(entry["date"], f"{name} date")
        require(not (isinstance(entry["checks"], dict) and E3_CHECK in entry["checks"]),
                f"{name}: physical E3 cannot stand in for host E3 evidence")
        exact_keys(entry["checks"], spec["checks"], f"{name} checks")
        failed = sorted(c for c, result in entry["checks"].items() if result != "PASS")
        require(not failed, f"{name}: {failed} not PASS")
        if factory:
            exact_keys(entry["pieces"], PIECE_KEYS, f"{name} pieces")
            for piece in PIECE_KEYS:
                require(entry["pieces"][piece] == prov["flashSupport"][env][f"{piece}.bin"],
                        f"{name}: {piece} is not the candidate's piece")
    require(record["approvedBy"] == "owner", "the record must be approved by the owner")
    check_date(record["approvedOn"], "approvedOn")


MAINTENANCE_RECORD_KEYS = ("schema", "policy", "authorization", "repository", "tag", "commit", "candidateRun",
                           "sumsSha256", "fixtures", "packages", "hostCovered", "approvedBy", "approvedOn")
PHYSICAL_ENTRY_KEYS = ("package", "env", "board", "file", "sha256", "verification", "install", "date", "checks",
                       "onlineIssue")
CI_ENTRY_KEYS = ("package", "env", "board", "file", "sha256", "verification")
AMENDED_RECORD_KEYS = tuple(k for k in MAINTENANCE_RECORD_KEYS if k != "fixtures") + ("amendment",
                                                                                      "contractEligible")


def check_amended_candidate(record, prov: dict, sums_bytes: bytes) -> None:
    """An amended record, and the candidate on disk, are exactly AMENDED_CANDIDATE."""
    for key, value in AMENDED_CANDIDATE.items():
        require(isinstance(record, dict) and record.get(key) == value,
                f"the v1.3.0 amendment holds only for {key} {value!r}, not {record.get(key)!r}")
    require(prov.get("repository") == AMENDED_CANDIDATE["repository"]
            and prov.get("commit") == AMENDED_CANDIDATE["commit"]
            and prov.get("runId") == AMENDED_CANDIDATE["candidateRun"]
            and sha256(sums_bytes) == AMENDED_CANDIDATE["sumsSha256"],
            "the v1.3.0 amendment holds only for its own candidate; this candidate is another")


def check_maintenance_acceptance(record, m: dict, v1: dict, sums_bytes: bytes, prov: dict,
                                 amended: bool = False) -> None:
    """A crosspoint-acceptance-maintenance/1 record: bound to the candidate like a
    v1 record; the flagships physically accepted with exactly their maintenance
    checks, each PASS; the other six CI-verified only, claiming no physical check;
    and the candidate's own E3 and post-launch host evidence. `amended` checks a
    v1.3.0 amendment record instead (RELEASING 2c): only for AMENDED_CANDIDATE,
    with the owner's pinned decision, no fixtures, and each flagship's
    NOT_PERFORMED checks listed under notPerformed and absent from its checks."""
    if amended:
        check_amended_candidate(record, prov, sums_bytes)
        exact_keys(record, AMENDED_RECORD_KEYS, "acceptance record")
        require(record["schema"] == AMENDED, f"record schema is not {AMENDED}")
        require(record["amendment"] == AMENDMENT, "record amendment is not the owner's fixed v1.3.0 decision")
        require(record["contractEligible"] is False, "an amended record is never DOGEAR contract-eligible")
    else:
        exact_keys(record, MAINTENANCE_RECORD_KEYS, "acceptance record")
        require(record["schema"] == MAINTENANCE, f"record schema is not {MAINTENANCE}")
    require(record["policy"] == MAINTENANCE_POLICY, f"record policy is not {MAINTENANCE_POLICY}")
    require(record["authorization"] == MAINTENANCE_AUTHORIZATION, "record authorization is not the fixed wording")
    require(record["repository"] == prov.get("repository"), "record names another repository")
    tag = record["tag"]
    require(isinstance(tag, str) and re.fullmatch(r"v[0-9]+\.[0-9]+\.[0-9]+", tag) is not None
            and str(prov.get("version", "")).endswith(f"-A{tag[1:]}"), f"record tag {tag!r} is not this candidate's")
    require(isinstance(record["commit"], str) and HEX40.fullmatch(record["commit"]) is not None
            and record["commit"] == prov.get("commit"), "record commit is not the candidate's")
    require(is_int(record["candidateRun"]) and record["candidateRun"] == prov.get("runId"),
            "record candidateRun is not the candidate's run")
    require(record["sumsSha256"] == sha256(sums_bytes), f"record sumsSha256 is not this {SUMS}")
    if not amended:  # an amended record ran no fixtures and names none
        check_fixtures(record["fixtures"], v1)
    sums = read_sums(sums_bytes)
    host = record["hostCovered"]
    require(isinstance(host, dict), "record hostCovered is missing; host evidence is mandatory")
    exact_keys(host, [E3_CHECK, POST_LAUNCH], "record hostCovered")
    check_e3_evidence(host[E3_CHECK], v1, repository=record["repository"], commit=record["commit"],
                      run=record["candidateRun"])
    require(sha256(canonical(host[E3_CHECK])) == sums[E3_EVIDENCE],
            f"the record's E3 host evidence is not the candidate's {E3_EVIDENCE}")
    check_post_launch_evidence(host[POST_LAUNCH], m, repository=record["repository"], commit=record["commit"],
                               run=record["candidateRun"])
    require(sha256(canonical(host[POST_LAUNCH])) == sums[POST_LAUNCH_EVIDENCE],
            f"the record's post-launch host evidence is not the candidate's {POST_LAUNCH_EVIDENCE}")
    entries = record["packages"]
    require(isinstance(entries, list) and all(isinstance(e, dict) for e in entries), "packages is not a list of objects")
    names = [e.get("package") for e in entries]
    require(len(names) == len(set(names)), "record repeats a package")
    require(names == packages(), f"record packages {names} are not exactly {packages()} in that order")
    for (env, _pkg, stem, factory, _legacy), entry in zip(RELEASE, entries):
        name, spec = entry["package"], m["packages"][entry["package"]]
        physical = name in FLAGSHIPS
        require(entry.get("verification") == spec["verification"],
                f"{name} must be recorded as {spec['verification']!r}" +
                ("" if physical else ": it is CI-verified only and not physically accepted"))
        if not physical:
            claimed = sorted(k for k in ("checks", "install", "date", "level", "onlineIssue", "notPerformed")
                             if k in entry)
            require(not claimed, f"{name} is CI-verified only: it cannot claim physical {claimed}")
        keys = PHYSICAL_ENTRY_KEYS + (("notPerformed",) if amended else ()) if physical else CI_ENTRY_KEYS
        exact_keys(entry, keys + (("pieces",) if factory else ()), name)
        require(entry["env"] == env, f"{name} was not built by {env}")
        require(entry["board"] == spec["board"], f"{name}: board must be {spec['board']!r}")
        require(entry["file"] == f"{stem}.bin" and entry["sha256"] == sums[f"{stem}.bin"],
                f"{name}: its file or digest is not the candidate's {stem}.bin")
        if physical:
            require(entry["install"] == spec["install"], f"{name}: install must be {spec['install']!r}")
            check_date(entry["date"], f"{name} date")
            checks = entry["checks"]
            substituted = sorted(c for c in (*REPLACED_503, NO_WRITES, E3_CHECK)
                                 if isinstance(checks, dict) and c in checks)
            require(not substituted, f"{name}: {substituted} cannot be physical checks under {MAINTENANCE_POLICY} "
                                     "(host-covered, or replaced by dogear-no-writes-offline)")
            required = spec["checks"]
            if amended:
                omitted = [c for c in spec["checks"] if c in NOT_PERFORMED]
                require(omitted == list(NOT_PERFORMED), f"{name}: the amendment's checks are not all {name}'s")
                require(entry["notPerformed"] == omitted,
                        f"{name}: notPerformed must be exactly {omitted}, in that order")
                listed = sorted(c for c in NOT_PERFORMED if isinstance(checks, dict) and c in checks)
                require(not listed, f"{name}: {listed} were not performed on v1.3.0 and cannot be recorded with "
                                    "any result")
                required = [c for c in spec["checks"] if c not in NOT_PERFORMED]
            exact_keys(checks, required, f"{name} checks")
            failed = sorted(c for c, result in checks.items() if result != "PASS")
            require(not failed, f"{name}: {failed} not PASS")
            # What dogear-online-current verified on hardware: the published issue it downloaded.
            online = entry["onlineIssue"]
            exact_keys(online, ("issueId", "issueSha256"), f"{name} onlineIssue")
            require(isinstance(online["issueId"], str) and ISSUE_ID.fullmatch(online["issueId"]) is not None,
                    f"{name} onlineIssue: bad issueId")
            require(isinstance(online["issueSha256"], str) and HEX64.fullmatch(online["issueSha256"]) is not None,
                    f"{name} onlineIssue: bad issueSha256")
        if factory:
            exact_keys(entry["pieces"], PIECE_KEYS, f"{name} pieces")
            for piece in PIECE_KEYS:
                require(entry["pieces"][piece] == prov["flashSupport"][env][f"{piece}.bin"],
                        f"{name}: {piece} is not the candidate's piece")
    require(record["approvedBy"] == "owner", "the record must be approved by the owner")
    check_date(record["approvedOn"], "approvedOn")


def record_policy(record) -> str:
    """The acceptance policy a record was checked under (crosspoint-acceptance/1 is full)."""
    schema = record.get("schema") if isinstance(record, dict) else None
    require(schema in ("crosspoint-acceptance/1", MAINTENANCE, AMENDED), f"unknown acceptance record schema {schema!r}")
    # An amended record is a post-launch maintenance record: its tag fields are exactly §2a's.
    return FULL_POLICY if schema == "crosspoint-acceptance/1" else MAINTENANCE_POLICY


def checked_acceptance(record_path, candidate_dir) -> tuple:
    """Validate the record against the candidate on disk, under the policy its
    schema names; return (canonical sha256, policy)."""
    candidate = Path(candidate_dir)
    prov = json.loads((candidate / PROVENANCE).read_text(encoding="ascii"))
    record = strict_json(Path(record_path).read_bytes())
    policy = record_policy(record)
    v1, sums_bytes = load_acceptance_schema(), (candidate / SUMS).read_bytes()
    if policy == FULL_POLICY:
        check_acceptance(record, v1, sums_bytes, prov)
    else:
        check_maintenance_acceptance(record, load_maintenance_schema(v1=v1), v1, sums_bytes, prov,
                                     amended=record["schema"] == AMENDED)
    return sha256(canonical(record)), policy


def acceptance_digest(record_path, candidate_dir) -> str:
    """Validate the record against the candidate on disk; return its canonical sha256."""
    return checked_acceptance(record_path, candidate_dir)[0]


def acceptance(args) -> None:
    verify_dir(argparse.Namespace(dir=args.candidate_dir, sums_sha256=None, commit=None, run_id=None,
                                  support=args.flash_support))
    digest, policy = checked_acceptance(args.record, args.candidate_dir)
    if policy == FULL_POLICY:
        print(f"acceptance OK: {len(packages())} packages, every required check PASS")
    elif strict_json(Path(args.record).read_bytes())["schema"] == AMENDED:
        print(f"acceptance OK ({MAINTENANCE_POLICY}, v1.3.0 amendment): physically accepted {' '.join(FLAGSHIPS)}, "
              f"every performed check PASS; NOT PERFORMED on each: {' '.join(NOT_PERFORMED)}; CI-verified only "
              f"{' '.join(p for p in packages() if p not in FLAGSHIPS)}; not DOGEAR contract-eligible")
    else:
        print(f"acceptance OK ({MAINTENANCE_POLICY}): physically accepted {' '.join(FLAGSHIPS)}, every required "
              f"check PASS; CI-verified only {' '.join(p for p in packages() if p not in FLAGSHIPS)}")
    print(f"acceptance-policy: {policy}")
    print(f"acceptance-sha256: {digest}")


# ---- tag, run, release -----------------------------------------------------

TAG_FIELDS = ("candidate-run", "candidate-commit", "candidate-sums-sha256", "physically-accepted",
              "acceptance-sha256")
# A post-launch maintenance tag adds exactly these two; a tag without
# acceptance-policy is a full (crosspoint-acceptance/1) tag, as before.
MAINTENANCE_TAG_FIELDS = ("acceptance-policy", "ci-verified")


def parse_tag_message(text: str) -> dict:
    fields = {}
    for line in text.splitlines():
        m = re.fullmatch(r"([a-z0-9-]+):\s*(.*?)\s*", line)
        if m:
            require(m.group(1) in TAG_FIELDS + MAINTENANCE_TAG_FIELDS, f"tag message has an unknown field {m.group(1)}")
            require(m.group(1) not in fields, f"tag message repeats {m.group(1)}")
            fields[m.group(1)] = m.group(2)
    if "acceptance-policy" in fields or "ci-verified" in fields:
        return parse_maintenance_tag_fields(fields)
    missing = [k for k in TAG_FIELDS if k not in fields]
    require(not missing, f"tag message lacks {', '.join(missing)}")
    require(re.fullmatch(r"[1-9][0-9]*", fields["candidate-run"]) is not None, "candidate-run is not a run id")
    require(HEX40.fullmatch(fields["candidate-commit"]) is not None, "candidate-commit is not a 40-hex commit")
    require(HEX64.fullmatch(fields["candidate-sums-sha256"]) is not None, "bad candidate-sums-sha256")
    require(HEX64.fullmatch(fields["acceptance-sha256"]) is not None, "bad acceptance-sha256")
    accepted = fields["physically-accepted"].split()
    require(len(accepted) == len(set(accepted)), "physically-accepted repeats a package")
    # Every package of the release, in order, must have been installed and checked on hardware.
    require(accepted == packages(), f"physically-accepted {accepted} must be exactly {packages()}, in that order")
    return fields


def parse_maintenance_tag_fields(fields: dict) -> dict:
    """A post-launch maintenance tag: the five fields plus acceptance-policy and
    ci-verified, each once; the flagships physically accepted and exactly the
    other six CI-verified, each in canonical order."""
    expected = TAG_FIELDS + MAINTENANCE_TAG_FIELDS
    missing = [k for k in expected if k not in fields]
    require(not missing, f"tag message lacks {', '.join(missing)}")
    require(fields["acceptance-policy"] == MAINTENANCE_POLICY,
            f"acceptance-policy must be {MAINTENANCE_POLICY}, not {fields['acceptance-policy']!r}")
    require(re.fullmatch(r"[1-9][0-9]*", fields["candidate-run"]) is not None, "candidate-run is not a run id")
    require(HEX40.fullmatch(fields["candidate-commit"]) is not None, "candidate-commit is not a 40-hex commit")
    require(HEX64.fullmatch(fields["candidate-sums-sha256"]) is not None, "bad candidate-sums-sha256")
    require(HEX64.fullmatch(fields["acceptance-sha256"]) is not None, "bad acceptance-sha256")
    accepted, ci = fields["physically-accepted"].split(), fields["ci-verified"].split()
    require(len(accepted) == len(set(accepted)) and len(ci) == len(set(ci)), "a tag package list repeats a package")
    require(accepted == list(FLAGSHIPS),
            f"physically-accepted {accepted} must be exactly {list(FLAGSHIPS)} under {MAINTENANCE_POLICY}")
    others = [p for p in packages() if p not in FLAGSHIPS]
    require(ci == others, f"ci-verified {ci} must be exactly {others}, in that order")
    return fields


def tag_policy(fields: dict) -> str:
    return fields.get("acceptance-policy", FULL_POLICY)


def parse_tag(args) -> None:
    fields = parse_tag_message(Path(args.message).read_text(encoding="utf-8"))
    require(fields["candidate-commit"] == args.commit,
            f"tag names candidate commit {fields['candidate-commit']}, but points at {args.commit}")
    for key in TAG_FIELDS:
        print(f"{key.replace('-', '_')}={fields[key]}")
    print(f"acceptance_policy={tag_policy(fields)}")
    if tag_policy(fields) == MAINTENANCE_POLICY:
        print(f"ci_verified={fields['ci-verified']}")


def check_run_json(run: dict, *, repository: str, workflow_id: int, commit: str, branch_prefix: str) -> None:
    """The run must be this repository's release_candidate.yml, dispatched on a
    release branch, completed successfully, at exactly `commit`. GitHub may
    report the path with an `@<ref>` suffix; the workflow id pins the identity."""
    path = str(run.get("path", "")).split("@", 1)[0]
    require(path == CANDIDATE_WORKFLOW, f"run is {run.get('path')!r}, not {CANDIDATE_WORKFLOW}")
    require(run.get("workflow_id") == workflow_id, f"run's workflow id {run.get('workflow_id')} is not {workflow_id}")
    require((run.get("repository") or {}).get("full_name") == repository, "run belongs to another repository")
    require((run.get("head_repository") or {}).get("full_name") == repository, "run built another repository's code")
    require(run.get("event") == "workflow_dispatch", f"run was triggered by {run.get('event')}, not a dispatch")
    require(run.get("status") == "completed" and run.get("conclusion") == "success",
            f"run is {run.get('status')}/{run.get('conclusion')}, not completed/success")
    require(str(run.get("head_branch", "")).startswith(branch_prefix),
            f"run was on {run.get('head_branch')!r}, not a {branch_prefix}* branch")
    require(run.get("head_sha") == commit, f"run built {run.get('head_sha')}, not {commit}")


def check_run(args) -> None:
    run = json.loads(Path(args.run_json).read_text(encoding="utf-8"))
    check_run_json(run, repository=args.repository, workflow_id=int(args.workflow_id), commit=args.commit,
                   branch_prefix=args.branch_prefix)
    print(f"run {run.get('id')} attempt {run.get('run_attempt')}: release_candidate.yml at {args.commit}")


def check_release_json(release: dict, sums_bytes: bytes, *, tag: str, state: str, require_immutable: bool) -> None:
    """Every asset of the release is one of the accepted files, with GitHub's own
    sha256 digest equal to the accepted one; nothing missing, nothing extra."""
    require(release.get("tag_name") == tag, f"release is {release.get('tag_name')}, not {tag}")
    require(release.get("draft") is (state == "draft"), f"release is not {state}")
    require(release.get("prerelease") is False, "release is a prerelease")
    if require_immutable:
        require(release.get("immutable") is True, "release is not immutable")
    expected = dict(read_sums(sums_bytes))
    expected[SUMS] = sha256(sums_bytes)
    assets = release.get("assets") or []
    names = [a.get("name") for a in assets]
    require(len(names) == len(set(names)), "release has duplicate asset names")
    require(sorted(names) == sorted(expected), f"release assets {sorted(names)} are not exactly {sorted(expected)}")
    for asset in assets:
        require(asset.get("state") == "uploaded", f"{asset.get('name')} is {asset.get('state')}, not uploaded")
        digest = asset.get("digest")
        require(isinstance(digest, str) and digest.startswith("sha256:"), f"{asset.get('name')} has no sha256 digest")
        require(digest.removeprefix("sha256:") == expected[asset["name"]],
                f"{asset['name']} digest {digest} is not the accepted sha256")


def verify_release(args) -> None:
    release = json.loads(Path(args.release_json).read_text(encoding="utf-8"))
    sums_bytes = Path(args.sums).read_bytes()
    check_release_json(release, sums_bytes, tag=args.tag, state=args.state, require_immutable=args.require_immutable)
    print(f"release {args.tag} ({args.state}{', immutable' if release.get('immutable') else ''}): "
          f"{len(release['assets'])} assets match {SUMS} sha256 {sha256(sums_bytes)}")


# ---- publish ---------------------------------------------------------------


class PublishNotApplied(Exception):
    """GitHub certainly did not apply the publication request (exit 3)."""


class PostPublishProblem(Exception):
    """The publication request may have been applied and the outcome is not
    verified (exit 2, "MAY BE PUBLISHED"). Deliberately not a Problem, so no
    preflight handler can catch it."""


class PublishUncertain(RuntimeError):
    """The publication request may have reached GitHub; its effect is unknown."""


PUBLISH_TIMEOUT_S = 120


def exec_failed(err: BaseException, exe: str) -> bool:
    """True only for an exec of `exe` itself failing: CPython raises the exec
    errno with the executable as `filename`. A post-fork child-setup failure can
    raise the same errno but names something else (e.g. a working directory)."""
    return (isinstance(err, OSError) and err.errno in (errno.ENOENT, errno.EACCES)
            and err.filename == exe)


class GitHub:
    """The few REST calls publish needs, through the authenticated `gh` CLI.

    publish_latest returns only when `gh` exits 0. It raises PublishNotApplied
    only when `gh` provably never executed: it is not on PATH or not executable
    (checked before anything is started), or Popen failed to exec exactly that
    file (ENOENT/EACCES naming it). Every other outcome may have been applied
    and raises PublishUncertain: any other Popen failure (pipes, fork, a
    post-fork child-setup error), any exception while communicating, waiting,
    decoding or timing out, and any non-zero exit whatever its status or
    message. stderr is never parsed to decide this."""

    def get(self, path: str):
        out = subprocess.run(["gh", "api", path], check=True, capture_output=True, text=True).stdout
        return json.loads(out)

    def publish_latest(self, repository: str, release_id: int) -> None:
        # Phase 0, before anything starts: a gh that cannot be found or executed never runs.
        exe = shutil.which("gh")
        if exe is None:
            raise PublishNotApplied("gh is not on PATH; nothing was run")
        if not os.access(exe, os.X_OK):
            raise PublishNotApplied(f"{exe} is not executable; nothing was run")
        argv = [exe, "api", "-X", "PATCH", f"repos/{repository}/releases/{release_id}",
                "-F", "draft=false", "-f", "make_latest=true"]
        # Phase 1, start: only a failure to exec exactly `exe` proves gh never ran.
        try:
            proc = subprocess.Popen(argv, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                    text=True)
        except BaseException as err:  # noqa: BLE001 -- classified below; Ctrl-C included
            if exec_failed(err, exe):
                raise PublishNotApplied(f"could not execute {exe}: {err}") from err
            raise PublishUncertain(f"starting gh failed and may have run it ({type(err).__name__}: {err})") from err
        # Phase 2, communication: the request may have reached GitHub from here on.
        try:
            _stdout, stderr = proc.communicate(timeout=PUBLISH_TIMEOUT_S)
        except BaseException as err:  # noqa: BLE001 -- includes timeouts, pipe errors, decoding, Ctrl-C
            try:
                proc.kill()
                proc.wait(timeout=5)
            except BaseException:  # noqa: BLE001 -- best effort; the outcome is uncertain regardless
                pass
            raise PublishUncertain(f"gh api PATCH started, then {type(err).__name__}: {err}") from err
        if proc.returncode != 0:
            raise PublishUncertain(f"gh api PATCH exited {proc.returncode}: {(stderr or '').strip()}")


def check_tag(gh, repository: str, tag: str, commit: str, run_id: int, sums_sha: str, acceptance_sha: str,
              policy: str = FULL_POLICY) -> None:
    ref = gh.get(f"repos/{repository}/git/ref/tags/{tag}")
    require((ref.get("object") or {}).get("type") == "tag", f"{tag} is not an annotated tag")
    obj = gh.get(f"repos/{repository}/git/tags/{ref['object']['sha']}")
    target = obj.get("object") or {}
    require(target.get("type") == "commit" and target.get("sha") == commit, f"{tag} does not point at {commit}")
    fields = parse_tag_message(obj.get("message", ""))
    require(int(fields["candidate-run"]) == run_id, f"{tag} names run {fields['candidate-run']}, not {run_id}")
    require(fields["candidate-commit"] == commit, f"{tag} names candidate commit {fields['candidate-commit']}")
    require(fields["candidate-sums-sha256"] == sums_sha, f"{tag} names a different accepted {SUMS} digest")
    require(fields["acceptance-sha256"] == acceptance_sha, f"{tag} names a different acceptance record")
    require(tag_policy(fields) == policy, f"{tag} names acceptance policy {tag_policy(fields)!r}, "
                                          f"but the acceptance record is {policy!r}")


def find_release(gh, repository: str, tag: str) -> dict:
    releases = gh.get(f"repos/{repository}/releases?per_page=100")
    require(isinstance(releases, list) and len(releases) < 100, "cannot list every release in one page")
    matching = [r for r in releases if r.get("tag_name") == tag]
    require(len(matching) == 1, f"{len(matching)} releases are tagged {tag}, not exactly one")
    return matching[0]


def publish(args, gh=None) -> None:
    """Preflight everything, then publish. Any failure before the PATCH changes
    nothing; a failure after it raises PostPublishProblem (exit 2)."""
    gh = gh or GitHub()
    repository, tag, commit, run_id = args.repository, args.tag, args.commit, int(args.run_id)
    require(re.fullmatch(r"v[0-9]+\.[0-9]+\.[0-9]+", tag) is not None, f"{tag} is not a release tag")
    candidate = Path(args.candidate)
    # 0. The accepted bytes on this machine are the candidate of that commit and run.
    verify_dir(argparse.Namespace(dir=candidate, sums_sha256=args.sums_sha256, commit=commit, run_id=run_id,
                                  support=None))
    sums_bytes = (candidate / SUMS).read_bytes()
    sums_sha = sha256(sums_bytes)
    # The owner-approved acceptance record of exactly these bytes; the tag must name it (step 2).
    acceptance_sha, policy = checked_acceptance(args.acceptance, candidate)
    # 1. Release immutability must already be on, or publishing could leave assets replaceable.
    setting = gh.get(f"repos/{repository}/immutable-releases")
    require(setting.get("enabled") is True, "release immutability is not enabled for this repository")
    # 2. The release: exactly one, still a draft, not a prerelease, its tag the accepted commit.
    release = find_release(gh, repository, tag)
    require(release.get("draft") is True, f"{tag} is already published; refusing to republish")
    require(release.get("prerelease") is False, f"{tag} is a prerelease")
    check_tag(gh, repository, tag, commit, run_id, sums_sha, acceptance_sha, policy)
    # 3. The candidate run, and the draft's assets against the accepted bytes.
    workflow_id = gh.get(f"repos/{repository}/actions/workflows/release_candidate.yml")["id"]
    check_run_json(gh.get(f"repos/{repository}/actions/runs/{run_id}"), repository=repository,
                   workflow_id=workflow_id, commit=commit, branch_prefix=f"release/{tag}")
    check_release_json(release, sums_bytes, tag=tag, state="draft", require_immutable=False)
    print(f"preflight OK: {tag} draft {release['id']} = run {run_id} at {commit}, {SUMS} sha256 {sums_sha}")
    if not args.publish:
        print("dry run: nothing changed (pass --publish to publish and mark latest)")
        return
    # 4. The only state change. PublishNotApplied (exit 3) means gh never started,
    # so nothing changed; any other failure from here on may leave the release public.
    try:
        gh.publish_latest(repository, release["id"])
    except PublishNotApplied:
        raise
    except BaseException as err:  # noqa: BLE001 -- includes KeyboardInterrupt
        raise PostPublishProblem(f"{tag} MAY BE PUBLISHED: the publication request failed after it may have "
                                 f"reached GitHub ({type(err).__name__}: {err}); check the release now") from err
    # 5. Re-fetch: published, latest, not a prerelease, immutable, same digests.
    # The request succeeded, so every failure of any kind is an uncertain outcome.
    try:
        published = gh.get(f"repos/{repository}/releases/{release['id']}")
        check_release_json(published, sums_bytes, tag=tag, state="published", require_immutable=True)
        latest = gh.get(f"repos/{repository}/releases/latest")
        require(latest.get("id") == release["id"] and latest.get("tag_name") == tag, f"latest is {latest.get('tag_name')}")
        check_tag(gh, repository, tag, commit, run_id, sums_sha, acceptance_sha, policy)
        print(f"published {tag} as latest: immutable, {len(published['assets'])} assets match the accepted bytes")
    except BaseException as err:  # noqa: BLE001 -- includes KeyboardInterrupt
        detail = (err.stderr or "").strip() if isinstance(err, subprocess.CalledProcessError) else err
        raise PostPublishProblem(f"{tag} MAY BE PUBLISHED but does not verify; check it now "
                                 f"({type(err).__name__}: {detail})") from err


def slice_factory(args) -> None:
    """The four X4 Pro USB pieces of a factory image (e.g. a published release's),
    so an install writes exactly its bytes and leaves NVS alone."""
    image = Path(args.factory).read_bytes()
    layout = partition_layout()
    ota_off, ota_size = layout["otadata"]
    app_off, _ = layout["app0"]
    nvs_off, nvs_size = layout["nvs"]
    require(all(b == 0xFF for b in image[nvs_off:nvs_off + nvs_size]), "factory image writes NVS")
    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=False)
    pieces = {
        "0x0-bootloader.bin": image[0x0:PARTITION_TABLE_OFFSET],
        "0x8000-partitions.bin": image[PARTITION_TABLE_OFFSET:nvs_off],
        f"0x{ota_off:x}-boot_app0.bin": image[ota_off:ota_off + ota_size],
        f"0x{app_off:x}-app.bin": image[app_off:],
    }
    for name, data in pieces.items():
        (out / name).write_bytes(data)
        print(f"{sha256(data)}  {name}")
    if args.app:
        require(pieces[f"0x{app_off:x}-app.bin"] == Path(args.app).read_bytes(), "app slice is not the release app")
        print("app slice equals", args.app)


def main(argv=None) -> int:
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = p.add_subparsers(dest="cmd", required=True)
    a = sub.add_parser("assemble")
    for opt in ("builds", "out", "support", "display", "commit", "ref", "run-id", "run-attempt", "workflow-ref",
                "repository", "e3-evidence", "post-launch-evidence"):
        a.add_argument(f"--{opt}", required=True)
    a.set_defaults(func=assemble)
    e = sub.add_parser("e3-evidence", help="the candidate run's E3 host-test PASS, from ctest's JUnit report")
    for opt in ("junit", "repository", "commit", "run-id", "out"):
        e.add_argument(f"--{opt}", required=True)
    e.set_defaults(func=e3_evidence)
    pl = sub.add_parser("post-launch-evidence",
                        help="the candidate run's post-launch (production-503) host-test PASS, from ctest's JUnit report")
    for opt in ("junit", "repository", "commit", "run-id", "out"):
        pl.add_argument(f"--{opt}", required=True)
    pl.set_defaults(func=post_launch_evidence)
    v = sub.add_parser("verify-dir")
    v.add_argument("dir")
    v.add_argument("--sums-sha256")
    v.add_argument("--commit")
    v.add_argument("--run-id")
    v.add_argument("--support", help="also check the X4 Pro flash pieces against each factory image")
    v.set_defaults(func=verify_dir)
    c = sub.add_parser("acceptance", help="validate an acceptance record; print the sha256 the tag must carry")
    c.add_argument("--record", required=True)
    c.add_argument("--candidate-dir", required=True)
    c.add_argument("--flash-support", required=True, help="the run's x4pro-flash-support artifact")
    c.set_defaults(func=acceptance)
    t = sub.add_parser("parse-tag")
    t.add_argument("message")
    t.add_argument("--commit", required=True, help="the commit the tag points at")
    t.set_defaults(func=parse_tag)
    r = sub.add_parser("check-run")
    r.add_argument("run_json")
    for opt in ("repository", "workflow-id", "commit", "branch-prefix"):
        r.add_argument(f"--{opt}", required=True)
    r.set_defaults(func=check_run)
    g = sub.add_parser("verify-release")
    g.add_argument("release_json")
    g.add_argument("--sums", required=True, help=f"the accepted {SUMS}")
    g.add_argument("--tag", required=True)
    g.add_argument("--state", choices=("draft", "published"), required=True)
    g.add_argument("--require-immutable", action="store_true")
    g.set_defaults(func=verify_release)
    u = sub.add_parser("publish", help="preflight, then (with --publish) publish the draft as latest")
    for opt in ("repository", "tag", "commit", "run-id", "candidate"):
        u.add_argument(f"--{opt}", required=True)
    u.add_argument("--sums-sha256", required=True, help=f"the accepted sha256 of {SUMS}")
    u.add_argument("--acceptance", required=True, help="the owner-approved acceptance record the tag names")
    u.add_argument("--publish", action="store_true", help="after a clean preflight, publish and mark latest")
    u.set_defaults(func=publish)
    s = sub.add_parser("slice-factory")
    s.add_argument("factory")
    s.add_argument("--out", required=True)
    s.add_argument("--app", help="also require the app slice to equal this file")
    s.set_defaults(func=slice_factory)
    args = p.parse_args(argv)
    try:
        args.func(args)
    except PostPublishProblem as err:
        print(f"::error::{err}", file=sys.stderr)
        return 2
    except PublishNotApplied as err:
        print(f"::error::not published: {err}", file=sys.stderr)
        return 3
    except subprocess.CalledProcessError as err:
        print(f"::error::{' '.join(err.cmd[:4])} failed: {(err.stderr or '').strip()}", file=sys.stderr)
        return 1
    except (Problem, OSError, ValueError, KeyError, AttributeError, TypeError) as err:
        print(f"::error::{err}" if not isinstance(err, KeyError) else f"::error::missing {err}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
