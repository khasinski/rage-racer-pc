#!/usr/bin/env python3
"""Lock what the sky draws during a race.

DrawSkyBackground is the largest function in the port and nothing watched it.
The pixel-comparison tests in this suite check the comparison tools rather than
the renderer, and the visual reference frames are captured from menus and the
intro camera, where the sky's horizon band and its skirt are not drawn at all:
deleting the whole skirt leaves every one of them passing.

So this captures four frames of an actual race and folds them into one number.
Deleting the skirt moves it, which is the only claim worth making about a
lock: that it would notice.
"""

from __future__ import annotations

import os
import subprocess
import sys
import tempfile
from pathlib import Path

# What the sky draws today. Run the test to see the number when it moves, and
# change it here only when the change to the sky was meant.
EXPECTED = 0xF72F3B6D


def main() -> int:
    executable, source = map(Path, sys.argv[1:3])
    with tempfile.TemporaryDirectory(prefix="rage-sky-") as directory:
        environment = os.environ.copy()
        environment.update(
            SDL_AUDIODRIVER="dummy",
            RAGE_PORT_DISABLE_HOST_INPUT="1",
            RAGE_PORT_SYNC_RANDOM="1",
            RAGE_PORT_SMOKE_CAPTURE_DIR=directory,
            RAGE_PORT_SMOKE_CAPTURE_SCENE="12",
            RAGE_PORT_SMOKE_CAPTURE_TIMER_MIN="100",
            RAGE_PORT_SMOKE_CAPTURE_TIMER_MAX="400",
            RAGE_PORT_SMOKE_CAPTURE_TIMER_STRIDE="100",
            RAGE_PORT_INPUT_SCRIPT="200-2000:CROSS",
        )
        result = subprocess.run(
            [
                executable, "--scenario", source / "race-scenario.ini",
                "--set", "race.class=4",
                "--set", "race.course=0",
                "--set", "run.frames=1500",
                "--set", "video.renderer=classic",
            ], cwd=source, env=environment,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
            timeout=600,
        )
        if result.returncode != 0:
            print(result.stdout, file=sys.stderr)
            return result.returncode or 1

        frames = sorted(Path(directory).glob("timer-*.ppm"))
        if len(frames) < 4:
            raise AssertionError(
                f"race captured {len(frames)} frames, expected 4\n{result.stdout}"
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
