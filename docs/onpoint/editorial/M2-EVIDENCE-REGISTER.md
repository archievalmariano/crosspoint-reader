# ON POINT M2 evidence register

Facts-only, in the format of `docs/onpoint/editorial/BULACAN-EVIDENCE-REGISTER.md`. Editorial provenance for the four Genesis route pairs and
the timetable corrections shipped by ON POINT M2. **Retrieval date for all rows: 2026-11-08.** The website posters' `Last-Modified` and sizes
were re-checked unchanged on 2026-11-09.

No page or image copies are stored in this repository. Full digests, image URLs and the frozen route data are in the owner's local M2 records.
Departures are in `src/activities/on_point/OnPointScheduleData.cpp`, and `test/on_point_m2/OnPointM2DataTest.cpp` holds a separate per-board
transcription of them.

**Tiers:** VERIFIED = operator's own, directly read, dated by the operator. PROVISIONAL = operator undated, or a dated secondary crediting
the operator. HISTORICAL = superseded. UNVERIFIED = leads only.

## Operator identity

| Check | Result |
|---|---|
| Website | `genesistransportserviceinc.com`; uses company name and logo; printed on every poster |
| Facebook | `facebook.com/GenesisTransportInc`, 341K followers; links to the website; address 704 EDSA, Cubao |
| Poster address | Cubao Terminal, 704 EDSA cor. New York, printed on the Cubao poster |
| Third-party link | Sunlight Air's release links to the operator's P2P page |
| Discrepancy | Posters print the handle `/GenesisTransportServiceInc`, which returned "content isn't available". The working page is `/GenesisTransportInc` |

## VERIFIED: Genesis posters

**P2P page:** https://www.genesistransportserviceinc.com/p2pbusservice. Caption reads "As of August 18, 2026", older than its images.

| Key | Route | Printed as of | `Last-Modified` (UTC) | SHA-256 (prefix) |
|---|---|---|---|---|
| CUB-CRK | Cubao > Clark Airport | 2026-09-29 | 2026-10-29 06:13:48 | `505309a5` |
| CRK-CUB | Clark Airport > Cubao | 2026-09-29 | 2026-10-29 06:13:48 | `f87e2765` |
| TRI-CRK | TriNoma > Clark Airport | 2026-09-29 | 2026-10-29 06:13:48 | `790484d5` |
| CRK-TRI | Clark Airport > TriNoma | 2026-09-29 | 2026-10-29 06:13:48 | `e7a3734f` |
| T3-CRK | NAIA 3 > Clark Airport | 2026-09-29 | 2026-10-29 06:13:56 | `55dd0ec8` |
| CRK-T3 | Clark Airport > NAIA 3,2,1 | 2026-09-29 | 2026-10-29 06:13:56 | `3a84d190` |
| PITX-CRK | PITX > Clark Airport | 2026-09-29 | 2026-10-29 06:13:57 | `30f1f582` |
| CRK-PITX | Clark Airport > PITX | 2026-09-29 | 2026-10-29 06:13:56 | `03070d56` |
| BAL-CRK | Balanga > Clark Airport | 2026-09-23 | 2026-10-29 06:15:46 | `22b98df6` |
| CRK-BAL | Clark Airport > Balanga | 2026-09-23 | 2026-10-29 06:15:46 | `8b0fd4e0` |

**JoyBus page:** https://www.genesistransportserviceinc.com/joybus. Caption reads "As of September 18, 2026".

| Key | Route | Printed as of | `Last-Modified` (UTC) | SHA-256 (prefix) |
|---|---|---|---|---|
| CUB-BAG-J | Cubao > Baguio, JoyBus | 2026-09-18 | 2026-09-25 07:04:50 | `692347af` |
| CUB-BAG-D | Cubao > Baguio, Deluxe | 2026-09-18 | 2026-09-25 07:04:50 | `8701f64b` |
| BAG-CUB | Baguio > Cubao | 2026-09-18 | 2026-09-25 07:04:50 | `c76e17c5` |
| PAS-BAG-J | Pasay > Baguio, JoyBus | 2026-09-18 | 2026-09-25 07:04:51 | `c889624c` |
| PAS-BAG-D | Pasay > Baguio, Deluxe | 2026-09-18 | 2026-09-25 07:04:56 | `435f59e8` |
| BAG-T3-PAS | Baguio > NAIA 3 / Pasay | 2026-09-18 | 2026-09-25 07:04:56 | `aded5021` |
| BAG-T321-PAS | Baguio > NAIA 3, 2, 1 / Pasay | 2026-09-18 | 2026-09-25 07:04:57 | `651179a0` |
| BAG-AVE-PAS | Avenida panels, including Baguio > Pasay via Avenida | 2026-09-18 | 2026-09-25 07:04:57 | `44449ae7` |

**Boarding, as printed:** PITX Gate 5, Bay 33; NAIA Terminal 3, Bay 21, Arrival Area; SM City Clark Transport Hub, Main Gate Flagpole Area; Clark
Airport Terminal 2, Arrival Area Bus Station; Genesis Terminal, Balanga; Genesis Terminal, San Fernando; in front of Metrobank, Santa Cruz,
Lubao. JoyBus posters print no boarding addresses.

**Holiday policy:** none printed. Every poster says schedules are subject to change without prior notice.

## VERIFIED: owner-observed Genesis Facebook boards (screenshots supplied 2026-11-08)

The owner viewed these on Genesis Transport's Facebook page and supplied screenshots. They're operator-branded boards with printed dates.
**The screenshots don't show post dates or permalinks,** so the Facebook posting date is unknown. The printed "As of" date is the timetable date, not the
posting date. The screenshots stay in the owner's local records and aren't in this repository. Facts only are carried forward.

| Key | Route | Printed as of | Departures | File SHA-256 (prefix) |
|---|---|---|---|---|
| CRK-T3-N03 | Clark Airport > NAIA 1, 2, 3 | **2026-11-03** | 23 | `84e58aee` |
| T3-CRK-N03 | NAIA 3 > Clark Airport (eight highlighted from PITX) | **2026-11-03** | 31 | `5fb931f2` |
| CRK-PITX-N04 | Clark Airport > PITX | **2026-11-04** | 8 | `d269f9d7` |
| PITX-CRK-N04 | PITX > Clark Airport | **2026-11-04** | 8 | `ee436a56` |
| CRK-PITX | Clark Airport > PITX | 2026-09-29 | 8 | `ee4844b1` |
| PITX-CRK | PITX > Clark Airport | 2026-09-29 | 8 | `56ec8a99` |
| CRK-T3 | Clark Airport > NAIA 3,2,1 | 2026-09-29 | 23 | `dfbad800` |
| T3-CRK | NAIA 3 > Clark Airport | 2026-09-29 | 31 | `d6a6b8a4` |
| CRK-TRI | Clark Airport > TriNoma | 2026-09-29 | 18 | `df26c84d` |
| TRI-CRK | TriNoma > Clark Airport | 2026-09-29 | 18 | `eda062d6` |
| CRK-CUB | Clark Airport > Cubao | 2026-09-29 | 11 | `c1f76284` |
| CUB-CRK | Cubao > Clark Airport | 2026-09-29 | 10 | `60c25bac` |

**Comparison:** the four November boards carry the same times as their 29 September versions. The eight 29 September screenshots equal the website's
copies. **November dates apply only to the four directions above.** Cubao, TriNoma, Baguio and Balanga keep their own dates.

**Printed anomalies, kept as printed:**
- Cubao > Clark's sixth row reads "1:10 AM". Interpreted as 13:10 (see Inferences).
- Cubao > Clark's 9:10 PM row shows TriNoma at 11:00 PM, a 110-minute gap against 50–60 minutes elsewhere. Kept as printed.

## Other sources

| Source | Tier | Used for |
|---|---|---|
| Shipped Clark ↔ Baguio poster 2026-07-07 (owner-supplied, 2026-09) | VERIFIED, aged 124 days | Unchanged shipped data |
| https://www.philippinebeaches.org/clark-airport-p2p-bus-guide-routes-schedules/, as of 2026-03-25, credited to Genesis's Facebook | PROVISIONAL | Corroborates Balanga and Clark ↔ Baguio |
| https://pinoytips.com/genesis-transport-clark-airport-p2p-2026/, 2026-10-04, citing posters dated 2026-08-18 | PROVISIONAL | Corroborates stable PITX and NAIA lists |
| https://www.topgear.com.ph/features/feature-articles/naia-cubao-p2p-bus-2026-guide-a4682-20260401-lfrm, trips as of 2026-02-01 | PROVISIONAL | Reserve only |
| https://www.victoryliner.com/p2p.aspx, posters undated | PROVISIONAL | Reserve only |
| https://www.facebook.com/UbeExpress/, posts of 2026-10-19 and 2026-11-07 | PROVISIONAL | Evidence of day-specific dispatch |
| https://www.topgear.com.ph/features/feature-articles/p2p-bus-service-pitx-clark-international-airport-a4682-20260318 and https://www.sunlightair.ph/news-releases/genesis-launches-pitx-clark-p2p-route | HISTORICAL | Superseded PITX ↔ Clark times |
| https://2gobusandferries.com/genesis-transport-naia-clark-p2p-bus-schedule-2026/, "Updated November 3, 2026" | UNVERIFIED | Lead only. Its claim was later confirmed for the NAIA directions by the owner's screenshots |
| https://2gobusandferries.com/ube-express-naia-pitx-loop-p2p-bus-schedule-fare-2026/, https://jontotheworld.com/naia-to-pitx-how-to-get-there-by-bus-lrt-taxi-or-grab-2026-guide/, https://pamasahe.com/routes/naia-pitx-metro-manila-ube-express-schedule-fares | UNVERIFIED | Reason for the C2 relabel |

## Inferences

| Inference | Reason |
|---|---|
| 13:10 for Cubao's printed "1:10 AM" | Poster's TriNoma 2:00 PM; TriNoma poster marks 2:00 PM from Cubao. Shipped as 13:10 since v1.0.2, with a code comment |
| DAILY for posters with no day restriction | Same reading as the shipped Genesis routes |
| "Thurs to Mon" means calendar days | Plain reading |
| Baguio > Pasay is a union of three panels | All name Pasay as a destination; every trip leaves Baguio |
| PITX-bound trips added to Clark > NAIA T3 (23 + 8 = 31) | The Clark > PITX board prints "NAIA Terminal 3, Bay 21" as a stop on those trips. The operator's Clark > NAIA board still lists 23 |
| NAIA 3 > Clark stays at 31 | Its eight highlighted PITX-origin trips are already part of the 31 |

## Firmware dates and labels

The boards print "As of" dates and don't say "effective". `effectiveDate` is the date from which ON POINT applies a timetable.

| Schedule | Direction | Source date | `effectiveDate` | Label |
|---|---|---|---|---|
| `p2p-pitx-clark-airport` | PITX > Clark Airport | As of 2026-11-04 | 2026-11-04 | PUBLISHED |
| `p2p-clark-airport-pitx` | Clark Airport > PITX | As of 2026-11-04 | 2026-11-04 | PUBLISHED |
| `genesis-cubao-baguio` | Cubao > Baguio | As of 2026-09-18 | 2026-09-18 | PUBLISHED |
| `genesis-baguio-cubao` | Baguio > Cubao | As of 2026-09-18 | 2026-09-18 | PUBLISHED |
| `genesis-pasay-baguio` | Pasay > Baguio | As of 2026-09-18 | 2026-09-18 | PUBLISHED |
| `genesis-baguio-pasay` | Baguio > Pasay | As of 2026-09-18 | 2026-09-18 | PUBLISHED |
| `p2p-clark-airport-balanga` | Clark Airport > Balanga | As of 2026-09-23 | 2026-09-23 | PUBLISHED |
| `p2p-balanga-clark-airport` | Balanga > Clark Airport | As of 2026-09-23 | 2026-09-23 | PUBLISHED |
| `p2p-clark-airport-naia-t3` | Clark Airport > NAIA T3 | As of 2026-11-03; PITX trips as of 2026-11-04 | 2026-08-18, unchanged | PUBLISHED |
| `p2p-naia-t3-clark-airport` | NAIA T3 > Clark Airport | As of 2026-11-03 | 2026-08-18, unchanged | PUBLISHED |
| `p2p-cubao-clark-airport`, `p2p-clark-airport-cubao` | Cubao ↔ Clark Airport | As of 2026-09-29 | 2026-08-18, unchanged | PUBLISHED |
| `p2p-trinoma-clark-airport`, `p2p-clark-airport-trinoma` | TriNoma ↔ Clark Airport | As of 2026-09-29 | 2026-08-18, unchanged | PUBLISHED |
| `p2p-naia-pitx`, `p2p-pitx-naia` | NAIA T3 ↔ PITX | UBE Express, as of 2026-05-01 | 2026-05-01, unchanged | **PROVISIONAL** (was PUBLISHED) |

November dates apply only to the directions shown with them. No November date was extended to any other route.

**Corrections to shipped routes**

| # | Direction(s) | Change |
|---|---|---|
| C1 | Clark Airport > NAIA T3 | Adds 03:00, 04:30, 07:30, 09:00, 10:45, 12:00, 15:30 and 19:00, for 31 trips (was 23). These are the Clark > PITX board's departures, which print NAIA Terminal 3 as a stop. The operator's Clark > NAIA board still lists 23, so the 31 is a combination of two operator boards |
| C2 | NAIA T3 ↔ PITX | Relabelled PROVISIONAL. Departures unchanged |
| C3 | NAIA T3 > Clark; Cubao and TriNoma ↔ Clark | Source notes updated. Departures unchanged |

## Uncertainties

None changes the shipped data.

| # | Uncertainty | Severity | Handling |
|---|---|---|---|
| U1 | Cubao and TriNoma boards may also have been reissued in November. Four other directions were, with unchanged times. No November board has been seen for these four | Low–medium | Stay at 2026-09-29; re-check |
| U2 | Cubao 9:10 PM reaches TriNoma at 11:00 PM, 110 minutes against 50–60 for other trips. It's on the operator's board, so it isn't a transcription error, but it may be a misprint | Low–medium | Kept as printed |
| U3 | Baguio > Pasay combines boards titled "NAIA 3 / Pasay" and "NAIA 3, 2, 1 / Pasay". "/" is read as serving both | Low–medium | PUBLISHED; disclosed |
| U4 | Clark ↔ Baguio's source is dated 2026-07-07, and no newer board has been reported | Low–medium | Stays PUBLISHED; not changed by M2 |
| U5 | Holidays and peak seasons: no Genesis policy found. Every poster says schedules are subject to change without prior notice | Medium, inherent | No overrides; the release text warns |
| U6 | NAIA T3 ↔ PITX trip count unknown: 8/7 shipped, other lists 12/12 and 16/14 | Medium | PROVISIONAL (C2) |
| U7 | "Only available from Thurs to Mon" is read as calendar days. Affects the 02:00 Pasay trip | Low | Recorded |
| U8 | 1:10 PM is an inferred reading inside a PUBLISHED direction | Low | Disclosed; basis in Inferences |

## Re-verification (90 days)

| Routes | Date |
|---|---|
| JoyBus routes (Cubao and Pasay ↔ Baguio) | 2026-12-17 |
| Clark ↔ Balanga | 2026-12-22 |
| Cubao and TriNoma ↔ Clark | 2026-12-28 |
| Clark ↔ NAIA T3 | 2027-02-01 |
| PITX ↔ Clark | 2027-02-02 |
