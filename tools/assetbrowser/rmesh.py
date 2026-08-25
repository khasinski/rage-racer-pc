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
MATERIAL_SCROLL_U = 1 << 31
MATERIAL_TERRAIN_FAR_VISIBLE = 1 << 30
MATERIAL_METADATA = 1 << 29
MATERIAL_TERRAIN_ENV_CLUT = 1 << 28
MATERIAL_DEPTH_BIAS_SHIFT = 16
HEADER = struct.Struct("<8sIIII")
VERTEX = struct.Struct("<3f3f4B2fI")


def _uv(face, corner):
    if not face.uv:
        return 0.0, 0.0
    u, v = face.uv[corner]
    # Integer PS1 UVs address texel centres. Mapping them to texel edges made
    # linear sampling blend every lookup with an unrelated neighbour.
    return (u + 0.5) / 256.0, (v + 0.5) / 256.0


def bank_to_bytes(bank, textures=None, scrolling_primitives=False,
                  terrain_primitives=False) -> bytes:
    """Encode a parsed bank as a portable indexed triangle stream."""
    texture_keys = {(item["tpage"], item["clut"],
                     tuple(item["texwin"]) if item.get("texwin") else None): index
                    for index, item in enumerate(textures or [])}
    vertices, indices, mesh_offsets = [], [], [0]
    for model in bank.models:
        for face in model.faces:
            first = len(vertices)
            color = face.rgb or (255, 255, 255)
            material = texture_keys.get((face.tpage, face.clut, face.texwin),
                                        0xFFFFFFFF)
            if (face.otbias != 0 or
                    (terrain_primitives and
                     (getattr(face, "flags", 0) & 2 or face.prim < 2))):
                material_index = 0xFFFF if material == 0xFFFFFFFF else material
                material = (material_index | MATERIAL_METADATA |
                            ((face.otbias & 0xFF) <<
                             MATERIAL_DEPTH_BIAS_SHIFT))
                if terrain_primitives and getattr(face, "flags", 0) & 2:
                    material |= MATERIAL_TERRAIN_FAR_VISIBLE
                if terrain_primitives and face.prim < 2:
                    material |= MATERIAL_TERRAIN_ENV_CLUT
            if (scrolling_primitives and face.prim == 3 and
                    (material & 0xFFFF) != 0xFFFF):
                material |= MATERIAL_SCROLL_U
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


def write_bank(path: Path, bank, textures=None,
               scrolling_primitives=False, terrain_primitives=False) -> None:
    path.write_bytes(bank_to_bytes(bank, textures, scrolling_primitives,
                                   terrain_primitives))
