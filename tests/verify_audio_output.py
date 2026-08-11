#!/usr/bin/env python3
"""Require non-silent PCM from the original VAB sound-effect path."""

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
        RAGE_PORT_SMOKE_FRAMES="700",
        RAGE_PORT_SMOKE_AUDIO_METRICS="1",
        RAGE_PORT_INPUT_SCRIPT="400:START,500:START,650:CROSS",
    )
    result = subprocess.run(
        [executable], cwd=source_dir, env=environment,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
        timeout=25,
    )
    if result.returncode != 0:
        print(result.stdout, file=sys.stderr)
        return result.returncode or 1
    metrics = re.search(r"audio metrics: frames=(\d+) energy=(\d+)", result.stdout)
    if metrics is None:
        raise AssertionError("audio backend did not report rendered PCM")
    frames, energy = map(int, metrics.groups())
    if frames < 10_000 or energy < 1_000_000:
        raise AssertionError(f"SPU output remained silent: frames={frames}, energy={energy}")
    print(f"SPU rendered audible VAB effects: frames={frames}, energy={energy}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
