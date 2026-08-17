#!/usr/bin/env python3
"""Lakeside Gate's waterfall survives near-course subdivision in classic."""

import os
import subprocess
import sys
import tempfile
from pathlib import Path


def main() -> int:
    executable, source = map(Path, sys.argv[1:3])
    with tempfile.TemporaryDirectory(prefix="rage-waterfall-") as directory:
        capture = Path(directory) / "waterfall.ppm"
        environment = os.environ.copy()
        environment["SDL_AUDIODRIVER"] = "dummy"
        result = subprocess.run(
            [
                executable,
                "--scenario", source / "race-scenario.ini",
                "--set", "race.course=2",
                "--set", "run.frames=3000",
                "--set", "stop.scene=12",
                "--set", "stop.timer=430",
                "--set", "start.player_track_point=225",
                "--set", "start.freeze=true",
                "--set", f"capture.path={capture}",
                "--set", "video.renderer=classic",
            ],
            cwd=source,
            env=environment,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            timeout=55,
        )
        if result.returncode != 0:
            print(result.stdout, file=sys.stderr)
            return result.returncode or 1
        if "scene=12 timer=430" not in result.stdout:
            raise AssertionError("waterfall scenario did not reach its capture point")

        header, pixels = capture.read_bytes().split(b"\n255\n", 1)
        if header.splitlines() != [b"P6", b"320 240"]:
            raise AssertionError(f"unexpected waterfall capture: {header!r}")

        # Four bright waterfall sheets occupy this HUD-free rock-face region.
        # The broken child-quad winding left fewer than 100 matching pixels.
        bright_water = 0
        for y in range(60, 135):
            for x in range(35, 210):
                offset = (y * 320 + x) * 3
                red, green, blue = pixels[offset:offset + 3]
                bright_water += red > 150 and green > 150 and blue > 120
        if bright_water < 3_000:
            raise AssertionError(
                f"classic waterfall disappeared after subdivision: {bright_water} pixels"
            )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
