#!/usr/bin/env python3
"""Require non-silent PCM from the original VAB sound-effect path."""

from __future__ import annotations

import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path


def main() -> int:
    executable = Path(sys.argv[1])
    source_dir = Path(sys.argv[2])
    with tempfile.TemporaryDirectory(prefix="rage-audio-") as directory:
        trace = Path(directory) / "spu.csv"
        environment = os.environ.copy()
        environment.update(
            SDL_AUDIODRIVER="dummy",
            RAGE_PORT_SMOKE_FRAMES="1400",
            RAGE_PORT_SMOKE_AUDIO_METRICS="1",
            RAGE_PORT_INPUT_SCRIPT=(
                "400:START,500:START,650:CROSS,950:CROSS,"
                "1100:CROSS,1200:CROSS"
            ),
            RAGE_PORT_SMOKE_STOP_SCENE="10",
            RAGE_PORT_SMOKE_STOP_SCENE_TIMER="40",
            RAGE_PORT_SPU_TRACE=str(trace),
        )
        result = subprocess.run(
            [executable], cwd=source_dir, env=environment,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
            timeout=30,
        )
        trace_rows = trace.read_text().splitlines()
    if result.returncode != 0:
        print(result.stdout, file=sys.stderr)
        return result.returncode or 1
    metrics = re.search(
        r"audio metrics: frames=(\d+) energy=(\d+) seq_notes=(\d+) "
        r"seq_voices=(\d+) "
        r"pitch_updates=(\d+) cdda=(\d+)",
        result.stdout,
    )
    if metrics is None:
        raise AssertionError("audio backend did not report rendered PCM")
    frames, energy, seq_notes, seq_voices, _pitch_updates, cdda = map(
        int, metrics.groups()
    )
    if frames < 10_000 or energy < 1_000_000:
        raise AssertionError(f"SPU output remained silent: frames={frames}, energy={energy}")
    if seq_notes == 0:
        raise AssertionError("menu SEQ opened but never dispatched a VAB note")
    if seq_voices < seq_notes:
        raise AssertionError(
            "menu SEQ lost VAB tone starts: "
            f"notes={seq_notes}, voices={seq_voices}"
        )
    if cdda != 0:
        raise AssertionError("prologue CD-DA was still playing under the menu sequence")
    # ROUND cue 0x19 is a two-tone stereo effect.  These are the retail
    # voice-register snapshots at scene timer 32.  Reversing the libsnd pan
    # attenuation makes both voices mono and is audible across many effects.
    if "22,8882,1024,8561,6289,33023,8128" not in trace_rows:
        raise AssertionError("ROUND cue lost the retail left-biased voice")
    if "23,8882,1031,6058,8561,33023,8128" not in trace_rows:
        raise AssertionError("ROUND cue lost the retail right-biased voice")
    print(
        f"SPU rendered VAB effects and menu sequence: frames={frames}, "
        f"energy={energy}, seq_notes={seq_notes}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
