#!/usr/bin/env python3
"""Capture stable reference frames for baseline and enhanced modern modes."""

import hashlib
import os
import subprocess
import sys
import tempfile
from pathlib import Path


CASES = {
    "baseline": ("auto", "nearest", "none", "off", (320, 240)),
    "enhanced": ("16:9", "linear", "fxaa", "vibrant", (426, 240)),
}
# Refreshed after the renderer and race-state cleanup. Both frames were
# inspected before recording them: the race start with its light gantry,
# banners and HUD, and the same scene in 16:9 with FXAA and vibrant grading.
MODERN_GOLDENS = {
    "baseline": "8fac628422f47ac1e6e15b89c06cf9733e822dedc42ea59ecb80ee1c99208cfe",
    "enhanced": "4c9a5cd89b579421a4546c5dfe754076ea512faaf39a8479918f9bce67a382dd",
}


def read_ppm(path: Path) -> tuple[tuple[int, int], bytes]:
    data = path.read_bytes()
    header, pixels = data.split(b"\n255\n", 1)
    lines = header.splitlines()
    if len(lines) != 2 or lines[0] != b"P6":
        raise AssertionError(f"invalid modern PPM header: {header!r}")
    width, height = map(int, lines[1].split())
    if len(pixels) != width * height * 3:
        raise AssertionError("truncated modern PPM")
    return (width, height), pixels


def main() -> int:
    executable, source = map(Path, sys.argv[1:3])
    digests = {}
    mismatches = {}
    with tempfile.TemporaryDirectory(prefix="rage modern ąę ") as directory:
        root = Path(directory)
        for name, (aspect, filtering, post, grading, dimensions) in CASES.items():
            capture = root / f"{name} żółty.ppm"
            scenario = root / f"{name} ustawienia.ini"
            scenario.write_text(
                f"""[modern]
# Lock one asset source. Otherwise these digests depend on whether a
# native-assets directory happens to sit beside the executable, and the
# importer and a prebuilt cache do not produce the same pixels.
assets = disc

[video]
renderer = modern
internal_scale = 1
aspect = {aspect}
texture_filter = {filtering}
post = {post}
grading = {grading}

[diagnostics]
modern_dump = {capture}
modern_dump_scene_id = 12
modern_dump_timer = 120

[race]
enabled = true
mode = grand-prix
series = gp
class = 0
course = 0
car = 3

[run]
frames = 2500

[stop]
scene = 12
timer = 130
""",
                encoding="utf-8",
            )
            environment = os.environ.copy()
            environment["SDL_AUDIODRIVER"] = "dummy"
            result = subprocess.run(
                [executable, "--scenario", scenario], cwd=source,
                env=environment, stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT, text=True, timeout=165,
            )
            if result.returncode != 0 or not capture.exists():
                print(result.stdout, file=sys.stderr)
                raise AssertionError(f"modern {name} capture failed")
            actual_dimensions, pixels = read_ppm(capture)
            if actual_dimensions != dimensions:
                raise AssertionError(
                    f"modern {name} dimensions {actual_dimensions} != {dimensions}"
                )
            if sum(channel != 0 for channel in pixels) < 100:
                raise AssertionError(f"modern {name} frame is effectively empty")
            digests[name] = hashlib.sha256(capture.read_bytes()).hexdigest()
            if digests[name] != MODERN_GOLDENS[name]:
                mismatches[name] = digests[name]
    if digests["baseline"] == digests["enhanced"]:
        raise AssertionError("modern enhancement pipeline did not affect output")
    if mismatches:
        raise AssertionError(f"modern references changed: {mismatches}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
