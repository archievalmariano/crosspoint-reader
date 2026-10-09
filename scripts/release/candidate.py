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
import hashlib
import json
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CANDIDATE_WORKFLOW = ".github/workflows/release_candidate.yml"
SUMS = "SHA256SUMS.txt"
PROVENANCE = "provenance.json"

# The release, one row per build: env, package suffix (as in the version
# string), asset stem, has a .factory.bin (X4 Pro), legacy copy for 1.0.1 devices.
RELEASE = (
    ("gh_release", "G", "firmware-x4-g", False, "firmware.bin"),
    ("x4pro-goto-gh_release", "G", "firmware-x4pro-g", True, None),
    ("x4pro-onpoint-gh_release", "P", "firmware-x4pro-p", True, None),
    ("x4pro-gh_release", "GP", "firmware-x4pro-gp", True, "firmware-x4pro.bin"),
)
# The X4 Pro USB pieces, written at their offsets; NVS is never written.
FLASH_PIECES = ("bootloader.bin", "partitions.bin", "boot_app0.bin")


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
    names = []
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


# ---- tag, run, release -----------------------------------------------------

TAG_FIELDS = ("candidate-run", "candidate-sums-sha256", "physically-accepted")


def parse_tag_message(text: str) -> dict:
    fields = {}
    for line in text.splitlines():
        m = re.fullmatch(r"([a-z0-9-]+):\s*(.*?)\s*", line)
        if m and m.group(1) in TAG_FIELDS:
            require(m.group(1) not in fields, f"tag message repeats {m.group(1)}")
            fields[m.group(1)] = m.group(2)
    missing = [k for k in TAG_FIELDS if k not in fields]
    require(not missing, f"tag message lacks {', '.join(missing)}")
    require(re.fullmatch(r"[1-9][0-9]*", fields["candidate-run"]) is not None, "candidate-run is not a run id")
    require(re.fullmatch(r"[0-9a-f]{64}", fields["candidate-sums-sha256"]) is not None, "bad candidate-sums-sha256")
    accepted = fields["physically-accepted"].split()
    require(len(accepted) == len(set(accepted)), "physically-accepted repeats a package")
    # Every package of the release must have been installed and checked on hardware.
    require(sorted(accepted) == sorted(packages()),
            f"physically-accepted {sorted(accepted)} must be exactly the release packages {sorted(packages())}")
    return fields


def parse_tag(args) -> None:
    fields = parse_tag_message(Path(args.message).read_text(encoding="utf-8"))
    for key in TAG_FIELDS:
        print(f"{key.replace('-', '_')}={fields[key]}")


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


class GitHub:
    """The few REST calls publish needs, through the authenticated `gh` CLI.

    publish_latest returns only when `gh` exits 0 (GitHub answered 2xx). It
    raises PublishNotApplied only when the request certainly had no effect:
    the `gh` process could not be created (no child ever existed), or GitHub
    answered 4xx (rejected before any change). Once the process exists, every
    other failure -- communicating, waiting, decoding, timing out, a 5xx, a
    network error, an exit with no HTTP status -- may have been applied and
    raises PublishUncertain."""

    def get(self, path: str):
        out = subprocess.run(["gh", "api", path], check=True, capture_output=True, text=True).stdout
        return json.loads(out)

    def publish_latest(self, repository: str, release_id: int) -> None:
        argv = ["gh", "api", "-X", "PATCH", f"repos/{repository}/releases/{release_id}",
                "-F", "draft=false", "-f", "make_latest=true"]
        # Phase 1, creation: if this raises, no child process exists and nothing was sent.
        try:
            proc = subprocess.Popen(argv, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                    text=True)
        except (OSError, ValueError, subprocess.SubprocessError) as err:
            raise PublishNotApplied(f"could not start gh: {err}") from err
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
        stderr = (stderr or "").strip()
        if proc.returncode == 0:
            return
        if re.search(r"\(HTTP 4[0-9][0-9]\)", stderr):
            raise PublishNotApplied(f"GitHub rejected the request: {stderr}")
        raise PublishUncertain(f"gh api PATCH exited {proc.returncode} without a 4xx rejection: {stderr}")


def check_tag(gh, repository: str, tag: str, commit: str, run_id: int, sums_sha: str) -> None:
    ref = gh.get(f"repos/{repository}/git/ref/tags/{tag}")
    require((ref.get("object") or {}).get("type") == "tag", f"{tag} is not an annotated tag")
    obj = gh.get(f"repos/{repository}/git/tags/{ref['object']['sha']}")
    target = obj.get("object") or {}
    require(target.get("type") == "commit" and target.get("sha") == commit, f"{tag} does not point at {commit}")
    fields = parse_tag_message(obj.get("message", ""))
    require(int(fields["candidate-run"]) == run_id, f"{tag} names run {fields['candidate-run']}, not {run_id}")
    require(fields["candidate-sums-sha256"] == sums_sha, f"{tag} names a different accepted {SUMS} digest")


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
    # 1. Release immutability must already be on, or publishing could leave assets replaceable.
    setting = gh.get(f"repos/{repository}/immutable-releases")
    require(setting.get("enabled") is True, "release immutability is not enabled for this repository")
    # 2. The release: exactly one, still a draft, not a prerelease, its tag the accepted commit.
    release = find_release(gh, repository, tag)
    require(release.get("draft") is True, f"{tag} is already published; refusing to republish")
    require(release.get("prerelease") is False, f"{tag} is a prerelease")
    check_tag(gh, repository, tag, commit, run_id, sums_sha)
    # 3. The candidate run, and the draft's assets against the accepted bytes.
    workflow_id = gh.get(f"repos/{repository}/actions/workflows/release_candidate.yml")["id"]
    check_run_json(gh.get(f"repos/{repository}/actions/runs/{run_id}"), repository=repository,
                   workflow_id=workflow_id, commit=commit, branch_prefix=f"release/{tag}")
    check_release_json(release, sums_bytes, tag=tag, state="draft", require_immutable=False)
    print(f"preflight OK: {tag} draft {release['id']} = run {run_id} at {commit}, {SUMS} sha256 {sums_sha}")
    if not args.publish:
        print("dry run: nothing changed (pass --publish to publish and mark latest)")
        return
    # 4. The only state change. PublishNotApplied (exit 3) means certainly nothing
    # changed; any other failure from here on may leave the release public.
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
        check_tag(gh, repository, tag, commit, run_id, sums_sha)
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
                "repository"):
        a.add_argument(f"--{opt}", required=True)
    a.set_defaults(func=assemble)
    v = sub.add_parser("verify-dir")
    v.add_argument("dir")
    v.add_argument("--sums-sha256")
    v.add_argument("--commit")
    v.add_argument("--run-id")
    v.add_argument("--support", help="also check the X4 Pro flash pieces against each factory image")
    v.set_defaults(func=verify_dir)
    t = sub.add_parser("parse-tag")
    t.add_argument("message")
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
