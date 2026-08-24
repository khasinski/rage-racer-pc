"""Compact renderer-runtime mesh cache emitted alongside the open glTF export.

The cache has no PS1 packet, GTE, CLUT or texture-window state.  It is
deliberately boring: a table of mesh ranges followed by indexed, conventional
vertices.  Texture pixels are emitted as adjacent `*.rgba` files by extract.
The native modern backend can load it without embedding a JSON/glTF parser.
"""

from __future__ import annotations

import struct
from pathlib import Path


MAGIC = b"RRMESH1\0"
VERSION = 1
HEADER = struct.Struct("<8sIIII")
VERTEX = struct.Struct("<3f3f4B2fI")


def _uv(face, corner):
    if not face.uv:
        return 0.0, 0.0
    u, v = face.uv[corner]
    if face.texwin is not None:
        width_u, width_v, off_u, off_v = face.texwin
        u = (u % width_u) + off_u
        v = (v % width_v) + off_v
    return u / 256.0, v / 256.0


def bank_to_bytes(bank, textures=None) -> bytes:
    """Encode a parsed bank as a portable indexed triangle stream."""
    texture_keys = {(item["tpage"], item["clut"]): index
                    for index, item in enumerate(textures or [])}
    vertices, indices, mesh_offsets = [], [], [0]
    for model in bank.models:
        for face in model.faces:
            first = len(vertices)
            color = face.rgb or (255, 255, 255)
            material = texture_keys.get((face.tpage, face.clut), 0xFFFFFFFF)
            for corner, vertex_index in enumerate(face.v):
                x, y, z = bank.vertices[vertex_index]
                if face.n:
                    nx, ny, nz = bank.normals[face.n[corner]]
                else:
                    nx, ny, nz = 0, 1, 0
                u, v = _uv(face, corner)
                # The same PS1 -> conventional coordinate conversion as glTF.
                vertices.append((float(x), float(-y), float(-z),
                                 float(nx), float(-ny), float(-nz),
                                 color[0], color[1], color[2], 255,
                                 u, v, material))
            indices.extend((first, first + 2, first + 1,
                            first + 1, first + 2, first + 3))
        mesh_offsets.append(len(indices))

    out = bytearray(HEADER.pack(MAGIC, VERSION, len(bank.models),
                                len(vertices), len(indices)))
    out.extend(struct.pack(f"<{len(mesh_offsets)}I", *mesh_offsets))
    for vertex in vertices:
        out.extend(VERTEX.pack(*vertex))
    out.extend(struct.pack(f"<{len(indices)}I", *indices))
    return bytes(out)


def write_bank(path: Path, bank, textures=None) -> None:
    path.write_bytes(bank_to_bytes(bank, textures))

