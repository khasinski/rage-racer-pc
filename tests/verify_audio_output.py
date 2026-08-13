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
        RAGE_PORT_SMOKE_FRAMES="1150",
        RAGE_PORT_SMOKE_AUDIO_METRICS="1",
        RAGE_PORT_INPUT_SCRIPT="400:START,500:START,650:CROSS,950:CROSS",
    )
    result = subprocess.run(
        [executable], cwd=source_dir, env=environment,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
        timeout=25,
    )
    if result.returncode != 0:
        print(result.stdout, file=sys.stderr)
        return result.returncode or 1
    metrics = re.search(
        r"audio metrics: frames=(\d+) energy=(\d+) seq_notes=(\d+)",
        result.stdout,
    )
    if metrics is None:
        raise AssertionError("audio backend did not report rendered PCM")
    frames, energy, seq_notes = map(int, metrics.groups())
    if frames < 10_000 or energy < 1_000_000:
        raise AssertionError(f"SPU output remained silent: frames={frames}, energy={energy}")
    if seq_notes == 0:
        raise AssertionError("menu SEQ opened but never dispatched a VAB note")
    print(
        f"SPU rendered VAB effects and menu sequence: frames={frames}, "
        f"energy={energy}, seq_notes={seq_notes}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
