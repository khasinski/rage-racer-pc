#!/usr/bin/env python3
"""Direct boot must carry a manual-only car's setup into race physics."""

import os
import re
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
        RAGE_PORT_SCENARIO_MODE="1",
        RAGE_PORT_SCENARIO_SERIES="1",
        RAGE_PORT_SCENARIO_CLASS="4",
        RAGE_PORT_SCENARIO_COURSE="3",
        RAGE_PORT_SCENARIO_CAR="12",
        RAGE_PORT_SMOKE_FRAMES="1200",
        RAGE_PORT_SMOKE_STOP_SCENE="12",
        RAGE_PORT_SMOKE_STOP_SCENE_TIMER="650",
        RAGE_PORT_RAW_INPUT_SCRIPT="1-1200:CROSS",
    )
    result = subprocess.run(
        [
            executable,
            "--set", "video.renderer=classic",
            "--set", "race.transmission=automatic",
            "--set", "start.freeze=false",
        ],
        cwd=source,
        env=environment,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        timeout=30,
    )
    if result.returncode != 0:
        print(result.stdout, file=sys.stderr)
        return result.returncode or 1
    if "car 12 does not offer automatic transmission; using manual" not in result.stdout:
        raise AssertionError(f"manual-only restriction was not applied\n{result.stdout}")
    if "direct boot car setup tires=3 transmission=manual" not in result.stdout:
        raise AssertionError(f"manual setup was not applied\n{result.stdout}")
    final = re.search(
        r"Rage Racer smoke stopped.* speed=(\d+).* accelerator=(\d+).*"
        r"race_phase=(\d+).* manual=(\d+)",
        result.stdout,
    )
    if final is None:
        raise AssertionError(f"missing final car state\n{result.stdout}")
    speed, accelerator, race_phase, manual = map(int, final.groups())
    if manual != 1 or race_phase != 2 or accelerator == 0 or speed == 0:
        raise AssertionError(
            "manual-only car did not drive: "
            f"speed={speed} accelerator={accelerator} "
            f"race_phase={race_phase} manual={manual}\n{result.stdout}"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
