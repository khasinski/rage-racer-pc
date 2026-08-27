"""Rage Racer model-bank parser.

Layout established from the game's own code, not guessed:

  src/main/PAL/main/asset/model_banks.c   RegisterModelBank / SelectModelBank
  asm/.../gte/submit_terrain_cells/SubmitModel.s      the batch walker
  src/main/PAL/main/render/terrain_submission.c:145+   SubmitModelFaces
  asm/.../submit_terrain_cells/EmitPoly{F4,FT4Raw,G4,GT4}.s   the four emitters
  jtbl_8007DA14 (asm/PAL/main/data/main/6BE64.data.s) type -> {emitter, stride}

Bank:
    bank[0]            model count N
    bank[1]            offset of the shared VERTEX pool   (SPAD_MODEL_TABLE1)
    bank[2]            offset of the shared NORMAL pool   (SPAD_MODEL_NORMALS)
    bank[3 .. 3+N-1]   offset of each model
  All offsets are relative to `bank` and are turned into absolute pointers by
  RegisterModelBank. Vertices and normals are 8 bytes each: SVECTOR
  { s16 x, y, z, pad }; SubmitModelFaces indexes both with `index << 3`.

Model = a zero-terminated list of batches:
    u16 type ; u16 faceCount ; faceCount * record[stride[type]]
    ... until a whole zero word.

    type 0  POLY_F4   stride 0x10
    type 1  POLY_FT4  stride 0x18
    type 2  POLY_G4   stride 0x18
    type 3  POLY_GT4  stride 0x20

Every record starts with four u16 vertex indices and ends with a signed byte
OT bias (a per-face depth nudge) at stride-3.
"""

from __future__ import annotations

import struct
from dataclasses import dataclass, field

STRIDE = {0: 0x10, 1: 0x18, 2: 0x18, 3: 0x20}
PRIM_NAME = {0: "F4", 1: "FT4", 2: "G4", 3: "GT4"}


def prim_name(p):
    return PRIM_NAME.get(p, f"type{p}")


def terrain_fixed_clut_offset(primitive_mode: int) -> int:
    """Return the palette-row offset encoded by fixed terrain modes 2..5."""
    return (primitive_mode - 2) & 1 if primitive_mode >= 2 else 0


@dataclass
class Face:
    prim: int                    # 0..3
    v: tuple                     # 4 vertex indices
    n: tuple | None = None       # 4 normal indices (G4 / GT4)
    rgb: tuple | None = None     # (r, g, b) flat colour (F4 / G4)
    uv: tuple | None = None      # ((u0,v0)..(u3,v3)) (FT4 / GT4)
    clut: int = 0
    tpage: int = 0
    otbias: int = 0
    # (widthU, widthV, offU, offV) of the GP0 0xE2 texture window, or None for
    # the whole 256x256 page. See texture_window() for where it comes from.
    texwin: tuple | None = None
    # Terrain records may carry a LINE_F3 colour command at +0x18. Retail
    # traces the four parent-quad edges behind the subdivided surface as a
    # one-pixel crack underlay. It is diagnostic metadata, not native mesh
    # geometry or a road-marking material.
    line_rgb: tuple | None = None
    line_lod: tuple | None = None


def texture_window(word: int) -> tuple | None:
    """Decode a GP0 0xE2 command into (widthU, widthV, offU, offV) in texels.

    The hardware rule is

        texcoord = (texcoord & ~(mask * 8)) | ((offset & mask) * 8)

    with mask/offset in 8-texel units at bits 0-4 / 5-9 / 10-14 / 15-19. Every
    mask on this disc (0, 16, 24, 28, 30, 31 - all 104795 of them) is a run of
    high bits, so `& ~(mask * 8)` is exactly `mod (256 - mask * 8)` and the
    window is a power-of-two tile that the coordinate wraps inside. Returning
    it as a width plus an origin lets the renderer do the wrap with a mod.
    """
    if (word >> 24) != 0xE2:
        return None
    w = word & 0xFFFFF
    mu, mv = w & 0x1F, (w >> 5) & 0x1F
    ou, ov = (w >> 10) & 0x1F, (w >> 15) & 0x1F
    if mu == 0 and mv == 0:
        return None
    return (256 - mu * 8, 256 - mv * 8, (ou & mu) * 8, (ov & mv) * 8)


@dataclass
class Model:
    index: int
    offset: int
    faces: list = field(default_factory=list)
    batches: list = field(default_factory=list)  # (type, count)


@dataclass
class Bank:
    count: int
    vertex_off: int
    normal_off: int
    model_offs: list
    vertices: list = field(default_factory=list)
    normals: list = field(default_factory=list)
    models: list = field(default_factory=list)


class ParseError(Exception):
    pass


def _svectors(buf: bytes, start: int, end: int) -> list:
    n = (end - start) // 8
    out = []
    for i in range(n):
        x, y, z, _pad = struct.unpack_from("<4h", buf, start + i * 8)
        out.append((x, y, z))
    return out


def parse_model(buf: bytes, start: int, limit: int) -> Model:
    m = Model(index=-1, offset=start)
    pos = start
    while True:
        if pos + 4 > limit:
            raise ParseError(f"batch header runs past the model at {pos:#x}")
        prim, count = struct.unpack_from("<HH", buf, pos)
        pos += 4
        if count == 0:
            break
        if prim not in STRIDE:
            raise ParseError(f"unknown primitive type {prim} at {pos - 4:#x}")
        stride = STRIDE[prim]
        if pos + count * stride > limit:
            raise ParseError(f"batch of {count} x{stride:#x} at {pos:#x} overruns")
        m.batches.append((prim, count))
        for _ in range(count):
            r = buf[pos : pos + stride]
            v = struct.unpack_from("<4H", r, 0)
            f = Face(prim=prim, v=v, otbias=struct.unpack_from("<b", r, stride - 3)[0])
            if prim == 0:
                f.rgb = (r[8], r[9], r[10])
            elif prim == 1:
                f.uv = (
                    (r[8], r[9]),
                    (r[0x0C], r[0x0D]),
                    (r[0x10], r[0x11]),
                    (r[0x12], r[0x13]),
                )
                f.clut = struct.unpack_from("<H", r, 0x0A)[0]
                f.tpage = struct.unpack_from("<H", r, 0x0E)[0]
            elif prim == 2:
                f.n = struct.unpack_from("<4H", r, 8)
                f.rgb = (r[0x10], r[0x11], r[0x12])
            elif prim == 3:
                f.n = struct.unpack_from("<4H", r, 8)
                f.uv = (
                    (r[0x10], r[0x11]),
                    (r[0x14], r[0x15]),
                    (r[0x18], r[0x19]),
                    (r[0x1A], r[0x1B]),
                )
                f.clut = struct.unpack_from("<H", r, 0x12)[0]
                f.tpage = struct.unpack_from("<H", r, 0x16)[0]
            m.faces.append(f)
            pos += stride
    m.end = pos
    return m


def parse_bank(buf: bytes, base: int, limit: int | None = None) -> Bank:
    """Parse the model bank whose word 0 sits at `base`."""
    if limit is None:
        limit = len(buf)
    if base + 4 > len(buf):
        raise ParseError("bank base out of range")
    count = struct.unpack_from("<I", buf, base)[0]
    if not (0 < count < 4096):
        raise ParseError(f"implausible model count {count}")
    need = base + 4 + (count + 2) * 4
    if need > len(buf):
        raise ParseError("bank header out of range")
    words = struct.unpack_from(f"<{count + 2}I", buf, base + 4)
    bank = Bank(count, words[0], words[1], list(words[2:]))

    # Offsets are bank-relative. In practice models come first, then the vertex
    # pool, then the normal pool; the parser asserts that rather than assuming.
    offs = sorted(set(bank.model_offs + [bank.vertex_off, bank.normal_off]))
    for o in offs:
        if base + o > len(buf):
            raise ParseError("bank offset out of range")

    def next_off(o):
        for c in offs:
            if c > o:
                return base + c
        return limit

    for i, o in enumerate(bank.model_offs):
        m = parse_model(buf, base + o, next_off(o))
        m.index = i
        bank.models.append(m)

    bank.vertices = _svectors(buf, base + bank.vertex_off, next_off(bank.vertex_off))
    bank.normals = _svectors(buf, base + bank.normal_off, next_off(bank.normal_off))

    # Cheap consistency check: no face may index past a pool.
    nv, nn = len(bank.vertices), len(bank.normals)
    for m in bank.models:
        for f in m.faces:
            if max(f.v) >= nv:
                raise ParseError(f"vertex index {max(f.v)} >= {nv}")
            if f.n and max(f.n) >= nn:
                raise ParseError(f"normal index {max(f.n)} >= {nn}")
    return bank


# --------------------------------------------------------------------------
# Course objects (RegisterCourseModels + SubmitCourseModel / TransformCourseModel).
#
#   record (12 bytes, two of the three words relocated):
#       u32 vertexListOffset, u32 vertexCount, u32 faceListOffset
#   vertices are the same 8-byte SVECTOR; each object has its OWN list.
#   The face list is the same batch shape, with its own strides:
#       0 F4 0x10   1 FT4 0x1C   2 subdivided-FT4 0x20   3 scrolling-FT4 0x20
# --------------------------------------------------------------------------

COURSE_STRIDE = {0: 0x10, 1: 0x1C, 2: 0x20, 3: 0x20}
COURSE_OTBIAS = {0: 0x0D, 1: 0x19, 2: 0x19, 3: 0x19}
COURSE_PRIM = {0: "F4", 1: "FT4", 2: "FT4-sub", 3: "FT4-scroll"}
# The two subdividing course emitters carry the texture window in the word the
# extra four bytes of stride buy them, exactly as the terrain ones do.
COURSE_TEXWIN = {2: 0x1C, 3: 0x1C}


def _uvface(prim, r, base_uv, base_rgb, otoff, winoff=None):
    f = Face(prim=prim, v=struct.unpack_from("<4H", r, 0),
             otbias=struct.unpack_from("<b", r, otoff)[0])
    if winoff is not None:
        f.texwin = texture_window(struct.unpack_from("<I", r, winoff)[0])
    if base_rgb is not None:
        f.rgb = (r[base_rgb], r[base_rgb + 1], r[base_rgb + 2])
    if base_uv is not None:
        f.uv = ((r[base_uv], r[base_uv + 1]),
                (r[base_uv + 4], r[base_uv + 5]),
                (r[base_uv + 8], r[base_uv + 9]),
                (r[base_uv + 10], r[base_uv + 11]))
        f.clut = struct.unpack_from("<H", r, base_uv + 2)[0]
        f.tpage = struct.unpack_from("<H", r, base_uv + 6)[0]
    return f


def parse_batches(buf, start, limit, strides, mkface):
    faces, batches, pos = [], [], start
    while True:
        if pos + 4 > limit:
            raise ParseError(f"batch header past end at {pos:#x}")
        prim, count = struct.unpack_from("<HH", buf, pos)
        pos += 4
        if count == 0:
            break
        if prim not in strides:
            raise ParseError(f"unknown primitive {prim} at {pos - 4:#x}")
        s = strides[prim]
        if pos + count * s > limit:
            raise ParseError(f"batch {prim}x{count} at {pos:#x} overruns")
        batches.append((prim, count))
        for _ in range(count):
            faces.append(mkface(prim, buf[pos : pos + s]))
            pos += s
    return faces, batches, pos


def parse_course_objects(buf: bytes, base: int, limit: int | None = None) -> Bank:
    """All course objects merged into one pseudo-bank with a concatenated pool."""
    if limit is None:
        limit = len(buf)
    count = struct.unpack_from("<I", buf, base)[0]
    if not (0 < count < 4096):
        raise ParseError(f"implausible course object count {count}")
    bank = Bank(count, 0, 0, [])
    for i in range(count):
        voff, vcount, foff = struct.unpack_from("<3I", buf, base + 4 + i * 12)
        if vcount > 4096 or base + voff + vcount * 8 > len(buf):
            raise ParseError(f"course object {i}: {vcount} vertices at {voff:#x}")
        first = len(bank.vertices)
        bank.vertices += _svectors(buf, base + voff, base + voff + vcount * 8)
        faces, batches, end = parse_batches(
            buf, base + foff, limit,
            COURSE_STRIDE,
            lambda p, r: _uvface(p, r, None if p == 0 else 0x0C, 8, COURSE_OTBIAS[p],
                                 COURSE_TEXWIN.get(p)),
        )
        for f in faces:
            if max(f.v) >= vcount:
                raise ParseError(f"course object {i}: vertex index {max(f.v)} >= {vcount}")
            f.v = tuple(x + first for x in f.v)
        m = Model(index=i, offset=foff, faces=faces, batches=batches)
        bank.models.append(m)
        bank.model_offs.append(foff)
    return bank


# --------------------------------------------------------------------------
# Terrain cells (InstallTerrainCellData + SubmitTerrainCells / ...CellFaces).
#
#   base + 0x0000  32x32 u16 cell grid: bits 0..9 cell index (0x3FF = empty),
#                  bits 10..15 region id. Row lookup is (31 - y) * 32 + x.
#   base + 0x0800  32x32 u32 region visibility bitmask
#   base + 0x1800  u32 cellCount, u32 vertexPoolOffset, u32 cellOffset[cellCount]
#
#   Each cell offset points at the same zero-terminated batch list. All cells
#   index one shared 8-byte-per-element vertex pool.
#
#   Stride comes from jtbl_8007D9F4 = {0x20, 0x20, 0x24, 0x24}, but the batch
#   type is not the table index. SubmitTerrainCellFaces at 0x80028120:
#       t7 = type - 2; if (t7 >= 0) index = t7;
#       else            index = type * 2 + SPAD[0x84]   (the env-mode-4 flag)
#   so type 0 -> entry 0 or 1, type 1 -> entry 2 or 3, and types 2..5 map
#   straight onto entries 0..3. Both members of each env-mode pair share a
#   stride, which is what makes the record walkable offline. Types 4 and 5 do
#   occur on the disc; entries 2 and 3 carry the extra GP0 0xE2 texture-window
#   word at +0x20, which is exactly the four bytes their stride adds.
#
#   That window is not decoration. SubmitTerrainCellFaces (0x80028624 and
#   0x8002877C) emits each such quad as a three-link packet chain:
#
#       +0x34  len 2  ->  +0x0C     E2 <window>            set the window
#       +0x0C  len 9  ->  +0x00     the POLY_FT4           draw
#       +0x00  len 2  ->  OT next   E2 00000000 + NOP      put it back
#
#   so the window is live for exactly this one quad, and every 0x20-stride quad
#   therefore draws through a full 256x256 window. Ignoring it moves 95% of the
#   UV corners on BIG1 to a different texel - it is what makes the road look
#   like masonry. The word is used as stored: the LOD path at 0x800283FC halves
#   the window AND the four UV pairs together, but only when (flags & 1) and
#   the quad is beyond z 0x800, so the stored pair is the near view.
# --------------------------------------------------------------------------

TERRAIN_STRIDE = {0: 0x20, 1: 0x24, 2: 0x20, 3: 0x20, 4: 0x24, 5: 0x24}
GRID_SIZE = 0x800
VIS_SIZE = 0x1000


def parse_terrain(buf: bytes, base: int, limit: int | None = None):
    if limit is None:
        limit = len(buf)
    grid = struct.unpack_from("<1024H", buf, base)
    tbl = base + GRID_SIZE + VIS_SIZE
    count, vpool = struct.unpack_from("<2I", buf, tbl)
    if not (0 < count < 1024):
        raise ParseError(f"implausible cell count {count}")
    offs = struct.unpack_from(f"<{count}I", buf, tbl + 8)
    bank = Bank(count, vpool, 0, list(offs))
    vstart = tbl + vpool
    bank.vertices = _svectors(buf, vstart, limit)

    def mk(p, r):
        f = _uvface(p, r, 0x08, 0x1C, 0x15,
                    0x20 if TERRAIN_STRIDE[p] == 0x24 else None)
        f.flags = r[0x14]
        line_command = struct.unpack_from("<I", r, 0x18)[0]
        # Only GP0 polyline opcodes describe visible authored edges. Zero and
        # other non-command words still pass retail's sign-bit branch, but the
        # PS1 GPU treats them as NOPs.
        if ((line_command >> 24) & 0xFC) == 0x48:
            f.line_rgb = (r[0x18], r[0x19], r[0x1A])
            f.line_lod = (r[0x16], r[0x17])
        # SubmitTerrainCellFaces maps stored modes 2..5 to dispatches 0..3;
        # odd dispatches select the adjacent CLUT row. Modes 0/1 depend on
        # the live environment-mode bit and remain dynamic runtime metadata.
        f.clut += terrain_fixed_clut_offset(p)
        return f

    # Each cell must consume exactly the bytes up to the next cell's offset.
    # With 198 cells per track that is a strong check on the stride table.
    bounds = sorted(set(offs) | {vpool})
    for i, o in enumerate(offs):
        stop = next(c for c in bounds if c > o)
        faces, batches, end = parse_batches(buf, tbl + o, vstart, TERRAIN_STRIDE, mk)
        if end != tbl + stop:
            raise ParseError(
                f"cell {i} ended at {end - tbl:#x}, next cell starts at {stop:#x}")
        for f in faces:
            if max(f.v) >= len(bank.vertices):
                raise ParseError(f"cell {i}: vertex index {max(f.v)} out of pool")
        bank.models.append(Model(index=i, offset=o, faces=faces, batches=batches))
    return bank, grid


def bank_to_json(bank: Bank) -> dict:
    return {
        "modelCount": bank.count,
        "vertexCount": len(bank.vertices),
        "normalCount": len(bank.normals),
        "vertices": [c for v in bank.vertices for c in v],
        "normals": [c for v in bank.normals for c in v],
        "models": [
            {
                "index": m.index,
                "offset": m.offset,
                "batches": [{"prim": p, "name": prim_name(p), "count": c} for p, c in m.batches],
                "faceCount": len(m.faces),
                "faces": [
                    {
                        "p": f.prim,
                        "v": list(f.v),
                        **({"n": list(f.n)} if f.n else {}),
                        **({"c": list(f.rgb)} if f.rgb else {}),
                        **(
                            {
                                "uv": [c for pair in f.uv for c in pair],
                                "clut": f.clut,
                                "tp": f.tpage,
                            }
                            if f.uv
                            else {}
                        ),
                        **({"tw": list(f.texwin)} if f.texwin else {}),
                        **({"line": list(f.line_rgb)} if f.line_rgb else {}),
                        **({"lineLod": list(f.line_lod)} if f.line_lod else {}),
                    }
                    for f in m.faces
                ],
            }
            for m in bank.models
        ],
    }
