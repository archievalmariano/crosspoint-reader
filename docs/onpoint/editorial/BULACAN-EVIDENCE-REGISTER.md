# BULACAN evidence register

Editorial provenance for ON POINT's BULACAN collection. This file is the evidence register. It is version-controlled
and kept separate from the firmware. Rows are keyed to the stable schedule IDs in
`src/activities/on_point/OnPointScheduleData.cpp`, or to an editorial candidate ID (`cand:…`). A candidate ID is not
a firmware ID.

**Research pass 1.** Retrieval date for all of it: **2026-10-25**. Documentation only. No timetable, label, holiday
rule, route entry or firmware was changed.

## 1. Protocol (applies to every row)

**Scope:**
- Fixed-schedule P2P commuter buses serving Bulacan, including routes boarded without a reservation.
- Excluded: ordinary provincial buses, rail and jeepneys.

**Evidence tiers:**

| Tier | Meaning |
|---|---|
| **VERIFIED** | A current, attributable operator or authoritative statement that was **directly accessed** and **dated by its publisher**. |
| **PROVISIONAL** | A statement that was directly accessed but is undated, secondary or partial. It can support a PROVISIONAL board. |
| **UNVERIFIED** | Referenced but not accessed, such as a search snippet; or a commuter comment; or a source older than one year with no corroboration. |

**Rules:**
- **Publication dates.** Dates are never inferred from search indexing. A page's "©2026" footer is not a publication
  date, and neither is a "last updated" banner on a directory page. Those are recorded as such and do not make the
  page an operator source.
- **Commuter content.** Commuter comments and posts in groups are leads only. They are never authoritative.
- **Derived arrays.** An operator-stated window plus an exact interval may support a derived array, with the
  derivation recorded. Approximate or ridership-dependent intervals never become guaranteed times.
- **Holidays.** Each operator's holiday policy must be stated by that operator. It is never inferred.
- **Archives.** Evidence copies are facts-only transcriptions, kept in this file: times, days, terminals and operator
  names. No page copies or images are stored. External archive snapshots (for example the Wayback Machine) were
  **not** requested, because that is an outward-facing action. The owner can decide on it.
- **Re-verification.** The target is 90 days. Provisional rows and rows with an announced change are checked earlier.

**Facebook access (limitation of this pass):**
- Facebook shows only a page's public intro and its latest post without login. Posts, photos and timetable images
  are behind the login wall.
- No login was attempted, and none will be: entering credentials is outside what Claude may do. Every timetable
  claimed to come from an "official post" therefore remains **not directly accessed** in this pass.

## 2. Sources accessed on 2026-10-25

| Key | Source | Kind | Access | Publisher date |
|---|---|---|---|---|
| FB-BP2P | https://www.facebook.com/BulacanP2P/ | Operator page (Bulacan P2P RPF, Pandi) | Direct: intro and latest post header only | Intro undated. Latest visible post is dated "October 14", 4:22 PM, titled TRAFFIC ADVISORY (body truncated) |
| FB-PGT | https://www.facebook.com/preciousgracetransport/ | Operator page (Precious Grace Transport Services) | Direct: intro only | Intro undated. The latest post (about 10 hours old) is unrelated, about interns |
| SAKAY | https://explore.sakay.ph/p2p and the route pages below | Route directory | Direct, in the built-in browser (WebFetch returned 403) | No date shown |
| P2PBUS | https://www.p2pbus.ph/schedules/… | Route directory | Direct | No date. Footer "©2026" only |
| CT-PGT | https://ph.commutetour.com/travel/transport/bus/precious-grace-transport/ | Directory | Direct | Banner "Last updated January 16, 2026" (the directory's own claim) |
| WV-SM | https://en.wikivoyage.org/wiki/Santa_Maria_(Bulacan) | Travel wiki | Direct | Terminal listing "updated Aug 2026" |
| AI-2018 | https://www.autoindustriya.com/truck-bus-news/dotr-launches-two-new-p2p-routes-from-trinoma-to-bulacan.html | News, credited to PIA-Bulacan | Direct | 2018-07-30 (`article:published_time`) |
| MM-2024 | https://www.moneymax.ph/government-services/blog/p2p-schedule-route-province-metro-manila | Blog | Direct (earlier in pass) | Updated 2024-03-21 |
| LUM | https://www.lumina.com.ph/news-and-blogs/blogs/p2p-schedules-guide-for-commuters-hassle-free-and-faster-travels/ | Blog | Search snippet | Undated (2023 per earlier pass; treat as old) |
| CT-BAL | https://ph.commutetour.com/ph/terminal/trinoma/ | Directory | Search snippet | Undated |
| TRIB-2025 | Daily Tribune, Guiguinto–QC launch | News | Search snippet | 2025-03-08 (as reported in the snippet; not accessed) |

**Not accessed:** the snippet saying a Bulacan P2P post changed last trips because of rain. It is **UNVERIFIED**, and
its permalink and date are unknown.

## 3. Operator statements (route existence only, no times)

**FB-BP2P** intro, quoted: "Bocaue Crossing to Trinoma vv, Balagtas to Trinoma vv, Pandi to Trinoma vv and Plaridel
to Trinoma / SM North EDSA vv and Walter Mart Guiguinto to Trinoma vv our only route."
- Address: No. 186 J. Bernardo St., Poblacion, Pandi, Bulacan.
- **Tier:** VERIFIED for operator and route existence. It is an operator statement, accessed directly, but undated.
  It is **no** evidence for times or holidays.

**FB-PGT** intro, quoted: "PGT P2P Service caters to travelers from Sta. Maria and Malolos, Bulacan to North Avenue
Quezon City (Trinoma) and vice versa with pick up and drop off at Bocaue tolgate."
- Contact on the page: (044) 815 5770, pgtransport2007@gmail.com.
- **Tier:** VERIFIED for operator and route existence (same caveats).
- **What it implies:**
  - Precious Grace's origins are **Santa Maria** and **Malolos**.
  - **Bocaue is a pick-up and drop-off point at the tollgate**, not a separate origin.
  - The Metro Manila end is described as "North Avenue … (Trinoma)". SM North EDSA is not named, though it faces
    TriNoma across North Avenue.

## 4. Existing firmware schedules: re-verification

All four are labelled PUBLISHED (`SourceStatus::Verified`), effective 2026-09-01, from commit `ceebac2a`
(2026-09-26). **No URL or date was recorded for the "official post" they cite.**

### 4.1 `p2p-balagtas-trinoma` / `p2p-trinoma-balagtas` (Bulacan P2P)

**Firmware:**

| Direction | Mon–Fri | Sat, Sun and 21 holidays (2026) |
|---|---|---|
| Balagtas → TriNoma | 04:30–20:00, every 30 min, 32 trips | 05:00–20:00, 31 trips |
| TriNoma → Balagtas | 06:00–22:30, every 30 min, 34 trips | 06:30–22:30, 33 trips |

**Evidence:**

| Source | Tier | Balagtas → TriNoma | TriNoma → Balagtas | Days | Notes |
|---|---|---|---|---|---|
| P2PBUS `trinoma-balagtas` | PROVISIONAL | 04:30–20:00, 32 listed, every 30 min | 06:00–22:30, 34 listed | **Mon–Fri only** | Matches the firmware weekday arrays exactly. Most likely the real origin of the firmware data. |
| SAKAY `R_GR0LVEE` | PROVISIONAL | 04:00–20:00 | 05:15–22:30 | "daily (Mondays to Sundays)" | Terminal "Ultra Mega Balagtas". ₱80. Range only, no departures. |
| CT-BAL (snippet) | UNVERIFIED | 04:30–20:00 (04:00 on Mondays, per earlier pass) | 05:30–22:30 | Daily | Fare ₱80 / ₱100 inconsistent |
| MM-2024 | UNVERIFIED (old) | 04:30–20:00 | 06:00–22:30 | — | The return is mislabelled "Trinoma → Bocaue" |
| LUM | UNVERIFIED (old) | Mon–Sat 04:00–18:30; Sun 04:30–20:30 | Mon–Sat 05:30–20:00; Sun 06:00–20:30 | — | Superseded by every later source |
| FB-BP2P | VERIFIED (existence) | — | — | — | Route exists |

**Assessment:**
- **Weekday:** consistent with P2PBUS. **Not** confirmed by any accessed operator timetable. SAKAY conflicts on the
  first trip (04:00 vs 04:30) and the first return (05:15 vs 06:00).
- **Weekend:** **no accessed source supports the firmware's weekend arrays** (05:00 / 06:30 first trips). SAKAY says
  the same hours run daily. P2PBUS lists weekdays only.
- **Holidays:** the firmware applies the weekend timetable on 21 dates. **No accessed operator statement of a holiday
  policy was found.** The code comment attributes it to "official posts", which could not be located.
- **Supersession:** unknown. No dated operator timetable was found to order the sources.
- **Reliability flag:** a rain-related last-trip change is reported (UNVERIFIED). Possible ad-hoc revisions.
- **Register tier for the shipped data:** weekday **PROVISIONAL**; weekend and holiday **UNVERIFIED**.
  - The firmware's PUBLISHED label is **not supported** by evidence accessed in this pass.
  - Not changed (see §8).

### 4.2 `p2p-caypombo-sm-north-edsa` / `p2p-sm-north-edsa-caypombo` (Precious Grace)

**Firmware:**
- Caypombo → SM North EDSA: **daily** 05:00–20:00, every 30 min, 31 trips.
- SM North EDSA → Caypombo: daily 06:00–22:30, 34 trips.
- No holiday overrides.
- The code comment says "departures may shift with ridership".

**Evidence:**

| Source | Tier | Caypombo → SM North | SM North → Caypombo | Days | Notes |
|---|---|---|---|---|---|
| P2PBUS `caypombo-bulacan-sm-north-edsa` | PROVISIONAL | 05:00–**19:45**, **25** listed: every 30 min to 10:00, then **hourly 11:00–15:00**, every 30 min from 15:30, last 19:45 | 06:00–22:30, **29** listed, the same midday hourly gap | **Mon–Fri** | Operator "Precious Grace" |
| SAKAY `R_ILMLMUJ` / `R_V4BB581` | PROVISIONAL | Weekdays 05:00–19:45; **weekends 06:00–19:45** | Weekdays 06:00–**10:30 AM** (probably a 10:30 PM typo); weekends 07:00–22:30 | Split weekday / weekend | Operator **"UBE Express"**. Stops: Caypombo → Sta Maria Junction → Bocaue Interchange → SM North EDSA. ₱100. |
| CT-PGT | PROVISIONAL | M–F 05:00–19:45; **Sat/Sun/Holiday** 06:00–19:45 | M–F 06:00–22:30; SSH 07:00–22:30 | M–F + **SSH** | ₱120. Also lists Caypombo ↔ TriNoma (TriNoma → Caypombo last trip **22:00**) |
| MM-2024 | UNVERIFIED (old) | 05:00–12:00 / 13:00–19:45 (noon break), to **TriNoma** | 06:00–12:00 / 13:00–22:30 | — | — |
| WV-SM | PROVISIONAL (wiki) | — | "every 30 minutes from 6:30AM to 10:30PM for Caypombo" | — | Terminal "Precious Grace P2P Transport Terminal, Norzagaray-Santa Maria Road, Caypombo" (14.848558, 120.98125) |
| AI-2018 | UNVERIFIED (2018) | Santa Maria → TriNoma 05:00–20:00, "every 30 minutes regardless of passenger count" | — | — | DOTr launch, 2018-07-30. Historical only. |
| FB-PGT | VERIFIED (existence) | — | — | — | Sta. Maria origin, TriNoma end |

**Assessment: the shipped data materially conflicts with every current source.**
1. **Last trip:** every current source says **19:45**; the firmware says 20:00.
2. **Service days:** every current source that states days gives **separate weekday and weekend/holiday** hours.
   CT-PGT and SAKAY agree on a weekend first trip of 06:00 and a return of 07:00. The firmware runs the weekday
   pattern every day.
3. **Midday:** P2PBUS shows hourly service from 10:00 to 15:00 (25 / 29 trips). MM-2024 shows a 12:00–13:00 break.
   The firmware has an unbroken 30-minute cycle (31 / 34 trips), so it **lists departures that may not exist**.
4. **Holidays:** CT-PGT (secondary) puts holidays with the weekend. The firmware has no holiday rule. No operator
   holiday statement was found.
5. **Operator:** SAKAY names **UBE Express**. FB-PGT, P2PBUS, CT-PGT and MM-2024 name Precious Grace. Unresolved
   (§5.3).
6. **Destination:** the operator intro names TriNoma, not SM North EDSA. Directories list both (§5.2).

- The firmware's array matches the 2018 launch pattern (05:00–20:00 every 30 min). It may descend from that.
- **Register tier for the shipped data: UNVERIFIED, with conflicting evidence.** The PUBLISHED label is **not
  supported** in this pass.
- Not changed (see §8). This is the highest-priority item.
- **Reliability flag:** **ridership-dependent dispatch**, per the firmware's own note; midday thinning; and an unclear
  operator.
- An offline board must not present midday departures as guaranteed until the operator confirms them.

## 5. Route-identity assessment

**Principle:** a route is one operator service between two terminals. Different barangay names, intermediate stops,
or directory entries for each destination do not create separate routes.

### 5.1 Caypombo vs Santa Clara (Santa Maria, Bulacan)

**Conclusion: probably one service. Santa Clara is a boarding point on the Caypombo line, not a separate route**
(confidence: medium).

**Evidence:**
- Caypombo and Santa Clara are both barangays of Santa Maria. The operator intro names only "Sta. Maria".
- WV-SM: Trinoma/SM North buses run "for Caypombo" "with a stopover at WalterMart Santa Maria in Barangay Santa
  Clara". Fares are P100 to WalterMart and P120 to Caypombo.
- CT-PGT: the Sta. Clara fare is ₱100 and the Caypombo fare ₱120, matching that stop structure. It also lists a
  separate "Waltermart Sta. Maria" origin with Caypombo's hours (05:00–19:45), a third name for the same area.
- SAKAY lists only Caypombo, with a "Sta Maria Junction" stop. That is plausibly the same boarding area, but this is
  **not confirmed**.
- A search summary attributed a Precious Grace **business address** "By-pass Rd., Sta. Clara" to
  santamariabulacan.com. That page, when fetched, contained no such text: it was unrelated spam. The claim is
  **UNVERIFIED** and discarded as evidence. If an office does exist in Sta. Clara while the terminal is at Caypombo,
  that would explain the directory naming, but this is unconfirmed.

**Conflict preserved:**
- P2PBUS and CT-PGT give Sta. Clara a last trip of **20:00** against Caypombo's **19:45**, with otherwise identical
  lists.
- That fits a bus leaving Caypombo at 19:45 and passing Sta. Clara around 20:00. It would also fit two dispatch
  points. It is not explained by the operator.
- The first departure is 05:00 at both, which argues against a simple "Caypombo + 15 min" offset.

**Resolution needed (operator):**
- Is Sta. Clara/WalterMart a stop on the Caypombo trips, or a separate dispatch?
- What is the scheduled time at Sta. Clara?

**Relocation:**
- No evidence of a relocation either way. No dated announcement was found.

### 5.2 TriNoma vs SM North EDSA (Precious Grace; Bulacan P2P Plaridel)

**Conclusion: unresolved.** Treat as one service with two adjacent North Avenue stops until shown otherwise.

- **Precious Grace:**
  - The operator intro says "North Avenue … (Trinoma)".
  - Directories (P2PBUS, CT-PGT, MM-2024) list *both* "→ SM North" and "→ TriNoma" with the same hours.
  - CT-PGT shows the last trip **from** TriNoma at 22:00 against 22:30 from SM North. That suggests one bus loop
    calling at both stops, in an order the directories don't state.
- **Bulacan P2P Plaridel:**
  - The intro reads "Plaridel to Trinoma / SM North EDSA vv".
  - SAKAY's TriNoma → Plaridel stop list is TriNoma, then SM North EDSA. So it is one route with two Metro stops.
- **Don't create** separate SM North and TriNoma routes for the same trips. For ON POINT, it's an editorial choice
  which stop to name, and the departure time must be the time at the named stop.

### 5.3 Precious Grace vs UBE Express (Caypombo; Robinsons Malolos)

**Conclusion: unresolved conflict.**
- SAKAY attributes "Caypombo to SM North" and "Robinsons Malolos to TriNoma" to **UBE Express**, with weekday and
  weekend variants.
- The Precious Grace operator intro claims Sta. Maria and Malolos. P2PBUS and CT-PGT attribute both to Precious
  Grace.
- No accessed source mentions UBE Express in Bulacan. UBE's documented routes are airport services.
- **Possible explanations:**
  - a SAKAY data error;
  - a franchise or consortium arrangement;
  - a change of operator.
- **Not resolved.** Needs an operator statement, or LTFRB franchise information.

### 5.4 Bocaue

**Conclusion: Bocaue is not an independent route.**
- **Precious Grace:**
  - "Bocaue tolgate" is a pick-up and drop-off point on the operator's own description.
  - Directory "Bocaue → SM North / TriNoma" entries (MM-2024: 05:30–12:00 / 13:00–19:00; CT-PGT: M–F 05:30–18:00,
    SSH 06:30–18:00) are most likely boarding windows at the tollgate stop. That is unconfirmed.
- **Bulacan P2P "Bocaue Crossing ↔ TriNoma":**
  - Named in the operator's intro, so it **is** a distinct route identity.
  - No times found in any source.

### 5.5 Malolos

- **Precious Grace:** P2PBUS lists Malolos ↔ TriNoma and Malolos ↔ SM North, with the same midday-gap pattern as
  Santa Clara.
- **"UBE Express" (SAKAY):** Robinsons Malolos ↔ TriNoma, hourly, with weekday and weekend variants.
- These are likely the **same** Robinsons Malolos service, given the 2018 DOTr launch and the operator intro. Pending
  §5.3.

## 6. Candidate inventory

| Candidate ID | Operator | Terminals | Best evidence | Times found | Days | Holiday policy | Tier | Confidence | Unresolved |
|---|---|---|---|---|---|---|---|---|---|
| `cand:bp2p-plaridel-trinoma` | Bulacan P2P (operator intro) | Primark Mall Plaridel ↔ TriNoma (return also calls at SM North EDSA) | SAKAY `R_KGW4HQS` | Plaridel → TriNoma: **explicit list** 04:00, 05:15 … 20:15 (every 75 min, 14 trips), but the stated range ends at **20:30**. TriNoma → Plaridel: 05:15–21:00, range only. | "daily" | Not stated | PROVISIONAL | Medium | The list vs range mismatch (20:15 vs 20:30). MM-2024 had 04:00–20:30 / 05:30–22:00. Return departures are unknown. |
| `cand:bp2p-pandi-trinoma` | Bulacan P2P (intro) | Shell Mapulang Lupa / Poblacion Pandi ↔ TriNoma | SAKAY `R_CTXR747`; P2PBUS `trinoma-pandi` | SAKAY: 04:00–19:30 daily; return 05:15–21:30. P2PBUS: Mon–Fri 04:30–19:00 every 30 min (30 trips); return 06:00–21:00 (31 trips) | Conflict: daily vs Mon–Fri | Not stated | PROVISIONAL | Medium | Which window is current. Weekends. Two Pandi stops. |
| `cand:bp2p-guiguinto-trinoma` | Bulacan P2P (intro; TRIB-2025 launch) | WalterMart Guiguinto ↔ TriNoma | SAKAY `R_2BBIR8W` | 04:30–19:30; return 06:00–21:30. Range only. | "daily" | Not stated | PROVISIONAL | Medium-low | Interval unknown. Launch date not directly accessed. |
| `cand:bp2p-bocauecrossing-trinoma` | Bulacan P2P (intro) | Bocaue Crossing ↔ TriNoma | FB-BP2P intro only | None | — | — | UNVERIFIED (times) | Low | Whether it runs, its terminal and its times |
| `cand:pgt-malolos-trinoma` | Precious Grace, **or** UBE Express (§5.3) | Robinsons Malolos ↔ TriNoma (± SM North) | P2PBUS; SAKAY `R_7UZ38YO` / `R_FRKA1MK`; CT-PGT; AI-2018 | P2PBUS (PG): M–F 05:00–20:00 with midday hourly gaps (26 trips); return 06:00–22:30 (29). SAKAY (UBE): M–F **hourly** 05:00–20:00 (16 trips), return 06:00–22:30; Sat–Sun hourly 06:00–20:00 (15), return 07:00–22:00. CT-PGT: M–F 05:00–20:00, SSH 06:00–20:00. | Weekday / weekend split | CT-PGT groups holidays with weekends (secondary) | PROVISIONAL, **conflicting** | Medium-low | The operator. Hourly vs 30-minute service. |
| `cand:pgt-caypombo` (≡ Sta. Clara, §5.1) | Precious Grace (or UBE, §5.3) | Caypombo ↔ TriNoma / SM North | See §4.2 | See §4.2 | See §4.2 | See §4.2 | — | — | This is the existing route, not a candidate. Kept for identity mapping only. |
| `cand:pgt-bocaue` | Precious Grace | Bocaue tollgate stop | §5.4 | Boarding windows only | — | — | Not a route | — | Folded into the PG services |
| `cand:pgt-nlet` | Precious Grace | SM North → NLET (CT-PGT only) | CT-PGT | 06:00–22:30 | M–F / SSH | — | UNVERIFIED | Low | NLET is in Bocaue (the bus terminal at the NLEX tollgate). Likely the same stop as §5.4. |

**Not found in any P2P source:**
- Marilao, Meycauayan, San Jose del Monte, Norzagaray, Baliuag and Hagonoy.
- Those towns are served by ordinary provincial buses, which are out of scope.

**Not P2P:** none of the accessed sources describes a qualifying **intra-Bulacan** P2P service. Every route found ends
in Quezon City.

## 7. Coverage-gap assessment

| Gap | Effect on ON POINT |
|---|---|
| **No dated operator timetable was directly accessible** for any Bulacan route (the Facebook login wall) | Nothing in BULACAN can currently be evidenced as PUBLISHED under the editorial standard. That includes the four shipped schedules. |
| The shipped Caypombo data conflicts with all current sources on last trip, service days and midday density | The board may show trips that don't run (midday, after 19:45) and the wrong weekend first trip |
| Shipped Balagtas weekend and holiday arrays have no accessed source | Weekend and holiday departures are unevidenced |
| **No operator holiday policy found** for either operator | The Balagtas holiday overrides are unevidenced. Precious Grace may in fact use a Sat/Sun/Holiday timetable (CT-PGT), which the firmware ignores. |
| Operator ambiguity (Precious Grace vs UBE Express) | Operator labels on Caypombo and Malolos are uncertain |
| Uncovered Bulacan P2P routes: Plaridel, Pandi, Guiguinto, Bocaue Crossing | Four operator-confirmed routes with no board |
| Uncovered Precious Grace Malolos route | One route with no board, operator unresolved |
| Volatility: ridership-dependent dispatch (PG), weather-related last-trip changes (BP2P, unverified), midday thinning | An offline board is unreliable for these routes unless clearly labelled. Earlier re-verification is needed. |

## 8. Status of the shipped labels (decision for the owner, no change made)

**What this pass found:** the four BULACAN schedules carry PUBLISHED. The register cannot support that under the
agreed standard:
- Balagtas weekday: PROVISIONAL;
- Balagtas weekend and holiday: UNVERIFIED;
- Caypombo: UNVERIFIED and conflicting.

**What was done:** as instructed, the firmware timetables, labels, holiday rules and effective dates are **untouched**.

**Options for a later, separately authorised change:**
- **(a)** Relabel the four as PROVISIONAL until promotion evidence arrives.
- **(b)** Correct Caypombo to the weekday/weekend split as PROVISIONAL.
- **(c)** Keep them as they are, pending operator contact.

This register recommends **(a) at minimum for Caypombo**, because its current PUBLISHED board conflicts with every
current source.

## 9. Ranked shortlist, with the exact evidence needed for PUBLISHED

### What every promotion requires

1. **A dated operator publication, directly accessed.** One of:
   - a Facebook post permalink, with the post's own date;
   - an operator website page;
   - a written reply from the operator (email or Messenger), saved with its date.
2. **It must give either:**
   - **every departure**, per direction; or
   - **an exact first trip, last trip and fixed interval**, per direction, with the operator stating that the interval
     is fixed. (The derivation is recorded here.)
3. **Service-day rules**, stated by the operator: weekday, Saturday, Sunday.
4. **The holiday policy**, stated by the operator. If none is stated, no holiday overrides are entered and the board
   stays PROVISIONAL for holidays.
5. **The terminal names or locations**, and which North Avenue stop the time refers to.
6. **A facts-only transcription** in this register, with the permalink, publication date and retrieval date.

**Practical route:**
- **(i)** The owner views the operator posts while logged in to Facebook, and gives the permalinks and screenshots.
  Claude transcribes them.
- **(ii)** Or the owner contacts the operator:
  - Precious Grace: (044) 815 5770 / pgtransport2007@gmail.com;
  - Bulacan P2P: via its Facebook page.

Claude does not send messages or log in.

### Ranked list

| Rank | Route | Why this rank | Exact evidence needed |
|---|---|---|---|
| **1** | `p2p-caypombo-sm-north-edsa` / reverse (existing, PG) | Shipped as PUBLISHED and conflicts with all sources | A dated PG timetable for **Caypombo** with: the weekday and weekend first and last trips; the full departure list, or confirmation that the interval is fixed through midday; the holiday policy; which North Avenue stop (TriNoma / SM North) and in what order; whether Sta. Clara/WalterMart is a stop and its time (§5.1); and confirmation of the operator vs SAKAY's "UBE Express" |
| **2** | `p2p-balagtas-trinoma` / reverse (existing, BP2P) | Shipped as PUBLISHED. Weekday plausible, weekend and holiday unevidenced. | A dated BP2P timetable giving: the weekday **and** weekend first and last trips each way (to settle 04:00 vs 04:30 and 05:15 vs 06:00); confirmation of a fixed 30-minute interval; the **holiday policy**; the terminal (Ultra Mega Balagtas?); and the rain or weather last-trip policy |
| 3 | `cand:bp2p-plaridel-trinoma` | Operator-confirmed. The only candidate with an explicit departure list. | A dated BP2P post confirming the 75-minute list (and whether the last trip is 20:15 or 20:30), the return departure list, days and holidays, and whether returns board at TriNoma, SM North or both |
| 4 | `cand:bp2p-pandi-trinoma` | Operator's home terminal. Two directory windows conflict. | A dated BP2P timetable settling daily vs Mon–Fri and 04:00 vs 04:30. Interval, holidays, and which Pandi boarding point. |
| 5 | `cand:pgt-malolos-trinoma` | Operator-confirmed origin, but the operator and frequency conflict | An operator statement (PG or UBE) on who runs Robinsons Malolos ↔ TriNoma, whether it's hourly or 30-minute with midday gaps, weekday and weekend lists, and holidays |
| 6 | `cand:bp2p-guiguinto-trinoma` | Operator-confirmed. Range only. | A dated BP2P timetable with the interval or a list, days and holidays. The launch article accessed directly for the start date. |
| 7 | `cand:bp2p-bocauecrossing-trinoma` | Named only in the intro, with no times anywhere | Confirmation that it currently runs, and its terminal and full timetable |
| — | Sta. Clara, Bocaue (PG), NLET | Not separate routes (§5) | Answered as part of rank 1 |

**Routes flagged as unsuitable for a PUBLISHED offline board until resolved:**
- every Precious Grace service (ridership-dependent dispatch, midday thinning, unclear operator);
- Bulacan P2P last trips in bad weather (unverified report).

These can still be offered as PROVISIONAL with first and last trips.

## 10. Re-verification schedule

| Item | Next check |
|---|---|
| Ranks 1–2 (shipped data) | On receipt of operator evidence. Otherwise 2026-11-24 (30 days, because they conflict or are provisional). |
| Candidates 3–7 | 2026-11-24 (provisional) |
| Operator intros FB-BP2P and FB-PGT | 2027-01-23 (90 days) |

## 11. Change log

| Date | Change |
|---|---|
| 2026-10-25 | Register created (research pass 1). Sources accessed on 2026-10-25. |
