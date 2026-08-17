#!/usr/bin/env python3
"""FMV failure is graceful when the movie is unreadable and paths are Unicode."""

import os
import subprocess
import sys
import tempfile
from pathlib import Path


def main() -> int:
    executable, source = map(Path, sys.argv[1:3])
    with tempfile.TemporaryDirectory(prefix="rage fmv ąę ") as directory:
        root = Path(directory)
        empty_path = root / "bez ffmpeg"
        temporary = root / "pliki tymczasowe żółte"
        empty_path.mkdir()
        temporary.mkdir()
        environment = os.environ.copy()
        environment.update(
            SDL_AUDIODRIVER="dummy",
            PATH=str(empty_path),
            TMPDIR=str(temporary),
            RAGE_PORT_SMOKE_FRAMES="380",
        )
        result = subprocess.run(
            [executable], cwd=source, env=environment,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
            timeout=25,
        )
        if result.returncode != 0:
            print(result.stdout, file=sys.stderr)
            raise AssertionError("missing FFmpeg terminated the game")
        expected = ("FFmpeg could not decode FMV", "could not extract FMV")
        if not any(message in result.stdout for message in expected):
            raise AssertionError(
                f"FMV fallback was not observable\n{result.stdout}"
            )
        leftovers = list(temporary.iterdir())
        if leftovers:
            raise AssertionError(f"FMV temporary files leaked: {leftovers}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
