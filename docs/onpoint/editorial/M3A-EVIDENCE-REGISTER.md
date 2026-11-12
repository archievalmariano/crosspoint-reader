# ON POINT M3-A evidence register (METRO MANILA)

Facts only, in the format of `docs/onpoint/editorial/M2-EVIDENCE-REGISTER.md`. This is the editorial provenance for the three RRCG
route pairs in the new METRO MANILA collection (`metro-manila`). **Retrieval date for all rows: 2026-11-13.**

No page or feed copies are stored in this repository. The feed snapshot, its digest, the transcription and the research registers are in
the owner's local M3 records. Departures are in `src/activities/on_point/OnPointScheduleData.cpp`, and
`test/on_point_m3/OnPointM3DataTest.cpp` holds a separate transcription of them.

**Tiers:** VERIFIED = operator's own, directly read, dated by the operator. OPERATOR-UNDATED = operator's own current listing with no
date: PROVISIONAL at most. SECONDARY = a dated third party crediting the operator. UNVERIFIED = leads only.

## Primary source

| Item | Finding |
|---|---|
| Source | RRCG's website schedule tool, `https://rrcgbus.ph/wp-json/data/trips` (JSON: departure time, origin and destination terminal, day field `WEEKDAY` / `SAT` / `SUN`) |
| Tier | **OPERATOR-UNDATED.** The feed carries no timetable date. The homepage's modification date (2026-11-09) is a site edit, not a timetable date |
| Snapshot | SHA-256 `be4ad13c13a67fcaf250a724160e182ed99d69eae55c94143af977fc084ce859`. Re-read three times on 2026-11-13 (00:23, 00:35 before implementation, 00:54 before freezing, Manila time): byte-identical each time |
| Why PROVISIONAL | Undated. The same feed lists 10 and 9 daily Batangas and Lipa trips, where RRCG's dated boards of 21 and 28 October 2026 show 12 each, so the feed's currency can't be established |
| Effective date | 2026-11-13, the retrieval date |

## Route pairs

All three pairs are **PROVISIONAL** in both directions. Each direction has three rules: Monday to Friday, Saturday, and Sunday. ATC ↔ SM
Megamall has no Sunday rule, so the board shows no service on Sundays.

| Pair (route ID) | Device labels | Weekday | Saturday | Sunday | Terminal notes |
|---|---|---|---|---|---|
| `p2p-atc-greenbelt` | ATC ALABANG ↔ GREENBELT | 33 / 32 | 28 / 28 | 16 / 16 | **Weekdays: Greenbelt 1. Weekends: Greenbelt 5** (both in Greenbelt, Ayala Center, Makati). The label GREENBELT is correct on every day. The building split is in the stored notes. Collection default |
| `p2p-starmall-alabang-starmall-shaw` | VTX ALABANG ↔ STARMALL SHAW | 33 / 32 | 21 / 20 | 20 / 19 | **VTX ALABANG** is the terminal's own name (Vista Terminal Exchange). **"Starmall Alabang" is RRCG's name for it and a documented alias.** It is kept in the route ID and the stored notes |
| `p2p-atc-sm-megamall` | ATC ALABANG ↔ SM MEGAMALL | 17 / 17 | 15 / 15 | none | Ortigas end: SM Megamall (see the conflict below) |

Counts are outbound / return. First and last trips:

- ATC → Greenbelt: weekday 05:30–21:30, Saturday 05:30–21:00, Sunday 06:00–21:00. Return: weekday 06:30–22:00, Saturday 06:30–22:00, Sunday 07:00–22:00.
- VTX → Shaw: weekday 05:30–21:30, Saturday 05:30–21:00, Sunday 06:00–20:30. Return: weekday 06:30–22:00, Saturday 06:30–22:00, Sunday 07:00–21:30.
- ATC → SM Megamall: weekday 06:00–20:30, Saturday 06:00–20:00. Return: weekday 07:00–22:00, Saturday 07:00–21:00.

## Supporting and conflicting sources

| Source | Date | Tier | Use |
|---|---|---|---|
| Pamasahe, "RRCG Transport announces holiday schedule for Eid'l Fitr" | 2026-03-18 | SECONDARY (reproduces an RRCG advisory) | All three services exist: "Alabang (ATC) – Makati" and "Alabang–Shaw" limited trips, "Alabang (ATC) – Ortigas (Megamall)" suspended for the day |
| Top Gear, Holy Week 2026 P2P schedules | 2026-03-31 | SECONDARY (credits RRCG) | ATC–Greenbelt 5 and Starmall Alabang–Starmall EDSA-Shaw. Holy Week closures and reduced weekend service |
| Pamasahe route pages, Alabang–Ortigas and Alabang–EDSA-Shaw | "Last updated September 2026" | UNVERIFIED | Lists labelled "daily" that match the feed's weekday (Ortigas) or Sunday (Shaw) lists, with small differences. Not used |
| EDSA Bus, "RRCG Transport Premium P2P Service – Alabang to Ortigas" | Undated (indexed about 2026-11-07) | UNVERIFIED | **Conflict:** it describes ATC ↔ Starmall EDSA-Shaw via SM Megamall. Its stops match the Starmall Alabang–Shaw service, so it appears to merge two routes. Both sources of operator origin say SM Megamall. Re-checked 2026-11-13 before freezing: no dated RRCG notice says otherwise |

## Holidays

No holiday overrides. RRCG publishes no standing holiday policy, only per-holiday advisories (for example Holy Week closures and limited
trips on Eid'l Fitr in 2026). The board therefore shows the ordinary weekday, Saturday or Sunday list on every date, including holidays and
Metro Manila special non-working days such as 16–18 November 2026. Every direction's notes say "Holiday service may differ."

## Not included

RRCG's Batangas, Lipa and Rizal routes (later collections). RRCG's one-way Galleria → Masinag trip (no mirrored direction). DNS Katipunan and
Fairview ↔ One Ayala (no current dated lists). Circuit Makati ↔ One Ayala (deferred).
