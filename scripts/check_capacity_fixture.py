#!/usr/bin/env python3
"""Post-link check of an ON POINT capacity build (x4pro-all-capacity).

Confirms the linked image really holds the whole synthetic fixture, so a
capacity measurement can't come out optimistic through merged or dropped data:

  - exactly 360 departure-array symbols FIXTURE_DEP_<s>_<r> in .flash.rodata,
    each count*2 bytes, at 360 distinct addresses, 85,920 bytes in total;
  - 120 FIXTURE_RULES_* and 120 FIXTURE_OVERRIDES_* tables;
  - the image holds every distinct fixture string the generator expects
    (none merged away);
  - optionally, .flash.rodata grew by at least the fixture's raw bytes versus a
    baseline ELF of the same package.

Usage: check_capacity_fixture.py --elf firmware.elf --bin firmware.bin
         --objdump <xtensa objdump> [--baseline-elf base.elf] [--json out.json]
Exit 1 on any finding.
"""

import argparse
import json
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
GEN = os.path.join(HERE, "..", "test", "fixtures", "on_point", "gen_capacity_fixture.py")

STRING_PATTERNS = {
    "origin": rb"FIXTURE ORIGIN \d{3}\.+",
    "destination": rb"FIXTURE DEST \d{3}-[AB]\.+",
    "operator": rb"FIXTURE OPERATOR \d{3}\.+",
    "note": rb"FIXTURE NOTE \d{3} [ -~]+?\.+(?=\x00)",
    "area": rb"FIXTURE AREA \d{3}-[OD]\.+",
    "id": rb"fixture-[a-z0-9-]+",
}


def expected():
    out = subprocess.check_output([sys.executable, GEN, "--report"], text=True)
    return json.loads(out)


SYMBOL_LINE = re.compile(r"^([0-9a-f]{8})\s.*?\s(\.\S+)\s+([0-9a-f]{8})\s+(.*)$")


def symbols(objdump, elf):
    """(name, section, address, size) for every FIXTURE_* symbol.

    Demangled names contain spaces ("(anonymous namespace)"), so parse the
    fixed objdump -t columns rather than splitting on whitespace.
    """
    text = subprocess.check_output([objdump, "-t", "-C", elf], text=True)
    rows = []
    for line in text.splitlines():
        if "FIXTURE_" not in line:
            continue
        match = SYMBOL_LINE.match(line)
        if match:
            rows.append((match.group(4).strip(), match.group(2), int(match.group(1), 16), int(match.group(3), 16)))
    return rows


def section_size(objdump, elf, name):
    text = subprocess.check_output([objdump, "-h", elf], text=True)
    for line in text.splitlines():
        parts = line.split()
        if len(parts) >= 3 and parts[1] == name:
            return int(parts[2], 16)
    return None


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--elf", required=True)
    ap.add_argument("--bin", required=True)
    ap.add_argument("--objdump", required=True)
    ap.add_argument("--baseline-elf")
    ap.add_argument("--json")
    args = ap.parse_args()

    exp = expected()
    problems = []
    rows = symbols(args.objdump, args.elf)

    deps = [(n, s, a, z) for n, s, a, z in rows if re.search(r"FIXTURE_DEP_\d+_\d$", n)]
    want_sizes = {int(k[len("rule"):]): v for k, v in exp["arraySizes"].items()}
    for name, section, addr, size in deps:
        rule = int(name.rsplit("_", 1)[1])
        if section != ".flash.rodata":
            problems.append(f"{name} in {section}, not .flash.rodata")
        if size != want_sizes.get(rule):
            problems.append(f"{name} is {size} B, want {want_sizes.get(rule)}")
    if len(deps) != exp["departureArrays"]:
        problems.append(f"{len(deps)} departure-array symbols, want {exp['departureArrays']}")
    if len({a for _, _, a, _ in deps}) != len(deps):
        problems.append("departure arrays share addresses (merged)")
    dep_bytes = sum(z for _, _, _, z in deps)
    if dep_bytes != exp["departureArrayBytes"]:
        problems.append(f"departure arrays total {dep_bytes} B, want {exp['departureArrayBytes']}")
    rules = [r for r in rows if re.search(r"FIXTURE_RULES_\d+$", r[0])]
    overrides = [r for r in rows if re.search(r"FIXTURE_OVERRIDES_\d+$", r[0])]
    for label, found in (("rules", rules), ("overrides", overrides)):
        if len(found) != exp["schedules"]:
            problems.append(f"{len(found)} {label} tables, want {exp['schedules']}")
    override_bytes = sum(z for _, _, _, z in overrides)
    rule_bytes = sum(z for _, _, _, z in rules)
    other = {n: z for n, _, _, z in rows if n.endswith("FIXTURE_SCHEDULES") or n.endswith("FIXTURE_ROUTES")}

    with open(args.bin, "rb") as f:
        image = f.read()
    strings = {}
    for key, pattern in STRING_PATTERNS.items():
        found = set(re.findall(pattern, image))
        strings[key] = len(found)
        want = exp["distinctStrings"][key]
        if len(found) != want:
            problems.append(f"{len(found)} distinct {key} strings in the image, want {want}")

    raw = dep_bytes + override_bytes + rule_bytes + sum(other.values()) + exp["stringBytes"]
    result = {
        "departureArrays": len(deps),
        "departureArrayBytes": dep_bytes,
        "rulesTables": len(rules),
        "rulesBytes": rule_bytes,
        "overrideTables": len(overrides),
        "overrideBytes": override_bytes,
        "scheduleAndRouteTables": other,
        "distinctStrings": strings,
        "fixtureRawBytes": raw,
    }
    if args.baseline_elf:
        base = section_size(args.objdump, args.baseline_elf, ".flash.rodata")
        cap = section_size(args.objdump, args.elf, ".flash.rodata")
        result["rodataBaseline"] = base
        result["rodataCapacity"] = cap
        result["rodataGrowth"] = cap - base
        if cap - base < raw - 4096:  # 4 KB slack: the 7 real routes it replaces leave the image
            problems.append(f".flash.rodata grew {cap - base} B, less than the fixture's raw {raw} B")

    result["problems"] = problems
    if args.json:
        with open(args.json, "w", encoding="utf-8") as f:
            json.dump(result, f, indent=2)
    print(json.dumps(result, indent=2))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
