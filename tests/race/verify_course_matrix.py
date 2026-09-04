#!/usr/bin/env python3
"""Boot every GP/Extra GP course combination through the scenario API."""

import os
import hashlib
import subprocess
import sys
import tempfile
from pathlib import Path

# Captures from the current, deterministic classic renderer.  These frames
# exercise every Grand Prix and Extra Grand Prix course before the race starts.
CLASSIC_GOLDENS = {
    ("gp", 0): "da080b332d8d875330a8ab51c069d456271ac6f60e512dfea9180d167b02e769",
    ("gp", 1): "899c3a8382c274c5a24a4fdd79574424a03c651a123a48751006703cdefba795",
    ("gp", 2): "a6e27cce24ce4f7e663c1022dc0a978e6a1f1e45a6027379a933fb2cc03d7af3",
    ("gp", 3): "2dd8e9fc72c9b2d1a5bf71bef34e382e29f9c0232d26c9747b3282e8e8758121",
    ("extra-gp", 0): "50b97585f7aa7c9b75de5d6d8652a1bf9dc9b0476e2d2c3378935c2c18f62293",
    ("extra-gp", 1): "c221cbe8f75dff984d978c4f0c18a94d1c8539a8a23c6de7c8995e305a3a9a24",
    ("extra-gp", 2): "ddaf5d412cdfcfbd8ff1a3b12eb481fa96ce40daeda7b8d6027441e6d286da85",
    ("extra-gp", 3): "a81c48c7646fb2b61ae4abd250565036bdc48ff2807dfaca75489588271f9ca0",
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
