#!/usr/bin/env python3
"""The release runtime must retain stderr diagnostics in a persistent file."""

import os
import subprocess
import sys
import tempfile
from pathlib import Path


def main() -> int:
    executable = Path(sys.argv[1])
    source = Path(sys.argv[2])
    with tempfile.TemporaryDirectory(prefix="rage-log-") as directory:
        log = Path(directory) / "runtime.log"
        environment = os.environ.copy()
        environment.update(
            SDL_AUDIODRIVER="dummy",
            RAGE_PORT_TEST_LOG="1",
            RAGE_PORT_LOG_PATH=str(log),
            RAGE_PORT_SMOKE_FRAMES="2",
        )
        result = subprocess.run([executable], cwd=source, env=environment,
                                timeout=45)
        if result.returncode != 0:
            return result.returncode or 1
        text = log.read_text(encoding="utf-8")
        for expected in ("=== Rage Racer session", "smoke state frame=1"):
            if expected not in text:
                raise AssertionError(f"diagnostic log missed {expected!r}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
