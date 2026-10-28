# ON POINT: BULACAN coverage inventory and research needs (planning only)

**Status:** an inventory of what's in the catalogue today, and what has to be researched before BULACAN can be called
complete and verified. **No timetables were gathered or changed, and no catalogue or firmware changes were made.**
Research starts only after the owner approves the editorial scope (§5).

**Sources for this inventory:**
- `src/activities/on_point/OnPointScheduleData.cpp`;
- `OnPointRegionData.cpp`;
- commit `ceebac2a` ("use operator timetables and holidays", 2026-09-26);
- the public ON POINT page.

## 1. What BULACAN contains today

The collection `bulacan` has 2 routes, i.e. 4 directional schedules.

| Route | Operator (as recorded) | Direction | Service days | First – last | Interval | Trips | Holidays |
|---|---|---|---|---|---|---|---|
| Balagtas ↔ TriNoma | Bulacan P2P | Balagtas → TriNoma | Mon–Fri | 04:30 – 20:00 | 30 min | 32 | 21 dates in 2026 use the weekend timetable |
| | | | Sat, Sun, holidays | 05:00 – 20:00 | 30 min | 31 | |
| | | TriNoma → Balagtas | Mon–Fri | 06:00 – 22:30 | 30 min | 34 | as above |
| | | | Sat, Sun, holidays | 06:30 – 22:30 | 30 min | 33 | |
| Caypombo ↔ SM North EDSA | Precious Grace | Caypombo → SM North EDSA | Daily | 05:00 – 20:00 | 30 min | 31 | none modelled |
| | | SM North EDSA → Caypombo | Daily | 06:00 – 22:30 | 30 min | 34 | none modelled |

**Metadata recorded for all 4 schedules:**
- `effectiveDate` 2026-09-01;
- `sourceStatus` Verified (shown as PUBLISHED);
- time zone Asia/Manila, UTC+8.

**Notes held in the code:**
- Balagtas: "Bulacan P2P official posts: first/last trips, 30-minute interval; holidays use the weekend timetable"
  (plus "as P2PBus.ph lists").
- Caypombo: "Precious Grace official post (current)… departures may shift with ridership".

**Holidays:** Balagtas ↔ TriNoma takes its weekend timetable on the 21 non-working days of 2026 (Proclamation
No. 1006, plus the NCR ASEAN Summit days, from `util/MarkedDates.h`). Special working days keep the weekday timetable.

## 2. Provenance assessment

| Gap | Detail | Risk |
|---|---|---|
| **No source references** | No URL, post date, page name or capture date is recorded anywhere (code, commit, docs). "Official posts" and "P2PBus.ph" are descriptions, not references. | The data can't be re-verified or audited, and its age isn't known. |
| **Times are derived, not published lists** | Every departure is generated from a first trip, a last trip and a 30-minute interval. The individual times weren't published as a list. | The real departures may not fall on exact 30-minute steps (skipped or peak-only trips, breaks). Precious Grace itself says departures "may shift with ridership". |
| **`effectiveDate` isn't the operator's date** | 2026-09-01 is a catalogue date, not a date the operators stated. | It can't show how current a timetable is. |
| **Verified status on derived data** | Shown as PUBLISHED although the departures are interpolated. | The label promises more than the source supports. Whether derived departures may be Verified is a decision (§5). |
| **Holiday coverage ends with 2026** | Balagtas has 2026 holidays only. The 2027 table is on the separate, unreleased `feature/ph-holidays-2027` branch, pending operator verification. | From 2027-01-01, Balagtas holidays fall back to the weekday timetable without any notice. |
| **Precious Grace holidays unknown** | Caypombo runs one daily timetable with no overrides. Whether it changes on holidays was never recorded. | Holidays may be wrong in either direction. |
| **One source each** | Neither route was cross-checked against a second source or the operator directly. | Single points of failure. |

**Consequence:** BULACAN today is **two corridors with unaudited, derived timetables**. Before it can be called
"complete and verified", both existing routes need re-verification against current, referenced operator sources.

## 3. What has to be researched (information needed)

### 3.1 For each existing route (re-verification)

1. **The current official source:** the operator's own channel (page, post, website or terminal notice), with its URL,
   publication date and the date we captured it. Keep an archived copy (a screenshot or saved page, locally).
2. **Departures:** the published list if one exists. Otherwise the first and last trips plus the exact interval rule,
   and whether trips are guaranteed or "subject to load".
3. **Service-day classes:** weekday, Saturday and Sunday as published, including whether Saturday differs from Sunday.
4. **Holiday policy:** which holidays (regular, special non-working, local) take which timetable, and whether any days
   have no service.
5. **Exceptions:** announced suspensions, special trips, Holy Week and Christmas schedules.
6. **Endpoints:** the official terminal and stop names as the operator writes them, and any intermediate pick-up points
   (we don't model them yet; recorded for later).
7. **Status:** still operating, and for how long the posted schedule has held.

### 3.2 Coverage discovery (new BULACAN routes)

1. **Operators** running scheduled commuter services from Bulacan origins into Metro Manila: premium P2P and other
   fixed-schedule bus services.
2. **For each:** origin locality and terminal, Metro Manila destination, schedule type (fixed departures vs. headway
   only), and whether an official schedule is published.
3. **Localities to check** (not a claim that services exist): the larger Bulacan origins of Metro Manila commuting,
   such as Malolos, Meycauayan, San Jose del Monte, Marilao, Bocaue, Balagtas, Guiguinto, Plaridel, Baliuag, Santa
   Maria and Norzagaray. The approved scope sets the list.
4. **Rail:** whether PNR or North–South Commuter Railway services to Bulacan are in operation with published
   timetables. Rail is a scope decision (§5).

### 3.3 Fit with the existing data model (no change proposed)

**Supported:**
- explicit departures per service-day class (weekday masks);
- dated overrides, including no-service days;
- cross-midnight trips;
- one effective-from date;
- a Verified/Provisional status;
- a free-text note.

**Not modelled:**
- headway-only services ("every 10–15 min");
- intermediate stops;
- date-ranged seasonal timetables (only per-date overrides);
- fares;
- a stored source reference or capture date. Today that lives only in comments.

**Capacity:** 7 routes today; validated up to 60 routes (P0 timing, P1 size guards); hard limit 128.

## 4. Proposed research procedure (for approval; not started)

1. **Re-verify Balagtas ↔ TriNoma and Caypombo ↔ SM North EDSA** against current official sources (§3.1). Record
   references and archive copies.
2. **Discover other BULACAN operators and routes** (§3.2). For each, record "has an official fixed schedule" yes or no,
   and include only the yeses.
3. **Fill a per-route source record** (§3.1 fields) before writing any timetable data. Status Provisional until the
   source standard (§5) is met.
4. **Review:** the owner, then Codex for data correctness. Only after that, catalogue changes, in a separate approved
   step.
5. **The same template then for CAVITE, LAGUNA and RIZAL.** NORTH LUZON and AIRPORT TRANSFERS get re-verified later.

## 5. Decisions needed before research

1. **Mode scope:** fixed-schedule buses only (P2P and similar), or also rail? Headway-only services can't be shown as
   departures; leave them out or note them?
2. **The meaning of BULACAN:** Bulacan origins into Metro Manila only, or also routes within Bulacan?
3. **Source standard for Verified:** which channels count as official (the operator's own page, website, terminal
   notice or direct confirmation), and whether **derived** departures (first/last plus interval) may ever be Verified,
   or must stay Provisional.
4. **Evidence:** keep a dated archive copy of every source locally (outside Git)? Recommended.
5. **Provenance in the data:** add a source reference and capture date per schedule (a future data-model change), or
   keep a separate source register file alongside the catalogue?
6. **Holidays:** verify each operator's holiday policy before modelling it (no assumptions), and how to handle 2027
   once its proclamation is verified.
7. **Review cadence:** how often a published timetable is re-checked, and what to do when a source can't be confirmed
   (Provisional, or remove).

## 6. Decisions taken and research status (2026-10-25)

All seven decisions above were taken by the owner:
- **Scope:** P2P only.
- **Geography:** Bulacan, including intra-Bulacan services.
- **Verified:** dated, attributable operator evidence. A derivation is allowed when the operator states an exact
  interval.
- **Archives:** dated references.
- **Provenance:** a separate register.
- **Holidays:** verified per operator, never inferred.
- **Re-verification:** a 90-day target.

**Research pass 1** is recorded in [editorial/BULACAN-EVIDENCE-REGISTER.md](editorial/BULACAN-EVIDENCE-REGISTER.md):
the evidence register, the route-identity assessment, the coverage gaps and the ranked shortlist. No firmware,
timetable, label or holiday rule was changed.

**Pass 3 (2026-10-25):** the owner approved an **eight-route BULACAN catalogue**:
- Balagtas, Pandi, Plaridel ↔ TriNoma, Plaridel ↔ SM North, Guiguinto (all Bulacan P2P);
- Caypombo, Malolos, Angat ↔ Monumento (all Precious Grace).

Each direction is reconciled in [editorial/BULACAN-RECONCILIATION.md](editorial/BULACAN-RECONCILIATION.md):
- **READY:** only the Balagtas Monday–Friday directions, as PROVISIONAL.
- **NOT COUNTDOWN-READY:** Guiguinto (no departure list) and Angat ↔ Monumento (variable 10–30-minute interval).
- **NEEDS RECONCILIATION:** everything else.

The recommended next batch relabels the shipped Balagtas and Caypombo schedules PROVISIONAL and adds no routes.

**Pass 4 (2026-10-25):** [editorial/BULACAN-READINESS.md](editorial/BULACAN-READINESS.md) applies the owner's
PUBLISHED / PROVISIONAL / NOT COUNTDOWN-READY standard.
- **Proposed first batch:** Balagtas, Caypombo, Pandi, Plaridel ↔ TriNoma and Malolos. That is five pairs, with
  Malolos outbound PUBLISHED.
- **Held:** Plaridel ↔ SM North, Guiguinto and Angat ↔ Monumento.
- **The 2027 holiday gap (§2) remains open.**

**Pass 6 (2026-10-25):** the first batch is narrowed to **Balagtas, Caypombo and Malolos**.
- **Plaridel:** one through-service, preferred designation Plaridel ↔ SM North EDSA; deferred.
- **Pandi:** deferred, because its weekend assumption is contradicted.
- **Guiguinto:** stays NOT COUNTDOWN-READY. A 2025 terminal timetable was transcribed.

See [editorial/BULACAN-READINESS.md](editorial/BULACAN-READINESS.md) §6.

**Pass 7 (2026-10-25):** the owner reinstated the **five-route first batch**: Balagtas, Caypombo, Pandi, Plaridel ↔
TriNoma and Malolos.
- **Plaridel ↔ SM North:** a documented candidate only.
- **Guiguinto:** stays out. The September 2025 terminal notice supports a weekday outbound candidate, but its weekend
  lists conflict with the 2026 windows and there is no return list.
- **March 2026 Guiguinto photo:** archived as a possible disruption-period schedule.

See [editorial/BULACAN-READINESS.md](editorial/BULACAN-READINESS.md) §7.

**Batch 1 implemented (2026-10-25, local, `1aaedbf4`):** six BULACAN pairs (Balagtas, Caypombo, Pandi, Plaridel ↔
TriNoma, Malolos, Guiguinto).
- **Guiguinto** is a development reconstruction and is **not cleared for public release**.
- **Plaridel ↔ SM North:** deferred.
- **Angat ↔ Monumento:** excluded.

See [editorial/BULACAN-BATCH1.md](editorial/BULACAN-BATCH1.md).

**2026-10-28:** owner decisions confirmed: effective date, Caypombo wording, identities, the first-trip evidence and
Guiguinto's risk acceptance. **2027 holidays integrated** (`f5a2127f`): every Bulacan P2P direction now follows
Balagtas's policy, and Precious Grace stays daily. This supersedes the "Holiday coverage ends with 2026" gap in §2.
**Open:** the Proclamation No. 1427 classification check, and the Eid dates.
