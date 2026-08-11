#!/usr/bin/env python3
"""Exercise the complete Grand Prix intro, countdown, and replay recording."""

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
        RAGE_PORT_SMOKE_FRAMES="2000",
        RAGE_PORT_INPUT_SCRIPT=(
            "400:START,500:START,650:CROSS,950:CROSS,"
            "1100:CROSS,1200:CROSS,1700-2000:CROSS"
        ),
    )
    result = subprocess.run(
        [executable], cwd=source_dir, env=environment,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
        timeout=45,
    )
    if result.returncode != 0:
        print(result.stdout, file=sys.stderr)
        return result.returncode or 1
    if "stopped at frame 2000, scene 12" not in result.stdout:
        raise AssertionError("Grand Prix did not survive through race start")
    player_state = re.search(r"speed=(-?\d+) accelerator=(-?\d+)", result.stdout)
    if player_state is None or int(player_state.group(1)) <= 0:
        raise AssertionError("Grand Prix did not accept acceleration after countdown")
    if int(player_state.group(2)) != 256:
        raise AssertionError("digital accelerator was not sampled at full pressure")
    for failure in (
        "primitive buffer exhausted", "misaligned OT link",
        "likely corrupted",
    ):
        if failure in result.stdout:
            raise AssertionError(f"Grand Prix reported {failure!r}")
    print("Grand Prix survived intro/countdown and entered a moving race")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
