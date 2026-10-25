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
| **OBS** (separate axis) | The owner's firsthand historical observation (§5.1). It establishes that a service or stop existed and was used, and it governs **identity**. It never establishes current times, days or holiday rules, and it never promotes a timetable on its own. |

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
| LAM-2022 | https://www.lamudi.com.ph/journal/p2p-terminals-schedules/ ("P2P Terminals and Schedules (2023)") | Blog | Direct, in the built-in browser (WebFetch returned 403) | `article:published_time` **2022-10-20**, despite the title. Header ranges and departure lists are internally inconsistent. Historical only. |
| CT-WM | https://ph.commutetour.com/ph/terminal/waltermart-sta-maria-bulacan/ | Directory | Direct | Banner "Updated Jan 19, 2026" |
| P2PBUS-PLR | p2pbus.ph `sm-north-plaridel`, `trinoma-plaridel`, `bocaue-bulacan-trinoma` | Directory | Direct (pass 2) | No date |
| ETIGO | https://etigo.com.ph/terminals/nlet-bocaue | Directory | Direct | "Last updated: March 17, 2026" |
| MAP-2026 | https://mappedatlas.com/navigate/philippines-commute-guides/p2p-bus-routes-schedules-guide/ | Guide | Search snippet only | Not accessed |
| PG-P-20251230 | Precious Grace FB post, "December 30, 2025" (2026 Trip Schedule; P2P + Route 9 Angat–Monumento) | Operator post | Owner screenshot (`PG-2025-12-30-post.webp`, sha256 `9bee868f…`); permalink not captured | **2025-12-30 (year shown)** |
| PG-P-0313 | Precious Grace Featured post, "March 13" (P2P Daily Trip Schedule, PG-G2) | Operator post | Owner screenshot (`PG-featured-mar13-may1.webp`, `07c6fde9…`; graphic also in `PG-G2-yellow.webp`) | 2026-03-13 (**year inferred**) |
| PG-P-0501 | Precious Grace Featured post, "May 1" (Angat–Monumento advisory) | Operator post | Same screenshot; graphic in `PG-ANGAT-Y.webp` | 2026-05-01 (**year inferred**; the text says "As of May 1, 2026") |

**Inaccessible (INACC):**
- **Every operator timetable post on Facebook** (login wall). Pass 2 searched for indexed post permalinks with dates
  for both operators and **found none**.
- **The Bulacan P2P rain advisory**, known only from a search snippet (quoted in §5.0). It is **UNVERIFIED**, and its
  permalink and date are unknown.

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

> **Superseded in part by pass 3 ([BULACAN-RECONCILIATION.md](BULACAN-RECONCILIATION.md)).**
> - **Balagtas:** Bulacan P2P post BP2P-0830 (dated "August 30", year not shown; read directly on 2026-10-25) now
>   supports **every** shipped Balagtas window, including the Sat/Sun/Holiday grouping.
> - **Caypombo:** the shipped arrays equal the derivation from owner-supplied Precious Grace graphic PG-G1, but
>   conflict with PG-G2 (last trip 20:30).
> - **Unchanged from pass 1:** the conflicts with secondary sources recorded below.

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
- **Stops:** Bocaue Crossing is a pickup stop on this service (owner observation O1, §5.1). No source gives its
  scheduled time. Times on the board are Balagtas and TriNoma departures.
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
   (§5.4).
6. **Destination:** the operator intro names TriNoma, not SM North EDSA. Directories list both (§5.3).

- The firmware's array matches the 2018 launch pattern (05:00–20:00 every 30 min). It may descend from that.
- **Register tier for the shipped data: UNVERIFIED, with conflicting evidence.** The PUBLISHED label is **not
  supported** in this pass.
- Not changed (see §8). This is the highest-priority item.
- **Reliability flag:** **ridership-dependent dispatch**, per the firmware's own note; midday thinning; and an unclear
  operator.
- An offline board must not present midday departures as guaranteed until the operator confirms them.

## 5. Service identity (revised in pass 2, 2026-10-25)

### 5.0 Methodology

Every location in a source is classified before it can count as a route. A listing in an online directory, whether a
page or a "from X to Y" row, is **never by itself evidence of a separate service**. Directories (P2PBUS, CT-*,
MM-2024, LAM-2022) routinely publish one entry per boarding point and per Metro Manila stop, often reusing the same
times.

| Class | Definition | ON POINT treatment |
|---|---|---|
| **S: independent service** | Its own buses or trips, with its own departures from its own origin terminal | Can become a route pair |
| **T: origin or end terminal** | Where an S service starts or ends, with dispatch | The endpoint of a route pair |
| **P: intermediate pickup or drop-off stop** | Trips of an S service pass through; there are no departures of its own | Recorded only as a stop of the parent service. No route pair. |
| **A: alias** | Another name for the same T or P (a barangay, mall or landmark name) | Recorded as an alias only |
| **D: distinct service sharing a locality** | A different S service with a terminal or stop in the same town | Assessed separately on its own evidence |

**Each S claim must show:** departures that are not simply the parent service's times at a stop; and either an
operator statement or a firsthand observation of dispatch at that terminal.

**Evidence classes in the identity matrix:**
- **OBS:** the owner's firsthand historical observation (2026-10-25 statement). This is strong evidence that a service
  or stop **existed and was used**. It is **not** evidence of current timetables, and it is not a substitute for
  current operator confirmation.
- **OP:** an operator statement, directly accessed (the Facebook intros, §3). Undated.
- **3P:** third-party claims (directories, wiki, news).
- **INACC:** referenced but inaccessible. This covers the Facebook posts behind the login wall, and the
  Bulacan P2P rain last-trip advisory known only from a search snippet: "Dahil sa patuloy na pag-ulan … pagbabago sa
  iskedyul ng huling biyahe (last trip)", which is undated.

### 5.1 Owner's firsthand observations (recorded 2026-10-25)

| # | Observation | Class |
|---|---|---|
| O1 | Bocaue Crossing is a **pickup point on the Balagtas ↔ TriNoma** service. The owner has boarded the Balagtas bus there many times. | OBS |
| O2 | TriNoma terminal: the owner recalls signage or services marked Balagtas, Pandi, Plaridel, Malolos, Caypombo and Clark. This is a recollection of terminal services, not a claim that each currently runs or has its own timetable. | OBS (recollection) |
| O3 | SM North EDSA: the owner recalls Caypombo and Plaridel. The owner has **personally ridden Plaridel → SM North**. | OBS (recollection + ridden) |
| O4 | Caypombo and Santa Clara are barangays within the same municipality, Santa Maria. | OBS / geography |

### 5.2 Service-identity matrix

| Location / listing | Parent service | Class | OBS | OP | 3P | Unresolved identity question |
|---|---|---|---|---|---|---|
| **Ultra Mega Balagtas** | BP2P Balagtas ↔ TriNoma | **T** | O1, O2 | Intro: "Balagtas to Trinoma vv" | SAKAY: terminal "Ultra Mega Balagtas"; P2PBUS; LAM-2022 | Exact terminal location is not operator-confirmed |
| **Bocaue Crossing** | BP2P Balagtas ↔ TriNoma | **P** (per O1) | **O1: boarded there many times** | Intro lists "Bocaue Crossing to Trinoma vv" as if a route | SAKAY's Balagtas stop list omits it. No 3P timetable. | Does BP2P *also* dispatch trips that start at Bocaue Crossing (a short-turn), or does the intro just advertise the stop? **Until shown otherwise: P, not a route.** |
| Bocaue tollgate / NLET (Precious Grace) | PG Santa Maria / Malolos services | **P** (operator wording: "pick up and drop off at Bocaue tolgate") | — | PG intro | P2PBUS "Bocaue Bulacan ↔ TriNoma/SM North" (05:30–19:00, the same midday pattern as Caypombo); CT-PGT "SM North → NLET"; LAM-2022 "Bocaue to SM North EDSA to Trinoma" with its own hours | Whether any PG trips start at Bocaue. The directory times look like the Caypombo pattern, not independent dispatch. Treated as P. |
| Bocaue: any other independent operator | — | **D?** none found | — | — | NLET is a provincial-bus transfer hub (etigo, updated 2026-03-17, lists no P2P operators); MAP-2026 lists the Bocaue–TriNoma operator as "Various" | **No independently operated Bocaue-origin P2P service was found.** Remains open. |
| **Caypombo** (Norzagaray–Santa Maria Rd) | PG Santa Maria ↔ North Avenue | **T** | O2, O3 | Intro: "Sta. Maria … to North Avenue … (Trinoma)" | WV-SM terminal listing; SAKAY (as "UBE Express"), P2PBUS, CT-PGT, LAM-2022 | Operator (§5.4) |
| **Santa Clara / WalterMart Sta. Maria** | Probably PG Caypombo service | **P + A** (likely) | O4 | Intro names only "Sta. Maria" | WV-SM: "stopover at WalterMart Santa Maria in Barangay Santa Clara" with fares of P100 (WalterMart) vs P120 (Caypombo). CT-WM (updated 2026-01-19) lists WalterMart Sta. Maria with **Caypombo's exact hours** (M–F 05:00–19:45, SSH 06:00–19:45). P2PBUS / CT-PGT list "Sta. Clara" ending at 20:00. LAM-2022 has separate Sta. Clara and "Sta. Maria" entries with different hours. | Whether any trips start at Sta. Clara or WalterMart (a separate dispatch, or "both"), and its scheduled time relative to Caypombo. Leading view: **one service; Sta. Clara = WalterMart = a P stop**. "Sta. Maria" is an alias for the municipality. |
| "Sta. Maria" (LAM-2022, AI-2018) | Same as above | **A** | O4 | Intro | AI-2018: DOTr "Santa Maria, Bulacan to Trinoma" launch | — |
| **TriNoma** | BP2P (all routes); PG; Genesis (Clark) | **T** (North Avenue end) | O2 | Both intros name TriNoma | All | — |
| **SM North EDSA** (vs TriNoma) | PG Caypombo; BP2P Plaridel | **A/P, or T** depending on the service | O3 | BP2P intro: "Plaridel to Trinoma / SM North EDSA vv". PG intro names only TriNoma. | SAKAY: TriNoma → Plaridel calls at SM North EDSA. CT-PGT: TriNoma → Caypombo last trip 22:00 vs SM North 22:30. | For PG: one loop calling at both, in an unstated order. For Plaridel: see the next two rows. |
| **Plaridel ↔ TriNoma** | BP2P | **S?** | O2 | Intro "Plaridel to Trinoma / SM North EDSA vv" | SAKAY (Primark Mall; departures every 75 min); P2PBUS M–F 04:00–20:30 / 05:30–22:00 every 30 min; LAM-2022 with its own hours | Is it the same trips as Plaridel ↔ SM North? |
| **Plaridel ↔ SM North EDSA** | BP2P | **S?** | **O3: ridden** | Same intro | P2PBUS: 05:30–22:30 every 30 min, both directions (Plaridel → SM North listed Mon–Sun); LAM-2022: Plaridel → SM North with **different hours** from Plaridel → TriNoma | **Separate Plaridel services?** The directories give different hours, and the owner rode a bus to SM North. But SAKAY shows TriNoma → Plaridel trips also calling at SM North. Possibly two services, or one service whose trips end at either stop. **Unresolved.** High priority. |
| **Pandi** | BP2P Pandi ↔ TriNoma | **T** (possibly two boarding points) | O2 | Intro "Pandi to Trinoma vv". Operator office: J. Bernardo St., Poblacion, Pandi. | SAKAY stops "Shell, Pandi, Mapulang Lupa" (origin) and "Poblacion Pandi"; LAM-2022 "Pandi P2P Terminal" | Which is the dispatch terminal (Mapulang Lupa or Poblacion), and whether the other is a P stop. Windows conflict (§6). |
| **Malolos (Robinsons Place Malolos)** | PG (intro) or "UBE Express" (SAKAY) | **T** | O2 (TriNoma) | PG intro: "Sta. Maria and Malolos" | SAKAY: hourly M–F / weekend; P2PBUS: PG with midday gaps; AI-2018 launch; LAM-2022 | Operator (§5.4). Whether it is a separate S service or shares PG's fleet and pattern. Also whether SM North is served. |
| WalterMart Guiguinto | BP2P Guiguinto ↔ TriNoma | **T** | — | Intro | SAKAY; TRIB-2025 (inaccessible) | Is it a new terminal, or a stop added to another BP2P line? Not established. |
| Clark (seen at TriNoma) | Genesis, NORTH LUZON | Not BULACAN | O2 | — | SAKAY lists Clark ↔ TriNoma (Genesis) | **Out of scope here.** It is already represented by the existing NORTH LUZON Genesis schedules, which run via TriNoma. Noted only to account for O2. |

### 5.3 Conclusions per focus pair

**Balagtas vs Bocaue Crossing: one service.**
- Bocaue Crossing is a **P** stop on Balagtas ↔ TriNoma (O1).
- The operator intro's "Bocaue Crossing to Trinoma vv" is **not** taken as a separate route without evidence of its
  own departures.
- **Open:** whether some trips start or end at Bocaue Crossing (short-turns).
- **ON POINT:** no new pair. Optionally, the Balagtas board could later mention the stop. That would be an editorial
  wording question, not a data-model change.

**Caypombo vs Santa Clara: probably one service.**
- Caypombo is the **T**. Santa Clara/WalterMart is a **P** stop and an **A** (directory alias). O4 and WV-SM agree,
  and CT-WM's identical hours fit this.
- The **20:00 vs 19:45** last-trip difference in two directories is unexplained. It doesn't, alone, establish separate
  dispatch.
- **ON POINT:** no new pair for Santa Clara.

**TriNoma vs SM North:**
- **Precious Grace:** **A/P** (one service, two North Avenue stops).
- **Bulacan P2P Plaridel:** **unresolved**. It may be two services; see the next item.
- **ON POINT:** don't duplicate pairs per North Avenue stop for the same trips.

**Separate Plaridel services: unresolved, and the most likely genuine split.**
- **For two services:** the owner rode Plaridel → SM North (O3); the operator lists both names; two directories give
  different hours.
- **Against:** SAKAY shows a TriNoma → Plaridel trip calling at SM North.
- **Needs:** an operator timetable showing whether the SM North and TriNoma trips are separate departures.

**Pandi:**
- **One service.** The terminal identity (Mapulang Lupa vs Poblacion) is open. The other location is probably a P
  stop.

**Malolos:**
- **One T** at Robinsons Place Malolos.
- The operator conflict (PG vs UBE) is open. It is a separate service from Caypombo only if it has its own
  departures, which the directories suggest but the operator hasn't confirmed.

### 5.4 Precious Grace vs UBE Express (Caypombo; Robinsons Malolos)

**Unresolved conflict.**
- SAKAY attributes "Caypombo to SM North" and "Robinsons Malolos to TriNoma" to **UBE Express**, with weekday and
  weekend variants.
- The Precious Grace operator intro claims Sta. Maria and Malolos. P2PBUS, CT-PGT and LAM-2022 attribute both to
  Precious Grace.
- No accessed source mentions UBE Express in Bulacan. UBE's documented routes are airport services.
- **Possible explanations:**
  - a SAKAY data error;
  - a franchise or consortium arrangement;
  - a change of operator.
- **Not resolved.** Needs an operator statement, or LTFRB franchise information.

### 5.5 Candidates: new pair, or stop/alias only

| Candidate | Verdict | Basis |
|---|---|---|
| Plaridel ↔ TriNoma | **May merit a pair** | T/S, operator-named, OBS O2 |
| Plaridel ↔ SM North EDSA | **May merit a pair**, *only if* confirmed as separate departures; otherwise an alias of the above | OBS O3 (ridden), operator-named, conflicting 3P |
| Pandi ↔ TriNoma | **May merit a pair** | T/S, operator home terminal, OBS O2 |
| Robinsons Malolos ↔ TriNoma | **May merit a pair** (operator to be resolved) | T, operator-named (PG), OBS O2 |
| WalterMart Guiguinto ↔ TriNoma | **May merit a pair** (low evidence) | Operator-named, no OBS |
| Bocaue Crossing | **Stop only** (on Balagtas) | O1 |
| Bocaue tollgate / NLET (PG) | **Stop only** | PG intro |
| Santa Clara / WalterMart Sta. Maria | **Stop + alias only** (pending the dispatch question) | O4, WV-SM, CT-WM |
| "Sta. Maria" | **Alias only** | Municipality name for Caypombo |
| SM North for PG Caypombo | **Alias / stop only** | PG intro names TriNoma; directories duplicate the times |
| Clark at TriNoma | **Out of scope** (NORTH LUZON, existing) | O2 |

## 6. Candidate inventory

| Candidate ID | Operator | Terminals | Best evidence | Times found | Days | Holiday policy | Tier | Confidence | Unresolved |
|---|---|---|---|---|---|---|---|---|---|
| `cand:bp2p-plaridel-trinoma` | Bulacan P2P (operator intro) | Primark Mall Plaridel ↔ TriNoma (return also calls at SM North EDSA per SAKAY) | SAKAY `R_KGW4HQS`; P2PBUS-PLR; OBS O2 | SAKAY: Plaridel → TriNoma **explicit list** 04:00, 05:15 … 20:15 (every 75 min, 14 trips), but the stated range ends at **20:30**; TriNoma → Plaridel 05:15–21:00, range only. P2PBUS: M–F 04:00–20:30 every 30 min (34); return 05:30–22:00 (34). | Conflict: "daily" vs M–F | Not stated | PROVISIONAL, **conflicting** (75 vs 30 min) | Medium | The interval. The list vs range mismatch. Return departures. Identity vs SM North (§5.3). |
| `cand:bp2p-plaridel-smnorth` | Bulacan P2P (operator intro) | Plaridel ↔ SM North EDSA | P2PBUS-PLR; LAM-2022; **OBS O3 (ridden)** | P2PBUS: 05:30–22:30 every 30 min (35) **both directions** (Plaridel → SM North listed Mon–Sun; SM North → Plaridel Mon–Fri). LAM-2022 (old): Mon–Sat 04:30–18:30, Sun to 18:00. | Unclear | Not stated | PROVISIONAL | Medium-low | **A separate service, or the same trips as Plaridel ↔ TriNoma?** (§5.3). The identical out and back windows on P2PBUS look like a templating error. |
| `cand:bp2p-pandi-trinoma` | Bulacan P2P (intro) | Shell Mapulang Lupa / Poblacion Pandi ↔ TriNoma | SAKAY `R_CTXR747`; P2PBUS `trinoma-pandi` | SAKAY: 04:00–19:30 daily; return 05:15–21:30. P2PBUS: Mon–Fri 04:30–19:00 every 30 min (30 trips); return 06:00–21:00 (31 trips) | Conflict: daily vs Mon–Fri | Not stated | PROVISIONAL | Medium | Which window is current. Weekends. Two Pandi stops. |
| `cand:bp2p-guiguinto-trinoma` | Bulacan P2P (intro; TRIB-2025 launch) | WalterMart Guiguinto ↔ TriNoma | SAKAY `R_2BBIR8W` | 04:30–19:30; return 06:00–21:30. Range only. | "daily" | Not stated | PROVISIONAL | Medium-low | Interval unknown. Launch date not directly accessed. |
| ~~`cand:bp2p-bocauecrossing-trinoma`~~ → **stop** | Bulacan P2P | Bocaue Crossing = a pickup stop on Balagtas ↔ TriNoma (O1) | FB-BP2P intro; OBS O1 | None | — | — | Not a route | — | Only whether some trips start there (short-turns) |
| `cand:pgt-malolos-trinoma` | Precious Grace, **or** UBE Express (§5.4) | Robinsons Malolos ↔ TriNoma (± SM North) | P2PBUS; SAKAY `R_7UZ38YO` / `R_FRKA1MK`; CT-PGT; AI-2018 | P2PBUS (PG): M–F 05:00–20:00 with midday hourly gaps (26 trips); return 06:00–22:30 (29). SAKAY (UBE): M–F **hourly** 05:00–20:00 (16 trips), return 06:00–22:30; Sat–Sun hourly 06:00–20:00 (15), return 07:00–22:00. CT-PGT: M–F 05:00–20:00, SSH 06:00–20:00. | Weekday / weekend split | CT-PGT groups holidays with weekends (secondary) | PROVISIONAL, **conflicting** | Medium-low | The operator. Hourly vs 30-minute service. |
| `cand:pgt-caypombo` (≡ Sta. Clara / WalterMart, §5.3) | Precious Grace (or UBE, §5.4) | Caypombo ↔ TriNoma / SM North | See §4.2 | See §4.2 | See §4.2 | See §4.2 | — | — | This is the existing route, not a candidate. Kept for identity mapping only. |
| `cand:pgt-bocaue` | Precious Grace | Bocaue tollgate stop | §5.2 | Boarding windows only (P2PBUS: 05:30–19:00, the Caypombo midday pattern) | — | — | Not a route | — | Folded into the PG services |
| `cand:pgt-nlet` | Precious Grace | SM North → NLET (CT-PGT only) | CT-PGT | 06:00–22:30 | M–F / SSH | — | UNVERIFIED | Low | NLET is in Bocaue (the bus terminal at the NLEX tollgate). Likely the same stop as `cand:pgt-bocaue` (§5.2). |

**Not found in any P2P source:**
- Marilao, Meycauayan, San Jose del Monte, Norzagaray, Baliuag and Hagonoy.
- Those towns are served by ordinary provincial buses, which are out of scope.

**Independent Bocaue-origin service:** none found. NLET (ETIGO) is a provincial-bus transfer hub with no P2P
operators listed. The Bocaue listings found are boarding points of BP2P Balagtas (O1) or of PG (tollgate). Remains
open, because the BP2P intro's "Bocaue Crossing to Trinoma" could still describe short-turn trips.

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
| Uncovered Bulacan P2P services: Plaridel (one or two services, §5.3), Pandi and Guiguinto. Bocaue Crossing is a stop, not a gap. | Three or four services with no board |
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

> **Superseded by the owner's approved eight-route catalogue (brief of 2026-10-25) and by pass 3
> ([BULACAN-RECONCILIATION.md](BULACAN-RECONCILIATION.md) §5).** Kept below as a record.

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
| **1** | `p2p-caypombo-sm-north-edsa` / reverse (existing, PG) | Shipped as PUBLISHED and conflicts with all sources | A dated PG timetable for **Caypombo** with: the weekday and weekend first and last trips; the full departure list, or confirmation that the interval is fixed through midday; the holiday policy; which North Avenue stop (TriNoma / SM North) and in what order; whether Sta. Clara/WalterMart is only a stop or also a dispatch point, and its scheduled time (§5.3); and confirmation of the operator vs SAKAY's "UBE Express" |
| **2** | `p2p-balagtas-trinoma` / reverse (existing, BP2P) | Shipped as PUBLISHED. Weekday plausible, weekend and holiday unevidenced. | A dated BP2P timetable giving: the weekday **and** weekend first and last trips each way (to settle 04:00 vs 04:30 and 05:15 vs 06:00); confirmation of a fixed 30-minute interval; the **holiday policy**; the terminal (Ultra Mega Balagtas?); the rain or weather last-trip policy; and whether any trips start or end at **Bocaue Crossing** (short-turns) or all of them only stop there |
| 3 | `cand:bp2p-plaridel-trinoma` + `cand:bp2p-plaridel-smnorth` (one identity question) | Operator-named, owner-ridden (O3), and the most likely genuine split | A dated BP2P Plaridel timetable that shows: **whether SM North and TriNoma trips are separate departures or the same trips** (and in which stop order); the interval (75 vs 30 min) and full lists each way; days and holidays. Only then decide **one or two** pairs. |
| 4 | `cand:bp2p-pandi-trinoma` | Operator's home terminal. Two directory windows conflict. | A dated BP2P timetable settling daily vs Mon–Fri and 04:00 vs 04:30. Interval, holidays, and which Pandi boarding point. |
| 5 | `cand:pgt-malolos-trinoma` | Operator-confirmed origin, but the operator and frequency conflict | An operator statement (PG or UBE) on who runs Robinsons Malolos ↔ TriNoma, whether it's hourly or 30-minute with midday gaps, weekday and weekend lists, and holidays |
| 6 | `cand:bp2p-guiguinto-trinoma` | Operator-confirmed. Range only. | A dated BP2P timetable with the interval or a list, days and holidays. The launch article accessed directly for the start date. |
| — | Bocaue Crossing | Stop on Balagtas (O1), not a route | Answered as part of rank 2 (the short-turn question) |
| — | Sta. Clara / WalterMart, Bocaue (PG), NLET, "Sta. Maria", SM North for PG | Stops or aliases (§5.5) | Answered as part of rank 1 |

**Routes flagged as unsuitable for a PUBLISHED offline board until resolved:**
- every Precious Grace service (ridership-dependent dispatch, midday thinning, unclear operator);
- Bulacan P2P last trips in bad weather (unverified report).

These can still be offered as PROVISIONAL with first and last trips.

## 10. Re-verification schedule

| Item | Next check |
|---|---|
| Ranks 1–2 (shipped data) | On receipt of operator evidence. Otherwise 2026-11-24 (30 days, because they conflict or are provisional). |
| Candidates 3–6 | 2026-11-24 (provisional) |
| Operator intros FB-BP2P and FB-PGT | 2027-01-23 (90 days) |

## 11. Change log

| Date | Change |
|---|---|
| 2026-10-25 | Register created (research pass 1). Sources accessed on 2026-10-25. |
| 2026-10-25 | Pass 5: dated Precious Grace posts added to §2. Readiness updated in [BULACAN-READINESS.md](BULACAN-READINESS.md). |
| 2026-10-25 | Pass 3: reconciliation of the approved eight-route catalogue in [BULACAN-RECONCILIATION.md](BULACAN-RECONCILIATION.md). Supersession notes added to §4 and §9. |
| 2026-10-25 | Pass 2: §5 rewritten as a service-identity methodology and matrix. Owner's firsthand observations O1–O4 added (OBS: historical, not timetable evidence). Bocaue Crossing reclassified from candidate route to a stop on Balagtas. Plaridel split into an identity question (TriNoma vs SM North). Sources LAM-2022, CT-WM, P2PBUS-PLR and ETIGO added. A search for dated operator FB permalinks found none. |
