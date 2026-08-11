#!/usr/bin/env python3
"""Verify the original 320x192 FMV is centered in a 4:3 320x240 output."""

from __future__ import annotations

import os
import subprocess
import sys
import tempfile
from pathlib import Path


def main() -> int:
    executable = Path(sys.argv[1])
    source_dir = Path(sys.argv[2])
    with tempfile.TemporaryDirectory(prefix="rage-fmv-test-") as directory:
        capture = Path(directory) / "fmv.ppm"
        environment = os.environ.copy()
        environment.update(
            SDL_AUDIODRIVER="dummy",
            RAGE_PORT_SMOKE_FRAMES="360",
            RAGE_PORT_CAPTURE_PATH=str(capture),
        )
        result = subprocess.run(
            [executable], cwd=source_dir, env=environment,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
            timeout=20,
        )
        if result.returncode != 0:
            print(result.stdout, file=sys.stderr)
            return result.returncode or 1

        data = capture.read_bytes()
        header, pixels = data.split(b"\n255\n", 1)
        if header.splitlines() != [b"P6", b"320 240"]:
            raise AssertionError(f"unexpected capture header: {header!r}")
        stride = 320 * 3
        top = pixels[:24 * stride]
        picture = pixels[24 * stride:216 * stride]
        bottom = pixels[216 * stride:]
        if any(top) or any(bottom):
            raise AssertionError("320x192 FMV is not vertically centered")
        if sum(value != 0 for value in picture) < 10_000:
            raise AssertionError("FMV picture region is empty")
    print("FMV is centered at y=24 in a 4:3 320x240 output")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
