#!/usr/bin/env python3
"""Resize and fullscreen transitions keep the modern presentation alive."""

import os
import subprocess
import sys
import tempfile
import time
from pathlib import Path


def xdo(*arguments: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        ["xdotool", *arguments], stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT, text=True, timeout=15,
    )


def main() -> int:
    executable, source = map(Path, sys.argv[1:3])
    with tempfile.TemporaryDirectory(prefix="rage okno ąę ") as directory:
        scenario = Path(directory) / "zmiana okna.ini"
        scenario.write_text(
            """[video]
renderer = modern

[diagnostics]
renderer_lifecycle = true

[race]
enabled = true
mode = grand-prix
series = gp
class = 0
course = 0
car = 3

[run]
frames = 2500

[stop]
scene = 12
timer = 120
""",
            encoding="utf-8",
        )
        environment = os.environ.copy()
        environment["SDL_AUDIODRIVER"] = "dummy"
        process = subprocess.Popen(
            [executable, "--scenario", scenario], cwd=source, env=environment,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
        )
        window = ""
        for _ in range(60):
            result = xdo("search", "--name", "Rage Racer")
            if result.returncode == 0 and result.stdout.strip():
                window = result.stdout.splitlines()[-1]
                break
            if process.poll() is not None:
                break
            time.sleep(0.1)
        if not window:
            process.kill()
            output, _ = process.communicate()
            raise AssertionError(f"game window was not found\n{output}")
        for width, height in ((800, 600), (1024, 576), (640, 480)):
            result = xdo("windowsize", window, str(width), str(height))
            if result.returncode != 0:
                process.kill()
                raise AssertionError(result.stdout)
        for _ in range(2):
            xdo("key", "--window", window, "F4")
            time.sleep(0.2)
        try:
            output, _ = process.communicate(timeout=165)
        except subprocess.TimeoutExpired:
            process.kill()
            output, _ = process.communicate()
            raise AssertionError(f"game hung after window transitions\n{output}")
        if process.returncode != 0 or "scene 12" not in output:
            raise AssertionError(f"window transitions broke the race\n{output}")
        if output.count("modern resources created generation=") != 1:
            raise AssertionError(f"modern target was not created exactly once\n{output}")
        if "resource setup failed" in output or "failed to enter fullscreen" in output:
            raise AssertionError(f"GPU/window lifecycle reported an error\n{output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
