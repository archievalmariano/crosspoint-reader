# BULACAN implementation readiness (passes 4–5)

**Basis:** the reconciliation at `befc5eeb` ([BULACAN-RECONCILIATION.md](BULACAN-RECONCILIATION.md)), as accepted by
the owner, re-assessed under the owner's standard of 2026-10-25. Documentation only. No firmware data, status label,
holiday rule or route entry was changed. No build, flash, push or release.

## Standard

| Recommendation | Meaning |
|---|---|
| **PUBLISHED** | The departure pattern and service-day rules are operator-backed, with no material unresolved conflict |
| **PROVISIONAL** | A credible, defensible departure sequence from operator windows plus secondary timetable information. The uncertainty is disclosed. |
| **NOT COUNTDOWN-READY** | Too little information to determine the actual departures. No intermediate trips are invented. |

**Rules applied to every row:**
- The operator's first and last trips are used **exactly as stated**.
- The secondary source supplies only the departure **pattern** inside that window. P2P Bus PH is undated and was
  re-read on 2026-10-25.
- A detailed but undated directory is never treated as current just because it is detailed. That covers P2P Bus PH's
  midday gaps and Sakay's 75-minute and hourly lists. These are recorded as contradictions, not adopted.

## Evidence now in hand

**Bulacan P2P:** post BP2P-0830 (images -1 to -5; reconciliation §1.1).
- Read directly by Claude, and **identical** to the owner's screenshots.
- Displayed as "August 30" with no year; 2026 is inferred from Facebook's display convention.
- No image states an interval.

**Precious Grace:** owner screenshots, **now seen**. Facts-only transcription:

| ID | Design | Heading | Rows (first – last) | Interval | Date evidence |
|---|---|---|---|---|---|
| **PG-G1** | White "POINT TO POINT" | — (no day wording) | CAYPOMBO/STA. CLARA/MALOLOS 05:00–20:00 · BOCAUE 05:30–20:00 · NORTH EDSA (TRINOMA AND SM) 06:00–22:30 | 30 min | None visible |
| **PG-G2** | Yellow Precious Grace brand, "PREMIUM POINT TO POINT BUS" | **"P2P DAILY TRIP SCHEDULE"** | CAYPOMBO/STA. CLARA 05:00–**20:30** · MALOLOS 05:00–20:00 · BOCAUE 05:30–20:00 · NORTH EDSA (TRINOMA AND SM) 06:00–22:30 | 30 min | None visible (shown in the PG page frame, with a "BOOK NOW" QR code) |
| **PG-ANGAT-W** | White | ANGAT-MONUMENTO | ANGAT 03:30–04:00 to 19:00 · MONUMENTO 05:30 to 23:00 | **10–30 min** | None |
| **PG-ANGAT-Y** | Yellow brand | "ANGAT-MONUMENTO DAILY TRIP SCHEDULE" | Same values | 10–30 min | **Post caption, partly cropped: "…of May 1, 2026 ang ating official Last Trip mula Monumento ay 11 PM."** 389 reactions, 42 comments, 64 shares. |

**Chronology (pass 5): established from owner screenshots of the Precious Grace page.** These are screenshots of the
page, not opened by Claude. The permalinks are not captured.

| Post | Post date as displayed | Year | Contents | Operator effective date | Supersession |
|---|---|---|---|---|---|
| **PG-P-20251230** | "December 30, 2025" | **Confirmed** (shown) | "Our 2026 Trip Schedule is out! … Point-to-Point and **Route 9 (Angat–Monumento)**". The images are **PG-G1** and **PG-ANGAT-W**. 134 reactions, 18 comments, 37 shares. | "2026 Trip Schedule". No start day given, so "from 2026" is all that is stated. | First 2026 schedule |
| **PG-P-0313** | "March 13" (a Featured post) | **Inferred 2026.** Facebook omits the current year, and it must postdate the December 2025 post, which shows its year. | **PG-G2** | Not stated | **Supersedes G1 for Caypombo/Sta. Clara (last trip 20:00 → 20:30)**, unless a later contrary post exists. Malolos, Bocaue and North EDSA are unchanged. |
| **PG-P-0501** | "May 1" (Featured; the screenshot reads "May 1") | **Inferred 2026**, consistent with its own text | "As of May 1, 2026 ang ating official Last Trip mula Monumento ay 11 PM." The image is **PG-ANGAT-Y**. | **2026-05-01, stated** (Monumento last trip) | Confirms the 23:00 Monumento last trip. The same value as December. |

**Not checked:**
- **Whether any post after May 2026 changes the P2P schedule.** The Featured section showed none, but the full post
  history is behind the login wall.
- **This is the "unless a later contrary announcement exists" condition.** It is recorded as residual, low-risk
  uncertainty.

**Scope flag:**
- **The operator itself separates "Point-to-Point" from "Route 9 (Angat–Monumento)".**
- Route 9 is a numbered route with a 10–30-minute interval. It is not presented as P2P.
- It stays an approved coverage candidate and NOT COUNTDOWN-READY. The owner may also want to revisit whether it
  belongs in a P2P-only catalogue.

**Local evidence copies** (outside Git, not published): `_worktrees/records/bulacan-evidence/2026-10-25-owner-screenshots/`.
It holds the nine screenshots with `SHA256SUMS`: G1 `52e86139…`, G2 `720155a1…`, Angat-Y `2ad63398…`.

## Readiness table

Day types: **Mon**, **Tue–Fri**, **SSH** (Sat/Sun/Holiday), **Daily**. "every 30" means every 30 minutes from the
first trip to the last, inclusive. Counts are trips.

| Route pair | Direction | Recommendation | Candidate pattern | Provenance | Contradictions / disclosure |
|---|---|---|---|---|---|
| **Balagtas ↔ TriNoma** (BP2P) | Balagtas → TriNoma | **PROVISIONAL** | Mon–Fri 04:30–20:00 every 30 (32) · SSH 05:00–20:00 every 30 (31). **This is the shipped data, unchanged.** | Windows and the Mon = Tue–Fri equality: BP2P-0830-5, -3, -2. Pattern: P2PBUS weekday list (matches exactly). | No source lists weekend departures; the weekend pattern applies the weekday interval. Bocaue Crossing is a stop (O1): its row equals Balagtas's window. Sakay: 04:00 first trip (undated). |
| | TriNoma → Balagtas | **PROVISIONAL** | Mon–Fri 06:00–22:30 every 30 (34) · SSH 06:30–22:30 every 30 (33). **Shipped, unchanged.** | BP2P-0830-4, -1. P2PBUS (matches exactly). | Weekend pattern as above. Sakay: 05:15 first trip (undated). |
| **Pandi ↔ TriNoma** (BP2P) | Pandi → TriNoma | **PROVISIONAL** | **Mon 03:30–19:30 every 30 (33)** · **Tue–Fri 04:30–19:30 every 30 (31)** · SSH 04:30–19:30 every 30 (31) | Windows: BP2P-0830-5, -3, -2. Pattern: P2PBUS (04:30…19:00, every 30). | P2PBUS stops at 19:00, but the operator's 19:30 last trip governs. **Monday 04:00 appears in no source.** It is pattern-continued between the operator's 03:30 and 04:30 trips; disclose it. Weekend pattern assumed. Sakay: 04:00–19:30 daily (undated). Boarding point (Mapulang Lupa or Poblacion) unconfirmed. |
| | TriNoma → Pandi | **PROVISIONAL** | Mon–Fri 06:00–21:30 every 30 (32) · SSH 06:30–21:30 every 30 (31) | BP2P-0830-4, -1. P2PBUS pattern. | P2PBUS stops at 21:00; the operator's 21:30 governs. Weekend pattern assumed. |
| **Plaridel ↔ TriNoma** (BP2P) | Plaridel → TriNoma | **PROVISIONAL** | **Mon 03:30–20:30 every 30 (35)** · **Tue–Fri 04:00–20:30 every 30 (34)** · SSH 04:30–20:30 every 30 (33) | Window: BP2P-0830-5, -3, -2. The "PLARIDEL" row names **no destination**. Pattern: P2PBUS Plaridel → TriNoma (04:00–20:30, every 30), which **matches the Tue–Fri window exactly**. | **The operator doesn't split Plaridel departures between TriNoma and SM North.** Some may run to SM North (O3). The times are departures from Plaridel toward North Avenue; disclose this. Sakay: 04:00, 05:15 … 20:15 every 75 min (undated, contradicts). Monday 03:30 pattern-continued. |
| | TriNoma → Plaridel | **PROVISIONAL** | Mon–Fri 06:00–21:00 every 30 (31) · SSH 06:30–21:00 every 30 (30) | BP2P-0830-4, -1. P2PBUS pattern. | P2PBUS lists 05:30–22:00, outside the operator window at both ends; the operator governs. Sakay's 75-minute interval contradicts. |
| **Plaridel ↔ SM North EDSA** (BP2P) | Plaridel → SM North | **NOT COUNTDOWN-READY** | None | No operator window for this destination | **Genuinely missing:** which Plaridel departures go to SM North. P2PBUS (05:30–22:30, Mon–Sun) is implausible against the operator's 20:30 Plaridel last trip. Reusing the Plaridel array would show the same buses on two routes. |
| | SM North → Plaridel | PROVISIONAL *(on hold)* | Mon–Fri 06:00–22:30 every 30 (34) · SSH 06:30–22:30 every 30 (33) | BP2P-0830-4, -1. P2PBUS pattern. | P2PBUS starts at 05:30; the operator's 06:00 governs. **On hold** because a `RoutePair` needs both directions, and the outbound is not ready. |
| **Guiguinto ↔ TriNoma** (BP2P) | WalterMart Guiguinto → TriNoma | **NOT COUNTDOWN-READY** | None | Windows: BP2P-0830-5, -3, -2 (Mon/Tue–Fri 04:30–19:30, SSH 05:00–19:30) | **Genuinely missing:** any departure list or interval for this route. P2PBUS has no page; Sakay gives ranges only. The windows fall on :00 and :30, which is consistent with 30 minutes but doesn't establish it. |
| | TriNoma → Guiguinto | **NOT COUNTDOWN-READY** | None | BP2P-0830-4, -1 (Mon–Fri 06:00–21:30, SSH 06:30–21:30) | As above. *Owner option, not recommended:* accept Bulacan P2P's network-wide 30-minute pattern, which would make it PROVISIONAL. |
| **Caypombo ↔ SM North EDSA** (PG) | Caypombo → SM North | **PUBLISHED** *(pass 5; was PROVISIONAL)* | **Daily 05:00–20:30 every 30 (32)**, per PG-G2. **The shipped array (05:00–20:00, 31) is superseded**: it omits the operator's 20:30 last trip. | Operator window, interval and days: PG-G2 (PG-P-0313, "DAILY", "INTERVAL 30 MINUTES"). It supersedes PG-G1 (PG-P-20251230). | **The G1 vs G2 conflict is resolved by chronology.** Residual: no later post was checked (see above). Times are at the Caypombo / Sta. Clara boarding area; Sta. Clara is a stop. Undated secondary contradictions are not adopted: P2PBUS 19:45 with an hourly midday; directories' weekend 06:00; Sakay's "UBE Express". |
| | SM North → Caypombo | **PROVISIONAL** (unchanged) | Daily 06:00–22:30 every 30 (34). **Shipped, unchanged.** | PG-G1 and PG-G2 agree (both dated) | **"NORTH EDSA (TRINOMA AND SM)" is one row for every PG destination.** It doesn't say which stop the 06:00 departs from, or whether each destination gets a bus every 30 minutes. P2PBUS shows an hourly midday (undated). |
| **Malolos ↔ TriNoma** (PG) | Malolos → TriNoma | **PUBLISHED** (confirmed in pass 5) | Daily 05:00–20:00 every 30 (31) | Operator-stated window, interval and days: PG-G2 (PG-P-0313, "DAILY"). **PG-G1 (PG-P-20251230) agrees**, so the value is stable across both 2026 publications. | No operator-internal conflict. Undated secondary contradictions are not adopted: P2PBUS hourly midday (26); Sakay's "UBE Express", hourly. The attribution is Precious Grace, per the operator's graphics and the owner's queue observation. |
| | TriNoma → Malolos | **PROVISIONAL** (unchanged) | Daily 06:00–22:30 every 30 (34) | PG-G1 and PG-G2 (the shared North EDSA row; both dated) | **The chronology doesn't resolve the North EDSA split.** The departure stop (TriNoma or SM) and whether each destination departs every 30 minutes are still not stated. |
| **Angat ↔ Monumento** (PG) | Angat → Monumento | **NOT COUNTDOWN-READY** (pending) | None | PG-ANGAT-W and -Y: first 03:30–04:00, last 19:00, daily | **Genuinely missing:** a fixed departure sequence. The "10–30 MINUTES" interval is variable, and the first trip is a range. |
| | Monumento → Angat | **NOT COUNTDOWN-READY** (pending) | None | PG-ANGAT-Y (PG-P-0501): last trip 23:00, effective 2026-05-01 (stated). PG-ANGAT-W (PG-P-20251230): the same value. | The window is confirmed, but the variable interval gives no departures. The operator calls it "Route 9", not P2P (scope flag above). |

## Missing information: what blocks a timetable, and what PROVISIONAL can disclose

| Route | **Blocks a timetable** | **Disclosed under PROVISIONAL** |
|---|---|---|
| Pandi | — | The weekend interval. Monday 04:00. P2PBUS ending 30 minutes early. The boarding point. |
| Plaridel ↔ TriNoma | — | How Plaridel departures split by destination (the board shows departures from Plaridel). Sakay's 75-minute list. Monday 03:30–04:00. The weekend interval. |
| Plaridel ↔ SM North | **The Plaridel → SM North departures** (the operator gives one combined Plaridel origin) | The return direction alone would be PROVISIONAL |
| Guiguinto | **Any route-specific interval or list** | — |
| Malolos | — | The return's North EDSA departure stop and interval. The publication date. |
| Caypombo | — | Outbound: none material (G2 is current by chronology). Return: the North EDSA ambiguity. Undated directory midday gaps. |
| Angat ↔ Monumento | **A fixed departure sequence** (the interval is variable) | — |

## Recommended first implementation batch (needs separate authorisation)

**The largest defensible batch: five BULACAN pairs, ten directions.**

| Pair | Change | Status (outbound / return) |
|---|---|---|
| Balagtas ↔ TriNoma | Relabel. Arrays, holiday overrides and effective date unchanged. Note cites BP2P-0830 and P2PBUS. | PROVISIONAL / PROVISIONAL |
| Caypombo ↔ SM North EDSA | **Outbound array corrected to PG-G2:** 05:00–20:30 (adds the 20:30 trip; 31 → 32). Return array unchanged. Notes cite the dated posts. | **PUBLISHED** / PROVISIONAL |
| **Pandi ↔ TriNoma** | New | PROVISIONAL / PROVISIONAL |
| **Plaridel ↔ TriNoma** | New | PROVISIONAL / PROVISIONAL |
| **Malolos ↔ TriNoma** | New | **PUBLISHED** / PROVISIONAL |

**Pass 5 effect on the batch:** the composition is **unchanged**, at the same five pairs.
- **Caypombo outbound:** PROVISIONAL → **PUBLISHED**, and its array gains the 20:30 trip.
- **Malolos:** outbound PUBLISHED confirmed; return PROVISIONAL.
- **No held route becomes ready.** Nothing in the new evidence concerns Bulacan P2P's Plaridel, Guiguinto or weekend
  gaps.

**Held:**
- **Plaridel ↔ SM North EDSA:** the outbound is missing.
- **Guiguinto ↔ TriNoma:** no interval.
- **Angat ↔ Monumento:** variable interval.

**Catalogue size:** 7 → 10 pairs. That is within `MAX_CATALOGUE_ROUTES` (128) and the validated 60-route capacity
fixture. BULACAN goes from 2 to 5 routes.

**Implementation notes for that batch (not done now):**
- **Monday rules:** Pandi → TriNoma and Plaridel → TriNoma need three `ServiceRule`s: Monday (mask bit 1), Tue–Fri and
  weekends. The model supports this.
- **Holidays:** new Bulacan P2P directions use their SSH arrays on the same 2026 non-working dates as Balagtas.
  - Precious Grace is "DAILY", so it gets no overrides.
  - The operator doesn't define "Holiday"; the existing mapping is kept and disclosed.
- **2027 holiday gap (kept open):**
  - The 2027 table is on the unreleased `feature/ph-holidays-2027` branch, pending verification (inventory §2).
  - From 2027-01-01, every Bulacan P2P direction (Balagtas, Pandi, Plaridel) falls back to its weekday timetable on
    holidays. The batch would widen that existing gap from one route to three.
  - It needs reconciling before 2027, separately from this batch.
- **Dates: three kinds, kept separate.**

| Kind | Meaning | Values |
|---|---|---|
| **Post date** | When the operator published | BP2P-0830: 2026-08-30 (year inferred). PG-P-20251230: 2025-12-30 (confirmed). PG-P-0313: 2026-03-13 (year inferred). PG-P-0501: 2026-05-01 (year inferred). |
| **Operator effective date** | When the operator says the schedule applies | BP2P: not stated ("updated schedule"). PG P2P: "2026" (December post), no day for G2. Angat: 2026-05-01 (stated). |
| **Catalogue `effectiveDate`** (firmware) | A valid-from gate: service days before it show no departures (`OnPointSchedule.cpp:96`) | Shipped BULACAN: 2026-09-01. **For new pairs this is an internal choice.** It only affects days before the chosen date, so any date up to the release has the same user-visible effect today. |

  - **Recommendation:** keep **2026-09-01** for the whole batch, as an internal catalogue date. Record the post dates
    and operator effective dates in the notes and the register, not in `effectiveDate`. The alternative is each
    route's own post date (Malolos and Caypombo: 2026-03-13), which only matters for days before September 2026.
- **Validation:** labels such as PANDI, PLARIDEL and MALOLOS need the existing label-width test. Host tests, the
  release guard and the matrix as usual.

## Exact evidence still missing

1. ~~**Precious Grace G1 and G2:** publication dates~~ **Resolved in pass 5.** Still open: the permalinks, and a check
   for any post after May 2026.
2. **Precious Grace's North EDSA row:** the departure stop (TriNoma or SM), and whether each destination departs
   every 30 minutes.
3. **Bulacan P2P Plaridel:** departures by destination (TriNoma vs SM North). This unblocks Plaridel ↔ SM North.
4. **Bulacan P2P Guiguinto:** any departure list or interval.
5. **Bulacan P2P:** a weekend departure list or interval statement. This would upgrade the SSH disclosures.
6. **Angat ↔ Monumento:** a fixed departure list, if one exists.
7. **2027 holidays:** Bulacan P2P's holiday policy for 2027 and the verified 2027 proclamation.

## Change log

| Date | Change |
|---|---|
| 2026-10-25 | Pass 4: readiness under the owner's standard |
| 2026-10-25 | **Pass 5:** Precious Grace chronology established from owner screenshots: 2025-12-30 (confirmed), 03-13 and 05-01 (2026 inferred). Caypombo outbound → PUBLISHED with G2's 20:30. Malolos confirmed. The Angat "Route 9" scope flag. Date kinds separated. |
