"""Small, complete native-asset cache used by renderer integration tests."""

from __future__ import annotations

import struct
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


def _write_test_mesh(path: Path) -> None:
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


def create_native_asset_fixture(root: Path) -> None:
    """Create the minimum versioned cache accepted by every native scene."""
    _write_test_mesh(root / "car.rmesh")
    (root / "material.rmat").write_text(
        "# rage-rmat v5\n0 " +
        " ".join(["material.rgba"] * 96) +
        " | material.rpaint\n",
        encoding="ascii",
    )
    (root / "material.rgba").write_bytes(
        bytes((255, 255, 255, 255)) * (256 * 256))
    (root / "material.rpaint").write_bytes(bytes((1,)) * (256 * 256))

    index = "# rage-rmesh-index v2\n" + "".join(
        f"{key} model car.rmesh material.rmat\n" for key in range(10, 75))
    for key in range(88, 136, 2):
        index += (
            f"{key} track-model-1 car.rmesh material.rmat\n"
            f"{key} track-model-2 car.rmesh material.rmat\n"
            f"{key} course car.rmesh material.rmat\n"
            f"{key} terrain car.rmesh material.rmat\n"
        )
    (root / "runtime-index.txt").write_text(index, encoding="ascii")
