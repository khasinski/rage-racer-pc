#!/usr/bin/env python3
"""Render every frontend and OPTION submenu long enough to catch bad host pointers."""

from __future__ import annotations

import os
import re
import subprocess
import sys
from pathlib import Path


def run(executable: Path, source_dir: Path, variable: str, expected: set[int]) -> None:
    environment = os.environ.copy()
    option_sweep = variable.endswith("OPTION_SWEEP")
    input_script = (
        "400:START,500:UP,520:CROSS" if option_sweep
        else "400:START,500:DOWN,520:CROSS"
    )
    environment.update(
        SDL_AUDIODRIVER="dummy",
        RAGE_PORT_SMOKE_FRAMES="2200",
        RAGE_PORT_INPUT_SCRIPT=input_script,
    )
    environment[variable] = "1"
    if option_sweep:
        environment["RAGE_PORT_SCENE_TRACE"] = "1"
    result = subprocess.run(
        [executable], cwd=source_dir, env=environment,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=135,
    )
    if result.returncode != 0:
        print(result.stdout, file=sys.stderr)
        raise AssertionError(f"{variable} exited with {result.returncode}")
    pattern = r"menu sweep .*screen=(\d+)" if variable.endswith("MENU_SWEEP") else r"option sweep .*mode=(\d+)"
    visited = {int(value) for value in re.findall(pattern, result.stdout)}
    if visited != expected:
        raise AssertionError(f"{variable} visited {sorted(visited)}, expected {sorted(expected)}")
    if option_sweep and not re.search(
        r"scene-frame .* scene=23 .*draws=[1-9]\d* .*faces=[1-9]\d*",
        result.stdout,
    ):
        raise AssertionError("OPTION controller screens emitted no 3D model faces")
    for failure in (
        "primitive buffer exhausted",
        "ERROR: AddressSanitizer",
        "runtime error:",
    ):
        if failure in result.stdout:
            raise AssertionError(f"{variable} reported {failure!r}")


def main() -> int:
    executable = Path(sys.argv[1])
    source_dir = Path(sys.argv[2])
    run(executable, source_dir, "RAGE_PORT_SMOKE_MENU_SWEEP", set(range(1, 13)))
    run(executable, source_dir, "RAGE_PORT_SMOKE_OPTION_SWEEP", set(range(1, 12)))
    print("Rendered all 12 frontend screens and all 11 OPTION modes")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
