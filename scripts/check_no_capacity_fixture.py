#!/usr/bin/env python3
"""Keep ON POINT's synthetic capacity fixture and device-test images out of release builds.

Three checks, each failing (exit 1) on any finding:

  --config platformio.ini [--workflow release_candidate.yml]
      Only the dev envs (`x4pro-all-capacity*`, `x4pro-all-devtest`) may name
      the fixture, bench or updates-disabled macros, the fixture sources, or
      the dev build scripts, and no other env may extend a dev env. Every dev
      env must refuse uploads. Every `*devtest` env must define
      CROSSPOINT_FIRMWARE_UPDATES_DISABLED, filter out the firmware-update
      sources and run on_point_devtest.py. No dev env may appear in the
      release candidate workflow's build matrix.

  --scan FILE...
      A built image (.bin / .elf) must not contain the dev markers (route IDs
      `fixture-`, labels `FIXTURE`, the `-CAPACITY` / `-DEVTEST` version
      suffixes). Run it on every release-env build.

  --scan-devtest FILE...
      A device-test image must carry the `-DEVTEST` identity and none of the
      firmware-update code: no update URL, and (in an .elf) no symbol of the
      update modules.
"""

import argparse
import configparser
import re
import struct
import sys

DEV_ENV = re.compile(r"^x4pro-all-(capacity(-[a-z0-9-]+)?|devtest)$")
DEVTEST_ENV = re.compile(r"devtest$")
CONFIG_MARKERS = ("ON_POINT_CAPACITY_FIXTURE", "ON_POINT_BENCH", "ON_POINT_COUNT_COMPARISONS", "test/fixtures",
                  "on_point_capacity_fixture.py", "CROSSPOINT_FIRMWARE_UPDATES_DISABLED", "on_point_devtest.py")
ARTIFACT_MARKERS = (b"fixture-", b"FIXTURE", b"-CAPACITY", b"ON_POINT_CAPACITY", b"DEVTEST")
UPDATE_SOURCES = ("network/OtaUpdater.cpp", "network/FirmwareFlasher.cpp", "network/OtaBootSwitch.cpp",
                  "network/FirmwareBoardTag.cpp", "activities/settings/OtaUpdateActivity.cpp",
                  "activities/settings/SdFirmwareUpdateActivity.cpp")
UPDATE_STRINGS = (b"api.github.com/repos/", b"/releases/latest")
# Mangled (length-prefixed) names of the update modules' classes and namespaces.
UPDATE_SYMBOLS = ("10OtaUpdater", "14firmware_flash", "8ota_boot", "9board_tag", "17OtaUpdateActivity",
                  "24SdFirmwareUpdateActivity")


def resolve(parser, section, key, depth=0):
    """A key's value, following `extends` (PlatformIO inheritance)."""
    if parser.has_option(section, key):
        return parser.get(section, key)
    parent = parser.get(section, "extends", fallback="").strip()
    if parent and depth < 10 and parser.has_section(parent):
        return resolve(parser, parent, key, depth + 1)
    return ""


def expand(parser, value, depth=0):
    """Expand `${section.key}` references (PlatformIO interpolation)."""
    def sub(match):
        section, key = match.group(1), match.group(2)
        found = resolve(parser, section, key) if parser.has_section(section) else ""
        return expand(parser, found, depth + 1) if depth < 10 else found
    return re.sub(r"\$\{([^}.]+(?:\.[^}.]+)*?)\.([A-Za-z0-9_]+)\}", sub, value)


def check_config(path, workflow):
    parser = configparser.ConfigParser(interpolation=None, strict=False, inline_comment_prefixes=(";",))
    parser.optionxform = str
    with open(path, encoding="utf-8") as f:
        parser.read_file(f)
    problems = []
    envs = {s[len("env:"):]: s for s in parser.sections() if s.startswith("env:")}
    if not any(DEV_ENV.match(e) for e in envs):
        problems.append("no x4pro-all-capacity env found (the guard would be vacuous)")
    if not any(DEV_ENV.match(e) and DEVTEST_ENV.search(e) for e in envs):
        problems.append("no devtest env found (the devtest rules would be vacuous)")
    for env_name in envs:
        if DEV_ENV.match(env_name):
            section = f"env:{env_name}"
            command = resolve(parser, section, "upload_command")
            if "exit 1" not in command:
                problems.append(f"[{section}] must refuse uploads (upload_command ... exit 1)")
            if DEVTEST_ENV.search(env_name):
                flags = expand(parser, resolve(parser, section, "build_flags"))
                if "-DCROSSPOINT_FIRMWARE_UPDATES_DISABLED" not in flags:
                    problems.append(f"[{section}] must define CROSSPOINT_FIRMWARE_UPDATES_DISABLED")
                src_filter = expand(parser, resolve(parser, section, "build_src_filter"))
                missing = [s for s in UPDATE_SOURCES if f"-<{s}>" not in src_filter]
                if missing:
                    problems.append(f"[{section}] must filter out {', '.join(missing)}")
                if "on_point_devtest.py" not in expand(parser, resolve(parser, section, "extra_scripts")):
                    problems.append(f"[{section}] must run scripts/on_point_devtest.py")
    for name, section in parser.items():
        dev = name.startswith("env:") and DEV_ENV.match(name[len("env:"):])
        for key, value in parser.items(name) if name != "DEFAULT" else []:
            hits = [m for m in CONFIG_MARKERS if m in value]
            if hits and not dev:
                problems.append(f"[{name}] {key} names {', '.join(hits)} outside a dev env")
            if name.startswith("env:") and not dev and key == "build_src_filter" and "${devtest." in value:
                problems.append(f"[{name}] uses the devtest source filter outside a dev env")
        if name.startswith("env:") and not dev:
            extends = parser.get(name, "extends", fallback="")
            for parent in re.split(r"[,\s]+", extends.strip()):
                if parent.startswith("env:") and DEV_ENV.match(parent[len("env:"):]):
                    problems.append(f"[{name}] extends dev env {parent}")
    if workflow:
        with open(workflow, encoding="utf-8") as f:
            text = f.read()
        for match in re.finditer(r"env:\s*([A-Za-z0-9_-]+)", text):
            if DEV_ENV.match(match.group(1)):
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


def elf_symbols(data):
    """Names in an ELF32 little-endian file's .symtab (empty if there is none)."""
    if data[:4] != b"\x7fELF" or data[4] != 1:
        return []
    shoff, = struct.unpack_from("<I", data, 0x20)
    shentsize, shnum = struct.unpack_from("<HH", data, 0x2E)
    sections = [struct.unpack_from("<IIIIIIIIII", data, shoff + i * shentsize) for i in range(shnum)]
    names = []
    for _, sh_type, _, _, offset, size, link, _, _, entsize in sections:
        if sh_type != 2 or not entsize:  # SHT_SYMTAB
            continue
        str_offset = sections[link][4]
        for i in range(size // entsize):
            name_offset, = struct.unpack_from("<I", data, offset + i * entsize)
            end = data.index(b"\0", str_offset + name_offset)
            names.append(data[str_offset + name_offset:end].decode("latin-1"))
    return names


def scan_devtest(paths):
    problems = []
    for path in paths:
        with open(path, "rb") as f:
            data = f.read()
        if b"-DEVTEST" not in data:
            problems.append(f"{path}: no -DEVTEST identity")
        for marker in UPDATE_STRINGS:
            if marker in data:
                problems.append(f"{path}: contains update string {marker.decode()!r}")
        symbols = elf_symbols(data)
        if path.endswith(".elf") and not symbols:
            problems.append(f"{path}: no symbol table to check")
        hits = sorted({s for s in symbols if any(u in s for u in UPDATE_SYMBOLS)})
        if hits:
            problems.append(f"{path}: {len(hits)} firmware-update symbol(s), e.g. {hits[0]}")
    return problems


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--config")
    ap.add_argument("--workflow")
    ap.add_argument("--scan", nargs="+", default=[])
    ap.add_argument("--scan-devtest", nargs="+", default=[])
    args = ap.parse_args()
    if not args.config and not args.scan and not args.scan_devtest:
        ap.error("give --config, --scan and/or --scan-devtest")
    problems = []
    if args.config:
        problems += check_config(args.config, args.workflow)
    if args.scan:
        problems += scan(args.scan)
    if args.scan_devtest:
        problems += scan_devtest(args.scan_devtest)
    for p in problems:
        print(f"capacity-fixture guard: {p}", file=sys.stderr)
    if not problems:
        what = ([f"config {args.config}"] if args.config else []) + ([f"{len(args.scan)} artifact(s)"] if args.scan else [])
        what += [f"{len(args.scan_devtest)} devtest image(s)"] if args.scan_devtest else []
        print(f"capacity-fixture guard: OK ({', '.join(what)})")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
