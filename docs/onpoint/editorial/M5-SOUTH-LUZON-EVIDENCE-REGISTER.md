# ON POINT M5 evidence register (SOUTH LUZON)

Facts only, in the format of `docs/onpoint/editorial/M4-LAGUNA-EVIDENCE-REGISTER.md`. This is the editorial provenance for the four RRCG
route pairs in the SOUTH LUZON collection (`south-luzon`).
- **Sources:** the operator boards are the owner's screenshots of RRCG posts, transcribed and digested in the owner's local records. No images are stored in this repository.
- **Data and test:** departures are in `src/activities/on_point/OnPointScheduleData.cpp`; `test/on_point_m5/OnPointM5DataTest.cpp` holds a separate transcription typed from the boards.
- **Re-reads:** before coding (2026-11-14 00:14 Manila) and again before freezing.

**All eight directions are PROVISIONAL.** Every list is a complete RRCG operator departure list. Not one direction has current
affirmative operator confirmation, and two pairs have conflicting RRCG sources (below). The conflicts are recorded, not resolved.

## Batangas Grand Terminal ↔ Newport World Resorts (`p2p-batangas-grand-newport`, collection default)

| Item | Finding |
|---|---|
| Operator | RRCG Transport (logo on board). Fare ₱460 / ₱368 |
| Board | "DAILY SCHEDULE", posted **2026-10-21** (owner-reported; the post date isn't visible in the screenshot). The board prints no date |
| Terminals | Outbound: **Batangas Grand Terminal » Newport World Resorts (NAIA Terminal 3)**. Return: **Newport World Resorts » Batangas Grand Terminal / Pier**. Note on board: "Connected to NAIA Terminal 3 via Runway Manila at Newport" |
| Device labels | **BATANGAS** ↔ **NEWPORT**. BATANGAS GRAND (303 px) and NEWPORT WORLD (297 px) don't fit beside REVERSE (289 px). BATANGAS is true for both the terminal and the return's "/ Pier" end |
| Lists | 12 each way, every day. Out 02:00–20:30; back 05:00–23:45 |
| Corroboration | RRCG's "NOVEMBER 1 & 2, 2026" board: identical times. A third-party list "provided for November 2026": identical except one malformed "12:30 NN" (board: 12:00 NN) |
| **Conflict** | **RRCG's website feed (undated, unchanged since at least 2026-11-08) lists 10 trips each way at different times.** Pamasahe (September 2026) is a subset of the feed. The sequence suggests the feed is stale, but that's an inference and isn't applied. The board lists are used, and the conflict stays open |
| Effective date stored | 2026-10-21 (board post date) |

## SM Lipa City ↔ Newport World Resorts (`p2p-sm-lipa-newport`)

| Item | Finding |
|---|---|
| Operator | RRCG Transport. Launched 2026-07-25. Fare ₱460 / ₱368 |
| Board | "DAILY SCHEDULE", posted **2026-10-28** (owner-reported; not visible in the screenshot). The board prints no date |
| Terminals | **SM Lipa City** ↔ **Newport World Resorts (NAIA Terminal 3)**; the same Runway Manila note |
| Device labels | **SM LIPA** ↔ **NEWPORT** |
| Lists | 12 each way, every day. Out 03:00–20:00; back 05:30–23:15 |
| Corroboration | The 1 & 2 November board and the November 2026 third-party list are identical |
| **Conflict** | **RRCG's website feed lists 9 trips each way at different times.** Recorded, not resolved |
| Effective date stored | 2026-10-28 |

## SM Masinag ↔ One Ayala (`p2p-sm-masinag-one-ayala`)

| Item | Finding |
|---|---|
| Operator | RRCG Transport. Fare ₱140 / ₱112 |
| Board | "WEEKDAY SCHEDULE", **undated**. Seen 2026-11-08 (owner screenshot). **Equals RRCG's website feed in both directions** |
| Terminals and stops | Header **SM MASINAG ↔ ONE AYALA**; "Antipolo » Makati" and "Makati » Antipolo". Outbound boarding also at **Robinsons Antipolo** (05:00–06:30, on the rows for Masinag 05:30–07:00) and **Ayala Malls Feliz** (special pick-up / drop-off, 15 minutes after Masinag). Makati special drop-offs: Rustans Makati / Paseo de Roxas / PBCOM / RCBC. Return: "5:00 PM onwards special drop-off at Robinsons Place Antipolo". The feed names the return destination "SM City Masinag". Intermediate stops aren't modelled |
| Device labels | **SM MASINAG** ↔ **ONE AYALA** (as printed) |
| Lists | Monday to Friday only. Out 10 (05:00–08:30, 17:00, 18:00); back 11 (15:00–21:00). No weekend service |
| Current operation | RRCG's 2026-11-13 advisory names "Antipolo - Makati" among its routes. That's dated evidence the service is running; it doesn't restate the times |
| Effective date stored | 2026-11-08 (first observed; the board is undated) |

## Sierra Valley (Cainta) ↔ One Ayala (`p2p-sierra-valley-one-ayala`)

| Item | Finding |
|---|---|
| Operator | RRCG Transport. Fare ₱90 / ₱72; the Robinsons Galleria drop-off is ₱60 / ₱48 |
| Boards | **"MONDAY & FRIDAY SCHEDULE"** and **"TUESDAY TO THURSDAY SCHEDULE"**, both **undated**, seen 2026-11-08 |
| Terminals and stops | **Sierra Valley (Cainta)** ↔ **One Ayala**. Makati special drop-offs: Rustans Makati / Paseo de Roxas / PBCOM / RCBC. Return: special drop-off at Robinsons Galleria. Not modelled |
| Device labels | **SIERRA VALLEY** ↔ **ONE AYALA** |
| Lists | Sierra Valley → One Ayala: 05:30, 06:00, 06:30, 07:00 on both boards. One Ayala → Sierra Valley: **Monday & Friday** 17:30, 18:00, 19:00, 20:00; **Tuesday to Thursday** 17:30, 18:00, 18:30, 19:30, 20:00. No weekend service |
| **Conflict** | RRCG's website feed carries **only the Tuesday-to-Thursday pattern** and lists no Monday or Friday trips. **The feed's omission isn't treated as a suspension:** Monday and Friday use the operator's Monday & Friday board. An undated aggregator (Sakay) groups the days differently again (not used) |
| Current operation | The 2026-11-13 advisory names "Cainta - Makati" among RRCG's routes |
| Effective date stored | 2026-11-08 |

## Special operations: RRCG advisory of 2026-11-13 (not encoded)

RRCG Facebook, 2026-11-13 14:01: "In observance of the upcoming ASEAN Summit, there will be **no operations on November 16, 17, and 18,
2026**, on the following routes: Cainta - Makati, Antipolo - Makati, Ortigas - Antipolo, Alabang (ATC) - Ortigas (Megamall). The Alabang
(ATC)-Makati and Alabang-Shaw routes will remain operational with limited scheduled trips…", with special boards for those two.

| Route in ON POINT | Effect on 16–18 Nov 2026 | Stored behaviour |
|---|---|---|
| SM Masinag ↔ One Ayala (M5) | No operations | Ordinary weekday list |
| Sierra Valley ↔ One Ayala (M5) | No operations | Ordinary weekday list |
| ATC Alabang ↔ SM Megamall (M3-A) | No operations | Ordinary weekday list |
| ATC Alabang ↔ Greenbelt (M3-A) | Limited scheduled trips | Ordinary weekday list |
| VTX Alabang ↔ Starmall Shaw (M3-A) | Limited scheduled trips | Ordinary weekday list |

These dates aren't encoded: it's a one-off operator advisory, not a standing holiday rule, and the dates fall before any release. The
existing holiday rules are unchanged: no overrides for any RRCG route. The M5 Rizal notes mention the suspension.

## Known limitation (for release review)

Route notes aren't visible on the device. The board shows only **PROVISIONAL**, so it doesn't tell a rider which kind of uncertainty
applies: an undated board, a board-versus-feed conflict, or a special-operations notice. This is carried forward as a release-review
limitation. No interface change is part of M5.
