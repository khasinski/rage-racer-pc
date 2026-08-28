#!/usr/bin/env python3
"""Reject the white/masonry terrain regression in the Grand Prix intro."""

from __future__ import annotations

import os
import subprocess
import sys
import tempfile
from pathlib import Path


def main() -> int:
    executable = Path(sys.argv[1])
    source_dir = Path(sys.argv[2])
    with tempfile.TemporaryDirectory(prefix="rage-grand-prix-test-") as directory:
        capture = Path(directory) / "grand-prix.ppm"
        environment = os.environ.copy()
        environment.update(
            SDL_AUDIODRIVER="dummy",
            RAGE_PORT_SMOKE_FRAMES="1500",
            RAGE_PORT_SMOKE_STOP_SCENE="12",
            RAGE_PORT_SMOKE_STOP_SCENE_TIMER="56",
            RAGE_PORT_INPUT_SCRIPT=(
                "400:START,500:START,650:CROSS,950:CROSS,"
                "1100:CROSS,1200:CROSS"
            ),
            RAGE_PORT_CAPTURE_PATH=str(capture),
            RAGE_PORT_SMOKE_CAMERA_STATE="1",
            RAGE_PORT_TERRAIN_TRACE_TIMER="56",
        )
        result = subprocess.run(
            [executable], cwd=source_dir, env=environment,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
            timeout=35,
        )
        if result.returncode != 0:
            print(result.stdout, file=sys.stderr)
            return result.returncode or 1
        if "stopped at frame 1420, scene 12" not in result.stdout:
            raise AssertionError("input script did not reach the Grand Prix intro")
        if "smoke synchronized stop frame=1420 scene=12 timer=56" not in result.stdout:
            raise AssertionError("scene/timer synchronized capture did not trigger")
        if "primitive buffer exhausted" in result.stdout:
            raise AssertionError("Grand Prix geometry exhausted the primitive buffer")
        if "unsupported command" in result.stdout:
            offending = next(
                line for line in result.stdout.splitlines()
                if "unsupported command" in line
            )
            raise AssertionError(
                f"Grand Prix submitted a malformed GP0 packet: {offending}"
            )
        if "ref_lap=100765" not in result.stdout:
            raise AssertionError(
                "Grand Prix intro lost the retail Calme GP record: "
                + result.stdout[-1000:]
            )
        if "time_text=1'40\"765" not in result.stdout:
            raise AssertionError("Grand Prix record text was not fully formatted")
        if not any(
            "terrain-lod timer=56" in line and
            "mirror=0" in line and "shift=10" in line
            for line in result.stdout.splitlines()
        ):
            raise AssertionError("main terrain lost the retail LOD shift 10")

        header, pixels = capture.read_bytes().split(b"\n255\n", 1)
        if header.splitlines() != [b"P6", b"320 240"]:
            raise AssertionError(f"unexpected capture header: {header!r}")
        colors = list(zip(pixels[0::3], pixels[1::3], pixels[2::3]))
        lower = colors[320 * 120:]
        near_white = sum(min(color) > 220 for color in lower)
        road_texture = sum(
            20 < sum(color) // 3 < 180 and max(color) - min(color) < 20
            for color in lower
        )
        bright_text = sum(min(color) > 180 for color in colors)
        mean_lower = sum(sum(color) for color in lower) / (3 * len(lower))
        car_region = [
            colors[y * 320 + x]
            for y in range(80, 205)
            for x in range(35, 240)
        ]
        dark_car_pixels = sum(max(color) < 70 for color in car_region)
        if (near_white > 2_000 or road_texture < 20_000 or
                bright_text < 500 or not 20 < mean_lower < 100 or
                dark_car_pixels < 12_000):
            raise AssertionError(
                "Grand Prix intro lost its car, road texture, or HUD: "
                f"near_white={near_white}, road_texture={road_texture}, "
                f"bright_text={bright_text}, mean_lower={mean_lower:.1f}, "
                f"dark_car_pixels={dark_car_pixels}"
            )
    print("Grand Prix intro retained its textured car, road, HUD, and record")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
