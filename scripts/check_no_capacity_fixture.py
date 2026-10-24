#!/usr/bin/env python3
"""Keep ON POINT's synthetic capacity fixture out of release builds.

Two checks, each failing (exit 1) on any finding:

  --config platformio.ini [--workflow release_candidate.yml]
      Only `x4pro-all-capacity*` envs may name the fixture or bench macros,
      the fixture sources, or the capacity build script, and no other env may
      extend a capacity env. No capacity env may appear in the release
      candidate workflow's build matrix.

  --scan FILE...
      A built image (.bin / .elf) must not contain the fixture's markers
      (route IDs `fixture-`, labels `FIXTURE`, the `-CAPACITY` version suffix).
      Run it on every release-env build.
"""

import argparse
import configparser
import re
import sys

CAPACITY_ENV = re.compile(r"^x4pro-all-capacity(-[a-z0-9-]+)?$")
CONFIG_MARKERS = ("ON_POINT_CAPACITY_FIXTURE", "ON_POINT_BENCH", "ON_POINT_COUNT_COMPARISONS", "test/fixtures",
                  "on_point_capacity_fixture.py")
ARTIFACT_MARKERS = (b"fixture-", b"FIXTURE", b"-CAPACITY", b"ON_POINT_CAPACITY")


def check_config(path, workflow):
    parser = configparser.ConfigParser(interpolation=None, strict=False, inline_comment_prefixes=(";",))
    parser.optionxform = str
    with open(path, encoding="utf-8") as f:
        parser.read_file(f)
    problems = []
    envs = {s[len("env:"):]: s for s in parser.sections() if s.startswith("env:")}
    if not any(CAPACITY_ENV.match(e) for e in envs):
        problems.append("no x4pro-all-capacity env found (the guard would be vacuous)")
    for name, section in parser.items():
        capacity = name.startswith("env:") and CAPACITY_ENV.match(name[len("env:"):])
        for key, value in parser.items(name) if name != "DEFAULT" else []:
            hits = [m for m in CONFIG_MARKERS if m in value]
            if hits and not capacity:
                problems.append(f"[{name}] {key} names {', '.join(hits)} outside a capacity env")
        if name.startswith("env:") and not capacity:
            extends = parser.get(name, "extends", fallback="")
            for parent in re.split(r"[,\s]+", extends.strip()):
                if parent.startswith("env:") and CAPACITY_ENV.match(parent[len("env:"):]):
                    problems.append(f"[{name}] extends capacity env {parent}")
    if workflow:
        with open(workflow, encoding="utf-8") as f:
            text = f.read()
        for match in re.finditer(r"env:\s*([A-Za-z0-9_-]+)", text):
            if CAPACITY_ENV.match(match.group(1)):
                problems.append(f"{workflow}: release candidate matrix builds {match.group(1)}")
    return problems


def scan(paths):
    problems = []
    for path in paths:
        with open(path, "rb") as f:
            data = f.read()
        for marker in ARTIFACT_MARKERS:
            count = data.count(marker)
            if count:
                problems.append(f"{path}: contains {marker.decode()!r} x{count}")
    return problems


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--config")
    ap.add_argument("--workflow")
    ap.add_argument("--scan", nargs="+", default=[])
    args = ap.parse_args()
    if not args.config and not args.scan:
        ap.error("give --config and/or --scan")
    problems = []
    if args.config:
        problems += check_config(args.config, args.workflow)
    if args.scan:
        problems += scan(args.scan)
    for p in problems:
        print(f"capacity-fixture guard: {p}", file=sys.stderr)
    if not problems:
        what = ([f"config {args.config}"] if args.config else []) + ([f"{len(args.scan)} artifact(s)"] if args.scan else [])
        print(f"capacity-fixture guard: OK ({', '.join(what)})")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
