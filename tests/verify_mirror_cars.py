#!/usr/bin/env python3
"""Modern mirror preserves complete rival models and their lower bodywork."""

import os
import subprocess
import sys
import tempfile
from pathlib import Path


def run(executable: Path, source: Path, output: Path, renderer: str) -> Path:
    output.mkdir(parents=True)
    environment = os.environ.copy()
    environment["SDL_AUDIODRIVER"] = "dummy"
    command = [
        executable, "--scenario", source / "race-scenario.ini",
        "--set", "run.frames=3000", "--set", "stop.scene=12",
        "--set", "stop.timer=431",
        "--set", "race.grid=0,1,2,3,4,5,6,7,8,9,10",
        "--set", "start.player_track_point=50",
        "--set", "start.rival_track_points=52,54,56",
        "--set", "start.freeze=true",
        "--set", f"capture.directory={output}",
        "--set", "capture.scene=12", "--set", "capture.timer_min=430",
        "--set", "capture.timer_max=430", "--set", "capture.timer_stride=1",
        "--set", f"video.renderer={renderer}",
    ]
    result = subprocess.run(command, cwd=source, env=environment,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            timeout=55)
    if result.returncode != 0:
        raise AssertionError(result.stdout.decode(errors="replace"))
    captures = list(output.glob("timer-00430-s12.ppm"))
    if len(captures) != 1:
        raise AssertionError(f"{renderer} produced {len(captures)} captures")
    return captures[0]


def ppm_pixels(path: Path):
    data = path.read_bytes()
    header, dimensions, maximum, pixels = data.split(b"\n", 3)
    if header != b"P6" or maximum != b"255":
        raise AssertionError(f"unexpected PPM header in {path}")
    width, height = dimensions.split()
    return int(width), int(height), pixels


def main() -> int:
    executable, source = map(Path, sys.argv[1:3])
    with tempfile.TemporaryDirectory(prefix="rage-mirror-cars-") as root_text:
        root = Path(root_text)
        classic = ppm_pixels(run(executable, source, root / "classic", "classic"))
        modern = ppm_pixels(run(executable, source, root / "modern", "modern"))
    if classic != modern:
        width, height, old = classic
        _, _, new = modern
        changed = sum(a != b for a, b in zip(old, new))
        raise AssertionError(
            f"mirror rival regression: {changed} differing channels at "
            f"{width}x{height}")
    width, height, pixels = classic
    # The controlled placement puts red/orange rival bodywork in the right
    # half of the mirror. This guards against two equally empty renderers.
    vivid_car_pixels = 0
    for y in range(18, min(54, height)):
        for x in range(190, min(235, width)):
            offset = (y * width + x) * 3
            red, green, blue = pixels[offset:offset + 3]
            if red > 80 and red > green * 3 // 2 and green > blue:
                vivid_car_pixels += 1
    if vivid_car_pixels < 8:
        raise AssertionError("controlled mirror frame contains no rival bodywork")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
