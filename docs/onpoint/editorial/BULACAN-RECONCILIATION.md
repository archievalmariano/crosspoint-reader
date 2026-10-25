# BULACAN timetable reconciliation (pass 3)

**What this is:** an editorial reconciliation for the approved eight-route BULACAN catalogue (owner's brief, 2026-10-25,
superseding the shortlist in [BULACAN-EVIDENCE-REGISTER.md](BULACAN-EVIDENCE-REGISTER.md) §9 where they conflict).
Retrieval date for everything: **2026-10-25**. Documentation only. `OnPointScheduleData.cpp`, the labels, holiday
rules and effective dates are **unchanged**. No build, flash, push or release.

**Status terms:**
- Proposed firmware statuses use only ON POINT's existing **PUBLISHED / PROVISIONAL**.
- **READY / NEEDS RECONCILIATION / NOT COUNTDOWN-READY** describe implementation readiness.
- VERIFIED and UNVERIFIED remain internal register terms.

## 1. Primary evidence

### 1.1 Bulacan P2P: post BP2P-0830 (directly accessed by Claude)

- **Post:** https://www.facebook.com/BulacanP2P/posts/pfbid0P3wmxSh7Lcs4DCPx4Zta35uhKf6JYa2dgVNBrNMoTZqmvExUQrwu9wbuGEjDyyUfl
- **Access:** public, without login. The text, the five images and the first image's caption page were readable. Other
  caption pages were cut off by the login wall.
- **Publication date:** displayed as **"August 30"**, with **no year shown**. Facebook omits the year for posts from
  the current year, and the comment ages fit (6–7 weeks old, including one dated "Aug.31(today)"). It is recorded as
  **2026-08-30, with the year inferred from display convention, not shown**.
- **Post text (summary):**
  - It is an "updated schedule" of trips from Bulacan to Quezon City, with the times in each image's caption.
  - A disclaimer says the operator does **not** run Sta. Maria ↔ TriNoma. That confirms Caypombo is not a Bulacan P2P
    service.
- **Each image** carries the disclaimer "Schedule are subject to change without prior notice".
- **No image states an interval or frequency.**

| Image | Photo fbid | Heading | Contents (first – last) |
|---|---|---|---|
| **BP2P-0830-1** | 122279968262249352 | Saturday/Sunday/Holiday | Trinoma–Balagtas 06:30–22:30 · Trinoma–Pandi 06:30–21:30 · Trinoma–Plaridel 06:30–21:00 · Trinoma–Guiguinto 06:30–21:30 · SM North Edsa–Plaridel 06:30–22:30 |
| **BP2P-0830-2** | 122279968346249352 | Saturday/Sunday/Holiday | Balagtas 05:00–20:00 · Pandi 04:30–19:30 · Plaridel 04:30–20:30 · Bocaue Crossing 05:00–20:00 · Walter Mart Guiguinto 05:00–19:30 |
| **BP2P-0830-3** | 122279968256249352 | Tuesday to Friday | Balagtas 04:30–20:00 · Pandi 04:30–19:30 · Plaridel 04:00–20:30 · Bocaue Crossing 04:30–20:00 · Walter Mart Guiguinto 04:30–19:30 |
| **BP2P-0830-4** | 122279968430249352 | Monday to Friday | Trinoma–Balagtas 06:00–22:30 · Trinoma–Pandi 06:00–21:30 · Trinoma–Plaridel 06:00–21:00 · Trinoma–Guiguinto 06:00–21:30 · SM North Edsa–Plaridel 06:00–22:30 |
| **BP2P-0830-5** | 122279968370249352 | Monday | Balagtas 04:30–20:00 · Pandi 03:30–19:30 · Plaridel 03:30–20:30 · Bocaue Crossing 04:30–20:00 · Walter Mart Guiguinto 04:30–19:30 |

**Image identification:** -1 was confirmed as fbid …262 on its photo page. The order of the others follows the
post's document order. Claude read all five images, and they match the owner's transcription exactly.

**What the headings show:**
- The **Bulacan-side** images (-2, -3, -5) name **origins only, not destinations**.
- The **Metro-side** images (-1, -4) name each pair.

**Archive:** facts-only transcription above. No image copies were downloaded; downloading is a separate permission.
The owner's own screenshots are the evidence copies.

### 1.2 Precious Grace: owner-supplied graphics (not seen by Claude)

> **Pass 4 update:** the owner supplied the screenshots, and they are now seen. The values below are confirmed.
> - **PG-G2 is headed "P2P DAILY TRIP SCHEDULE".**
> - The yellow Angat graphic's post caption refers to a last-trip policy "as of May 1, 2026".
>
> See [BULACAN-READINESS.md](BULACAN-READINESS.md).
>
> **Pass 5:** the chronology is established.
> - PG-G1 was posted **2025-12-30** (year shown), as the "2026 Trip Schedule".
> - PG-G2 was posted **March 13**; 2026 is inferred. **It supersedes G1:** Caypombo's last trip is 20:30.
> - The Angat advisory was posted **May 1**; 2026 is inferred. It states effect "as of May 1, 2026".

The two Precious Grace graphics are referred to in the brief, but the images were **not available in this session**.
The values below are the owner's transcription. Post permalinks, publication dates and any day-type wording are
**unknown**. PG's public page shows only its intro (register §3).

| Image | Caypombo / Sta. Clara | Malolos | Bocaue | North EDSA (TriNoma and SM) | Stated interval |
|---|---|---|---|---|---|
| **PG-G1** | 05:00–20:00 (shared row with Malolos) | 05:00–20:00 | 05:30–20:00 | 06:00–22:30 | 30 min |
| **PG-G2** | 05:00–**20:30** | 05:00–20:00 | 05:30–20:00 | 06:00–22:30 | 30 min |

**Interpretation limits (preserved):**
- The **G1 vs G2 conflict** on Caypombo's last trip is unresolved until the chronology is known.
- **One "North EDSA" row serves every PG destination.** It doesn't say whether each destination gets a bus every 30
  minutes, or the 30 minutes are shared. Nor does it say which of TriNoma or SM North the time applies to.
- **Caypombo and Sta. Clara share one row, with identical times.** A bus can't leave both at the same minute, so
  these are service windows by boarding area, not stop-by-stop times. That supports Sta. Clara as a stop or alias
  (register §5.3), but leaves its scheduled time unknown.
- **Day applicability is not given** in the transcription. It is unknown whether the windows apply daily.

### 1.3 Precious Grace: Angat ↔ Monumento graphic (owner-supplied, not seen)

| Direction | First trip | Last trip | Interval |
|---|---|---|---|
| Angat → Monumento | 03:30–04:00 | 19:00 | 10–30 min |
| Monumento → Angat | 05:30 | 23:00 | 10–30 min |

**No other source** (P2PBus.ph, Sakay, directories, news) lists this service.

### 1.4 Secondary and corroborating sources

**P2PBUS** (https://www.p2pbus.ph/schedules, re-read directly on 2026-10-25):
- Undated; footer "©2026" only.
- **Every Bulacan page lists Monday–Friday only.** The single exception is Plaridel → SM North, labelled "Monday –
  Sunday".
- There are **no weekend or holiday arrays**.
- There are **no pages** for Guiguinto, Angat or Monumento.

**Others:** SAKAY, CT-PGT and LAM-2022 are as in the register §2.

## 2. Reconciliation table (each direction separately)

**Legend:**
- **Op window:** the operator's first–last trips.
- **P2PBUS list:** individually listed departures, as listed. "Pattern" means Claude's description of that list, not
  a P2PBUS claim.
- **Derivation:** the array that could be built and its basis. "None" means no defensible array exists.

### 2.1 Bulacan P2P

| # | Direction | Op evidence (image) | Op window by day type | Op interval | P2PBUS list (M–F only) | Agreement / conflict | Derivation | Status | Readiness | Needed before implementation |
|---|---|---|---|---|---|---|---|---|---|---|
| 1 | **Balagtas → TriNoma** | -5, -3, -2 | Mon 04:30–20:00 · Tue–Fri 04:30–20:00 · SSH 05:00–20:00 | none stated | 04:30…20:00, 30-min pattern, **32** | **Mon–Fri agrees exactly.** Weekend: no secondary list. SAKAY (04:00–20:00 daily) conflicts on the first trip and is outranked. | **M–F:** the P2PBUS list (32), operator-bounded. **SSH:** none sourced. A 30-min array (05:00–20:00, 31) would be an interpolation. | PROVISIONAL | **M–F READY (PROVISIONAL); SSH NEEDS RECONCILIATION** | A weekend departure list, or an operator interval statement. The meaning of "Holiday" (§4). |
| 2 | **TriNoma → Balagtas** | -4, -1 | M–F 06:00–22:30 · SSH 06:30–22:30 | none | 06:00…22:30, 30-min, **34** | M–F agrees exactly. SSH: no list. | M–F: P2PBUS (34). SSH: none sourced (06:30–22:30 every 30 = 33 would be interpolation). | PROVISIONAL | M–F READY; SSH NEEDS RECONCILIATION | As row 1 |
| 3 | **Pandi → TriNoma** | -5, -3, -2 | **Mon 03:30**–19:30 · Tue–Fri 04:30–19:30 · SSH 04:30–19:30 | none | 04:30…**19:00**, 30-min, **30** | **Conflict:** last trip 19:30 (op) vs 19:00 (P2PBUS). Monday 03:30 is absent from P2PBUS. SAKAY says 04:00–19:30 daily. | None without extending P2PBUS. *Option, not adopted:* P2PBUS + 19:30 for Tue–Fri, + 03:30 (+ 04:00?) for Monday. That would be an invented departure unless the operator confirms it. | PROVISIONAL (once resolved) | **NEEDS RECONCILIATION** | Operator departures or interval. Whether the 03:30 Monday trip is a single extra trip. The 19:30 trip. The weekend list. Which Pandi boarding point (Mapulang Lupa vs Poblacion). |
| 4 | **TriNoma → Pandi** | -4, -1 | M–F 06:00–21:30 · SSH 06:30–21:30 | none | 06:00…**21:00**, 30-min, **31** | **Conflict:** last trip 21:30 vs 21:00. | None (as row 3) | PROVISIONAL (once resolved) | NEEDS RECONCILIATION | The 21:30 trip. Interval. Weekend list. |
| 5 | **Plaridel → TriNoma** | -5, -3, -2 (**origin "PLARIDEL" only; destination not stated**) | Mon **03:30**–20:30 · Tue–Fri 04:00–20:30 · SSH 04:30–20:30 | none | 04:00…20:30, 30-min, **34** | The Tue–Fri window agrees. Monday 03:30 is absent from P2PBUS. **SAKAY lists 04:00, 05:15 … 20:15 (75-min), conflicting.** The operator does not say which Plaridel departures go to TriNoma vs SM North (§3). | None yet. P2PBUS Tue–Fri would be defensible **only if** all Plaridel departures go to TriNoma, which O3 contradicts. | PROVISIONAL (once resolved) | **NEEDS RECONCILIATION** | **Departures by destination from Plaridel.** The 30- vs 75-min interval. Monday 03:30. The weekend list. |
| 6 | **TriNoma → Plaridel** | -4, -1 | M–F 06:00–**21:00** · SSH 06:30–21:00 | none | **05:30**…**22:00**, 30-min, **34** | **Conflict at both ends:** 06:00 vs 05:30, and 21:00 vs 22:00. P2PBUS looks stale or mixed with SM North. | None (P2PBUS outside the operator window at both ends) | PROVISIONAL (once resolved) | NEEDS RECONCILIATION | An operator departure list or interval |
| 7 | **Plaridel → SM North EDSA** | No separate operator row (Bulacan side lists "PLARIDEL" only). OBS O3: ridden. | Unknown: within Plaridel's 03:30/04:00/04:30–20:30 windows | none | 05:30…**22:30**, 30-min, 35, labelled Mon–Sun | **P2PBUS is implausible:** a 22:30 last trip from Plaridel exceeds the operator's 20:30 Plaridel last trip, and the list mirrors the SM North → Plaridel page. | None | — | **NEEDS RECONCILIATION** (no defensible array) | As row 5 |
| 8 | **SM North EDSA → Plaridel** | -4, -1 | M–F 06:00–22:30 · SSH 06:30–22:30 | none | **05:30**…22:30, 30-min, **35** | **Conflict:** first trip 06:00 vs 05:30. The window **differs from TriNoma → Plaridel** (last trip 22:30 vs 21:00). | None without truncation. *Option, not adopted:* P2PBUS minus 05:30 = 06:00…22:30 (34), which removes a listed trip. | PROVISIONAL (once resolved) | NEEDS RECONCILIATION | Operator confirmation of the 06:00 start and the interval. Weekend list. |
| 9 | **Guiguinto → TriNoma** | -5, -3, -2 ("Walter Mart Guiguinto") | Mon 04:30–19:30 · Tue–Fri 04:30–19:30 · SSH 05:00–19:30 | none | **no page** | SAKAY: 04:30–19:30 daily (range only). This agrees with the weekday window. | **None.** No departure sequence or interval in any source. | — | **NOT COUNTDOWN-READY** (no departures) | Any departure list or a stated interval |
| 10 | **TriNoma → Guiguinto** | -4, -1 | M–F 06:00–21:30 · SSH 06:30–21:30 | none | no page | SAKAY: 06:00–21:30 daily (range only). Agrees for weekdays. | None | — | NOT COUNTDOWN-READY | As row 9 |
| — | *Bocaue Crossing* (stop on Balagtas, O1) | -5, -3, -2 | Mon/Tue–Fri 04:30–20:00 · SSH 05:00–20:00 | none | — | **Identical to Balagtas's windows.** A bus can't leave Balagtas and pass Bocaue Crossing at the same minute, so either these are windows by boarding point, or some trips start at Bocaue Crossing. | Not a route | — | Not applicable | Whether any trips start at Bocaue Crossing. Its scheduled time relative to Balagtas (for wording only). |

### 2.2 Precious Grace

| # | Direction | Op evidence | Op window | Op interval | P2PBUS list (M–F only) | Agreement / conflict | Derivation | Status | Readiness | Needed before implementation |
|---|---|---|---|---|---|---|---|---|---|---|
| 11 | **Caypombo → SM North EDSA** | PG-G1, PG-G2 (undated, days unknown) | G1 05:00–**20:00** · G2 05:00–**20:30** | 30 min (both) | 05:00…10:00 every 30, 11:00…15:00 hourly, 15:30…19:00 every 30, then **19:45** = **25** | **G1 vs G2** last trip. Both conflict with P2PBUS's **midday hourly gap** and its **19:45** last trip. Directories give a later weekend start (06:00). SAKAY attributes it to "UBE Express". | **G1:** 05:00–20:00 every 30 = **31**, which is **identical to the shipped array**. **G2:** 05:00–20:30 every 30 = 32. Neither can be chosen without chronology. | PROVISIONAL | **NEEDS RECONCILIATION** | The dates of G1 and G2 and which is current. The day applicability (weekend 06:00?). Whether the 30-min interval holds through midday. Whether the time is at SM North or TriNoma. The "may shift with ridership" caveat (§4). |
| 12 | **SM North EDSA → Caypombo** | PG-G1, PG-G2 ("North EDSA (TriNoma and SM)") | 06:00–22:30 (both) | 30 min | 06:00…10:00, 11:00…15:00 hourly, 15:30…22:30 = **29** | G1 and G2 agree. P2PBUS's midday hourly gap conflicts. The shared North EDSA row is ambiguous across destinations. | 06:00–22:30 every 30 = **34**, **identical to the shipped array** | PROVISIONAL | NEEDS RECONCILIATION | Whether the North EDSA row gives each destination its own half-hourly departures. The departure stop. Day applicability. Midday. |
| 13 | **Malolos → TriNoma** | PG-G1, PG-G2 (both 05:00–20:00) | 05:00–20:00 | 30 min | 05:00…10:00, 11:00…15:00 hourly, 15:30…20:00 = **26** | G1 and G2 agree. **Conflict:** P2PBUS midday hourly (26), and SAKAY "UBE Express" **hourly** (16 weekdays; weekends 06:00–20:00, 15). The PG graphics and the owner's queue observation support **Precious Grace**. | 05:00–20:00 every 30 = 31 (operator-derived), *if* the interval holds all day | PROVISIONAL | NEEDS RECONCILIATION | Dates and day applicability. 30-min vs midday-hourly vs hourly. Formal operator attribution (PG vs UBE). Whether the times are at TriNoma or SM North. |
| 14 | **TriNoma → Malolos** | PG-G1, PG-G2 | 06:00–22:30 | 30 min | 06:00…10:00, 11:00…15:00 hourly, 15:30…22:30 = **29** | As row 13. SAKAY (UBE): M–F 06:00–22:30, weekends 07:00–22:00. | 06:00–22:30 every 30 = 34, conditional as in row 13 | PROVISIONAL | NEEDS RECONCILIATION | As row 13 |
| 15 | **Angat → Monumento** | PG Angat graphic | First 03:30–04:00, last 19:00 | **10–30 min (variable)** | no page | No corroboration anywhere | **None.** A countdown can't be built from a variable interval. | — | **NOT COUNTDOWN-READY** | A fixed departure list. Without one it stays a documented coverage candidate only. No frequency mode is proposed. |
| 16 | **Monumento → Angat** | PG Angat graphic | 05:30–23:00 | 10–30 min | no page | — | None | — | NOT COUNTDOWN-READY | As row 15 |

## 3. Answers to the brief's questions

**Q1. Which Bulacan P2P directions can be provisionally populated from P2PBus.ph without contradicting operator
evidence?**
- **Only Balagtas → TriNoma and TriNoma → Balagtas, Monday–Friday.** P2PBUS matches the operator windows exactly
  (rows 1–2).
- Plaridel → TriNoma, Tuesday–Friday, also matches the window. But it can't be used until the operator shows which
  Plaridel departures go to TriNoma (row 5).
- Every other P2PBUS direction **conflicts** with BP2P-0830 at the first or last trip:
  - Pandi: both directions;
  - TriNoma → Plaridel;
  - SM North → Plaridel;
  - Plaridel → SM North.
- **No direction has a sourced weekend or holiday array.** P2PBUS is weekday-only.

**Q2. Which directions need Monday, Tuesday–Friday and weekend/holiday distinctions?**
- **Pandi → TriNoma** and **Plaridel → (TriNoma / SM North)** need three day types: Monday has a 03:30 first trip.
- **Balagtas, Guiguinto and Bocaue Crossing:** the Monday window equals Tue–Fri, so Mon–Fri + SSH is enough.
- **All Metro-outbound directions:** Mon–Fri + SSH.
- **The model can represent all of this.** `ServiceRule.weekdayMask` allows a Monday-only mask (bit 1) separate from
  Tue–Fri. Holidays use `DateOverride`, as Balagtas does today.

**Q3. Are TriNoma ↔ Plaridel and SM North ↔ Plaridel independent services or terminal variants?**
- **From Metro Manila: independently scheduled.** BP2P-0830-4 and -1 give separate windows with the same first trip
  and different last trips (21:00 vs 22:30). That isn't consistent with one bus calling at both stops in fixed order.
- **From Plaridel: unresolved.** The operator lists a single "PLARIDEL" origin with no destination split. The owner's
  ride (O3) proves SM North-bound departures exist.
- **Readiness:** two pairs are justified editorially (as approved), but neither Plaridel-origin direction can be
  populated until the operator gives departures by destination.

**Q4. Can Caypombo and Malolos be derived reliably from Precious Grace's 30-minute graphics?**
- **Arithmetically yes, reliably not yet.**
- **Malolos is the more consistent case:** both graphics give 05:00–20:00 and 06:00–22:30, so 31 and 34 trips.
- **Caypombo has two conflicting last trips** (20:00 vs 20:30).
- **Both are blocked by:**
  - the unknown publication dates;
  - the unknown day applicability;
  - the shared "North EDSA" row;
  - P2PBUS's midday thinning;
  - for Malolos, SAKAY's hourly "UBE" listing.
- **Notably,** the shipped Caypombo arrays equal the PG-G1 derivation exactly. That supersedes the register's pass-1
  finding that the shipped data "conflicts with every current source". It is now **operator-consistent with G1** but
  still **conflicting with G2 and the secondary midday pattern**.

**Q5. Which schedules remain unsuitable for fixed countdowns?**
- **Angat ↔ Monumento** (both directions): the 10–30-minute variable interval rules it out.
- **Guiguinto ↔ TriNoma** (both directions): no departure sequence exists in any source.
- **Plaridel → SM North:** no operator window of its own, and an implausible secondary list.
- Every **Precious Grace** direction stays NEEDS RECONCILIATION until the midday pattern and the chronology are
  settled.

**Q6. How do the additions compare with the existing catalogue and the current Balagtas and Caypombo data?**
- **Catalogue size:**
  - Today: 7 route pairs (BULACAN 2, NORTH LUZON 3, AIRPORT TRANSFERS 2).
  - With all eight BULACAN pairs: 13. That is well within `MAX_CATALOGUE_ROUTES` (128) and the 60-route capacity
    fixture already validated.
  - With only the countdown-capable candidates: at most 11, because Angat and Guiguinto stay out.
- **Shipped Balagtas data:**
  - BP2P-0830 now backs **every window** of the shipped Balagtas data, including the Sat/Sun/**Holiday** grouping. The
    holiday rule was unevidenced in pass 1.
  - The weekday intermediate departures match P2PBUS.
  - The weekend intermediate departures (every 30 min) have **no source**.
  - Since no image states an interval, the arrays rest on a secondary source. **PROVISIONAL is the defensible label,
    not PUBLISHED.**
- **Shipped Caypombo data:**
  - It matches PG-G1 exactly.
  - It runs daily, while the PG graphics' day applicability is unknown and directories give a later weekend start.
  - **PROVISIONAL is the defensible label** until G1 vs G2 and the day rules are resolved.

## 4. Model and interface limitations (reported, not changed)

1. **One status per direction.** `Schedule` has one `SourceStatus`. Balagtas mixes sourced weekday arrays with
   unsourced weekend arrays, so the direction can only be as strong as its weakest rule: PROVISIONAL. Per-rule status
   would need a model change. **Not proposed.**
2. **"Holiday" is undefined by the operator.** BP2P's "Saturday/Sunday/Holiday" doesn't say which holidays. The
   firmware applies the weekend timetable on 21 non-working 2026 dates; special working days keep weekdays. That
   mapping is a reasonable reading, but the operator hasn't confirmed it. Holiday behaviour is unchanged, and the 2027
   holiday branch is not merged.
3. **No stop or alias display.** A board can't say "also boards at Bocaue Crossing" or "times at TriNoma; also calls
   at SM North". This is acceptable: these stay in the register. A note string could carry them later.
4. **No frequency mode** (by decision). Angat ↔ Monumento can't be shown as a countdown.
5. **"Departures may shift with ridership"** appears in the shipped Caypombo note. No accessed or transcribed source
   in this pass contains that wording. The owner should say whether the PG graphics carry it.

## 5. Recommended next implementation batch (needs separate authorisation)

**Batch A: correct existing provenance. No new routes; small and low-risk.**
1. Relabel **Balagtas ↔ TriNoma** (both directions) **PROVISIONAL**. Keep the arrays, the holiday overrides and the
   effective date 2026-09-01: BP2P-0830 is dated 2026-08-30 and supports the windows. Update the note to cite
   BP2P-0830 and P2PBUS, and to say "weekend intermediate times not published".
2. Relabel **Caypombo ↔ SM North EDSA** (both directions) **PROVISIONAL**. Keep the arrays, because they equal the
   PG-G1 derivation. Update the note to cite PG-G1, and record the G2 (20:30) conflict in the register.
3. Host tests and the release config guard only. Device work is optional and wasn't requested: a label change carries
   no device risk beyond a routine install.

**No new route enters firmware in Batch A**, because none is READY. The approved catalogue becomes implementable as
evidence arrives:

**Batch B (conditional):** add **Malolos ↔ TriNoma** as PROVISIONAL with the operator-derived 31 / 34 arrays, once
the owner confirms:
- the PG graphics' dates and day wording;
- that the 30-minute interval runs through midday.

**Batch C (conditional):** add **Pandi**, **Plaridel ↔ TriNoma** and **Plaridel ↔ SM North** once BP2P gives
departures, or an interval, by destination. Use three day types for the Bulacan side.

**Held as coverage candidates:**
- **Guiguinto ↔ TriNoma:** no departure sequence.
- **Angat ↔ Monumento:** variable interval.

**Evidence requests, in order of value:**
1. **BP2P:** a full departure list, or an explicit interval, for Pandi and Plaridel (split by TriNoma vs SM North),
   Guiguinto, and Balagtas weekends.
2. **PG-G1 and PG-G2:** permalinks or post dates, and any day-type and "ridership" wording. Also, whether the
   half-hourly interval holds between 10:00 and 15:00.
3. **PG:** the North EDSA departure stop and order (TriNoma / SM North), per destination.
4. **Malolos operator attribution:** PG vs UBE Express.

## 6. Change log

| Date | Change |
|---|---|
| 2026-10-25 | Pass 5: PG chronology. G2 supersedes G1. See BULACAN-READINESS.md. |
| 2026-10-25 | Pass 4: owner screenshots seen. The readiness table is in BULACAN-READINESS.md. |
| 2026-10-25 | Created (pass 3). BP2P-0830 accessed directly and transcribed per image. PG graphics recorded from the owner's transcription. P2PBUS re-read for all 11 Bulacan pages. |
