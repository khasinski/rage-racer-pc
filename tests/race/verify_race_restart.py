#!/usr/bin/env python3
"""Repeatedly tear down and re-enter the live race scene."""

import os
import subprocess
import sys
import tempfile
from pathlib import Path


def main() -> int:
    executable, source = map(Path, sys.argv[1:3])
    with tempfile.TemporaryDirectory(prefix="rage restart ąę ") as directory:
        scenario = Path(directory) / "powtórne wejście.ini"
        scenario.write_text(
            """[race]
enabled = true
mode = grand-prix
series = gp
class = 0
course = 0
car = 3

[run]
frames = 2520

[hooks]
restart_race_frames = 2300,2400
""",
            encoding="utf-8",
        )
        environment = os.environ.copy()
        environment["SDL_AUDIODRIVER"] = "dummy"
        result = subprocess.run(
            [executable, "--scenario", scenario], cwd=source, env=environment,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
            timeout=180,
        )
    if result.returncode != 0:
        print(result.stdout, file=sys.stderr)
        raise AssertionError("re-entering the race terminated the game")
    if result.stdout.count("smoke race restart frame=") != 2:
        raise AssertionError(f"both race restarts were not executed\n{result.stdout}")
    final = [line for line in result.stdout.splitlines()
             if line.startswith("Rage Racer smoke stopped")]
    if not final or "scene 12" not in final[-1]:
        raise AssertionError(f"game did not finish in a live race\n{result.stdout}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
