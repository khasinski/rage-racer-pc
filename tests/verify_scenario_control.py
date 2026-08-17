#!/usr/bin/env python3
"""Verify scenario validation and scene-relative menu automation."""

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
    result = subprocess.run(
        [executable], cwd=source, env=environment,
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
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
