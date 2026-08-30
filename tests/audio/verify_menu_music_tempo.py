#!/usr/bin/env python3
"""Require the menu sequence to keep its tempo in real time, not in frames.

The car and course select screens are the only place the game sequences its
own music: a SEQ opened against the VAB that loads with the car-select assets
and started from EnterCourseSelectScreen. libsnd measures musical time in
ticks of a sixtieth of a second, and retail asked for exactly that with
SsSetTickMode(SS_TICK60), which on a PAL console kept the music at sixty ticks
a second while the picture ran at fifty.

The host services the sequencer from the game loop instead of from a counter
interrupt, so it is easy to spend one frame as one tick and let the music
follow the video standard. That is silent everywhere else in the game, because
nothing else is sequenced, and on PAL it plays the menu music a fifth slow.

The same scripted route, stopped at the same scene and scene timer, is the
same number of frames under either standard but a fifth more seconds under
PAL. So the note-on count must grow with the standard, not stay put.
"""

from __future__ import annotations

import os
import re
import subprocess
import sys
from pathlib import Path

# The route is frame-scripted and the stop condition is a scene timer, so both
# runs cover the same frames. Only the seconds those frames take differ.
ROUTE = "400:START,500:START,650:CROSS,950:CROSS,1100:CROSS,1200:CROSS"
FRAMES = "1400"
STOP_SCENE = "10"
STOP_SCENE_TIMER = "40"

# 60/50. The count is notes, not ticks, so it lands near the ratio rather than
# on it; a clock that follows the frame rate lands on 1.00 instead.
EXPECTED_RATIO = 60.0 / 50.0
RATIO_SLACK = 0.09

# A ratio alone would still hold if both standards ran the sequencer at, say,
# ninety hertz. Pin the PAL count as well: 1400 PAL frames of this route are
# 156 note-ons at the retail sixty-hertz clock, and 124 at fifty.
PAL_NOTES = 156
PAL_NOTES_SLACK = 12


def run(executable: Path, source_dir: Path, standard: str) -> int:
    environment = os.environ.copy()
    environment.update(
        SDL_AUDIODRIVER="dummy",
        RAGE_PORT_SMOKE_FRAMES=FRAMES,
        RAGE_PORT_SMOKE_AUDIO_METRICS="1",
        RAGE_PORT_INPUT_SCRIPT=ROUTE,
        RAGE_PORT_SMOKE_STOP_SCENE=STOP_SCENE,
        RAGE_PORT_SMOKE_STOP_SCENE_TIMER=STOP_SCENE_TIMER,
    )
    result = subprocess.run(
        [
            executable,
            "--set", "video.renderer=classic",
            "--set", f"timing.standard={standard}",
        ],
        cwd=source_dir, env=environment,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
        timeout=120,
    )
    if result.returncode != 0:
        print(result.stdout, file=sys.stderr)
        raise AssertionError(f"{standard} run failed with {result.returncode}")
    metrics = re.search(r"seq_notes=(\d+) ", result.stdout)
    if metrics is None:
        print(result.stdout, file=sys.stderr)
        raise AssertionError(f"{standard} run reported no audio metrics")
    return int(metrics.group(1))


def main() -> int:
    executable = Path(sys.argv[1])
    source_dir = Path(sys.argv[2])

    pal = run(executable, source_dir, "pal")
    ntsc = run(executable, source_dir, "ntsc")

    if ntsc == 0 or pal == 0:
        raise AssertionError(
            f"the menu sequence never played: pal={pal}, ntsc={ntsc}"
        )
    ratio = pal / ntsc
    if abs(ratio - EXPECTED_RATIO) > RATIO_SLACK:
        raise AssertionError(
            "the menu sequence clock followed the video standard instead of "
            f"real time: pal={pal}, ntsc={ntsc}, ratio={ratio:.3f} "
            f"(expected {EXPECTED_RATIO:.3f})"
        )
    if abs(pal - PAL_NOTES) > PAL_NOTES_SLACK:
        raise AssertionError(
            "the menu sequence did not run at the retail sixty-hertz tick: "
            f"pal notes={pal} (expected {PAL_NOTES})"
        )
    print(
        f"menu sequence kept its tempo: pal={pal}, ntsc={ntsc}, "
        f"ratio={ratio:.3f}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
