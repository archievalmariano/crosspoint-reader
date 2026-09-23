#!/usr/bin/env python3
"""Generate monochrome On Point visual-direction SVG previews.

This is a design-only host tool. It does not import or modify firmware code.
The SVGs are deliberately limited to primitives available in GfxRenderer:
solid rectangles, polygons, strong rules, and fixed-size text.
"""

from __future__ import annotations

from dataclasses import dataclass
from html import escape
from pathlib import Path


WIDTH = 480
HEIGHT = 800
HERE = Path(__file__).resolve().parent
REPO_ROOT = HERE.parents[1]
SVG_DIR = HERE / "svg"

NOTO_REGULAR = (REPO_ROOT / "lib/EpdFont/builtinFonts/source/NotoSans/NotoSans-Regular.ttf").as_uri()
NOTO_BOLD = (REPO_ROOT / "lib/EpdFont/builtinFonts/source/NotoSans/NotoSans-Bold.ttf").as_uri()
UBUNTU_REGULAR = (REPO_ROOT / "lib/EpdFont/builtinFonts/source/Ubuntu/Ubuntu-Regular.ttf").as_uri()
UBUNTU_BOLD = (REPO_ROOT / "lib/EpdFont/builtinFonts/source/Ubuntu/Ubuntu-Bold.ttf").as_uri()

FONT_CSS = """
@font-face {
  font-family: 'Preview Noto Sans';
  src: url('%s');
  font-weight: 400;
}
@font-face {
  font-family: 'Preview Noto Sans';
  src: url('%s');
  font-weight: 700;
}
@font-face {
  font-family: 'Preview Ubuntu';
  src: url('%s');
  font-weight: 400;
}
@font-face {
  font-family: 'Preview Ubuntu';
  src: url('%s');
  font-weight: 700;
}
text { font-family: 'Preview Noto Sans', sans-serif; }
.utility { font-family: 'Preview Ubuntu', sans-serif; }
""" % (NOTO_REGULAR, NOTO_BOLD, UBUNTU_REGULAR, UBUNTU_BOLD)

DIGITS = {
    "0": ("111", "101", "101", "101", "111"),
    "1": ("010", "110", "010", "010", "111"),
    "2": ("111", "001", "111", "100", "111"),
    "3": ("111", "001", "111", "001", "111"),
    "4": ("101", "101", "111", "001", "001"),
    "5": ("111", "100", "111", "001", "111"),
    "6": ("111", "100", "111", "101", "111"),
    "7": ("111", "001", "010", "010", "010"),
    "8": ("111", "101", "111", "101", "111"),
    "9": ("111", "101", "111", "001", "111"),
}


@dataclass(frozen=True)
class State:
    slug: str
    origin: str
    destination: str
    now: str | None
    minutes: str | None
    departure: str | None
    following: tuple[str, ...] = ()
    badge: str | None = None
    date: str = "WED 16 SEP"
    progress: int = 2
    next_date: str | None = None


STATES = {
    "normal": State(
        "normal", "TRINOMA", "BALAGTAS", "21:12", "18", "21:30", ("22:00", "22:30 LAST"), progress=2
    ),
    "near": State(
        "near", "TRINOMA", "BALAGTAS", "21:26", "04", "21:30", ("22:00", "22:30 LAST"), progress=4
    ),
    "last": State(
        "last", "TRINOMA", "BALAGTAS", "22:12", "18", "22:30", (), badge="LAST SERVICE", progress=2
    ),
    "before-first": State(
        "before-first",
        "BALAGTAS",
        "TRINOMA",
        "03:50",
        "40",
        "04:30",
        ("05:00", "05:30"),
        badge="FIRST SERVICE",
        progress=1,
    ),
    "service-ended": State(
        "service-ended",
        "BALAGTAS",
        "TRINOMA",
        "20:17",
        None,
        "04:30",
        (),
        date="WED 16 SEP",
        next_date="THU 17 SEP",
    ),
    "clock-unavailable": State(
        "clock-unavailable", "TRINOMA", "BALAGTAS", None, None, None, (), date="DATE UNAVAILABLE"
    ),
}


def root(inner: str, width: int = WIDTH, height: int = HEIGHT) -> str:
    # Quick Look always emits square thumbnails. A square wrapper prevents it
    # from stretching portrait art; render_pngs.sh crops this back to the
    # declared output dimensions.
    canvas = max(width, height)
    offset_x = (canvas - width) // 2
    offset_y = (canvas - height) // 2
    return f'''<svg xmlns="http://www.w3.org/2000/svg" width="{canvas}" height="{canvas}" viewBox="0 0 {canvas} {canvas}" data-output-width="{width}" data-output-height="{height}">
<style>{FONT_CSS}</style>
<rect width="{canvas}" height="{canvas}" fill="#fff"/>
<g transform="translate({offset_x} {offset_y})">
{inner}
</g>
</svg>\n'''


def text(
    value: str,
    x: int,
    y: int,
    size: int,
    *,
    weight: int = 400,
    fill: str = "#000",
    anchor: str = "start",
    spacing: float = 0,
    utility: bool = False,
    transform: str | None = None,
) -> str:
    klass = ' class="utility"' if utility else ""
    transform_attr = f' transform="{transform}"' if transform else ""
    return (
        f'<text{klass} x="{x}" y="{y}" font-size="{size}" font-weight="{weight}" '
        f'fill="{fill}" text-anchor="{anchor}" letter-spacing="{spacing}"{transform_attr}>'
        f"{escape(value)}</text>"
    )


def rect(x: int, y: int, width: int, height: int, fill: str = "#000") -> str:
    return f'<rect x="{x}" y="{y}" width="{width}" height="{height}" fill="{fill}"/>'


def rule(x1: int, y1: int, x2: int, y2: int, width: int = 3, color: str = "#000") -> str:
    return f'<line x1="{x1}" y1="{y1}" x2="{x2}" y2="{y2}" stroke="{color}" stroke-width="{width}"/>'


def modular_number(value: str, x: int, y: int, cell: int, *, fill: str = "#000", gap: int | None = None) -> str:
    gap = cell // 2 if gap is None else gap
    pieces: list[str] = []
    cursor = x
    for digit in value:
        rows = DIGITS[digit]
        for row_index, row in enumerate(rows):
            for col_index, bit in enumerate(row):
                if bit == "1":
                    pieces.append(rect(cursor + col_index * cell, y + row_index * cell, cell - 3, cell - 3, fill))
        cursor += cell * 3 + gap
    return "\n".join(pieces)


def down_mark(x: int, y: int, color: str = "#fff") -> str:
    return "\n".join(
        [
            rule(x, y, x, y + 27, 4, color),
            f'<polygon points="{x-7},{y+21} {x+7},{y+21} {x},{y+31}" fill="{color}"/>',
        ]
    )


def hourglass(
    x: int,
    y: int,
    width: int,
    height: int,
    progress: int,
    *,
    inverse: bool = False,
    stroke: int = 4,
) -> str:
    color = "#fff" if inverse else "#000"
    mid_x = x + width // 2
    mid_y = y + height // 2
    bottom = y + height
    pieces = [
        rule(x, y, x + width, y, stroke, color),
        rule(x, bottom, x + width, bottom, stroke, color),
        rule(x, y, mid_x, mid_y, stroke, color),
        rule(x + width, y, mid_x, mid_y, stroke, color),
        rule(mid_x, mid_y, x, bottom, stroke, color),
        rule(mid_x, mid_y, x + width, bottom, stroke, color),
    ]
    progress = max(0, min(5, progress))
    upper = 5 - progress
    lower = progress
    if upper:
        fill_y = mid_y - 5 - upper * (height // 12)
        half = max(5, upper * width // 12)
        pieces.append(f'<polygon points="{mid_x},{mid_y-5} {mid_x-half},{fill_y} {mid_x+half},{fill_y}" fill="{color}"/>')
    if lower:
        fill_y = bottom - 5 - lower * (height // 12)
        half = max(5, lower * width // 12)
        pieces.append(f'<polygon points="{mid_x},{fill_y} {mid_x-half},{bottom-5} {mid_x+half},{bottom-5}" fill="{color}"/>')
    return "\n".join(pieces)


def utility_header(now: str | None, *, inverse: bool = False) -> str:
    color = "#fff" if inverse else "#000"
    right = now if now else "--:--"
    return "\n".join(
        [
            text("ON POINT", 24, 31, 15, weight=700, fill=color, spacing=2.2, utility=True),
            text(right, 456, 31, 15, weight=700, fill=color, anchor="end", spacing=0.8, utility=True),
        ]
    )


def route_band(state: State, y: int = 54, height: int = 136) -> str:
    return "\n".join(
        [
            rect(0, y, WIDTH, height),
            text(state.origin, 24, y + 38, 29, weight=700, fill="#fff", spacing=0.5),
            down_mark(35, y + 51),
            text(state.destination, 58, y + 102, 29, weight=700, fill="#fff", spacing=0.5),
            text("PROVISIONAL", 456, y + 117, 10, weight=700, fill="#fff", anchor="end", spacing=1.6, utility=True),
        ]
    )


def footer(date: str) -> str:
    return "\n".join(
        [
            rule(24, 718, 456, 718, 3),
            text(date, 24, 746, 12, weight=700, spacing=1.1, utility=True),
            text("LEFT / RIGHT  SWITCH", 456, 746, 11, weight=700, anchor="end", spacing=0.8, utility=True),
            text("SCHEDULED TIMETABLE • UTC+8", 24, 778, 10, spacing=1.0, utility=True),
            text("HOME", 456, 778, 10, weight=700, anchor="end", spacing=1.2, utility=True),
        ]
    )


def signal_ledger(state: State) -> str:
    pieces = [utility_header(state.now), route_band(state)]
    if state.slug == "service-ended":
        pieces.extend(
            [
                text("SERVICE", 24, 278, 57, weight=700, spacing=-1.5),
                text("ENDED", 24, 337, 57, weight=700, spacing=-1.5),
                rect(24, 373, 432, 5),
                text("NEXT SCHEDULED SERVICE", 24, 419, 13, weight=700, spacing=1.5, utility=True),
                text(state.departure or "", 24, 504, 74, weight=700, spacing=-2),
                text(state.next_date or "", 456, 499, 18, weight=700, anchor="end", spacing=1.0),
                text("TIMETABLE RESUMES", 456, 531, 11, anchor="end", spacing=1.2, utility=True),
                text("No departure is implied before this time.", 24, 611, 15, utility=True),
            ]
        )
    elif state.slug == "clock-unavailable":
        pieces.extend(
            [
                rect(24, 226, 14, 223),
                text("CLOCK", 64, 296, 56, weight=700, spacing=-1),
                text("NOT SET", 64, 357, 56, weight=700, spacing=-1),
                rule(64, 390, 456, 390, 5),
                text("NO COUNTDOWN SHOWN", 64, 431, 13, weight=700, spacing=1.5, utility=True),
                text("Sync the RTC once in Settings.", 64, 500, 18, weight=700),
                text("On Point will not substitute a guessed time.", 64, 535, 14, utility=True),
            ]
        )
    else:
        if state.badge:
            pieces.extend([rect(24, 214, 148, 28), text(state.badge, 36, 233, 11, weight=700, fill="#fff", spacing=1.2, utility=True)])
        else:
            pieces.append(text("UNTIL SCHEDULED DEPARTURE", 24, 232, 12, weight=700, spacing=1.4, utility=True))
        pieces.extend(
            [
                modular_number(state.minutes or "", 24, 264, 44),
                text("MIN", 436, 459, 15, weight=700, anchor="middle", spacing=2.0, utility=True,
                     transform="rotate(-90 436 459)"),
                hourglass(376, 282, 64, 116, state.progress),
                rule(24, 510, 456, 510, 5),
                text("SCHEDULED DEPARTURE", 24, 542, 11, weight=700, spacing=1.3, utility=True),
                text(state.departure or "", 24, 590, 42, weight=700, spacing=-1),
                text("FOLLOWING", 236, 542, 11, weight=700, spacing=1.3, utility=True),
            ]
        )
        for index, item in enumerate(state.following[:2]):
            pieces.append(text(item, 236, 576 + index * 35, 21 if "LAST" not in item else 18, weight=700))
        if not state.following:
            pieces.append(text("—", 236, 584, 24, weight=700))
        if state.badge == "LAST SERVICE":
            pieces.append(text("FINAL DEPARTURE TODAY", 24, 666, 12, weight=700, spacing=1.2, utility=True))
        elif state.badge == "FIRST SERVICE":
            pieces.append(text("SERVICE DAY BEGINS", 24, 666, 12, weight=700, spacing=1.2, utility=True))
        else:
            pieces.append(text("30 MIN INTERVAL", 24, 666, 12, weight=700, spacing=1.2, utility=True))
    pieces.append(footer(state.date))
    return "\n".join(pieces)


def transit_spine(state: State) -> str:
    pieces = [rect(0, 0, 126, HEIGHT), text("ON POINT", 28, 45, 14, weight=700, fill="#fff", spacing=2, utility=True)]
    pieces.extend(
        [
            rule(42, 112, 42, 318, 5, "#fff"),
            '<circle cx="42" cy="118" r="9" fill="#fff"/>',
            '<circle cx="42" cy="312" r="9" fill="#fff"/>',
            text(state.origin, 73, 126, 17, weight=700, fill="#fff", transform="rotate(90 73 126)"),
            text(state.destination, 73, 323, 17, weight=700, fill="#fff", transform="rotate(90 73 323)"),
            text("PROVISIONAL", 63, 740, 10, weight=700, fill="#fff", anchor="middle", spacing=1.4, utility=True,
                 transform="rotate(-90 63 740)"),
            text(state.now or "--:--", 456, 35, 14, weight=700, anchor="end", utility=True),
            text("MINUTES TO GO", 154, 82, 12, weight=700, spacing=1.5, utility=True),
            modular_number(state.minutes or "18", 154, 126, 42),
            rule(154, 375, 456, 375, 5),
            text("SCHEDULED", 154, 414, 11, weight=700, spacing=1.4, utility=True),
            text(state.departure or "21:30", 154, 468, 44, weight=700),
            text("FOLLOWING", 154, 530, 11, weight=700, spacing=1.4, utility=True),
            text("22:00", 154, 572, 22, weight=700),
            text("22:30  LAST", 290, 572, 18, weight=700),
            rule(154, 684, 456, 684, 3),
            text(state.date, 154, 720, 12, weight=700, spacing=1, utility=True),
            text("← / →", 456, 720, 12, weight=700, anchor="end", utility=True),
            text("SCHEDULED TIMETABLE", 154, 760, 10, spacing=1, utility=True),
        ]
    )
    return "\n".join(pieces)


def reverse_field(state: State) -> str:
    pieces = [utility_header(state.now), route_band(state, 54, 122), rect(0, 202, WIDTH, 340)]
    pieces.extend(
        [
            text("UNTIL SCHEDULED DEPARTURE", 24, 237, 11, weight=700, fill="#fff", spacing=1.3, utility=True),
            modular_number(state.minutes or "18", 24, 270, 46, fill="#fff"),
            text("MIN", 456, 482, 17, weight=700, fill="#fff", anchor="end", spacing=2, utility=True),
            hourglass(386, 284, 58, 108, state.progress, inverse=True),
            text("SCHEDULED", 24, 586, 11, weight=700, spacing=1.4, utility=True),
            text(state.departure or "21:30", 24, 632, 38, weight=700),
            text("NEXT", 237, 586, 11, weight=700, spacing=1.4, utility=True),
            text("22:00  /  22:30 LAST", 237, 625, 18, weight=700),
        ]
    )
    pieces.append(footer(state.date))
    return "\n".join(pieces)


def open_poster(state: State) -> str:
    pieces = [utility_header(state.now), rule(24, 54, 456, 54, 5)]
    pieces.extend(
        [
            text(state.origin, 24, 102, 31, weight=700),
            rule(26, 128, 430, 128, 4),
            f'<polygon points="430,120 456,128 430,136" fill="#000"/>',
            text(state.destination, 456, 178, 31, weight=700, anchor="end"),
            text("SCHEDULED / NOT LIVE", 24, 210, 10, weight=700, spacing=1.5, utility=True),
            modular_number(state.minutes or "18", 55, 252, 48),
            text("MIN", 425, 464, 17, weight=700, anchor="end", spacing=2, utility=True),
            rule(24, 514, 456, 514, 5),
            text("DEPARTURE", 24, 551, 11, weight=700, spacing=1.4, utility=True),
            text(state.departure or "21:30", 24, 602, 43, weight=700),
            text("FOLLOWING", 252, 551, 11, weight=700, spacing=1.4, utility=True),
            text("22:00", 252, 593, 21, weight=700),
            text("22:30  LAST", 354, 593, 17, weight=700),
            text("PROVISIONAL TIMETABLE", 24, 675, 11, weight=700, spacing=1.4, utility=True),
        ]
    )
    pieces.append(footer(state.date))
    return "\n".join(pieces)


def pass2_header() -> str:
    return "\n".join(
        [
            text("ON POINT", 24, 31, 20, weight=700, spacing=1.0),
            text("P2P SCHEDULE", 456, 31, 10, weight=700, anchor="end", spacing=1.0, utility=True),
            rule(24, 52, 456, 52, 5),
        ]
    )


def pass2_route(state: State) -> str:
    return "\n".join(
        [
            text(state.origin, 24, 100, 31, weight=700, spacing=0.3),
            rule(24, 130, 438, 130, 4),
            '<polygon points="438,121 460,130 438,139" fill="#000"/>',
            text(state.destination, 456, 178, 31, weight=700, anchor="end", spacing=0.3),
        ]
    )


def pass2_footer(label: str, *, date_available: bool = True) -> str:
    left = label if date_available else "RTC REQUIRED"
    return "\n".join(
        [
            rule(24, 726, 456, 726, 3),
            text(left, 24, 758, 11, weight=700, spacing=1.0, utility=True),
            text("PROVISIONAL", 456, 758, 10, weight=700, anchor="end", spacing=1.4, utility=True),
        ]
    )


def refined_open_poster(state: State, *, with_hourglass: bool = False) -> str:
    pieces = [pass2_header(), pass2_route(state)]
    if state.slug == "service-ended":
        pieces.extend(
            [
                text("SERVICE", 24, 296, 60, weight=700, spacing=-1.5),
                text("ENDED", 24, 358, 60, weight=700, spacing=-1.5),
                rule(24, 392, 456, 392, 5),
                text("NEXT SCHEDULED", 24, 434, 11, weight=700, spacing=1.4, utility=True),
                text(state.departure or "", 24, 520, 78, weight=700, spacing=-2.0),
                text(state.next_date or "", 456, 487, 18, weight=700, anchor="end", spacing=0.8),
                text("FIRST TRIP", 456, 520, 11, weight=700, anchor="end", spacing=1.2, utility=True),
                text("NO MORE TRIPS TODAY", 24, 620, 11, weight=700, spacing=1.0, utility=True),
            ]
        )
        pieces.append(pass2_footer(state.date))
        return "\n".join(pieces)

    if state.slug == "clock-unavailable":
        pieces.extend(
            [
                rect(24, 246, 12, 214),
                text("CLOCK", 64, 312, 57, weight=700, spacing=-1.2),
                text("NOT SET", 64, 373, 57, weight=700, spacing=-1.2),
                rule(64, 408, 456, 408, 5),
                text("NO COUNTDOWN SHOWN", 64, 448, 11, weight=700, spacing=1.4, utility=True),
                text("Sync the RTC once in Settings.", 64, 520, 18, weight=700),
                text("On Point never substitutes a guessed time.", 64, 557, 14, utility=True),
            ]
        )
        pieces.append(pass2_footer("RTC REQUIRED", date_available=False))
        return "\n".join(pieces)

    tag = None
    if state.slug == "last":
        tag = "LAST"
    elif state.slug == "before-first":
        tag = "FIRST TRIP"

    pieces.append(text("UNTIL NEXT SCHEDULED DEPARTURE", 24, 218, 11, weight=700, spacing=1.4, utility=True))
    if tag:
        tag_width = 78 if tag == "LAST" else 112
        tag_x = 456 - tag_width
        pieces.extend(
            [
                rect(tag_x, 194, tag_width, 30),
                text(tag, tag_x + tag_width // 2, 215, 11, weight=700, fill="#fff", anchor="middle", spacing=1.2,
                     utility=True),
            ]
        )

    cell = 50 if with_hourglass else 54
    digit_gap = 20 if with_hourglass else 22
    pieces.append(modular_number(state.minutes or "", 24, 252, cell, gap=digit_gap))
    pieces.append(
        text("MIN", 442, 500, 15, weight=700, anchor="middle", spacing=2.0, utility=True,
             transform="rotate(-90 442 500)")
    )
    if with_hourglass:
        pieces.append(hourglass(390, 320, 50, 88, state.progress))

    pieces.extend(
        [
            rule(24, 536, 456, 536, 5),
            text("SCHEDULED", 24, 570, 11, weight=700, spacing=1.4, utility=True),
            text(state.departure or "", 24, 622, 43, weight=700, spacing=-1.0),
            text("NEXT", 236, 570, 11, weight=700, spacing=1.4, utility=True),
        ]
    )

    if state.following:
        pieces.append(text(state.following[0], 236, 613, 20, weight=700))
        if len(state.following) > 1:
            pieces.append(text(state.following[1], 456, 613, 18, weight=700, anchor="end"))
    else:
        pieces.append(text("—", 236, 613, 22, weight=700))

    if state.slug == "last":
        metadata = "FINAL TRIP TODAY"
    elif state.slug == "before-first":
        metadata = "FIRST SCHEDULED TRIP"
    else:
        metadata = "WEEKDAY / EVERY 30 MIN"
    pieces.append(text(metadata, 24, 684, 11, weight=700, spacing=1.2, utility=True))
    pieces.append(pass2_footer(state.date))
    return "\n".join(pieces)


def pass3_countdown_lockup(state: State, *, treatment: str) -> str:
    if state.slug not in {"normal", "near"}:
        raise ValueError("Pass 3 lockups are defined only for NORMAL and NEAR")

    pieces = [pass2_header(), pass2_route(state)]
    pieces.append(text("UNTIL DEPARTURE", 24, 218, 11, weight=700, spacing=1.4, utility=True))

    if treatment == "baseline-large":
        # The unit shares the giant numeral's visual baseline. The larger glass
        # occupies the remaining right rail without changing the ledger below.
        pieces.extend(
            [
                modular_number(state.minutes or "", 24, 252, 50, gap=20),
                hourglass(382, 294, 62, 96, state.progress, stroke=6),
                text("MIN", 354, 500, 19, weight=700, spacing=1.8, utility=True),
                rule(24, 536, 456, 536, 5),
                text("SCHEDULED", 24, 570, 11, weight=700, spacing=1.4, utility=True),
                text(state.departure or "", 24, 622, 43, weight=700, spacing=-1.0),
                text("NEXT", 236, 570, 11, weight=700, spacing=1.4, utility=True),
                text(state.following[0], 236, 613, 20, weight=700),
                text(state.following[1], 456, 613, 18, weight=700, anchor="end"),
                text("WEEKDAY / EVERY 30 MIN", 24, 684, 11, weight=700, spacing=1.2, utility=True),
            ]
        )
    elif treatment == "stacked-medium":
        # The unit becomes its own line directly beneath the numeral. The
        # medium glass remains inside the numeral field and the ledger shifts
        # down slightly to preserve separation.
        pieces.extend(
            [
                modular_number(state.minutes or "", 24, 240, 50, gap=20),
                hourglass(388, 306, 56, 70, state.progress, stroke=5),
                text("MIN", 24, 525, 17, weight=700, spacing=2.6, utility=True),
                rule(24, 554, 456, 554, 5),
                text("SCHEDULED", 24, 588, 11, weight=700, spacing=1.4, utility=True),
                text(state.departure or "", 24, 640, 43, weight=700, spacing=-1.0),
                text("NEXT", 236, 588, 11, weight=700, spacing=1.4, utility=True),
                text(state.following[0], 236, 631, 20, weight=700),
                text(state.following[1], 456, 631, 18, weight=700, anchor="end"),
                text("WEEKDAY / EVERY 30 MIN", 24, 690, 11, weight=700, spacing=1.2, utility=True),
            ]
        )
    else:
        raise ValueError(f"Unknown Pass 3 treatment: {treatment}")

    pieces.append(pass2_footer(state.date))
    return "\n".join(pieces)


def final_open_poster(state: State) -> str:
    """Render the approved production direction across every required state."""
    pieces = [pass2_header(), pass2_route(state)]

    if state.slug == "service-ended":
        pieces.extend(
            [
                text("SERVICE", 24, 296, 60, weight=700, spacing=-1.5),
                text("ENDED", 24, 358, 60, weight=700, spacing=-1.5),
                rule(24, 392, 456, 392, 5),
                text("NEXT SCHEDULED SERVICE", 24, 434, 11, weight=700, spacing=1.4, utility=True),
                text(state.departure or "", 24, 520, 78, weight=700, spacing=-2.0),
                text("2026-09-17", 456, 487, 18, weight=700, anchor="end", spacing=0.8),
                text("FIRST TRIP", 456, 520, 11, weight=700, anchor="end", spacing=1.2, utility=True),
                text("NO MORE TRIPS TODAY", 24, 620, 11, weight=700, spacing=1.0, utility=True),
            ]
        )
        pieces.append(pass2_footer("2026-09-16"))
        return "\n".join(pieces)

    if state.slug == "clock-unavailable":
        pieces.extend(
            [
                rect(24, 246, 12, 214),
                text("CLOCK", 64, 312, 57, weight=700, spacing=-1.2),
                text("NOT SET", 64, 373, 57, weight=700, spacing=-1.2),
                rule(64, 408, 456, 408, 5),
                text("NO COUNTDOWN SHOWN", 64, 448, 11, weight=700, spacing=1.4, utility=True),
                text("Sync the RTC once in Settings.", 64, 520, 18, weight=700),
                text("On Point never substitutes a guessed time.", 64, 557, 14, utility=True),
            ]
        )
        pieces.append(pass2_footer("RTC REQUIRED", date_available=False))
        return "\n".join(pieces)

    pieces.append(text("UNTIL DEPARTURE", 24, 218, 11, weight=700, spacing=1.4, utility=True))
    if state.slug in {"last", "before-first"}:
        label = "LAST" if state.slug == "last" else "FIRST TRIP"
        tag_width = 78 if state.slug == "last" else 112
        tag_x = 456 - tag_width
        pieces.extend(
            [
                rect(tag_x, 194, tag_width, 30),
                text(
                    label,
                    tag_x + tag_width // 2,
                    215,
                    11,
                    weight=700,
                    fill="#fff",
                    anchor="middle",
                    spacing=1.2,
                    utility=True,
                ),
            ]
        )

    pieces.extend(
        [
            modular_number(state.minutes or "", 24, 252, 50, gap=20),
            hourglass(382, 294, 62, 96, state.progress, stroke=6),
            text("MIN", 354, 500, 19, weight=700, spacing=1.8, utility=True),
            rule(24, 536, 456, 536, 5),
            text("SCHEDULED", 24, 570, 11, weight=700, spacing=1.4, utility=True),
            text(state.departure or "", 24, 622, 43, weight=700, spacing=-1.0),
            text("NEXT", 236, 570, 11, weight=700, spacing=1.4, utility=True),
        ]
    )

    if state.following:
        pieces.append(text(state.following[0], 236, 613, 20, weight=700))
        if len(state.following) > 1:
            pieces.append(text(state.following[1], 456, 613, 18, weight=700, anchor="end"))
    else:
        pieces.append(text("—", 236, 613, 22, weight=700))

    if state.slug == "last":
        metadata = "FINAL TRIP TODAY"
    elif state.slug == "before-first":
        metadata = "FIRST SCHEDULED TRIP"
    else:
        metadata = "EVERY 30 MIN"
    pieces.append(text(metadata, 24, 684, 11, weight=700, spacing=1.2, utility=True))
    pieces.append(pass2_footer("2026-09-16"))
    return "\n".join(pieces)


def route_list_preview(selected_route: str = "BALAGTAS") -> str:
    """Render the production main-route hierarchy and evenly spaced route list."""
    routes = [
        ("CALAMBA", "BGC"),
        ("CAYPOMBO", "SM NORTH EDSA"),
        ("UP TOWN CENTER", "ONE AYALA"),
    ]
    pieces = [pass2_header(), text("MAIN ROUTE", 24, 90, 11, weight=700, spacing=1.4, utility=True)]

    main_selected = selected_route == "BALAGTAS"
    pieces.extend(
        [
            rect(24, 102, 10 if main_selected else 4, 150),
            text("BALAGTAS", 52, 139, 27, weight=700),
            rule(52, 164, 438, 164, 5),
            '<polygon points="438,155 460,164 438,173" fill="#000"/>',
            text("TRINOMA", 456, 222, 27, weight=700, anchor="end"),
            text("ALL ROUTES / A-Z", 24, 298, 11, weight=700, spacing=1.4, utility=True),
        ]
    )

    for row, (origin, destination) in enumerate(routes):
        y = 316 + row * 126
        selected = selected_route == origin
        pieces.extend(
            [
                rect(24, y, 8 if selected else 3, 104),
                text(origin, 48, y + 33, 19, weight=700),
                rule(48, y + 48, 440, y + 48, 4),
                f'<polygon points="440,{y+40} 460,{y+48} 440,{y+56}" fill="#000"/>',
                text(destination, 456, y + 91, 19, weight=700, anchor="end"),
            ]
        )

    pieces.extend(
        [
            rule(24, 726, 456, 726, 3),
            text(
                "MAIN ROUTE" if main_selected else "HOLD TO SET AS MAIN",
                24,
                758,
                11,
                weight=700,
                spacing=1.0,
                utility=True,
            ),
            text("PROVISIONAL", 456, 758, 10, weight=700, anchor="end", spacing=1.4, utility=True),
        ]
    )
    return "\n".join(pieces)


def write_svg(name: str, inner: str, width: int = WIDTH, height: int = HEIGHT) -> Path:
    path = SVG_DIR / f"{name}.svg"
    path.write_text(root(inner, width, height), encoding="utf-8")
    return path


def contact_sheet(items: list[tuple[str, str]], columns: int, scale: float, title: str) -> tuple[str, int, int]:
    card_width = int(WIDTH * scale)
    card_height = int(HEIGHT * scale)
    gap_x = 42
    gap_y = 66
    margin = 34
    title_height = 76
    rows = (len(items) + columns - 1) // columns
    sheet_width = margin * 2 + columns * card_width + (columns - 1) * gap_x
    sheet_height = title_height + margin + rows * card_height + (rows - 1) * gap_y + margin
    pieces = [text(title, margin, 48, 25, weight=700, spacing=0.3)]
    for index, (label, inner) in enumerate(items):
        col = index % columns
        row = index // columns
        x = margin + col * (card_width + gap_x)
        y = title_height + row * (card_height + gap_y)
        pieces.append(rect(x - 3, y - 3, card_width + 6, card_height + 6))
        pieces.append(rect(x, y, card_width, card_height, "#fff"))
        pieces.append(f'<g transform="translate({x} {y}) scale({scale})">{inner}</g>')
        pieces.append(text(label, x, y + card_height + 28, 14, weight=700, spacing=0.6, utility=True))
    return "\n".join(pieces), sheet_width, sheet_height


def main() -> None:
    SVG_DIR.mkdir(parents=True, exist_ok=True)
    normal = STATES["normal"]
    candidates = [
        ("A — SIGNAL + LEDGER", signal_ledger(normal)),
        ("B — TRANSIT SPINE", transit_spine(normal)),
        ("C — REVERSE FIELD", reverse_field(normal)),
        ("D — OPEN POSTER", open_poster(normal)),
    ]
    for label, inner in candidates:
        slug = label.split("—", 1)[1].strip().lower().replace(" ", "-").replace("+", "and")
        write_svg(f"candidate-{slug}", inner)

    state_items: list[tuple[str, str]] = []
    for slug, state in STATES.items():
        inner = signal_ledger(state)
        write_svg(f"state-{slug}", inner)
        state_items.append((slug.upper().replace("-", " "), inner))

    candidate_sheet, candidate_width, candidate_height = contact_sheet(candidates, columns=2, scale=0.46,
                                                                         title="ON POINT — FOUR LAYOUT DIRECTIONS")
    write_svg("contact-candidates", candidate_sheet, candidate_width, candidate_height)
    state_sheet, state_width, state_height = contact_sheet(state_items, columns=3, scale=0.36,
                                                            title="RECOMMENDED DIRECTION — REQUIRED STATES")
    write_svg("contact-required-states", state_sheet, state_width, state_height)

    pass2_items: list[tuple[str, str]] = []
    for slug, state in STATES.items():
        inner = refined_open_poster(state)
        write_svg(f"pass2-primary-{slug}", inner)
        pass2_items.append((slug.upper().replace("-", " "), inner))

    pass2_alternate = refined_open_poster(normal, with_hourglass=True)
    write_svg("pass2-alternate-hourglass-normal", pass2_alternate)
    pass2_comparison, comparison_width, comparison_height = contact_sheet(
        [("PRIMARY — NO HOURGLASS", refined_open_poster(normal)), ("ALTERNATE — SMALL HOURGLASS", pass2_alternate)],
        columns=2,
        scale=0.46,
        title="PASS 2 — OPEN POSTER REFINEMENT",
    )
    write_svg("pass2-contact-primary-alternate", pass2_comparison, comparison_width, comparison_height)
    pass2_sheet, pass2_width, pass2_height = contact_sheet(
        pass2_items, columns=3, scale=0.36, title="PASS 2 — REFINED OPEN POSTER STATES"
    )
    write_svg("pass2-contact-required-states", pass2_sheet, pass2_width, pass2_height)

    pass3_items: list[tuple[str, str]] = []
    for treatment, label in (
        ("baseline-large", "A — LARGE"),
        ("stacked-medium", "B — MEDIUM"),
    ):
        for slug in ("normal", "near"):
            inner = pass3_countdown_lockup(STATES[slug], treatment=treatment)
            write_svg(f"pass3-{treatment}-{slug}", inner)
            pass3_items.append((f"{label} / {slug.upper()}", inner))
    pass3_sheet, pass3_width, pass3_height = contact_sheet(
        pass3_items, columns=2, scale=0.46, title="PASS 3 — COUNTDOWN LOCKUPS"
    )
    write_svg("pass3-contact-countdown-lockups", pass3_sheet, pass3_width, pass3_height)

    final_items: list[tuple[str, str]] = []
    for slug, state in STATES.items():
        inner = final_open_poster(state)
        write_svg(f"final-{slug}", inner)
        final_items.append((slug.upper().replace("-", " "), inner))
    final_sheet, final_width, final_height = contact_sheet(
        final_items, columns=3, scale=0.36, title="FINAL DIRECTION — PRODUCTION STATES"
    )
    write_svg("final-contact-required-states", final_sheet, final_width, final_height)
    write_svg("route-list-main-selected", route_list_preview())
    write_svg("route-list-secondary-selected", route_list_preview("CAYPOMBO"))
    print(f"Generated {len(list(SVG_DIR.glob('*.svg')))} SVG previews in {SVG_DIR}")


if __name__ == "__main__":
    main()
