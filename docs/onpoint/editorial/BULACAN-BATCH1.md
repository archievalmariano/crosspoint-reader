# BULACAN batch 1: implementation plan and directional timetables

**Status:** implemented locally on `feature/onpoint-stage2` at `1aaedbf4`, for a **local development build** and
Codex's data-integrity review.

**Not done:** push, release or device flash. Those need separate approval.

**Scope (owner decision, 2026-10-25):** six BULACAN pairs.
- **In:** Balagtas, Caypombo, Pandi, Plaridel ↔ TriNoma, Malolos and Guiguinto.
- **Deferred:** Plaridel ↔ SM North EDSA.
- **Excluded:** Angat ↔ Monumento ("Route 9", variable interval).

**Evidence:**
- [BULACAN-EVIDENCE-REGISTER.md](BULACAN-EVIDENCE-REGISTER.md): the sources.
- [BULACAN-READINESS.md](BULACAN-READINESS.md): the reasoning. §8 covers the Guiguinto alternatives.

## 1. Provenance classes

Every departure belongs to one class:

| Class | Meaning |
|---|---|
| **PUB** | **Operator-published:** a first or last trip stated by the operator, or a departure derived from operator first/last trips plus an **operator-stated** interval |
| **SEC** | **Secondary source:** an interior departure taken from P2PBus.ph's undated route-specific list, inside operator first/last trips |
| **HIST** | **Historical primary:** printed on a photographed terminal notice (WalterMart Guiguinto, photo posted 2025-09-12) |
| **ASSUMED** | **Pattern-assumed:** an interior departure from an interval no source lists for that day type or direction |
| **RECON** | **Reconstructed:** a Guiguinto departure chosen from analysed alternatives (owner-approved development reconstruction) |

**Status labels** (ON POINT's only two):
- **PUBLISHED** only where every departure is PUB and no material conflict remains.
- **PROVISIONAL** for everything else.
- The firmware holds one status per direction, so a direction is as strong as its weakest rule.

## 2. Catalogue

| Index | Route ID (outbound) | Schedules | Origin → destination | Operator | Collection |
|---|---|---|---|---|---|
| 0 | `p2p-balagtas-trinoma` | 0 / 1 | BALAGTAS ↔ TRINOMA | Bulacan P2P | bulacan (default) |
| 1 | `p2p-caypombo-sm-north-edsa` | 2 / 3 | CAYPOMBO ↔ SM NORTH EDSA | Precious Grace | bulacan |
| 7 | `p2p-pandi-trinoma` | 14 / 15 | PANDI ↔ TRINOMA | Bulacan P2P | bulacan |
| 8 | `p2p-plaridel-trinoma` | 16 / 17 | PLARIDEL ↔ TRINOMA | Bulacan P2P | bulacan |
| 9 | `p2p-malolos-trinoma` | 18 / 19 | MALOLOS ↔ TRINOMA | Precious Grace | bulacan |
| 10 | `p2p-guiguinto-trinoma` | 20 / 21 | GUIGUINTO ↔ TRINOMA | Bulacan P2P | bulacan |

- **Appended, not inserted:** indices 0–6 and every existing route ID are unchanged, so saved main routes and
  collection defaults keep resolving.
- **Catalogue size:** 7 → 11 pairs (14 → 22 schedules). BULACAN goes from 2 to 6 members, which is two list pages.
- **Catalogue `effectiveDate`:** 2026-09-01 for all six pairs (decision default, §6). This is an internal valid-from
  date, distinct from the operators' post dates and effective dates (register §2).

## 3. Directional timetables

`a–b /30` means every 30 minutes from a to b inclusive. "SSH" means Saturday, Sunday and the 2026 non-working
holidays (date overrides).

### 3.1 Balagtas ↔ TriNoma (Bulacan P2P): PROVISIONAL / PROVISIONAL (shipped arrays, unchanged)

| Direction | Mon–Fri | SSH |
|---|---|---|
| Balagtas → TriNoma | 04:30–20:00 /30 (32). Ends PUB, interior SEC. | 05:00–20:00 /30 (31). Ends PUB, interior ASSUMED. |
| TriNoma → Balagtas | 06:00–22:30 /30 (34). Ends PUB, interior SEC. | 06:30–22:30 /30 (33). Ends PUB, interior ASSUMED. |

- **Relabel:** PUBLISHED → PROVISIONAL.
- **Known tension:** the only Bulacan P2P weekend lists found (Guiguinto, 2025) run every 45 minutes.
- **Stop:** Bocaue Crossing is a stop on this service, not shown.

### 3.2 Caypombo ↔ SM North EDSA (Precious Grace): **PUBLISHED** / PROVISIONAL

| Direction | Daily |
|---|---|
| Caypombo → SM North EDSA | 05:00–**20:30** /30 (32), all PUB (PG-P-0313). **Adds 20:30**, superseding the 2025-12-30 graphic's 20:00. |
| SM North EDSA → Caypombo | 06:00–22:30 /30 (34), all PUB, unchanged |

- **Why the return is PROVISIONAL:** the operator's single "North EDSA (TriNoma and SM)" row doesn't say which stop
  the time applies to, or whether each destination departs every 30 minutes.
- **No holiday overrides:** the operator calls it "DAILY".
- **Note text:** the unsourced "may shift with ridership" wording is removed from the code comment (decision
  default, §6).

### 3.3 Pandi ↔ TriNoma (Bulacan P2P): PROVISIONAL / PROVISIONAL (new)

| Direction | Mon | Tue–Fri | SSH |
|---|---|---|---|
| Pandi → TriNoma | 03:30–19:30 /30 (33) | 04:30–19:30 /30 (31) | 04:30–19:30 /30 (31) |
| TriNoma → Pandi | 06:00–21:30 /30 (32), the same as Tue–Fri | 06:00–21:30 /30 (32) | 06:30–21:30 /30 (31) |

- **Classes:** all ends PUB (BP2P-0830). Weekday interiors SEC.
- **Where P2PBus.ph falls short:** its list stops at 19:00 (out) and 21:00 (back). The operator's 19:30 and 21:30
  last trips are PUB and govern.
- **Monday 04:00 is ASSUMED**: between the operator's 03:30 and the P2PBus.ph 04:30 start. SSH interiors are ASSUMED.
- **Encoding:** Tue–Fri and SSH share one array, rule mask Tue–Sat + Sun.

### 3.4 Plaridel ↔ TriNoma (Bulacan P2P): PROVISIONAL / PROVISIONAL (new)

| Direction | Mon | Tue–Fri | SSH |
|---|---|---|---|
| Plaridel → TriNoma | 03:30–20:30 /30 (35) | 04:00–20:30 /30 (34) | 04:30–20:30 /30 (33) |
| TriNoma → Plaridel | 06:00–21:00 /30 (31) | 06:00–21:00 /30 (31) | 06:30–21:00 /30 (30) |

- **Classes:** all ends PUB. Tue–Fri interiors SEC: P2PBus.ph matches the operator window exactly. Monday 03:30 is
  PUB; the Monday trips before 04:00 and all SSH interiors are ASSUMED.
- **Return:** P2PBus.ph's 05:30–22:00 overrun is not adopted.
- **Identity:** one through-service via TriNoma and SM North. These are boarding-terminal departures from Plaridel
  and from TriNoma.
- **Not adopted:** Sakay's 75-minute list, an undated contradiction. SM North times are not derived from these.

### 3.5 Malolos ↔ TriNoma (Precious Grace): **PUBLISHED** / PROVISIONAL (new)

| Direction | Daily |
|---|---|
| Malolos → TriNoma | 05:00–20:00 /30 (31), all PUB (PG-P-0313; PG-P-20251230 agrees) |
| TriNoma → Malolos | 06:00–22:30 /30 (34). PUB pattern, but the North EDSA row is ambiguous, so PROVISIONAL. |

**Not adopted** (undated secondary): P2PBus.ph's hourly midday, and Sakay's "UBE Express" hourly list. The attribution
is Precious Grace.

### 3.6 Guiguinto ↔ TriNoma (Bulacan P2P): PROVISIONAL / PROVISIONAL (new, development reconstruction)

**Guiguinto → TriNoma, Mon–Fri (27). Ends PUB + HIST; interior HIST (TN-GUI-20250912):**
- 04:30, 05:00, 05:30, 06:00, 06:30, 07:00, 07:30, 08:00, 08:30, 09:00, 09:30, 10:00, 10:45, 11:30;
- 12:15, 13:00, 13:45, 14:30, 15:00, 15:30, 16:00, 16:30, 17:00, 17:30, 18:00, 18:45, 19:30.

**Guiguinto → TriNoma, SSH (21). Candidate S-C, selected:**

| Departure | Class |
|---|---|
| **05:00** | PUB (2026 first trip; RECON placement: replaces the 2025 Sunday 04:45, which falls outside the 2026 window) |
| 05:30, 06:15, 07:00, 07:45, 08:30, 09:15, 10:00, 10:45, 11:30, 12:15, 13:00, 13:45, 14:30, 15:15, 16:00, 16:45, 17:30, 18:15, 19:00 | HIST (the 2025 **Sunday** list), **applied to Saturday and holidays as RECON** |
| 19:30 | PUB + HIST |

**TriNoma → Guiguinto, Mon–Fri (32). Candidate R-A, selected:** 06:00–21:30 /30. Ends PUB; 30 interior departures
RECON.

**TriNoma → Guiguinto, SSH (21). Candidate RS-B, selected:** 06:30, 07:15, 08:00, 08:45, 09:30, 10:15, 11:00, 11:45,
12:30, 13:15, 14:00, 14:45, 15:30, 16:15, 17:00, 17:45, 18:30, 19:15, 20:00, 20:45, 21:30. Ends PUB; 19 interior
departures RECON.

**Why these candidates (alternatives in readiness §8, all retained):**

| Cell | Selected | Why | Alternatives and expected discrepancy |
|---|---|---|---|
| Out SSH | **S-C** | The only candidate whose every time is PUB or HIST. It needs no unsupported closing trip (S-B's 19:15) and keeps no trip known to be outside the 2026 window. | S-A (every 30, 30 trips): mean 11.6 min apart; 61% of arrival minutes ≥ 10 min apart. S-B (every 45 from 05:00): 19.4 min, 98%. S-D rejected (05:00/05:15 pair). **Saturday risk:** if Saturdays still follow the 2025 Saturday grid, every interior departure is 15 minutes off. |
| Back Mon–Fri | **R-A** | The same method as every other Bulacan P2P return in this batch. R-B's constant turnaround is falsified at the end (a 120-minute last-trip gap against 90). | R-B (mirrored, 28 trips): mean 4.8 min; 25% of minutes ≥ 10 min apart, concentrated 12:00–16:00 and after 19:30. R-A shows 4 more trips than R-B. |
| Back SSH | **RS-B** | Consistent with the selected 45-minute weekend outbound. The 06:30 + 45k grid lands exactly on the operator's 21:30. | RS-A (every 30, 31 trips): mean 12.5 min; 66% of minutes ≥ 10 min apart. RS-B shows 10 fewer trips. |

**Notes and disclosure:**
- The firmware notes read "Development reconstruction".
- **Nothing claims operator endorsement of the intermediate times.**

**Release check (owner, 2026-10-25):** Guiguinto's inferred countdowns are **approved for a local development build
only.**
- Before any public release, a current terminal timetable or equivalent operator confirmation is required.
- If it is still unavailable at relaunch, the publication decision goes back to the owner.
- Recorded in §5 as a release-blocking item.

## 4. Holidays (2027 integrated 2026-10-28)

**Calendar source: Proclamation No. 1427** (signed 2026-09-08; Official Gazette, uploaded 2026-09-09), merged from
`feature/ph-holidays-2027` (`4467604b`) at `a86bbce8`.
- **2027 non-working days (18):** Jan 1; Feb 6; Mar 25, 26, 27; Apr 9; May 1; Jun 12; Aug 21, 30; Nov 1, 2, 30; Dec 8,
  24, 25, 30, 31.
- **Special working day:** EDSA, Feb 25. It keeps the weekday timetable.
- **Classification confirmed (owner, 2026-10-28):** checked against the **Supreme Court E-Library** text of Proclamation
  No. 1427 and **DOLE's 2026-09-30 advisory**.
  - **Feb 6, Mar 27, Nov 2 and Dec 24, 2027 are additional special non-working days.**
  - **Feb 25 is a special working day.**
  - The earlier Lawphil layout discrepancy is **closed**; the implementation was already correct and is unchanged.
- **Pending:** Eid'l Fitr and Eid'l Adha await their own proclamations (Section 2). No dates are entered or inferred
  until those are issued.
- **No amendment found** (search, 2026-10-28).

**Operator policies (owner decision, 2026-10-28):**

| Operator | Routes | Holiday handling |
|---|---|---|
| **Bulacan P2P (RPF)** | Balagtas, Pandi, Plaridel, Guiguinto, both ways | One override per 2026 and 2027 non-working day, running **each route's own** Saturday/Sunday/holiday departures. Balagtas keeps its hand-written overrides; the others use `bulacanP2pHolidays()`. A Monday holiday replaces Pandi's and Plaridel's Monday rule. |
| **Precious Grace** | Caypombo, Malolos, both ways | **None.** The DAILY timetable runs on holidays. |

**Holiday policy basis:** the operator's 2026 "Saturday/Sunday/Holiday" graphics. A 2027 operator advisory would
replace this mapping.

**Tripwire:** `HolidayOverridesFollowEachOperatorsPolicy` now **passes**: every Bulacan P2P direction carries
Balagtas's 39 dates.

**The holiday calendar ends on 2027-12-31.** From 2028-01-01, holidays run their ordinary day-of-week timetables until
2028 dates are added. That is **future maintenance** after the 2028 proclamation, not a blocker for this release.

## 5. Release checks (blocking any public release of this batch)

1. **Guiguinto, both directions:** the **owner's editorial risk acceptance is recorded (2026-10-28)**. The documented
   reconstruction is accepted as an **explicitly PROVISIONAL publication candidate**, although no current exact
   timetable exists.
   - This is **not operator verification.**
   - The terminal notice FO-RPF-20261028 corroborates only TriNoma → Guiguinto's first and last trips, and gives a
     30–45-minute range.
   - **The gate stays closed:** Guiguinto ships publicly only with the owner's **separate final release
     authorisation**.
2. ~~**2027 holidays**~~ **Integrated** (§4, `f5a2127f`). The residual check is item 7.
3. **TriNoma first trips (register §12.4):** decide whether the undated terminal notice's earlier first trips
   (Balagtas 05:30, Plaridel 05:00, Pandi probably 05:30) or the 2026-08-30 graphic's 06:00 stand at release. Any
   change needs separate authorisation.
4. **The PROVISIONAL definition** (§8) appears in the editorial and public release notes.
5. **The Plaridel ↔ SM North designation**, if it is later chosen to replace `p2p-plaridel-trinoma`. Saved main routes
   would fall to the no-main state. The two boarding locations stay **unmerged** (register §12.1).
7. ~~**The Proclamation No. 1427 classification check**~~ **Closed (2026-10-28).** Confirmed against the Supreme Court
   E-Library and DOLE's 2026-09-30 advisory (§4).
   - **Still to watch, not blocking:** the Eid proclamations (add their dates when issued), any amendment, and 2028
     dates (future maintenance).
8. **Commit attribution before any public push:** `4467604b` carries a personal email. The correction is proposed in
   the readiness summary and **not yet applied**.
6. ~~**Device validation**~~ **Done:** image `638a50a9…`, hardware PASS on 2026-10-26.

## 6. Editorial decisions (confirmed by the owner, 2026-10-28)

| Decision | Confirmed |
|---|---|
| Catalogue `effectiveDate` | **2026-09-01** for all six pairs: a catalogue applicability date, **not** the operators' effective date |
| Caypombo wording | The unsupported "may shift with ridership" statement stays **removed** |
| Route identities | The six pairs and their stable IDs are retained. Caypombo ↔ SM North EDSA keeps its label, with **TriNoma documented as another boarding point** (register §12.2). No Plaridel ↔ SM North entry yet. |
| TriNoma first trips | Both the dated 2026-08-30 graphics and the undated terminal notice are kept as evidence. **No earlier departures are prepended and no arrays change.** An earlier first trip may also imply other frequency differences (register §12.4). |
| Guiguinto | Accepted as a **PROVISIONAL public-release candidate**, limitations stated: an editorial risk acceptance, **not operator verification**. **Publication still needs separate final authorisation.** No special evidence trip is required. |
| PROVISIONAL in release notes | **"Estimated departures; actual times may vary."** No device UI change in this batch. |

## 7. Verification (local)

**Host tests: 697/697.** That is the previous 692 plus 5 new `OnPointBulacanDataTest` tests:
- every direction against an independent "HH:MM" transcription;
- holiday policy per operator;
- operator endpoints;
- collection membership;
- on-device day-type resolution (Monday, a Monday holiday, Saturday, Sunday).

**Mutation checks:** all caught.

| Mutation | Tests failing |
|---|---|
| Guiguinto 10:45 → 10:40 | 4 |
| Caypombo outbound PUBLISHED → PROVISIONAL | 2 + 4 |
| Pandi holiday overrides removed | 2 |
| Pandi Monday rule disabled | 1 + 2 |
| Guiguinto weekend 19:00 dropped | 4 |

**Updated expectations, each tied to the data change:**
- schedule test (route count, statuses, sort order, Caypombo 20:30);
- route-list golden: regenerated from the v1.2.0 reference functions; that generator reproduces the old 7-route golden
  exactly, 56/56 rows;
- four departure-golden rows (Caypombo last 20:30);
- region membership and synthetic validation tables;
- six activity tests: BULACAN now has two pages, and the one-page swipe check moved to Airport Transfers;
- the invalid-catalogue stub.

**Builds:**

| Image | Size | SHA-256 | Notes |
|---|---|---|---|
| `x4pro-all-gh_release` | 5,780,656 B | `18841263…` | Headroom 772,944 B (11.8%) in `0x640000`; no ON POINT warnings |
| `x4pro-all-devtest` | 5,755,056 B | `4eb80792…` | Not v1.2.1-integrated, and **not installed** |

- **`pio check`:** PASSED.
**V0–V3 matrix (run 2, on `61a29187`)**

| Run | Builds | Host tests | Notes |
|---|---|---|---|
| V0 | 9/9; release and devtest scans OK; capacity positive controls OK | 697/697 | **Post-link check: pass** |
| V1 (+ v1.2.1) | 5/5 | 701/701 | Clean merge |
| V2 (+ 2027 holidays) | 5/5 | 701/702 | Clean merge |
| V3 (+ both) | 5/5 | 705/706 | Clean merges |

REFCHECK OK after each run.

**The one V2/V3 failure is the intended 2027 tripwire.** `OnPointBulacanData.HolidayOverridesFollowEachOperatorsPolicy`
fails because the 2027 branch gives Balagtas 2027 holidays but not Pandi, Plaridel or Guiguinto (§4). It turns green
when the 2027 reconciliation adds their dates. Its failure message says so (`dac79426`, a comment and message change
only).

**V0–V3 matrix (run 3, 2026-10-28, on `c2a63e8a`: 2027 holidays integrated)**

| Run | Builds | Host tests | Notes |
|---|---|---|---|
| V0 | 9/9; scans and controls OK | 707/707 | Post-link check passes |
| V1 (+ v1.2.1) | 5/5 | 711/711 | Clean merge |
| V2 (+ 2027 branch, already contained) | 5/5 | 707/707 | Clean |
| V3 (+ both) | 5/5 | 711/711 | Clean merges |

**`HolidayOverridesFollowEachOperatorsPolicy` passes in all four** (no failures in any `junit.xml`). REFCHECK OK
throughout. The only commit after `c2a63e8a` (`4a9a9c5c`) is documentation.

**Run 1 (on `1aaedbf4`) found two problems, both fixed:**
1. **A one-comment conflict with the 2027 branch.** Fixed by keeping the Precious Grace comment's first line unchanged
   (`d54640d0`).
2. **The capacity post-link growth check.** Its fixed 4 KB allowance for the replaced shipped catalogue was too small
   for 11 routes. It is now measured from the baseline ELF's on_point data symbols (5,405 B) plus 4 KB for strings
   (`61a29187`). Margin: 2,871 B. A negative control that is 4 KB short still fails.

## 8. What PROVISIONAL means (editorial and public notes)

For release notes and editorial copy. **The device UI is unchanged**: it still shows the word PROVISIONAL.

> **PROVISIONAL:** estimated departures; actual times may vary.

The release-note wording is as confirmed by the owner (2026-10-28). Background for the editorial copy: PROVISIONAL times
come from the operators' published first and last trips and the best available pattern, and buses may be held for
passengers or vehicles. **PUBLISHED** departures follow an operator-stated timetable pattern.

**Basis:**
- the operators' stated interval ranges (Bulacan P2P: 30–45 minutes, FO-RPF-20261028);
- the anecdotal dispatch observation OBS-20261028.
