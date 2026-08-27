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
MODERN_GOLDENS = {
    "baseline": "fdde24231fe598afbee3263be94174ce4b61765fef3ecba1f34f45c400695931",
    "enhanced": "9887a2375ddb561d1bfd5103161d2982973f1ccaf2ff2183b9f88711c277f348",
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
                f"""[video]
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
                stderr=subprocess.STDOUT, text=True, timeout=55,
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
