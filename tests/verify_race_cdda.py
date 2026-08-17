#!/usr/bin/env python3
"""Verify that the race switches from the prologue CD-DA track to race BGM."""

from __future__ import annotations

import os
import re
import subprocess
import sys
from pathlib import Path


def main() -> int:
    executable = Path(sys.argv[1])
    source_dir = Path(sys.argv[2])
    environment = os.environ.copy()
    environment.update(
        SDL_AUDIODRIVER="dummy",
        RAGE_PORT_SMOKE_FRAMES="1800",
        RAGE_PORT_INPUT_SCRIPT=(
            "400:START,500:START,650:CROSS,950:CROSS,"
            "1100:CROSS,1200:CROSS"
        ),
    )
    result = subprocess.run(
        [executable], cwd=source_dir, env=environment,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
        timeout=40,
    )
    if result.returncode != 0:
        print(result.stdout, file=sys.stderr)
        return result.returncode or 1
    tracks = [
        int(value)
        for value in re.findall(r"opened track .*\(Track (\d+)\)\.bin", result.stdout)
    ]
    # Track 1 is the data track containing the opening movie's XA stream.
    # The final opened track must still be the requested race CD-DA music.
    if len(tracks) < 2 or tracks[0] != 1 or tracks[-1] <= 2:
        raise AssertionError(
            f"race did not switch from prologue CD-DA to race BGM: {tracks}"
        )
    if "stopped at frame 1800, scene 12" not in result.stdout:
        raise AssertionError("script did not remain in the active race")
    print(f"audio switched from intro XA to race CD-DA track {tracks[-1]}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
