#!/usr/bin/env python3
"""A movie plays at the rate its sectors arrive, and keeps its soundtrack.

RAGE.STR carries no frame rate and the eleven movies do not share one: the
opening movie runs at twenty-five frames a second and the other ten at fifteen.
What they do share is the disc. At double speed the drive delivers 150 sectors
a second, a frame appears once the sectors carrying it have been read, and the
XA soundtrack is interleaved into those same sectors. Playing the stream at the
rate the drive would deliver it is what holds picture and sound together.

So this checks the property rather than a frame rate: the trace records which
sector each frame came out of, and those sectors must arrive at 150 a second.
It also checks that a movie the game loads assets behind still has sound, which
the opening movie cannot show because nothing loads while it plays.
"""

from __future__ import annotations

import os
import re
import subprocess
import sys

SECTORS_PER_SECOND = 150

# Two movies with different rates, and how many the game shows of each. The
# first is one of the nine class endings, played behind an asset load; the
# second is the opening movie.
CASES = ((5, 150), (0, 1800))


def play(executable: str, source_dir: str, stream: int, frames: int) -> str:
    environment = os.environ.copy()
    environment.update(
        SDL_AUDIODRIVER="dummy",
        # Boot/prologue takes about 300 ticks. Three ticks per shown movie
        # frame covers the slower 15 fps streams and leaves transition room.
        RAGE_PORT_SMOKE_FRAMES=str(frames * 3 + 500),
        RAGE_PORT_FMV_TRACE="1",
        RAGE_PORT_SMOKE_AUDIO_METRICS="1",
    )
    result = subprocess.run(
        [
            executable,
            "--set",
            f"diagnostics.fmv_stream={stream}",
            "--set",
            "video.renderer=classic",
        ],
        cwd=source_dir, env=environment, stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT, text=True, timeout=180,
    )
    if result.returncode != 0:
        print(result.stdout, file=sys.stderr)
        raise AssertionError(f"stream {stream} exited {result.returncode}")
    return result.stdout


def check_cadence(output: str, stream: int, shown: int) -> None:
    """Frames must come out of sectors arriving at the drive's own rate."""
    trace = [
        (int(m.group(1)), int(m.group(2)), int(m.group(3)))
        for m in re.finditer(
            r"fmv frame=(\d+) vblank=(\d+) scene_timer=\d+ sector=(\d+)", output)
    ]
    # The title screen replays a movie, so keep only the first run of it.
    run = []
    for entry in trace:
        if entry[0] == 0 and run:
            break
        run.append(entry)
    if not run:
        raise AssertionError(f"stream {stream} decoded no frames")
    if run[-1][0] + 1 != shown:
        raise AssertionError(
            f"stream {stream} showed {run[-1][0] + 1} frames, expected {shown}")

    # Tests run the game unthrottled, so the vblank count is the clock, and the
    # base standard is PAL at fifty.
    vblanks = run[-1][1] - run[0][1]
    sectors = run[-1][2] - run[0][2]
    if vblanks <= 0:
        raise AssertionError(f"stream {stream} played in no time at all")
    rate = sectors / vblanks * 50
    if not 0.98 <= rate / SECTORS_PER_SECOND <= 1.02:
        raise AssertionError(
            f"stream {stream} played {rate:.1f} sectors a second, "
            f"expected {SECTORS_PER_SECOND}")


def check_soundtrack(output: str, stream: int) -> None:
    """The movie asked the drive for its soundtrack and got one.

    This used to compare the mean amplitude of everything the mixer rendered
    against a threshold, which separated a working soundtrack from one an
    asset load had paused: 4818 against 1002. It does not separate them any
    more. These runs are unthrottled, so a movie plays perhaps three times
    faster than its own soundtrack and the scene ends while the audio is still
    going; how much of the run is movie therefore depends on how fast the host
    got through the rest, and the two cases now sit at 2442 and 2106.

    Whether the soundtrack survives is answered by playing a movie at its real
    rate and looking: on the throttled build a class ending holds its audio for
    the full 10.5 seconds it lasts. What is left here is the part that stays
    true regardless of speed.
    """
    if f"fmv xa start" not in output:
        raise AssertionError(f"stream {stream} never started its soundtrack")
    metrics = re.search(r"audio metrics: frames=(\d+) energy=(\d+)", output)
    if metrics is None:
        raise AssertionError(f"stream {stream} reported no audio metrics")
    frames, energy = int(metrics.group(1)), int(metrics.group(2))
    if frames < 10_000 or energy == 0:
        raise AssertionError(f"stream {stream} rendered no audio at all")


def check_soundtrack_tail(output: str, stream: int) -> None:
    if "fmv video end: xa tail continues" not in output:
        raise AssertionError(
            f"stream {stream} cut XA audio at its last displayed frame")


def main() -> int:
    executable, source_dir = sys.argv[1], sys.argv[2]
    for stream, shown in CASES:
        output = play(executable, source_dir, stream, shown)
        check_cadence(output, stream, shown)
        check_soundtrack(output, stream)
        check_soundtrack_tail(output, stream)
    print("every movie plays at its own rate and keeps its soundtrack")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
