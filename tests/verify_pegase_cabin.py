#!/usr/bin/env python3
"""The Age Pegase cabin must not inherit the painted body texture."""

from __future__ import annotations

import os
import subprocess
import sys
import tempfile
from pathlib import Path


def main() -> int:
    executable, source = map(Path, sys.argv[1:3])
    with tempfile.TemporaryDirectory(prefix="rage-pegase-cabin-") as directory:
        capture = Path(directory) / "pegase.ppm"
        environment = os.environ.copy()
        environment["SDL_AUDIODRIVER"] = "dummy"
        result = subprocess.run([
            executable, "--scenario", source / "race-scenario.ini",
            "--set", "run.frames=3000",
            "--set", "stop.scene=12", "--set", "stop.timer=431",
            "--set", "race.car=2",
            "--set", "race.grid=2,1,0,3,4,5,6,7,8,9,10",
            "--set", "start.player_track_point=0",
            "--set", "start.rival_track_points=2,4,6",
            "--set", "start.freeze=true", "--set", "start.camera=1",
            "--set", "video.renderer=modern",
            "--set", "video.internal_scale=1",
            "--set", "video.aspect=16:9",
            "--set", f"diagnostics.modern_dump={capture}",
            "--set", "diagnostics.modern_dump_frame=620",
        ], cwd=source, env=environment, stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT, text=True, timeout=165)
        if result.returncode != 0 or not capture.exists():
            raise AssertionError("Age Pegase capture failed\n" + result.stdout)
        if "course=0 car=2" not in result.stdout:
            raise AssertionError("controlled race did not select the Age Pegase")

        header, pixels = capture.read_bytes().split(b"\n255\n", 1)
        if header.splitlines() != [b"P6", b"426 240"]:
            raise AssertionError(f"unexpected Pegase capture header: {header!r}")

        def dark_pixels(x0: int, x1: int) -> int:
            dark = 0
            for y in range(181, 205):
                for x in range(x0, x1):
                    offset = (y * 426 + x) * 3
                    if max(pixels[offset:offset + 3]) < 80:
                        dark += 1
            return dark

        left = dark_pixels(191, 211)
        right = dark_pixels(212, 232)
        if left < 350 or right < 350 or abs(left - right) > 100:
            raise AssertionError(
                "Age Pegase cabin picked up asymmetric body paint: "
                f"dark_left={left}, dark_right={right}"
            )
    print("Age Pegase cabin stays dark and symmetric in the modern renderer")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
