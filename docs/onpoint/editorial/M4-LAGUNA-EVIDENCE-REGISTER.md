# ON POINT M4 evidence register (LAGUNA)

Facts only, in the format of `docs/onpoint/editorial/M2-EVIDENCE-REGISTER.md`. This is the editorial provenance for the three route pairs in
the LAGUNA collection (`laguna`). The operator boards are the owner's screenshots of operator Facebook posts, transcribed and digested in
the owner's local records. No images are stored in this repository. Departures are in `src/activities/on_point/OnPointScheduleData.cpp`;
`test/on_point_m4/OnPointM4DataTest.cpp` holds a separate transcription typed from the boards.

**All six directions are PROVISIONAL.** The departure lists are complete and come from the operators, but no current operator
statement confirms they're still in force. Not finding a newer notice doesn't establish currency. Only a dated operator post that
re-issues or confirms a timetable could support PUBLISHED. Sources were re-read on 2026-11-13 before coding and again before freezing.

## Nuvali ↔ One Ayala (`p2p-nuvali-one-ayala`, collection default)

| Item | Finding |
|---|---|
| Publisher | Nuvali official Facebook page (Ayala Land Estates). Boards titled "P2P NUVALI SCHEDULE", with the TAS Trans Corp. watermark ("PREMIUM POINT TO POINT BUS, STA. ROSA – MAKATI") |
| Date | Posted **2025-11-12**. Printed "NEW SCHEDULE UNTIL FURTHER NOTICE"; no effective date. Effective date stored: 2025-11-12 |
| Terminals and stops | Nuvali Transport Terminal (Santa Rosa, Laguna). Makati: **One Ayala** terminal, then **Circuit Makati**. The board's footer: "The last trip is only at One Ayala terminal and will no longer proceed to Circuit". The 2025-11-12 advisory names the line "Nuvali – One Ayala – Circuit Makati (and vice versa)" |
| Device labels | **NUVALI** ↔ **ONE AYALA**. The board prints the columns "NUVALI TO MAKATI" and "ONE AYALA TO NUVALI" |
| Lists | Out (Nuvali → Makati): 22 Mon–Fri, 15 Sat, 14 Sun. Back (One Ayala → Nuvali): 22 / 15 / 14. Printed duplicates stored once (weekday 7:00 PM out; Saturday 12:00 back) |
| Not modelled | Circuit → Nuvali (a second Makati boarding point on the same line; not counted again). Bel-Air Sta. Rosa → Nuvali (morning only, one direction, relationship unknown) |
| Holidays | Board: **"HOLIDAYS - NO OPERATIONS"**. One Ayala post (2026-03-05): "No trips on holiday". **Stored:** an empty override (NO SERVICE TODAY) on every nationwide regular holiday and special non-working day in the shared 2026–2027 calendar, both directions (36 dates each). **Including special non-working days is our interpretation** of the general wording, not an operator-enumerated list. Metro Manila-only days (2026-11-16 to 18) and special working days (EDSA anniversary) keep the ordinary lists |
| Limits | Eid'l Fitr and Eid'l Adha aren't in the calendar until proclaimed, so the board shows trips on them. Dates after 2027 fall back to the ordinary lists until the calendar is extended |
| Saturday conflict | Two undated or unsourced guides (P2PBus.ph; Windowseat, 2026-05-05) list weekdays only. That's an omission, not proof of cancellation. Dated estate and terminal posts (Ayala Land 2026-04-02: "Limited trips for Saturday and Sunday"; One Ayala 2026-03-05: Saturday and Sunday windows) still describe weekend service. The operator's Saturday and Sunday lists are kept |

## WalterMart Cabuyao ↔ Makati (`p2p-cabuyao-makati`) and ↔ BGC (`p2p-cabuyao-bgc`)

| Item | Finding |
|---|---|
| Publisher | Calamba P2P Facebook page (address: Saint Rose Transit Terminal, Crossing, Calamba). A "PREMIUM POINT TO POINT BUS" poster; no operator name printed |
| Date | Posted **2026-08-31**. Printed "REGULAR TRIPS WILL RESUME ON TUESDAY SEPTEMBER 1, 2026". Effective date stored: 2026-09-01. Earlier posts (2026-07-03 launch, 2026-07-17) are superseded |
| Origin | **WALTERMART CABUYAO** (as printed). Device label **WM CABUYAO**: the full name, 393 px, doesn't fit beside the board's REVERSE control (289 px), while WM CABUYAO (229 px) fits everywhere |
| Makati end | Printed **"MAKATI (ONE AYALA / LANDMARK)"**: two named stops, not one verified terminal. Device label **MAKATI** |
| BGC end | Printed **"BGC TAGUIG (MARKET MARKET)"**, without exclamation marks. Device label **MARKET MARKET** (281 px; 8 px to spare beside REVERSE). Alias: BGC Taguig |
| Lists | Monday to Friday only, 10 trips each direction. Cabuyao → Makati 05:00–18:30; Cabuyao → BGC 05:00–19:00; Makati → Cabuyao 06:00–20:00; BGC → Cabuyao 06:00–21:00 |
| Route notes | **Outbound** (Cabuyao → Makati, Cabuyao → BGC): the 06:00, 07:00 and 08:00 departures via Mayapa Exit; all other departures via Pulo Exit. **Return** (Makati → Cabuyao, BGC → Cabuyao): every departure via Pulo Exit, as listed on the operator board. Exits are recorded in the notes, not modelled. Whether the Makati and BGC trips use separate buses is unknown |
| Holidays | The operator states no standing holiday rule, so **no overrides**; the notes say holiday service may differ. Calamba P2P posted "NO OPERATION on Cabuyao to BGC Taguig, Cabuyao to Makati on NOVEMBER 2, 2026 (Monday)" on 2026-11-02. That's one dated notice for one past date, not a rule, and it isn't encoded |

## Not included

Calamba (Saint Rose Transit) ↔ Makati, BGC and Lawton: the operator publishes operating hours only, and hours never become departure
countdowns. HOLD.
