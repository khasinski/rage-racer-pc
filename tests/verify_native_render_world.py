#!/usr/bin/env python3
"""Prove that a live race reaches the native renderer asset path."""

from __future__ import annotations

import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path

from native_asset_fixture import create_native_asset_fixture, write_test_png


def main() -> int:
    executable, source = map(Path, sys.argv[1:3])
    with tempfile.TemporaryDirectory(prefix="rage-native-world-") as directory:
        root = Path(directory)
        mod_root = root / "mod"
        scenario = root / "scenario.ini"
        create_native_asset_fixture(root)
        (mod_root / "textures").mkdir(parents=True)
        write_test_png(mod_root / "textures" / "terrain.png", 64, 32)
        (mod_root / "mod.toml").write_text(
            """[mod]
id = "native-world-test"

[textures]
"track.big1.terrain.material.0" = "textures/terrain.png"
""",
            encoding="ascii",
        )
        scenario.write_text(
            """[video]
renderer = modern

[race]
enabled = true
mode = grand-prix
class = 0
course = 0
car = 3

[run]
frames = 900

[stop]
scene = 12
timer = 20
""",
            encoding="ascii",
        )
        environment = os.environ.copy()
        environment.update(
            SDL_AUDIODRIVER="dummy",
            RAGE_PORT_MODERN_ASSETS=str(root),
            RAGE_PORT_MODERN_ASSET_TRACE="1",
            RAGE_PORT_MODS_DIRECTORY=str(mod_root),
        )
        result = subprocess.run(
            [executable, "--scenario", scenario], cwd=source, env=environment,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
            timeout=35,
        )
        if result.returncode != 0:
            print(result.stdout, file=sys.stderr)
            return result.returncode or 1
        if "native GPU pipeline ready" not in result.stdout:
            raise AssertionError("native GPU shaders or pipelines were not created")
        if ("semantic texture mod native-world-test" not in result.stdout or
                "native texture override "
                "track.big1.terrain.material.0 <- textures/terrain.png "
                "(64x32)" not in result.stdout):
            raise AssertionError(
                "semantic PNG provider did not override the PS1 cache\n" +
                result.stdout[-4000:]
            )
        if "native car paint asset=" not in result.stdout:
            raise AssertionError(
                "semantic player paint was not applied to an imported material\n" +
                result.stdout[-4000:]
            )

        matches = re.findall(
            r"native world frame=\d+ camera=(\d+) instances=(\d+) "
            r"cached=(\d+) textures=\d+ vertices=(\d+) spans=(\d+)",
            result.stdout,
        )
        if not any(all(int(value) > 0 for value in match) for match in matches):
            raise AssertionError(
                "live race did not produce a native draw stream\n" +
                result.stdout[-4000:]
            )
        draws = re.findall(
            r"native draws frame=\d+ draws=(\d+) vertices=(\d+)",
            result.stdout,
        )
        if not any(int(count) > 0 and int(vertices) > 0
                   for count, vertices in draws):
            raise AssertionError(
                "complete native world was not submitted to the GPU\n" +
                result.stdout[-4000:]
            )

        attract = subprocess.run(
            [executable,
             "--set", "video.renderer=modern",
             "--set", "race.enabled=false",
             "--set", "boot.direct=false",
             "--set", "run.frames=2500",
             "--set", "stop.scene=30",
             "--set", "stop.timer=200"],
            cwd=source, env=environment,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
            timeout=35,
        )
        if attract.returncode != 0:
            print(attract.stdout, file=sys.stderr)
            return attract.returncode or 1
        if "scene=30 timer=200" not in attract.stdout:
            raise AssertionError(
                "smoke run did not reach attract driving mode\n" +
                attract.stdout[-4000:]
            )
        shadows = re.findall(
            r"native shadow map frame=\d+ draws=(\d+) masked=(\d+)",
            attract.stdout,
        )
        if not shadows or not any(int(draws) > 0 and int(masked) > 0
                                  for draws, masked in shadows):
            raise AssertionError(
                "attract mode did not render alpha-masked car geometry into "
                "the shadow map\n" + attract.stdout[-4000:]
            )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
