# BULACAN implementation readiness (passes 4–7)

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

> **Pass 6 supersedes this batch.** See §6.6: three pairs (Balagtas, Caypombo, Malolos). Pandi and Plaridel are
> deferred.

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
| 2026-10-25 | **Pass 7:** Guiguinto photos of Sep 2025 (clean) and Mar 31 (possible disruption, excluded). The owner reinstates the five-route batch, with Plaridel ↔ TriNoma retained. Guiguinto outbound weekday is a candidate, but the weekend conflicts and there is no return: NOT COUNTDOWN-READY. |
| 2026-10-25 | **Pass 6:** the Plaridel identity is one through-service (designation Plaridel ↔ SM North EDSA; TriNoma a stop), deferred. Guiguinto terminal timetable 2025 transcribed: weekday 28, weekend every 45 min; still NOT COUNTDOWN-READY. The weekend 30-minute assumption is contradicted, so Pandi is deferred. Batch: 3 pairs. |
| 2026-10-25 | **Pass 5:** Precious Grace chronology established from owner screenshots: 2025-12-30 (confirmed), 03-13 and 05-01 (2026 inferred). Caypombo outbound → PUBLISHED with G2's 20:30. Malolos confirmed. The Angat "Route 9" scope flag. Date kinds separated. |

## Pass 6: Plaridel identity and Guiguinto terminal timetable (2026-10-25)

**Supersedes** the batch recommendation above where they differ. Documentation only. *(The batch part was itself superseded by pass 7, §7.8: the owner reinstated the five-route batch.)*

### 6.1 New evidence

Both items are owner screenshots, stored in the local evidence folder with SHA-256 sums. Individuals' names are
omitted here.

| ID | What | Class | Date | Weight |
|---|---|---|---|---|
| **TN-GUI-20250625** | A Facebook user's photo of a physical notice, "P2P BUS TERMINAL TIME SCHEDULE — WalterMart Guiguinto – Trinoma", with separate MONDAY–FRIDAY and SATURDAY–SUNDAY lists and handwritten amendments. File `TN-GUI-20250625-photo.webp` (`9aa1a590…`). | **A terminal notice, photographed.** Not operator-branded and not an operator post, but stronger than commentary: it is the notice posted at the boarding terminal. | Photo post **2025-06-25 (year shown)**. The notice itself is undated. | Historical secondary. It shows what was posted in mid-2025, not what runs in 2026. |
| **COM-PLR-20250705** | A community-group question, "May pa-sm north po ba ang p2p plaridel? or trinoma lang?", and a reply: "Dadaan po muna ng Trinoma then SM North." File `COM-PLR-20250705.png` (`ce997936…`). | **A commuter comment.** Never authoritative. | Post 2025-07-05 (year shown). The reply is shown as "1y". | Corroboration only |

### 6.2 Guiguinto terminal timetable: exact transcription (TN-GUI-20250625)

**Direction:** WalterMart Guiguinto → TriNoma. The list starts at 04:30, which is the Guiguinto-origin first trip;
TriNoma's first trip is 06:00. It ends at 19:30, the Guiguinto-origin last trip.

**Legend:**
- **P:** printed.
- **H:** handwritten amendment over or beside print.
- **T:** handwritten on a taped slip that covers the printed rows.
- **?:** ambiguous.

**Monday–Friday (28 departures as amended):**

| # | Time | Source | Note |
|---|---|---|---|
| 1–7 | 04:30, 05:00, 05:30, 06:00, 06:30, 07:00, 07:30 | P | — |
| 8 | 08:00 | P "08:" + H "00" (blue) | The printed minutes are overwritten; the original value is unknown |
| 9 | 08:30 | P "0…0 AM" + H "8:3" (black) | The printed digits are overwritten; the original is unknown |
| 10 | 09:00 | P "09" + H ":00" (blue) | The printed minutes are overwritten |
| 11 | 09:30 | H "9" + P ":30 AM" | The printed hour is overwritten |
| 12 | 10:00 | T, beside printed "AM" | Taped slip |
| 13 | 10:45 | T, handwritten "AM" | — |
| 14 | 11:30 | T | **?** No AM/PM visible. AM is inferred from its position. |
| 15 | 12:15 | T, printed "NN" | — |
| 16–20 | 13:00, 13:45, 14:30, 15:00, 15:30 | T, printed "PM" | — |
| 21 | 16:00 | T, printed "PM" | — |
| 22 | 16:20 | T | **?** Reads "4:20"; "4:30" can't be fully excluded |
| 23 | 16:40 | T, marker "PM" over print | **?** Reads "4:40" |
| 24–26 | 17:00, 17:30, 18:00 | T, printed "PM" | — |
| 27 | 18:45 | T | **?** No suffix (below the printed rows). PM inferred. |
| 28 | 19:30 | T | **?** No suffix. PM inferred. |

- **Spacing:** every 30 minutes from 04:30 to 10:00, then 45 to 14:30. After that: 30, 30, 30, then 20, 20, 20, then
  30, 30, then 45, 45.
- **The sequence is not uniform.** The amendments show the weekday list was revised by hand at least once before
  June 2025.

**Saturday–Sunday, with a handwritten "HOLIDAY" slip added to the heading (21 departures, all printed):**
- 04:30, 05:15, 06:00, 06:45, 07:30, 08:15, 09:00, 09:45, 10:30, 11:15, 12:00 NN;
- 12:45, 13:30, 14:15, 15:00, 15:45, 16:30, 17:15, 18:00, 18:45, 19:30.
- That is **exactly every 45 minutes, from 04:30 to 19:30.** No ambiguities.

### 6.3 Source-by-source comparison for Guiguinto → TriNoma

| Day type | BP2P-0830, 2026 (operator window) | TN-GUI-20250625, 2025 list | Sakay (undated) | P2PBUS | Verdict |
|---|---|---|---|---|---|
| Mon | 04:30–19:30 | M–F 04:30…19:30 (28) | 04:30–19:30 daily | No page | **The window agrees at both ends** |
| Tue–Fri | 04:30–19:30 | Same list | Same | — | **Agrees at both ends** |
| Sat/Sun/Holiday | **05:00**–19:30 | **04:30**…19:30, every 45 min (21) | Same as daily | — | **Conflict at the first trip.** 05:00 isn't on the 2025 grid (04:30 + 45k), and the 19:30 last trip can't be reached from 05:00 in 45-minute steps. The 2025 sequence **can't be fitted to the 2026 window without changing times.** |

**TriNoma → Guiguinto:** no departure list exists in any source. The notice covers the Guiguinto origin only.

### 6.4 Guiguinto reassessment

**Guiguinto → TriNoma, Mon–Fri: a defensible PROVISIONAL sequence exists.**
- It would be the 28 departures as amended in TN-GUI-20250625. Their first and last trips equal the 2026 operator
  window exactly.
- **Disclose:** it dates from 2025, carries handwritten revisions, and four entries are ambiguous (#14, #22, #23, #27/28
  for the suffix).

**Guiguinto → TriNoma, Sat/Sun/Holiday: NOT COUNTDOWN-READY.**
- The only list contradicts the 2026 first trip. Shifting or trimming it would silently modify times.

**TriNoma → Guiguinto (all days): NOT COUNTDOWN-READY.** There is no list.

**Pair verdict: NOT COUNTDOWN-READY.** A direction needs a rule for every day it runs: a missing weekend rule would
show *no service* on days the bus runs. The return direction has no data at all.

**Exactly what prevents it:**
1. The 2026 weekend and holiday departure list from Guiguinto.
2. Any TriNoma → Guiguinto departure list.

The currency of the weekday list is disclosable, not blocking.

### 6.5 Plaridel: updated service-identity assessment

**Evidence for one through-service calling at both North Avenue malls:**

| Evidence | Class | Supports |
|---|---|---|
| The BP2P intro: "Plaridel to Trinoma / SM North EDSA vv" | OP (undated) | One route named with both stops |
| BP2P-0830-5, -3 and -2: a **single "PLARIDEL" origin row**, with no destination split | OP (2026) | One Plaridel dispatch stream |
| COM-PLR-20250705: "Dadaan po muna ng Trinoma then SM North" (it passes TriNoma first, then SM North) | Commuter (2025) | Plaridel → TriNoma → SM North |
| Sakay, TriNoma → Plaridel stops: TriNoma, then SM North EDSA, then Primark Plaridel | 3P (undated) | TriNoma → SM North → Plaridel on the return |
| Owner: rode Plaridel → SM North (O3). Also boarded at SM North toward Plaridel and found passengers already aboard. | OBS | A through-service in both directions; on the return, TriNoma is boarded before SM North |

**Evidence that not all trips serve both stops the same way:**

| Evidence | Implication |
|---|---|
| BP2P-0830-4 / -1: TriNoma–Plaridel 06:00–**21:00**; SM North Edsa–Plaridel 06:00–**22:30** | Departures continue from SM North for 90 minutes after TriNoma's last. So at least the late-evening trips **don't call at TriNoma** (or are dispatched from SM North). Both first trips are 06:00, which is unexplained if every bus calls at TriNoma first. |

**Conclusion: one Plaridel service identity, not two.**
- On the Bulacan side, all Plaridel departures form one stream. They go to North Avenue via TriNoma and end at SM North
  (per the commuter and the owner).
- On the Metro side, departures to Plaridel board at TriNoma and then SM North until about 21:00. **Later departures
  appear to be SM North-only.**
- **Confidence: medium.** The two Metro-side windows are both operator-stated.
- The exact relationship between a TriNoma departure time and the same bus's SM North time isn't stated.
- This follows the owner's preference: **one future pair, designated Plaridel ↔ SM North EDSA, with TriNoma documented
  as a stop.** The approved catalogue's two Plaridel entries collapse into one unique service. That is an owner
  decision, recorded as recommended.

**Are departure times supported in both directions for Plaridel ↔ SM North EDSA?**

| Direction | Boarding-terminal departures | Assessment |
|---|---|---|
| **Plaridel → SM North** | Departures *from Plaridel* are the same stream whatever the North Avenue end. Window: Mon 03:30 / Tue–Fri 04:00 / SSH 04:30 to 20:30 (BP2P-0830). Weekday pattern: only P2PBUS's uniform 30-minute Plaridel → TriNoma list (undated). | Weekday: PROVISIONAL is defensible on its face. **Weakened by TN-GUI**: the one real Bulacan P2P list is irregular and amended, so P2PBUS's uniform lists may be templated. **Weekend: unsupported.** |
| **SM North → Plaridel** | Window 06:00–22:30 M–F, 06:30–22:30 SSH (BP2P-0830). Pattern: only P2PBUS (05:30…22:30, every 30, undated; conflicts with the 06:00 first trip). | **Not supported.** Before 21:00, a bus's SM North time is its TriNoma time plus an unknown number of minutes. After 21:00, the SM North-only departures have no list. Using TriNoma times as SM North times is excluded by the owner's rule. **NOT COUNTDOWN-READY.** |

**Fallback: Plaridel ↔ TriNoma temporarily. Not recommended.**
- **Its return** (TriNoma 06:00–21:00) rests on P2PBUS's 05:30–22:00 list, which overruns the operator window at
  both ends. That suggests staleness. Its weekend is unsupported too.
- **Stable IDs:** route IDs are stable keys in `OnPointStore` (saved main routes). Shipping `p2p-plaridel-trinoma` and
  later replacing it with a Plaridel ↔ SM North identity would orphan saved defaults.
- **Recommendation: defer Plaridel entirely.**

### 6.6 Implications for the first batch

**TN-GUI also tests the assumption** used for every Bulacan P2P weekend array in the earlier passes: that weekends
keep the weekday 30-minute interval. **It is the only Bulacan P2P weekend list found, and it runs every 45 minutes,
not 30.** So that assumption is no longer defensible as PROVISIONAL for routes with no weekend evidence of their own.

| Pair | Pass 4/5 | **Pass 6** | Reason |
|---|---|---|---|
| Balagtas ↔ TriNoma | PROVISIONAL (relabel) | **PROVISIONAL (relabel), retained as the owner directed** | Weekday departures: P2PBUS, matching the operator windows. **The weekend arrays (every 30) are now in tension with TN-GUI.** Disclose this explicitly in the note. Getting a Balagtas weekend list is the top Bulacan P2P evidence priority. Already shipped. |
| Caypombo ↔ SM North EDSA | Outbound PUBLISHED (G2) / return PROVISIONAL | **Unchanged** | Precious Grace states the interval and "DAILY" itself. TN-GUI doesn't apply. |
| Malolos ↔ TriNoma | PUBLISHED / PROVISIONAL | **Unchanged** | As above |
| Pandi ↔ TriNoma | PROVISIONAL (new) | **Defer** (recommended) | Weekend arrays rely on the now-contradicted assumption, and P2PBUS's weekday list already disagrees with the operator's last trip. *Owner option:* accept a disclosed weekend assumption to keep it. |
| Plaridel (either designation) | PROVISIONAL (new, ↔ TriNoma) | **Defer** | §6.5 |
| Guiguinto ↔ TriNoma | NOT COUNTDOWN-READY | **NOT COUNTDOWN-READY** (Mon–Fri outbound has a candidate) | §6.4 |
| Angat ↔ Monumento | NOT COUNTDOWN-READY | **Unchanged** | Variable interval; "Route 9" scope flag |

**Recommended first batch, focused on unique services with supported boarding-terminal departures:**
- **Balagtas ↔ TriNoma:** relabel, plus a stronger weekend disclosure.
- **Caypombo ↔ SM North EDSA:** add G2's 20:30 trip; outbound PUBLISHED, return PROVISIONAL.
- **Malolos ↔ TriNoma:** new.

That is **three pairs and one new route.** BULACAN goes from 2 to 3, and the catalogue from 7 to 8. If the owner
accepts a disclosed weekend assumption, Pandi ↔ TriNoma can be added as a fourth.

**Evidence that would unlock the deferred routes:**
- **Plaridel:** an SM North → Plaridel departure list, or the TriNoma-to-SM North running time plus the SM North-only
  evening departures. Weekend lists in both directions.
- **Pandi:** a weekend departure list, which Bulacan P2P does post as terminal notices. A current weekday list would
  also settle the 19:00 vs 19:30 last trip.
- **Guiguinto:** the 2026 weekend list and any TriNoma → Guiguinto list.
- **Balagtas:** a weekend list, to confirm or correct the shipped weekend arrays.

## Pass 7: Guiguinto terminal photos and owner decisions (2026-10-25)

**Owner decisions recorded. They supersede §6.6 where different.**
1. **The approved five-route batch stands:** Balagtas, Caypombo, Pandi, Plaridel ↔ TriNoma and Malolos. Each uses its
   defensible PROVISIONAL pattern and the agreed directional PUBLISHED classifications.
2. **Plaridel ↔ TriNoma stays in the batch.** The shared TriNoma/SM North journey is investigated, but the stops'
   times are never treated as interchangeable.
3. **Plaridel ↔ SM North EDSA** is a documented candidate only, until boarding-terminal-specific departures exist.
4. **Photographed terminal timetables** are primary historical operational evidence. A post date shows when the photo
   was posted, not when the schedule took effect.
5. **The March 31 photo is excluded** from the normal-service sequence and archived as a **possible disruption-period
   schedule**. The owner notes that bus operators reduced trips during the March 2026 fuel-price crisis. There is no
   proof that this caused the Guiguinto changes.
6. **Terminal photos are not a prerequisite** for routes that already have defensible provisional evidence.

### 7.1 New evidence

| ID | What | Date | Class |
|---|---|---|---|
| **TN-GUI-20250912** | A photo of the printed WalterMart Guiguinto – TriNoma terminal notice: Monday–Friday, plus separate **Saturday** and **Sunday** lists. A boxed "HOLIDAY" label sits on the Saturday heading. File `TN-GUI-20250912-photo.webp` (`2f5c4bc5…`). | Photo post **2025-09-12 (year shown)**. The notice is undated. | Terminal notice (primary, historical) |
| **TN-GUI-0331** | The same printed notice, with handwritten weekday-afternoon changes. File `TN-GUI-0331-POSSIBLE-DISRUPTION.webp` (`61aeea97…`). | Photo post "March 31". **The year isn't shown.** 2026 is likely by Facebook convention but not confirmed. | **Possible disruption-period schedule.** Archived, and excluded from the candidate. |

### 7.2 Transcription: TN-GUI-20250912 (all printed; no handwriting; legibility good)

**Monday–Friday (27):**
- 04:30, 05:00, 05:30, 06:00, 06:30, 07:00, 07:30, 08:00, 08:30, 09:00, 09:30, 10:00, 10:45, 11:30;
- 12:15 NN, 13:00, 13:45, 14:30, 15:00, 15:30, 16:00, **16:30**, 17:00, 17:30, 18:00, 18:45, 19:30.

**Saturday, with the "HOLIDAY" label (21):** every 45 minutes from 04:30 to 19:30.
- 04:30, 05:15, 06:00, 06:45, 07:30, 08:15, 09:00, 09:45, 10:30, 11:15;
- 12:00 NN, 12:45, 13:30, 14:15, 15:00, 15:45, 16:30, 17:15, 18:00, 18:45, 19:30.

**Sunday (21):**
- 04:45, 05:30, 06:15, 07:00, 07:45, 08:30, 09:15, 10:00, 10:45, 11:30;
- 12:15 NN, 13:00, 13:45, 14:30, 15:15, 16:00, 16:45, 17:30, 18:15, 19:00, **19:30**.
- That is every 45 minutes from 04:45 to 19:00, then a 30-minute step to the 19:30 last trip.

**Reading flags:**
- The "HOLIDAY" box is attached to the Saturday heading. This is read as *holidays use the Saturday list*. Its
  placement is clear, but it is an attached label, not part of the printed table.
- The 11:30 AM entries are all printed with AM. No ambiguities remain.

**Against the June 2025 photo (TN-GUI-20250625):**
- **Weekday:** identical except the 16:00–17:00 segment. June's handwritten 16:20 and 16:40 are replaced by a printed
  16:30. So the weekday list was revised, and 27 trips replace 28.
- **Weekend:** June showed one combined Saturday–Sunday list (04:30, every 45). September splits Saturday (04:30
  start) from **Sunday (04:45 start)**.

### 7.3 Transcription: TN-GUI-0331 (possible disruption period; not a candidate)

**Monday–Friday:**
- **Printed, unchanged:** the morning is identical to September 2025. The afternoon runs 12:15 NN, 13:00, 13:45,
  14:30, 15:00, 15:30, 16:00.
- **Printed and struck through by hand:** 16:30, 17:00, 17:30, 18:00, 18:45, 19:30. The strokes are heavy; the
  underlying values are inferred from the September print, with which they agree where legible.
- **Handwritten:** "4 PM ONWARDS MAY BYAHE PA PO." ("From 4 PM onwards there are still trips"), and "LAST TRIP 7:30".
- **Reading:** after 16:00 the fixed departures were withdrawn. Trips continued at **unspecified** times until 19:30.

**Saturday and Sunday:** printed, and identical to September 2025. No "HOLIDAY" label is visible in this photo.

### 7.4 Comparison with the 2026 operator windows (BP2P-0830)

| Day type | 2026 operator window | TN-GUI-20250912 | Result |
|---|---|---|---|
| Mon | 04:30–19:30 | M–F 04:30…19:30 (27) | **Both ends agree** |
| Tue–Fri | 04:30–19:30 | Same | **Both ends agree** |
| Sat | **05:00**–19:30 | 04:30…19:30 (every 45) | **The first trip conflicts.** 05:00 isn't on the 04:30 + 45k grid. |
| Sun | **05:00**–19:30 | 04:45…19:30 | **The first trip conflicts.** 05:00 isn't in the list. |
| Holiday | **05:00**–19:30 (with Sat/Sun) | The Saturday list (label) | Conflicts as Saturday does. 2026 also groups Saturday and Sunday together, where 2025 split them. |

### 7.5 Guiguinto verdict

**WalterMart Guiguinto → TriNoma, Mon–Fri: a PROVISIONAL candidate is supported.**
- The **27 printed departures of TN-GUI-20250912**, whose first and last trips equal the 2026 operator window exactly.
- **Disclose:**
  - The list dates from 2025 (posted 2025-09-12; effective date unknown).
  - March 2026 showed a possible disruption after 16:00.
  - The 2026 sequence hasn't been confirmed.

**Sat/Sun/Holiday: no defensible sequence.**
- Both 2025 weekend lists start before the 2026 operator's 05:00 first trip.
- Making them fit would mean removing or shifting times, which is not allowed.

**TriNoma → WalterMart Guiguinto: no evidence at all.** Every photo is of the Guiguinto-origin notice. Nothing will be
invented.

**Pair verdict: NOT COUNTDOWN-READY. Flagged for later reconciliation.**
- **It can't be included without inventing departures.** A direction needs every operating day covered; leaving
  weekends out would show "no service". The return has no departures at all.
- **What's missing:**
  1. The 2026 weekend (and holiday) departure list from Guiguinto.
  2. Any TriNoma → Guiguinto departure list.
- **Disclosable, not blocking:** whether the September 2025 weekday list is still current.

### 7.6 What the Guiguinto photos say about other Bulacan P2P weekend arrays (disclosure, not a block)

**Pattern:** across three terminal photos (June 2025, September 2025, March 2026), Guiguinto's weekend service was
**every 45 minutes**, never 30. Weekday service was mostly every 30 minutes, with 45-minute midday gaps. This is one
route's evidence. It doesn't establish other routes' patterns.

**Effect on the batch:** the Bulacan P2P weekend arrays in the batch assume a 30-minute interval inside the operator's
weekend windows:
- Balagtas, shipped;
- Pandi, new;
- Plaridel ↔ TriNoma, new.

**Per decision 6, this does not block them.** Their notes and this register must disclose that the weekend
departures are pattern-assumed, and that the only Bulacan P2P weekend lists found (Guiguinto) run at 45 minutes.
Photographing the Balagtas, Pandi and Plaridel weekend notices is the **first** target for the next commute.

### 7.7 Plaridel, as directed

**Plaridel ↔ TriNoma (in the batch):** the through-service evidence (§6.5) makes **TriNoma the first North Avenue
boarding point on the return** (Sakay's stop order; the owner found passengers already aboard at SM North). So
TriNoma departures *are* boarding-terminal departures.
- **Return:** M–F 06:00–21:00 and SSH 06:30–21:00, every 30 minutes (BP2P-0830 window, P2PBUS pattern; P2PBUS's
  05:30–22:00 overrun is not adopted).
- **Outbound:** the Plaridel stream. Mon 03:30, Tue–Fri 04:00 and SSH 04:30, each to 20:30, every 30 minutes.
  Disclosed: the operator's single Plaridel row doesn't say whether every departure continues to SM North.

**Plaridel ↔ SM North EDSA (documented candidate only):**
- SM North departure times are never derived from TriNoma's.
- **Needed:** the SM North → Plaridel departure list, including the SM North-only trips after 21:00, plus the
  operator's confirmation of the stop order.

**ID note:** if a single Plaridel ↔ SM North designation is chosen later, it replaces `p2p-plaridel-trinoma`. Saved
main-route choices would then need the store's existing unknown-ID handling (the no-main state). Record that before
release.

### 7.8 Recommendation

**The approved five-route batch is still the best first development batch.**

| Pair | Outbound / return | Basis |
|---|---|---|
| Balagtas ↔ TriNoma | PROVISIONAL / PROVISIONAL (shipped arrays) | §3 and §7.6 disclosure |
| Caypombo ↔ SM North EDSA | **PUBLISHED** / PROVISIONAL (outbound to 20:30) | Pass 5 |
| Pandi ↔ TriNoma | PROVISIONAL / PROVISIONAL | Readiness table; §7.6 disclosure |
| Plaridel ↔ TriNoma | PROVISIONAL / PROVISIONAL | §7.7 |
| Malolos ↔ TriNoma | **PUBLISHED** / PROVISIONAL | Pass 5 |

**Why it is the best:**
- Every direction has a sequence bounded by **operator-stated** first and last trips, with no invented endpoints.
- The Precious Grace directions rest on an operator-stated interval.
- The Bulacan P2P weekday directions rest on a secondary route-specific pattern.
- The Bulacan P2P weekends are the disclosed weak point.

**Guiguinto:** **not included.** It can't be included without inventing weekend and return departures. It is flagged
for reconciliation once the 2026 weekend notice and a TriNoma → Guiguinto list are photographed.

**Still needed before authorising implementation:**
- The catalogue `effectiveDate`: 2026-09-01 is recommended.
- Whether the "may shift with ridership" wording stays in the Caypombo note.
- The 2027 holiday gap, which widens to three Bulacan P2P routes (Balagtas, Pandi, Plaridel).
