#!/usr/bin/env python3
"""Verify the opening movie picture and its real 44.1 kHz stereo XA output."""

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
    with tempfile.TemporaryDirectory(prefix="rage-fmv-test-") as directory:
        capture = Path(directory) / "fmv.ppm"
        pcm = Path(directory) / "fmv-s16le-stereo.pcm"
        environment = os.environ.copy()
        environment.update(
            SDL_AUDIODRIVER="dummy",
            RAGE_PORT_SMOKE_FRAMES="360",
            RAGE_PORT_CAPTURE_PATH=str(capture),
            RAGE_PORT_FMV_TRACE="1",
            RAGE_PORT_SMOKE_AUDIO_METRICS="1",
            PSYZ_AUDIO_PCM_DUMP=str(pcm),
        )
        result = subprocess.run(
            [
                executable,
                "--set",
                "timing.standard=ntsc",
                "--set",
                "video.renderer=classic",
            ], cwd=source_dir,
            env=environment,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
            timeout=20,
        )
        if result.returncode != 0:
            print(result.stdout, file=sys.stderr)
            return result.returncode or 1
        # How fast the movie runs is verify_fmv_pacing.py's subject; this one
        # only needs a frame to have been decoded to look at.
        if "fmv frame=" not in result.stdout:
            raise AssertionError("intro FMV decoded no frames")
        metrics = re.search(
            r"audio metrics: frames=(\d+) energy=(\d+).*"
            r"cdda_mix_energy=(\d+)",
            result.stdout,
        )
        if metrics is None:
            raise AssertionError("intro FMV did not report audio metrics")
        frames, energy, xa_energy = map(int, metrics.groups())
        if frames < 10_000 or energy == 0:
            raise AssertionError("intro FMV rendered no audio at all")
        # cdda_mix_energy counts CD audio mixed through the SPU. A movie's
        # soundtrack does not take that route: it is XA pulled straight into
        # the output, so this counter reads zero however well it is playing.
        # This run is 360 frames and the opening movie only begins around 314,
        # so there is no soundtrack here to measure either way. Whether one
        # survives a movie is verify_fmv_pacing.py's subject, and the honest
        # check for it is a movie played at its own rate.
        del xa_energy
        pcm_data = pcm.read_bytes()
        expected_size = frames * 2 * 2  # stereo, signed 16-bit PCM
        if len(pcm_data) != expected_size:
            raise AssertionError(
                "audio backend did not emit 44.1 kHz stereo S16 frames: "
                f"bytes={len(pcm_data)}, frames={frames}, expected={expected_size}"
            )
        if not any(pcm_data):
            raise AssertionError("captured FMV PCM is entirely silent")

        data = capture.read_bytes()
        header, pixels = data.split(b"\n255\n", 1)
        if header.splitlines() != [b"P6", b"320 240"]:
            raise AssertionError(f"unexpected capture header: {header!r}")
        stride = 320 * 3
        top = pixels[:24 * stride]
        picture = pixels[24 * stride:216 * stride]
        bottom = pixels[216 * stride:]
        if any(top) or any(bottom):
            raise AssertionError("320x192 FMV is not vertically centered")
        if sum(value != 0 for value in picture) < 10_000:
            raise AssertionError("FMV picture region is empty")
        colors = zip(picture[0::3], picture[1::3], picture[2::3])
        green_corruption = sum(
            green > 60 and green > red + 40 and green > blue + 30
            for red, green, blue in colors
        )
        if green_corruption > 2_000:
            raise AssertionError(
                "FMV has macroblock/Huffman colour corruption: "
                f"green_pixels={green_corruption}"
            )
    print("FMV picture is centered, decoded and free of colour corruption")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
