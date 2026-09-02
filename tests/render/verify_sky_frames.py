#!/usr/bin/env python3
"""Lock what the sky draws during a race.

DrawSkyBackground is the largest function in the port and nothing watched it.
The pixel-comparison tests in this suite check the comparison tools rather than
the renderer, and the visual reference frames are captured from menus and the
intro camera, where the sky's horizon band and its skirt are not drawn at all:
deleting the whole skirt leaves every one of them passing.

So this captures frames of an actual race and folds them into one number.
Deleting the skirt moves it, which is the only claim worth making about a
lock: that it would notice.

Four frames were not enough. The tiled half of the sky only draws once the
course sets a row base, and a change to the step it advances by left the first
four frames identical; a wider window over the same race notices it.
"""

from __future__ import annotations

import os
import subprocess
import sys
import tempfile
from pathlib import Path

# This hashes complete race frames because the terrain is needed to prove that
# the skirt closes the horizon. A deliberate physics or camera correction can
# therefore move it too; inspect the changed frames before accepting a value.
# The current baseline includes the restored one-based right-edge knockback
# mode (876d88f3), synchronizes random state when the race scene starts, and
# uses the bounded 32-row visible-cell mask (59701638).
EXPECTED = 0x7EACFEEE


def main() -> int:
    executable, source = map(Path, sys.argv[1:3])
    with tempfile.TemporaryDirectory(prefix="rage-sky-") as directory:
        environment = os.environ.copy()
        environment.update(
            SDL_AUDIODRIVER="dummy",
            RAGE_PORT_DISABLE_HOST_INPUT="1",
            RAGE_PORT_SYNC_RANDOM="12@0=1",
            RAGE_PORT_SMOKE_CAPTURE_DIR=directory,
            RAGE_PORT_SMOKE_CAPTURE_SCENE="12",
            RAGE_PORT_SMOKE_CAPTURE_TIMER_MIN="100",
            RAGE_PORT_SMOKE_CAPTURE_TIMER_MAX="1200",
            RAGE_PORT_SMOKE_CAPTURE_TIMER_STRIDE="50",
            RAGE_PORT_INPUT_SCRIPT=(
                "200-900:CROSS,950-1150:CROSS+LEFT,1200-1400:CROSS+RIGHT,"
                "1450-1550:SQUARE,1600-2200:CROSS+LEFT,2250-4000:CROSS"
            ),
        )
        result = subprocess.run(
            [
                executable, "--scenario", source / "race-scenario.ini",
                "--set", "race.class=4",
                "--set", "race.course=0",
                "--set", "run.frames=2600",
                "--set", "video.renderer=classic",
            ], cwd=source, env=environment,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
            timeout=600,
        )
        if result.returncode != 0:
            print(result.stdout, file=sys.stderr)
            return result.returncode or 1

        frames = sorted(Path(directory).glob("timer-*.ppm"))
        if len(frames) < 23:
            raise AssertionError(
                f"race captured {len(frames)} frames, expected 23\n{result.stdout}"
            )

        digest = 2166136261
        for frame in frames:
            for byte in frame.read_bytes():
                digest = ((digest ^ byte) * 16777619) & 0xFFFFFFFF

        if digest != EXPECTED:
            raise AssertionError(
                f"the sky draws differently: {len(frames)} race frames digest "
                f"to {digest:#010x}, expected {EXPECTED:#010x}"
            )
    print(f"the sky draws the same {len(frames)} race frames it always did")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
