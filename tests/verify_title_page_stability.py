#!/usr/bin/env python3
"""Ensure the static title artwork is identical on both framebuffer pages."""

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
        raise AssertionError(f"unexpected capture format: {magic!r}")
    width, height = (int(value) for value in dimensions.split())
    if len(pixels) != width * height * 3:
        raise AssertionError(f"truncated capture: {path}")
    return width, height, pixels


def main() -> int:
    executable = Path(sys.argv[1])
    source_dir = Path(sys.argv[2])
    with tempfile.TemporaryDirectory(prefix="rage-title-pages-") as directory:
        capture_dir = Path(directory)
        environment = os.environ.copy()
        environment.update(
            SDL_AUDIODRIVER="dummy",
            RAGE_PORT_SMOKE_FRAMES="499",
            RAGE_PORT_SMOKE_CAPTURE_DIR=str(capture_dir),
            RAGE_PORT_SMOKE_CAPTURE_TIMER_STRIDE="1",
            RAGE_PORT_SMOKE_CAPTURE_SCENE="4",
            RAGE_PORT_SMOKE_CAPTURE_TIMER_MIN="180",
            RAGE_PORT_SMOKE_CAPTURE_TIMER_MAX="181",
            RAGE_PORT_SMOKE_CAPTURE_ALL_PHASES="1",
        )
        result = subprocess.run(
            [executable], cwd=source_dir, env=environment,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
            timeout=20,
        )
        if result.returncode != 0:
            print(result.stdout, file=sys.stderr)
            return result.returncode or 1

        captures = sorted(capture_dir.glob("timer-*.ppm"))
        if len(captures) != 2:
            raise AssertionError(
                f"expected two title captures, found {len(captures)}\n{result.stdout}"
            )
        width0, height0, first = read_ppm(captures[0])
        width1, height1, second = read_ppm(captures[1])
        if (width0, height0) != (320, 240) or (width1, height1) != (320, 240):
            raise AssertionError("title captures are not 320x240")

        # The PRESS START prompt below this region intentionally animates.
        stable_bytes = 320 * 190 * 3
        differences = sum(
            left != right
            for left, right in zip(first[:stable_bytes], second[:stable_bytes])
        )
        if differences:
            raise AssertionError(
                f"title artwork differs across framebuffer pages in {differences} channels"
            )

        # The 240x24 RAGE RACER image is a PS1 SPRT, so its UV must advance
        # exactly one texel per output pixel. Host triangle interpolation used
        # to move the lower edge of the first GE one pixel to the right.
        def pixel(data: bytes, x: int, y: int) -> tuple[int, int, int]:
            offset = (y * 320 + x) * 3
            return tuple(data[offset:offset + 3])

        if max(pixel(first, 135, 175)) < 80 or max(pixel(first, 138, 175)) > 20:
            raise AssertionError("RAGE RACER logo sprite has a one-pixel UV shift")

    print("Title artwork is pixel-stable across both framebuffer pages")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
