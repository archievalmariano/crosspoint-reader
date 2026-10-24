#!/usr/bin/env python3
"""Generate ON POINT's synthetic worst-case capacity fixture (TEST/CAPACITY ONLY).

Writes a C++ translation unit that replaces OnPointScheduleData.cpp in host
capacity tests and the dev-only `x4pro-all-capacity` firmware env. It is never
part of a release build (see scripts/check_no_capacity_fixture.py). Nothing in
it is a real route, operator or time: every ID starts with `fixture-`, every
label with `FIXTURE`.

Shape (Stage 1 plan, Revision 3, section 7):
  60 route pairs = 120 directional schedules, each with its own arrays.
  Rules: weekday (Mon-Fri) 130 trips from 04:00, Saturday 120 from 05:00,
  Sunday 108 from 06:00; element i = first + 10*i + jitter, jitter in 0..8 from
  32-bit FNV-1a of (s, r, i), so every array is unique and strictly increasing.
  40 date overrides per schedule (1st and 15th, Jan-Oct, 2026 and 2027); the
  15 Mar and 15 Sep of each year are explicit non-service days, the rest use
  the schedule's own Sunday array.
  Strings at their worst-case lengths; routes come in 20 groups of 3 sharing an
  origin, two of each group also sharing a destination (ID tie-break).

Collections: 6 over the 60 routes. Collection k holds routes 10k..10k+14
(mod 60), so neighbours share 5 routes; collection 0 also holds 30..34, so
those are in 3 collections. Every route is in at least one.

The generator checks its own output (uniqueness, validity) and exits non-zero
on any violation. `--report` prints the expected totals as JSON.
"""

import argparse
import json
import struct
import sys

PAIRS = 60
SCHEDULES = 2 * PAIRS
# (weekday mask, first minute, count) for weekday, Saturday, Sunday.
RULES = [(0b0111110, 240, 130), (0b1000000, 300, 120), (0b0000001, 360, 108)]
MAX_SERVICE_MINUTE = 2879
OVERRIDE_YEARS = (2026, 2027)
OVERRIDE_MONTHS = range(1, 11)
OVERRIDE_DAYS = (1, 15)
EMPTY_OVERRIDES = {(3, 15), (9, 15)}
ID_MAX = 47  # CrossPointSettings::onPointMainRouteId is char[48]
LABEL_LEN = 24
NOTE_LEN = 160


def fnv1a32(data):
    h = 0x811C9DC5
    for b in data:
        h ^= b
        h = (h * 0x01000193) & 0xFFFFFFFF
    return h


def departures(s, r):
    _, first, count = RULES[r]
    return [first + 10 * i + fnv1a32(struct.pack("<III", s, r, i)) % 9 for i in range(count)]


def pad(text, length, fill="."):
    if len(text) > length:
        raise ValueError(f"{text!r} longer than {length}")
    return text + fill * (length - len(text))


def pair_id(k):
    # Half the pairs at the settings buffer's maximum, half short (16 chars).
    return pad(f"fixture-{k:03d}-", ID_MAX, "x") if k % 2 == 0 else pad(f"fixture-{k:03d}-", 16, "s")


def return_id(k):
    return pad(f"fixture-ret-{k:03d}-", ID_MAX if k % 2 == 0 else 20, "r")


def origin_label(group):
    return pad(f"FIXTURE ORIGIN {group:03d}", LABEL_LEN)


def destination_label(group, member):
    return pad(f"FIXTURE DEST {group:03d}-{'A' if member < 2 else 'B'}", LABEL_LEN)


def operator_label(s):
    return pad(f"FIXTURE OPERATOR {s:03d}", LABEL_LEN)


def area_label(k, side):
    return pad(f"FIXTURE AREA {k:03d}-{side}", LABEL_LEN)


def note(s):
    return pad(f"FIXTURE NOTE {s:03d} synthetic capacity schedule, not a real timetable.", NOTE_LEN)


def override_dates():
    return [(y, m, d) for y in OVERRIDE_YEARS for m in OVERRIDE_MONTHS for d in OVERRIDE_DAYS]


def build():
    pairs = []
    for k in range(PAIRS):
        group, member = divmod(k, 3)
        out_s, ret_s = 2 * k, 2 * k + 1
        origin, dest = origin_label(group), destination_label(group, member)
        pairs.append({
            "k": k, "id": pair_id(k), "out": out_s, "ret": ret_s,
            "originArea": area_label(k, "O"), "destinationArea": area_label(k, "D"),
            "schedules": [
                {"s": out_s, "id": pair_id(k), "origin": origin, "destination": dest},
                {"s": ret_s, "id": return_id(k), "origin": dest, "destination": origin},
            ],
        })
    return pairs


COLLECTIONS = 6


def collections():
    """(id, member route indices, default route index) per collection."""
    result = []
    for k in range(COLLECTIONS):
        members = [(10 * k + i) % PAIRS for i in range(15)]
        if k == 0:
            members += list(range(30, 35))
        result.append((f"fixture-collection-{k}", members, members[0]))
    return result


def check(pairs):
    problems = []
    groups = collections()
    covered = {m for _, members, _ in groups for m in members}
    if covered != set(range(PAIRS)):
        problems.append("collections don't cover every route")
    if any(len(set(members)) != len(members) or not 10 <= len(members) <= 20 for _, members, _ in groups):
        problems.append("a collection has duplicate members or is outside 10..20 routes")
    if max(sum(r in members for _, members, _ in groups) for r in range(PAIRS)) < 3:
        problems.append("no route is in three collections")
    arrays = {}
    for p in pairs:
        for sch in p["schedules"]:
            for r, (_, _, count) in enumerate(RULES):
                dep = departures(sch["s"], r)
                if len(dep) != count:
                    problems.append(f"schedule {sch['s']} rule {r}: {len(dep)} != {count}")
                if any(b <= a for a, b in zip(dep, dep[1:])):
                    problems.append(f"schedule {sch['s']} rule {r}: not strictly increasing")
                if dep[0] < 0 or dep[-1] > MAX_SERVICE_MINUTE:
                    problems.append(f"schedule {sch['s']} rule {r}: out of range")
                arrays.setdefault(tuple(dep), []).append((sch["s"], r))
    duplicates = {k: v for k, v in arrays.items() if len(v) > 1}
    if len(arrays) != SCHEDULES * len(RULES) or duplicates:
        problems.append(f"{len(arrays)} distinct arrays (want {SCHEDULES * len(RULES)}); duplicates {list(duplicates.values())[:3]}")
    masks = [m for m, _, _ in RULES]
    if any(a & b for i, a in enumerate(masks) for b in masks[i + 1:]) or sum(masks) != 0b1111111:
        problems.append("rule masks overlap or don't cover the week")
    ids = [p["id"] for p in pairs] + [p["schedules"][1]["id"] for p in pairs]
    if len(set(ids)) != len(ids):
        problems.append("duplicate route IDs")
    if any(len(i) > ID_MAX for i in ids):
        problems.append("route ID longer than the settings buffer")
    dates = override_dates()
    if len(set(dates)) != len(dates) or dates != sorted(dates):
        problems.append("override dates not unique and sorted")
    return problems


def expected(pairs):
    strings = {
        "origin": {s["origin"] for p in pairs for s in p["schedules"] if s["origin"].startswith("FIXTURE ORIGIN")},
        "destination": {s["destination"] for p in pairs for s in p["schedules"] if s["destination"].startswith("FIXTURE DEST")},
        "operator": {operator_label(s) for s in range(SCHEDULES)},
        "note": {note(s) for s in range(SCHEDULES)},
        "area": {p[a] for p in pairs for a in ("originArea", "destinationArea")},
        "id": {p["id"] for p in pairs} | {p["schedules"][1]["id"] for p in pairs},
    }
    departures_total = SCHEDULES * sum(c for _, _, c in RULES)
    return {
        "pairs": PAIRS,
        "schedules": SCHEDULES,
        "departureArrays": SCHEDULES * len(RULES),
        "departuresTotal": departures_total,
        "departureArrayBytes": departures_total * 2,
        "arraySizes": {f"rule{r}": RULES[r][2] * 2 for r in range(len(RULES))},
        "overridesPerSchedule": len(override_dates()),
        "emptyOverridesPerSchedule": sum(1 for y, m, d in override_dates() if (m, d) in EMPTY_OVERRIDES),
        "distinctStrings": {k: len(v) for k, v in strings.items()},
        "stringBytes": sum(len(x) + 1 for v in strings.values() for x in v),
    }


def cstr(text):
    return '"' + text.replace("\\", "\\\\").replace('"', '\\"') + '"'


def emit(pairs):
    out = []
    w = out.append
    w("// GENERATED by test/fixtures/on_point/gen_capacity_fixture.py -- do not edit.")
    w("// TEST/CAPACITY ONLY: synthetic ON POINT catalogue, never in a release build.")
    w("#ifndef ON_POINT_CAPACITY_FIXTURE")
    w('#error "ON POINT capacity fixture: host capacity tests and x4pro-all-capacity only"')
    w("#endif")
    w("")
    w("#include <cstring>")
    w("#include <iterator>")
    w("")
    w('#include "activities/on_point/OnPointRegions.h"')
    w('#include "activities/on_point/OnPointScheduleData.h"')
    w("")
    w("namespace on_point {")
    w("namespace {")
    w("")
    w(f"constexpr size_t FIXTURE_SCHEDULE_COUNT = {SCHEDULES};")
    w(f"constexpr size_t FIXTURE_ROUTE_COUNT = {PAIRS};")
    w("")
    for s in range(SCHEDULES):
        for r in range(len(RULES)):
            dep = departures(s, r)
            w(f"constexpr uint16_t FIXTURE_DEP_{s}_{r}[] = {{{', '.join(map(str, dep))}}};")
    w("")
    for s in range(SCHEDULES):
        rules = ", ".join(f"{{0b{m:07b}, {{FIXTURE_DEP_{s}_{r}, {c}}}}}" for r, (m, _, c) in enumerate(RULES))
        w(f"constexpr ServiceRule FIXTURE_RULES_{s}[] = {{{rules}}};")
    w("")
    for s in range(SCHEDULES):
        entries = []
        for (y, m, d) in override_dates():
            if (m, d) in EMPTY_OVERRIDES:
                entries.append(f"{{{{{y}, {m}, {d}}}, {{nullptr, 0}}}}")
            else:
                entries.append(f"{{{{{y}, {m}, {d}}}, {{FIXTURE_DEP_{s}_2, {RULES[2][2]}}}}}")
        w(f"constexpr DateOverride FIXTURE_OVERRIDES_{s}[] = {{{', '.join(entries)}}};")
    w("")
    w("constexpr Schedule FIXTURE_SCHEDULES[FIXTURE_SCHEDULE_COUNT] = {")
    for p in pairs:
        for sch in p["schedules"]:
            s = sch["s"]
            w(f"    {{{cstr(sch['id'])}, {cstr(operator_label(s))}, {cstr(sch['origin'])}, {cstr(sch['destination'])},")
            w(f'     "Asia/Manila", 8 * 60, {{2026, 1, 1}}, FIXTURE_RULES_{s}, std::size(FIXTURE_RULES_{s}),')
            w(f"     FIXTURE_OVERRIDES_{s}, std::size(FIXTURE_OVERRIDES_{s}), {cstr(note(s))}, SourceStatus::Provisional}},")
    w("};")
    w("")
    w("constexpr RoutePair FIXTURE_ROUTES[FIXTURE_ROUTE_COUNT] = {")
    for p in pairs:
        w(f"    {{{cstr(p['id'])}, {p['out']}, {p['ret']}, {cstr(p['originArea'])}, {cstr(p['destinationArea'])}}},")
    w("};")
    w("")
    w("static_assert(std::size(FIXTURE_SCHEDULES) == FIXTURE_SCHEDULE_COUNT);")
    w("static_assert(std::size(FIXTURE_ROUTES) == FIXTURE_ROUTE_COUNT);")
    w("")
    w("}  // namespace")
    w("")
    w("// Same contract as OnPointScheduleData.cpp, over the fixture catalogue.")
    w("const Schedule& scheduleAt(const size_t index) {")
    w("  return FIXTURE_SCHEDULES[index < FIXTURE_SCHEDULE_COUNT ? index : 0];")
    w("}")
    w("size_t routeCount() { return FIXTURE_ROUTE_COUNT; }")
    w("const RoutePair& routeAt(const size_t index) { return FIXTURE_ROUTES[index < FIXTURE_ROUTE_COUNT ? index : 0]; }")
    w("const Schedule& scheduleForRoute(const size_t routeIndex, const bool reversed) {")
    w("  const RoutePair& route = routeAt(routeIndex);")
    w("  return scheduleAt(reversed ? route.returnScheduleIndex : route.outboundScheduleIndex);")
    w("}")
    w("size_t routeIndexInAlphabeticalOrder(const size_t position) {")
    w("  if (position >= FIXTURE_ROUTE_COUNT) return 0;")
    w("  for (size_t candidate = 0; candidate < FIXTURE_ROUTE_COUNT; ++candidate) {")
    w("    const Schedule& candidateSchedule = scheduleAt(FIXTURE_ROUTES[candidate].outboundScheduleIndex);")
    w("    size_t earlierRoutes = 0;")
    w("    for (size_t other = 0; other < FIXTURE_ROUTE_COUNT; ++other) {")
    w("      if (other == candidate) continue;")
    w("      const Schedule& otherSchedule = scheduleAt(FIXTURE_ROUTES[other].outboundScheduleIndex);")
    w("      int comparison = strcmp(otherSchedule.origin, candidateSchedule.origin);")
    w("      if (comparison == 0) comparison = strcmp(otherSchedule.destination, candidateSchedule.destination);")
    w("      if (comparison == 0) comparison = strcmp(FIXTURE_ROUTES[other].routeId, FIXTURE_ROUTES[candidate].routeId);")
    w("      if (comparison < 0) ++earlierRoutes;")
    w("    }")
    w("    if (earlierRoutes == position) return candidate;")
    w("  }")
    w("  return 0;")
    w("}")
    w("size_t routeIndexForId(const char* routeId) {")
    w("  if (routeId) {")
    w("    for (size_t index = 0; index < FIXTURE_ROUTE_COUNT; ++index) {")
    w("      if (strcmp(FIXTURE_ROUTES[index].routeId, routeId) == 0) return index;")
    w("    }")
    w("  }")
    w("  return 0;")
    w("}")
    w("")
    w("// Same contract as OnPointRegionData.cpp: overlapping collections over the fixture.")
    w("namespace {")
    for k, (_, members, _) in enumerate(collections()):
        w(f"constexpr uint8_t FIXTURE_COLLECTION_{k}[] = {{{', '.join(map(str, members))}}};")
    w("constexpr Region FIXTURE_REGIONS[] = {")
    for k, (cid, _, default) in enumerate(collections()):
        w(f"    {{{cstr(cid)}, FIXTURE_COLLECTION_{k}, std::size(FIXTURE_COLLECTION_{k}), {cstr(pairs[default]['id'])}}},")
    w("};")
    w("}  // namespace")
    w("const Region* regionTable() { return FIXTURE_REGIONS; }")
    w("size_t regionTableSize() { return std::size(FIXTURE_REGIONS); }")
    w("")
    w("}  // namespace on_point")
    return "\n".join(out) + "\n"


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--out", help="write the C++ fixture here")
    ap.add_argument("--report", action="store_true", help="print the expected totals as JSON")
    args = ap.parse_args()
    pairs = build()
    problems = check(pairs)
    if problems:
        for p in problems:
            print(f"fixture check failed: {p}", file=sys.stderr)
        return 1
    if args.out:
        text = emit(pairs)
        try:
            with open(args.out, encoding="utf-8") as f:
                if f.read() == text:
                    return 0 if not args.report else print(json.dumps(expected(pairs), indent=2)) or 0
        except FileNotFoundError:
            pass
        with open(args.out, "w", encoding="utf-8") as f:
            f.write(text)
    if args.report:
        print(json.dumps(expected(pairs), indent=2))
    return 0


if __name__ == "__main__":
    sys.exit(main())
