#!/usr/bin/env python3
"""Prove that a live race reaches the native renderer asset path."""

from __future__ import annotations

import os
import re
import struct
import subprocess
import sys
import tempfile
from pathlib import Path


HEADER = struct.Struct("<8sIIII")
VERTEX = struct.Struct("<3f3f4B2fI")


def write_test_mesh(path: Path) -> None:
    mesh_count = 6
    vertices = []
    indices = []
    offsets = [0]
    for mesh in range(mesh_count):
        first = len(vertices)
        x = float(mesh * 4)
        vertices.extend((
            (x - 1.0, 0.0, 0.0, 0.0, 1.0, 0.0,
             255, 255, 255, 255, 0.0, 0.0, 0xFFFFFFFF),
            (x + 1.0, 0.0, 0.0, 0.0, 1.0, 0.0,
             255, 255, 255, 255, 1.0, 0.0, 0xFFFFFFFF),
            (x, 2.0, 0.0, 0.0, 1.0, 0.0,
             255, 255, 255, 255, 0.5, 1.0, 0xFFFFFFFF),
        ))
        indices.extend((first, first + 1, first + 2))
        offsets.append(len(indices))

    payload = bytearray(HEADER.pack(
        b"RRMESH1\0", 1, mesh_count, len(vertices), len(indices)))
    payload.extend(struct.pack(f"<{len(offsets)}I", *offsets))
    for vertex in vertices:
        payload.extend(VERTEX.pack(*vertex))
    payload.extend(struct.pack(f"<{len(indices)}I", *indices))
    path.write_bytes(payload)


def main() -> int:
    executable, source = map(Path, sys.argv[1:3])
    with tempfile.TemporaryDirectory(prefix="rage-native-world-") as directory:
        root = Path(directory)
        mesh = root / "car.rmesh"
        scenario = root / "scenario.ini"
        write_test_mesh(mesh)
        (root / "runtime-index.txt").write_text(
            "".join(f"{key} model car.rmesh -\n" for key in range(10, 75)),
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
        )
        result = subprocess.run(
            [executable, "--scenario", scenario], cwd=source, env=environment,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
            timeout=35,
        )
        if result.returncode != 0:
            print(result.stdout, file=sys.stderr)
            return result.returncode or 1

        matches = re.findall(
            r"native world frame=\d+ camera=(\d+) instances=(\d+) "
            r"cached=(\d+) vertices=(\d+) spans=(\d+)", result.stdout)
        if not any(all(int(value) > 0 for value in match) for match in matches):
            raise AssertionError(
                "live race did not produce a native draw stream\n" +
                result.stdout[-4000:]
            )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
