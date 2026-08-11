#!/usr/bin/env python3
"""Exercise the retail finish, replay, result, and prize-screen path."""

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
        RAGE_PORT_SMOKE_FRAMES="3500",
        RAGE_PORT_SMOKE_FINISH_FRAME="2000",
        RAGE_PORT_SMOKE_AUTO_CONFIRM_FRAME="3000",
        RAGE_PORT_INPUT_SCRIPT=(
            "400:START,500:START,650:CROSS,950:CROSS,"
            "1100:CROSS,1200:CROSS,1700-2400:CROSS"
        ),
    )
    result = subprocess.run(
        [executable], cwd=source_dir, env=environment,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
        timeout=70,
    )
    if result.returncode != 0:
        print(result.stdout, file=sys.stderr)
        return result.returncode or 1
    for transition in (
        "scene=17 frontend=3",
        "scene=18 frontend=3",
        "scene=19 frontend=3",
    ):
        if transition not in result.stdout:
            raise AssertionError(f"Grand Prix missed result transition {transition}")
    final_line = next(
        (line for line in result.stdout.splitlines()
         if "stopped at frame 3500" in line), ""
    )
    if not any(f"scene {scene}," in final_line for scene in (8, 10, 11, 12)):
        raise AssertionError("Grand Prix did not advance from results to next round")
    for failure in ("primitive buffer exhausted", "ERROR: AddressSanitizer"):
        if failure in result.stdout:
            raise AssertionError(f"result path reported {failure!r}")
    print("Grand Prix reached results, awarded prize money, and started next round")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
