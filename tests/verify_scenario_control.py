#!/usr/bin/env python3
"""Verify scenario validation, and both ways a scenario reaches a race.

A scenario boots straight into its race by default, which is the fast path and
never touches the frontend. Driving the retail menus is still supported and is
what boot.direct=false asks for, so both are exercised here: the menu route for
its scene-relative confirms, and the default for reaching the race at all.
"""

import os
import subprocess
import sys
from pathlib import Path


def main() -> int:
    executable = Path(sys.argv[1])
    source = Path(sys.argv[2])
    environment = os.environ.copy()
    environment.update(
        SDL_AUDIODRIVER="dummy",
        RAGE_PORT_SCENARIO="1",
        RAGE_PORT_SCENARIO_MODE="99",
        RAGE_PORT_SCENARIO_SERIES="99",
        RAGE_PORT_SCENARIO_CLASS="99",
        RAGE_PORT_SCENARIO_COURSE="99",
        RAGE_PORT_SCENARIO_CAR="99",
        RAGE_PORT_SCENARIO_GRID="0,1,2",
        RAGE_PORT_SMOKE_FRAMES="2600",
        RAGE_PORT_SMOKE_STOP_SCENE="12",
        RAGE_PORT_SMOKE_STOP_SCENE_TIMER="1",
    )
    menus = dict(environment)
    menus["RAGE_PORT_MODS_DIRECTORY"] = ""
    result = subprocess.run(
        [executable, "--set", "boot.direct=false"], cwd=source, env=menus,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
        timeout=45,
    )
    if result.returncode != 0:
        print(result.stdout, file=sys.stderr)
        return result.returncode or 1
    required = (
        "ignoring invalid RAGE_PORT_SCENARIO_MODE=99",
        "ignoring invalid RAGE_PORT_SCENARIO_GRID",
        "scenario confirm scene=4 phase=0",
        "scenario confirm scene=8",
        "smoke synchronized stop",
    )
    missing = [text for text in required if text not in result.stdout]
    if missing:
        raise AssertionError(f"scenario control missed: {missing}\n{result.stdout}")

    # The default route skips the frontend entirely and still reaches the race.
    direct = subprocess.run(
        [executable], cwd=source, env=environment,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
        timeout=45,
    )
    if direct.returncode != 0:
        print(direct.stdout, file=sys.stderr)
        return direct.returncode or 1
    for text in ("scenario direct boot entered the race", "smoke synchronized stop"):
        if text not in direct.stdout:
            raise AssertionError(
                f"direct boot missed {text!r}\n{direct.stdout}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
