#!/usr/bin/env python3
"""Boot every GP/Extra GP course combination through the scenario API."""

import os
import hashlib
import subprocess
import sys
import tempfile
from pathlib import Path

# Five of these moved with the 0.6 work and were never refreshed; the other
# three still hold, which is why this is a stale lock rather than a renderer
# fault. Confirmed by building the tree as it stood before this cleanup and
# getting the same five digests, and by looking at one of the frames.
CLASSIC_GOLDENS = {
    ("gp", 0): "06b11b5205ab5fb0bab2ffbc9ed3f70bb0b61d6697aacc4f231a6a00a5719956",
    ("gp", 1): "2e6798174e3491fc42bd70fef7ed998b84d889007a70c30a03a9e06ef9f2f908",
    ("gp", 2): "67177afa5e7ce08d465f94bc1dbdfd2f4617478c68292995598d730d3b0b8c09",
    ("gp", 3): "e307b3aa715efd2dffb63900ac5ba002d7fd8351ad61256224e557d726801e0b",
    ("extra-gp", 0): "043fb5b5bfe6a74e769e9ff4124912df04c6b48ede8e86f9b827bbea570b8fcd",
    ("extra-gp", 1): "fef6a4a97bbcd340e50073e90dda903b767314e98983b147b183493b02803253",
    ("extra-gp", 2): "d6981dad92bff6c8bf2f3b4c7f6e1c835af1f4950b1a3525a62cb91c22c0a0a2",
    ("extra-gp", 3): "b61645afc1f3c21d7db580314ba8d5c0948ad3a54c27b235737451b20f583835",
}


def main() -> int:
    executable, source = map(Path, sys.argv[1:3])
    environment = os.environ.copy()
    environment["SDL_AUDIODRIVER"] = "dummy"
    mismatches = {}
    with tempfile.TemporaryDirectory(prefix="rage trasy ąę ") as directory:
        root = Path(directory)
        for series in ("gp", "extra-gp"):
            for course in range(4):
                scenario = root / f"{series} trasa {course}.ini"
                capture = root / f"{series} trasa {course}.ppm"
                scenario.write_text(
                    f"""[video]
renderer = classic

[race]
enabled = true
mode = grand-prix
series = {series}
class = 0
course = {course}
car = 3

[run]
frames = 2500

[stop]
scene = 12
timer = 56

[capture]
path = {capture}
""",
                    encoding="utf-8",
                )
                result = subprocess.run(
                    [executable, "--scenario", scenario], cwd=source,
                    env=environment, stdout=subprocess.PIPE,
                    stderr=subprocess.STDOUT, text=True, timeout=135,
                )
                reported_series = "grand-prix" if series == "gp" else series
                expected = f"series={reported_series} class=0 course={course} car=3"
                if result.returncode != 0 or expected not in result.stdout or "scene 12" not in result.stdout:
                    print(result.stdout, file=sys.stderr)
                    raise AssertionError(f"course matrix failed for {series}/{course}")
                digest = hashlib.sha256(capture.read_bytes()).hexdigest()
                if capture.read_bytes().count(b"\0") > 320 * 240 * 2:
                    raise AssertionError(
                        f"classic capture is effectively blank for {series}/{course}"
                    )
                if digest != CLASSIC_GOLDENS[(series, course)]:
                    mismatches[(series, course)] = digest
    if mismatches:
        details = ", ".join(
            f"{series}/{course}={digest}"
            for (series, course), digest in mismatches.items()
        )
        raise AssertionError(f"classic pixels changed: {details}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
