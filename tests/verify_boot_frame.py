#!/usr/bin/env python3
"""Characterize the retail copyright frame rendered through the native HAL."""

from __future__ import annotations

import os
import subprocess
import sys
import tempfile
from pathlib import Path


def read_ppm(path: Path) -> tuple[int, int, bytes]:
    data = path.read_bytes()
    header, pixels = data.split(b"\n255\n", 1)
    magic, dimensions = header.splitlines()
    if magic != b"P6":
        raise ValueError("capture is not a binary PPM")
    width, height = map(int, dimensions.split())
    if len(pixels) != width * height * 3:
        raise ValueError("capture has a truncated pixel payload")
    return width, height, pixels


def main() -> int:
    executable = Path(sys.argv[1])
    source_dir = Path(sys.argv[2])
    with tempfile.TemporaryDirectory(prefix="rage-boot-test-") as directory:
        capture = Path(directory) / "boot.ppm"
        environment = os.environ.copy()
        environment.update(
            SDL_AUDIODRIVER="dummy",
            RAGE_PORT_SMOKE_FRAMES="20",
            RAGE_PORT_CAPTURE_PATH=str(capture),
        )
        result = subprocess.run(
            [executable],
            cwd=source_dir,
            env=environment,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            timeout=45,
        )
        if result.returncode != 0:
            print(result.stdout, file=sys.stderr)
            return result.returncode or 1

        width, height, pixels = read_ppm(capture)
        colors = list(zip(pixels[0::3], pixels[1::3], pixels[2::3]))
        non_black = sum(color != (0, 0, 0) for color in colors)
        cyan = sum(g > 70 and b > 60 and r < 20 for r, g, b in colors)
        white = sum(r > 220 and g > 220 and b > 220 for r, g, b in colors)
        if (width, height) != (320, 240):
            raise AssertionError(f"expected 320x240, got {width}x{height}")
        if non_black < 8_000 or cyan < 3_000 or white < 500:
            raise AssertionError(
                "copyright frame missing: "
                f"non_black={non_black}, cyan={cyan}, white={white}"
            )
    print("boot copyright frame rendered through the native HAL")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
