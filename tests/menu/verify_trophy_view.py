#!/usr/bin/env python3
"""The Trophy View selector must use OPTION's text texture page."""

import os
import subprocess
import sys
import tempfile
from pathlib import Path


def main() -> int:
    executable, source = map(Path, sys.argv[1:3])
    with tempfile.TemporaryDirectory(prefix="rage trophy view ") as directory:
        capture = Path(directory) / "trophy.ppm"
        environment = os.environ.copy()
        environment.update(
            SDL_AUDIODRIVER="dummy",
            RAGE_PORT_INPUT_SCRIPT="400:START,500:UP,520:CROSS",
        )
        result = subprocess.run(
            [
                executable,
                "--set", "run.frames=2200",
                "--set", "hooks.option_sweep=true",
                "--set", "stop.scene=23",
                "--set", "stop.timer=350",
                "--set", f"capture.path={capture}",
            ],
            cwd=source,
            env=environment,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            timeout=135,
        )
        if result.returncode != 0:
            print(result.stdout, file=sys.stderr)
            return result.returncode or 1

        header, pixels = capture.read_bytes().split(b"\n255\n", 1)
        width, height = map(int, header.splitlines()[-1].split())
        if (width, height) != (320, 480):
            raise AssertionError(f"unexpected Trophy View size: {width}x{height}")

        # DATA and EXIT occupy this box.  With trophy page 0x3e inherited by
        # mistake it contains only a few broken trophy fragments (~124 bright
        # pixels); the actual labels contain comfortably more than 600.
        bright = 0
        for y in range(52, 116):
            for x in range(32, 80):
                offset = (y * width + x) * 3
                red, green, blue = pixels[offset:offset + 3]
                bright += red > 180 and green > 180 and blue > 180
        if bright < 500:
            raise AssertionError(
                f"Trophy View selector sprites use the wrong texture page: {bright}"
            )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
