#!/usr/bin/env python3
"""Prove that a live race reaches the native renderer asset path."""

from __future__ import annotations

import os
import re
import struct
import subprocess
import sys
import tempfile
import zlib
from pathlib import Path


HEADER = struct.Struct("<8sIIII")
VERTEX = struct.Struct("<3f3f4B2fI")


def write_test_png(path: Path, width: int, height: int) -> None:
    def chunk(kind: bytes, payload: bytes) -> bytes:
        return (struct.pack(">I", len(payload)) + kind + payload +
                struct.pack(">I", zlib.crc32(kind + payload) & 0xFFFFFFFF))

    rows = b"".join(
        b"\0" + bytes((32, 192, 255, 255)) * width
        for _ in range(height)
    )
    path.write_bytes(
        b"\x89PNG\r\n\x1a\n" +
        chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)) +
        chunk(b"IDAT", zlib.compress(rows)) +
        chunk(b"IEND", b"")
    )


def write_test_mesh(path: Path) -> None:
    mesh_count = 1024
    vertices = []
    indices = []
    offsets = [0]
    for mesh in range(mesh_count):
        first = len(vertices)
        x = float(mesh * 4)
        vertices.extend((
            (x - 1.0, 0.0, 0.0, 0.0, 1.0, 0.0,
             255, 255, 255, 255, 0.0, 0.0, 0),
            (x + 1.0, 0.0, 0.0, 0.0, 1.0, 0.0,
             255, 255, 255, 255, 1.0, 0.0, 0),
            (x, 2.0, 0.0, 0.0, 1.0, 0.0,
             255, 255, 255, 255, 0.5, 1.0, 0),
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
        material_map = root / "material.rmat"
        material_pixels = root / "material.rgba"
        material_paint = root / "material.rpaint"
        mod_root = root / "mod"
        scenario = root / "scenario.ini"
        write_test_mesh(mesh)
        material_map.write_text(
            "# rage-rmat v5\n0 " +
            " ".join(["material.rgba"] * 96) +
            " | material.rpaint\n", encoding="ascii")
        material_pixels.write_bytes(bytes((255, 255, 255, 255)) * (256 * 256))
        material_paint.write_bytes(bytes((1,)) * (256 * 256))
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
        index = "# rage-rmesh-index v2\n" + "".join(
            f"{key} model car.rmesh material.rmat\n" for key in range(10, 75))
        index += (
            "88 track-model-1 car.rmesh material.rmat\n"
            "88 track-model-2 car.rmesh material.rmat\n"
            "88 course car.rmesh material.rmat\n"
            "88 terrain car.rmesh material.rmat\n"
        )
        (root / "runtime-index.txt").write_text(index, encoding="ascii")
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
            r"cached=(\d+) vertices=(\d+) spans=(\d+)", result.stdout)
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
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
