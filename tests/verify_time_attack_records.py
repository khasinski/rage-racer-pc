#!/usr/bin/env python3
"""Exercise Time Attack through finish, results, and record-name entry."""

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
        RAGE_PORT_SMOKE_FRAMES="3250",
        RAGE_PORT_SMOKE_FINISH_FRAME="2000",
        RAGE_PORT_SMOKE_AUTO_CONFIRM_FRAME="3000",
        RAGE_PORT_INPUT_SCRIPT=(
            "400:START,500:START,610:DOWN,650:CROSS,950:CROSS,1100:CROSS,"
            "1200:CROSS,1264-2600:CROSS"
        ),
    )
    result = subprocess.run(
        [executable], cwd=source_dir, env=environment,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=70,
    )
    if result.returncode != 0:
        print(result.stdout, file=sys.stderr)
        return result.returncode or 1
    for transition in ("scene=17 frontend=3", "scene=20 frontend=3", "scene=21 frontend=3"):
        if transition not in result.stdout:
            raise AssertionError(f"Time Attack missed transition {transition}")
    for failure in ("primitive buffer exhausted", "ERROR: AddressSanitizer"):
        if failure in result.stdout:
            raise AssertionError(f"record path reported {failure!r}")
    print("Time Attack rendered results and record-name entry")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
