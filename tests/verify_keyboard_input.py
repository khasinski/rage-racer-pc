#!/usr/bin/env python3
"""Prove a short SDL key tap survives into the game's PS1 pad edge."""

from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path


def main() -> int:
    executable = Path(sys.argv[1])
    source_dir = Path(sys.argv[2])
    environment = os.environ.copy()
    environment.update(
        SDL_AUDIODRIVER="dummy",
        RAGE_PORT_SMOKE_FRAMES="30",
        RAGE_PORT_TEST_KEY_TAP="Return",
        RAGE_PORT_INPUT_DEBUG="1",
    )
    result = subprocess.run(
        [executable], cwd=source_dir, env=environment,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
        timeout=15,
    )
    if result.returncode != 0:
        print(result.stdout, file=sys.stderr)
        return result.returncode or 1
    if "keyboard pad mask=0800" not in result.stdout:
        raise AssertionError("SDL Return tap did not map to PAD_START")
    if "type=41 held=0800 pressed=0800" not in result.stdout:
        raise AssertionError("PAD_START did not produce a game rising edge")
    print("short SDL Return tap produced the game's PAD_START rising edge")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
