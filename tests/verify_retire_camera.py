#!/usr/bin/env python3
"""Retiring must retain the chase camera instead of launching a track flyby."""

import os
import re
import subprocess
import sys
from pathlib import Path


def main() -> int:
    executable, source = map(Path, sys.argv[1:3])
    environment = os.environ.copy()
    environment.update(
        SDL_AUDIODRIVER="dummy",
        RAGE_PORT_SCENARIO="1",
        RAGE_PORT_SMOKE_FRAMES="2500",
        RAGE_PORT_SMOKE_RETIRE="1",
        RAGE_PORT_SMOKE_CAMERA_STATE="1",
    )
    result = subprocess.run([executable], cwd=source, env=environment,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            text=True, timeout=150)
    if result.returncode != 0:
        print(result.stdout, file=sys.stderr)
        return result.returncode or 1
    if not re.search(r"scene \d+,.*race_phase=5.*retire_camera=1", result.stdout):
        raise AssertionError(f"RETIRE did not enter chase-camera state\n{result.stdout}")
    if "camera: pos=" not in result.stdout:
        raise AssertionError("RETIRE camera state was not observable")
    if "ERROR:" in result.stdout or "runtime error:" in result.stdout:
        raise AssertionError(f"RETIRE emitted a runtime error\n{result.stdout}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
