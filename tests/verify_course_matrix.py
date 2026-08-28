#!/usr/bin/env python3
"""Boot every GP/Extra GP course combination through the scenario API."""

import os
import hashlib
import subprocess
import sys
import tempfile
from pathlib import Path

CLASSIC_GOLDENS = {
    ("gp", 0): "fd981cf3534baf2726b0eefc7c449ba0f7223561570cb021ea6d205211608b18",
    ("gp", 1): "2e6798174e3491fc42bd70fef7ed998b84d889007a70c30a03a9e06ef9f2f908",
    ("gp", 2): "e9a56ce0fe4858caa3fafefc4086396d48e53f3422d251ce9ebfb04977e9f8d9",
    ("gp", 3): "e307b3aa715efd2dffb63900ac5ba002d7fd8351ad61256224e557d726801e0b",
    ("extra-gp", 0): "12fc4eb0cce7c3326c78e5f33934f4101813ad9d3229d2741333c88bf1455a01",
    ("extra-gp", 1): "b2867d97c2452c1bb03304cdea195526e8a926f2f810f3cd03f22cba11e53412",
    ("extra-gp", 2): "d6981dad92bff6c8bf2f3b4c7f6e1c835af1f4950b1a3525a62cb91c22c0a0a2",
    ("extra-gp", 3): "2a20df63be79cf36c0ee8ebc711ad69b3a617c874bad4ef550dcc4416922cd43",
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
                    stderr=subprocess.STDOUT, text=True, timeout=45,
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
