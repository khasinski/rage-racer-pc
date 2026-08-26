#!/usr/bin/env python3
"""Modern mode must fail clearly instead of selecting a third renderer."""

import os
import subprocess
import sys
import tempfile
from pathlib import Path


def main() -> int:
    executable, source = map(Path, sys.argv[1:3])
    with tempfile.TemporaryDirectory(prefix="rage-missing-assets-") as directory:
        missing = Path(directory) / "does-not-exist"
        environment = os.environ.copy()
        environment["SDL_AUDIODRIVER"] = "dummy"
        result = subprocess.run(
            [executable,
             "--set", "video.renderer=modern",
             "--set", f"modern.assets={missing}",
             "--set", "run.frames=1"],
            cwd=source,
            env=environment,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            timeout=15,
        )
    if result.returncode == 0:
        raise AssertionError("modern mode accepted a missing native cache")
    required = (
        f"native asset cache unavailable: {missing}",
        "refusing to start modern renderer without native assets",
    )
    missing_messages = [message for message in required
                        if message not in result.stdout]
    if missing_messages:
        raise AssertionError(
            f"missing modern startup diagnostics: {missing_messages}\n"
            f"{result.stdout}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
