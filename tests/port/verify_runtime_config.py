#!/usr/bin/env python3
"""INI scenarios and renderer switching work without RAGE_PORT_* options."""

import os
import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "support"))

from native_asset_fixture import create_native_asset_fixture


def main() -> int:
    executable, source = map(Path, sys.argv[1:3])
    with tempfile.TemporaryDirectory(prefix="rage-runtime-config-") as directory:
        root = Path(directory)
        native_assets = root / "native-assets"
        native_assets.mkdir()
        create_native_asset_fixture(native_assets)
        scenario = root / "scenario.ini"
        scenario.write_text(
            f"""[race]
mode = grand-prix
series = extra-gp
class = 2
course = 1
car = 4

[video]
renderer = classic
toggle_renderer_key = F9

[modern]
assets = {native_assets}

[run]
frames = 2700

[stop]
scene = 12
timer = 100

[hooks]
toggle_renderer_frame = 230
""",
            encoding="utf-8",
        )
        environment = os.environ.copy()
        environment["SDL_AUDIODRIVER"] = "dummy"
        for key in tuple(environment):
            if key.startswith("RAGE_PORT_"):
                del environment[key]
        result = subprocess.run(
            [executable, "--scenario", scenario], cwd=source, env=environment,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
            timeout=150,
        )
    if result.returncode != 0:
        print(result.stdout, file=sys.stderr)
        return result.returncode or 1
    required = (
        "renderer toggle=F9; active=classic",
        "renderer switched to modern",
        "scenario mode=grand-prix series=extra-gp class=2 course=1 car=4",
        "scene 12",
    )
    missing = [entry for entry in required if entry not in result.stdout]
    if missing:
        raise AssertionError(f"runtime configuration missed {missing}\n{result.stdout}")
    match = __import__("re").search(r"capture_faces=(\d+)", result.stdout)
    if match is None or int(match.group(1)) == 0:
        raise AssertionError(
            "classic-to-modern toggle did not activate semantic capture\n"
            + result.stdout
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
