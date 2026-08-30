#!/usr/bin/env python3
"""The in-process MDEC decoder plays FMV without external executables."""

import os
import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "support"))

from disc_image import require_disc


def main() -> int:
    executable, source = map(Path, sys.argv[1:3])
    require_disc(source)
    with tempfile.TemporaryDirectory(prefix="rage fmv ąę ") as directory:
        root = Path(directory)
        empty_path = root / "pusta ścieżka"
        temporary = root / "pliki tymczasowe żółte"
        empty_path.mkdir()
        temporary.mkdir()
        environment = os.environ.copy()
        environment.update(
            SDL_AUDIODRIVER="dummy",
            PATH=str(empty_path),
            TMPDIR=str(temporary),
            RAGE_PORT_SMOKE_FRAMES="380",
            RAGE_PORT_FMV_TRACE="1",
        )
        result = subprocess.run(
            [executable], cwd=source, env=environment,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
            timeout=75,
        )
        if result.returncode != 0:
            print(result.stdout, file=sys.stderr)
            raise AssertionError("in-process FMV playback terminated the game")
        if "fmv frame=" not in result.stdout:
            raise AssertionError(f"in-process FMV playback was not observable\n{result.stdout}")
        leftovers = list(temporary.iterdir())
        if leftovers:
            raise AssertionError(f"FMV temporary files leaked: {leftovers}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
