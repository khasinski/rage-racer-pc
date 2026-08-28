#!/usr/bin/env python3
"""Characterize a retail prologue frame rendered by the native geometry path."""

from __future__ import annotations

import os
import subprocess
import sys
import tempfile
from pathlib import Path


def main() -> int:
    executable = Path(sys.argv[1])
    source_dir = Path(sys.argv[2])
    style = sys.argv[3] if len(sys.argv) > 3 else "international"
    with tempfile.TemporaryDirectory(prefix="rage-prologue-test-") as directory:
        capture = Path(directory) / "prologue.ppm"
        environment = os.environ.copy()
        environment.update(
            SDL_AUDIODRIVER="dummy",
            RAGE_PORT_SMOKE_FRAMES="1250",
            # Exercise the same raw BIOS packet -> edge path used by the
            # interactive SDL keyboard, not the direct smoke edge override.
            RAGE_PORT_RAW_INPUT_SCRIPT="400:START,500:START,650:CROSS",
            RAGE_PORT_CAPTURE_PATH=str(capture),
        )
        arguments = [executable]
        if style == "japanese":
            arguments.extend(("--set", "content.prologue=japanese"))
        result = subprocess.run(
            arguments, cwd=source_dir, env=environment,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
            timeout=25,
        )
        if result.returncode != 0:
            print(result.stdout, file=sys.stderr)
            return result.returncode or 1
        if "primitive buffer exhausted" in result.stdout:
            raise AssertionError("prologue geometry exhausted the primitive buffer")
        if style == "japanese" and \
                "using the extended Japanese-release prologue text" not in result.stdout:
            raise AssertionError("Japanese prologue option was not applied")

        header, pixels = capture.read_bytes().split(b"\n255\n", 1)
        if header.splitlines() != [b"P6", b"320 240"]:
            raise AssertionError(f"unexpected capture header: {header!r}")
        colors = list(zip(pixels[0::3], pixels[1::3], pixels[2::3]))
        non_black = sum(any(color) for color in colors)
        bright_text = sum(min(color) > 180 for color in colors)
        colored_geometry = sum(
            max(color) > 40 and max(color) - min(color) > 20
            for color in colors
        )
        lower_geometry = sum(any(color) for color in colors[320 * 120:])
        if (non_black < 15_000 or bright_text < 1_000 or
                colored_geometry < 6_000 or lower_geometry < 8_000):
            raise AssertionError(
                "prologue lost text, cars, or track geometry: "
                f"non_black={non_black}, bright_text={bright_text}, "
                f"colored_geometry={colored_geometry}, "
                f"lower_geometry={lower_geometry}"
            )
    print(f"{style} prologue text and world geometry rendered through the native path")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
