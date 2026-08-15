#!/usr/bin/env python3
"""Verify the pitched effect voices run during a race.

The engine drone, skids and impacts are one SPU voice each whose pitch the
game retargets every frame through SsUtChangePitch with old_note fixed at
0x3C (the key-on note). PsyQ's implementation never updates the stored note,
so those calls keep succeeding; a reimplementation that records new_note
freezes every pitched effect after its first change. Regression test for
that: drive a race and require a steady stream of accepted pitch updates.
"""

from __future__ import annotations

import os
import re
import subprocess
import sys
from pathlib import Path


def main() -> int:
    executable = Path(sys.argv[1])
    source_dir = Path(sys.argv[2])
    environment = os.environ.copy()
    environment.update(
        SDL_AUDIODRIVER="dummy",
        RAGE_PORT_SMOKE_FRAMES="2200",
        RAGE_PORT_SMOKE_AUDIO_METRICS="1",
        RAGE_PORT_INPUT_SCRIPT=(
            "400:START,500:START,650:CROSS,1015:CROSS,"
            "1165:CROSS,1265:CROSS,1533-10000:CROSS"
        ),
    )
    result = subprocess.run(
        [executable], cwd=source_dir, env=environment,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
        timeout=60,
    )
    if result.returncode != 0:
        print(result.stdout, file=sys.stderr)
        return result.returncode or 1
    metrics = re.search(r"pitch_updates=(\d+)", result.stdout)
    if metrics is None:
        raise AssertionError("audio backend did not report metrics")
    pitch_updates = int(metrics.group(1))
    if pitch_updates < 500:
        raise AssertionError(
            f"pitched effect voices stalled: pitch_updates={pitch_updates}"
        )
    print(f"pitched effects alive: pitch_updates={pitch_updates}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
