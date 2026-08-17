#!/usr/bin/env python3
"""Boot every GP/Extra GP course combination through the scenario API."""

import os
import hashlib
import subprocess
import sys
import tempfile
from pathlib import Path

CLASSIC_GOLDENS = {
    ("gp", 0): "2d8878fa87fb454a2813330a95141cb6e6274deed331bf6bbd6af5c24f30dfa5",
    ("gp", 1): "dd477bc2e237178e5b026c05270f023ee50cf3c3652f866f61bf2ed32fcbd54a",
    ("gp", 2): "6ad14caf8d2f30aa68a37c56c243e12fcf52ead984343a8af91c0550e6008e22",
    ("gp", 3): "bf02b16a1a2679c97199244a2f1b7381851fbf7d757ef90cabfddd15119b9a00",
    ("extra-gp", 0): "f17206b6bb5636e0aaeb38d7b9a7d16ffdb0cf47972029cc4e194f9b358d9f90",
    ("extra-gp", 1): "0c047943a70dad84d152a6563f41d33232f45a32c0ebc1355aa4fc320fd12078",
    ("extra-gp", 2): "47d626a98dda4abe7439ae732acc0b6f40b30e826ed58c17e60990de1aeab034",
    ("extra-gp", 3): "e7cc913cd5e6088f6bfc452af9aab8691f65a7d9bd7bc37194cb8c21b8f8f374",
}


def main() -> int:
    executable, source = map(Path, sys.argv[1:3])
    environment = os.environ.copy()
    environment["SDL_AUDIODRIVER"] = "dummy"
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
timer = 1

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
                if digest != CLASSIC_GOLDENS[(series, course)]:
                    raise AssertionError(
                        f"classic pixels changed for {series}/{course}: {digest}"
                    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
