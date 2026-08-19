#!/usr/bin/env python3
"""A one-race scenario relinquishes control after a real finish."""

import os
import subprocess
import sys
from pathlib import Path


def main() -> int:
    executable, source = map(Path, sys.argv[1:3])
    environment = os.environ.copy()
    environment.update(
        SDL_AUDIODRIVER="dummy",
        RAGE_PORT_SMOKE_FRAMES="3500",
        RAGE_PORT_SMOKE_FINISH_FRAME="2000",
        RAGE_PORT_SMOKE_AUTO_CONFIRM_FRAME="3000",
    )
    result = subprocess.run(
        [
            executable,
            "--scenario", source / "race-scenario.ini",
            # The markers record the frontend state, which only the menu route
            # sets; booting straight into the race never enters the frontend.
            "--set", "boot.direct=false",
            "--set", "race.after_finish=menu",
        ],
        cwd=source,
        env=environment,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        timeout=65,
    )
    if result.returncode != 0:
        print(result.stdout, file=sys.stderr)
        return result.returncode or 1
    required = (
        "scenario mode=grand-prix",
        "scenario race finished after_finish=menu",
        "scenario automation stopped after finish",
        "scene=17 frontend=3",
    )
    missing = [text for text in required if text not in result.stdout]
    if missing:
        raise AssertionError(f"after-finish flow missed {missing}\n{result.stdout}")
    stopped = result.stdout.index("scenario automation stopped after finish")
    if "scenario confirm" in result.stdout[stopped:]:
        raise AssertionError("scenario continued automatic menu input after finish")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
