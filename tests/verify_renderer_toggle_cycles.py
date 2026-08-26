#!/usr/bin/env python3
"""Exercise repeated classic/modern switches during a live race."""

import os
import subprocess
import sys
import tempfile
from pathlib import Path

from native_asset_fixture import create_native_asset_fixture


def main() -> int:
    executable, source = map(Path, sys.argv[1:3])
    with tempfile.TemporaryDirectory(prefix="rage przełączanie ") as directory:
        root = Path(directory)
        native_assets = root / "native assets"
        native_assets.mkdir()
        create_native_asset_fixture(native_assets)
        scenario = root / "renderer żółty.ini"
        scenario.write_text(
            f"""[race]
enabled = true
mode = grand-prix
series = gp
class = 0
course = 0
car = 3

[video]
renderer = classic

[modern]
assets = {native_assets}

[run]
frames = 2450

[stop]
scene = 12
timer = 250

[hooks]
toggle_renderer_frames = 300,340,380,420

[diagnostics]
renderer_lifecycle = true
""",
            encoding="utf-8",
        )
        environment = os.environ.copy()
        environment["SDL_AUDIODRIVER"] = "dummy"
        result = subprocess.run(
            [executable, "--scenario", scenario], cwd=source, env=environment,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
            timeout=55,
        )
    if result.returncode != 0:
        print(result.stdout, file=sys.stderr)
        return result.returncode or 1
    switches = [
        line.rsplit(" ", 1)[-1]
        for line in result.stdout.splitlines()
        if "renderer switched to " in line
    ]
    if switches != ["modern", "classic", "modern", "classic"]:
        raise AssertionError(f"unexpected renderer cycle: {switches}\n{result.stdout}")
    if "scene 12" not in result.stdout:
        raise AssertionError("renderer cycling did not reach the live race")
    created = result.stdout.count("modern resources created generation=")
    destroyed = result.stdout.count("modern resources destroyed generation=")
    if (created, destroyed) != (2, 2):
        raise AssertionError(
            f"renderer resources were not recreated cleanly: "
            f"created={created}, destroyed={destroyed}\n{result.stdout}"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
