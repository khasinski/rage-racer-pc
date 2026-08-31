#!/usr/bin/env python3
"""Every HUD time drawn at an edge must be anchored to it.

The widescreen layout pushes the HUD out to the edges of the picture, and a
coordinate written for a 4:3 screen stays where it was while the labels around
it move. That is easy to miss because the drawing exists more than once: the
split times are drawn by DrawSplitTimes during a race and again by
UpdateSplitTimes while a ghost replays, which is the path time attack takes.
The second copy kept its raw coordinates long after the first was anchored,
and the times on the left and the total on the right sat alone in the middle
of the picture.

So this reads the sources rather than a picture: a time drawn near either edge
has to go through HudLeftX, HudRightX or HudAnchorX. Coordinates in the middle
of the canvas are left alone, because that is where the game means them.
"""

from pathlib import Path
import re


CANVAS = 320
EDGE_BAND = 80

root = Path(__file__).resolve().parents[2]
sources = sorted((root / "src/main/PAL/main").rglob("*.c"))

CALL = re.compile(
    r"\b(DrawTimeValue|DrawMinuteSecondTime|DrawText8x8)\s*\(\s*"
    r"(0x[0-9A-Fa-f]+|\d+)\s*,")

failures = []
checked = 0

for source in sources:
    text = source.read_text()
    for match in CALL.finditer(text):
        x = int(match.group(2), 0)
        if EDGE_BAND <= x < CANVAS - EDGE_BAND:
            continue          # the middle of the picture does not move
        checked += 1
        line = text.count("\n", 0, match.start()) + 1
        failures.append(
            f"{source.relative_to(root)}:{line}: {match.group(1)} draws at "
            f"x={x}, in the band the widescreen layout moves, without "
            f"HudLeftX/HudRightX/HudAnchorX")

if failures:
    for failure in failures:
        print("FAIL", failure)
    raise SystemExit(1)

print("every HUD time near an edge is anchored to it")
