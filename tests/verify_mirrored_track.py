#!/usr/bin/env python3
"""Mirrored courses retain main-view 3D and driver-relative steering."""

import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path


def run(executable: Path, source: Path, work: Path, mirrored: bool) -> tuple[int, str]:
    environment = os.environ.copy()
    environment.update(
        SDL_AUDIODRIVER="dummy",
        RAGE_PORT_SCENARIO="1",
        RAGE_PORT_SMOKE_FRAMES="2450",
        RAGE_PORT_INPUT_SCRIPT="2250-2450:LEFT+X",
        RAGE_PORT_SCENE_TRACE="1",
    )
    if mirrored:
        environment["RAGE_PORT_SMOKE_MIRROR_TRACK"] = "1"
    result = subprocess.run([executable], cwd=work, env=environment,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            text=True, timeout=55)
    if result.returncode != 0:
        print(result.stdout, file=sys.stderr)
        raise RuntimeError(f"mirrored={mirrored} exited {result.returncode}")
    match = re.search(r"steer=(-?\d+) course_mirror=(\d+)", result.stdout)
    if match is None:
        raise AssertionError("missing mirrored steering diagnostics")
    return int(match.group(1)), result.stdout


def main() -> int:
    executable, source = map(Path, sys.argv[1:3])
    with tempfile.TemporaryDirectory(prefix="rage-mirror-course-") as directory:
        work = Path(directory)
        (work / "assets").symlink_to(source / "assets", target_is_directory=True)
        (work / "rage-port.cfg").write_text(
            "renderer=modern\nmodern.draw_distance=2\n", encoding="utf-8")
        normal_steer, _ = run(executable, source, work, False)
        mirror_steer, output = run(executable, source, work, True)
    if normal_steer == 0 or mirror_steer == 0 or (normal_steer > 0) != (mirror_steer > 0):
        raise AssertionError(
            f"mirrored steering reversed: normal={normal_steer} mirror={mirror_steer}"
        )
    race_frames = [line for line in output.splitlines()
                   if "scene-frame" in line and " scene=12 " in line]
    if not race_frames or not any(re.search(r"faces=[1-9]\d*", line) for line in race_frames):
        raise AssertionError("modern mirrored course lost its main-view 3D faces")
    if "ERROR:" in output or "runtime error:" in output:
        raise AssertionError(f"mirrored course emitted a runtime error\n{output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
