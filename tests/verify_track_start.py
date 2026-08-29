#!/usr/bin/env python3
"""Scenario track-point starts produce valid player and rival track states."""

import os
import re
import subprocess
import sys
from pathlib import Path


def main() -> int:
    executable, source = map(Path, sys.argv[1:3])
    environment = os.environ.copy()
    environment["SDL_AUDIODRIVER"] = "dummy"
    command = [
        executable, "--scenario", source / "race-scenario.ini",
        "--set", "run.frames=2600",
        "--set", "stop.scene=12",
        "--set", "stop.timer=1",
        "--set", "race.grid=0,1,2,3,4,5,6,7,8,9,10",
        "--set", "start.player_track_point=120",
        "--set", "start.rival_track_points=118,116,114,112",
    ]
    result = subprocess.run(
        command, cwd=source, env=environment,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
        timeout=150,
    )
    if result.returncode != 0:
        print(result.stdout, file=sys.stderr)
        return result.returncode or 1
    required = (
        r"scenario-start player point=120 .*progress=[1-9]\d* section=\d+",
        r"scenario-start rival=0 point=118 .*progress=[1-9]\d* section=\d+",
        r"scenario-start rival=2 point=114 .*progress=[1-9]\d* section=\d+",
        r"custom track start applied player=120 rivals=4 points=\d+",
        r"smoke synchronized stop",
    )
    missing = [pattern for pattern in required
               if re.search(pattern, result.stdout) is None]
    if missing:
        raise AssertionError(f"track start missed {missing}\n{result.stdout}")
    if "ERROR:" in result.stdout or "runtime error:" in result.stdout:
        raise AssertionError(f"track start emitted a runtime error\n{result.stdout}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
