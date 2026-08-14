#!/usr/bin/env python3
"""Characterize the modern-renderer scene capture layer (plan phase R1).

Runs the deterministic prologue scenario twice with RAGE_PORT_SCENE_TRACE and
requires: identical traces across runs, semantic 3D draws and terrain cells in
the race demo, a populated 2D packet layer, 3D packets excluded from it, and
no capture overflows anywhere.
"""

from __future__ import annotations

import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path

LINE = re.compile(
    r"scene-frame frame=(\d+) scene=(-?\d+) timer=(-?\d+) draws=(\d+) "
    r"terrain=(\d+) cells=(\d+) packets=(\d+) skipped3d=(\d+) "
    r"overflow=(\d+),(\d+),(\d+),(\d+) hash=([0-9a-f]{16})"
)


def run_scenario(executable: Path, source_dir: Path, trace: Path) -> None:
    environment = os.environ.copy()
    environment.update(
        SDL_AUDIODRIVER="dummy",
        RAGE_PORT_SMOKE_FRAMES="1250",
        RAGE_PORT_RAW_INPUT_SCRIPT="400:START,500:START,650:CROSS",
        RAGE_PORT_SCENE_TRACE=str(trace),
    )
    result = subprocess.run(
        [executable],
        cwd=source_dir,
        env=environment,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        timeout=120,
    )
    if result.returncode != 0:
        print(result.stdout, file=sys.stderr)
        raise AssertionError(f"smoke run failed: {result.returncode}")


def main() -> int:
    executable = Path(sys.argv[1])
    source_dir = Path(sys.argv[2])
    with tempfile.TemporaryDirectory(prefix="rage-scene-capture-") as directory:
        first = Path(directory) / "first.log"
        second = Path(directory) / "second.log"
        run_scenario(executable, source_dir, first)
        run_scenario(executable, source_dir, second)

        first_text = first.read_text()
        if first_text != second.read_text():
            raise AssertionError("scene capture is not deterministic")

        frames = [LINE.match(line) for line in first_text.splitlines()]
        if any(match is None for match in frames):
            raise AssertionError("malformed scene-frame line in trace")
        rows = [tuple(int(g, 16 if i == 12 else 10) for i, g in
                      enumerate(match.groups())) for match in frames]
        if len(rows) < 1200:
            raise AssertionError(f"expected >=1200 captured frames, got {len(rows)}")

        for row in rows:
            if any(row[8:12]):
                raise AssertionError(f"capture overflow at frame {row[0]}: {row}")

        race = [row for row in rows if row[1] == 32]
        if not race:
            raise AssertionError("scenario never reached the race demo scene")
        with_models = [row for row in race if row[3] >= 5]
        with_terrain = [row for row in race if row[4] >= 1 and row[5] >= 32]
        with_skipped = [row for row in race if row[7] >= 100]
        if len(with_models) < 100:
            raise AssertionError(
                f"expected >=100 race frames with >=5 model draws, got {len(with_models)}"
            )
        if len(with_terrain) < 100:
            raise AssertionError(
                f"expected >=100 race frames with terrain cells, got {len(with_terrain)}"
            )
        if len(with_skipped) < 100:
            raise AssertionError(
                "3D packet exclusion looks broken: "
                f"{len(with_skipped)} race frames skipped >=100 packets"
            )
        if not any(row[6] > 0 for row in rows):
            raise AssertionError("no 2D packets captured anywhere")
    print(
        "scene capture characterized: "
        f"{len(rows)} frames, {len(race)} race frames, "
        f"max draws={max(row[3] for row in rows)}, "
        f"max packets={max(row[6] for row in rows)}"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
