#!/usr/bin/env python3
"""Exercise the complete Grand Prix intro, countdown, and replay recording."""

from __future__ import annotations

import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path


def main() -> int:
    executable = Path(sys.argv[1])
    source_dir = Path(sys.argv[2])
    with tempfile.TemporaryDirectory(prefix="rage-race-start-") as directory:
        capture = Path(directory) / "race.ppm"
        environment = os.environ.copy()
        environment.update(
            SDL_AUDIODRIVER="dummy",
            RAGE_PORT_SMOKE_FRAMES="2000",
            RAGE_PORT_INPUT_SCRIPT=(
                "400:START,500:START,650:CROSS,950:CROSS,"
                "1100:CROSS,1200:CROSS,1700-2000:CROSS"
            ),
            RAGE_PORT_CAPTURE_PATH=str(capture),
        )
        result = subprocess.run(
            [executable], cwd=source_dir, env=environment,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
            timeout=45,
        )
        if result.returncode != 0:
            print(result.stdout, file=sys.stderr)
            return result.returncode or 1
        header, payload = capture.read_bytes().split(b"\n255\n", 1)
        if header.splitlines() != [b"P6", b"320 240"]:
            raise AssertionError(f"unexpected race capture header: {header!r}")
        pixels = list(zip(payload[0::3], payload[1::3], payload[2::3]))

        def speed_glyph_mask(index: int) -> tuple[bool, ...]:
            return tuple(
                pixels[y * 320 + x][0] > 120
                and pixels[y * 320 + x][1] > 80
                and pixels[y * 320 + x][2] < 120
                for y in range(148, 156)
                for x in range(264 + index * 8, 272 + index * 8)
            )

        # At this deterministic state the displayed speed is 063.  The old
        # native layout made g_PlayerSpeed a separate zeroed object rather
        # than the retail alias of g_PlayerCar.speed, yielding 000; its last
        # two glyph masks were therefore identical.
        if speed_glyph_mask(1) == speed_glyph_mask(2):
            raise AssertionError("speedometer still renders zeroed speed digits")

        needle_pixels = sum(
            r > 100 and 25 < g < 150 and b < 70
            for y in range(170, 225)
            for r, g, b in pixels[y * 320 + 225:y * 320 + 310]
        )
        if needle_pixels < 8:
            raise AssertionError("tachometer needle is missing")
    if "stopped at frame 2000, scene 12" not in result.stdout:
        raise AssertionError("Grand Prix did not survive through race start")
    player_state = re.search(r"speed=(-?\d+) accelerator=(-?\d+)", result.stdout)
    if player_state is None or int(player_state.group(1)) <= 0:
        raise AssertionError("Grand Prix did not accept acceleration after countdown")
    if int(player_state.group(2)) != 256:
        raise AssertionError("digital accelerator was not sampled at full pressure")
    for failure in (
        "primitive buffer exhausted", "misaligned OT link",
        "likely corrupted",
    ):
        if failure in result.stdout:
            raise AssertionError(f"Grand Prix reported {failure!r}")
    print("Grand Prix survived intro/countdown and entered a moving race")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
