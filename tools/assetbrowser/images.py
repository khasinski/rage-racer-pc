"""Image assets and the VRAM they build.

From src/main/PAL/main/asset/image_upload.c:

  UploadImageAsset(asset):
      ptr = asset + 4                     # word 0 is skipped by the loader
      loop: size = *(s32*)ptr; ptr += 4
            while size > 0: the next (size & ~3) bytes are one chunk
      a size <= 0 ends the chain

  UploadImageBlock(chunk):
      flags = *(s32*)(chunk + 4)
      if flags & 8:  the first GameImageBlock is at chunk + 8, and the next one
                     starts at that block + (block->size & ~3)
      then one more GameImageBlock, uploaded when w > 0 and h > 0

  GameImageBlock (include/game/asset.h):
      +0x00 s32 size    block size in bytes
      +0x04 u16 x       VRAM destination, in 16-bit units
      +0x06 u16 y
      +0x08 u16 w       width in 16-bit units
      +0x0A u16 h
      +0x0C u8  pixels[]

LoadImage moves 16-bit VRAM words, so a block covers w*h*2 bytes at (x, y) of
the 1024 x 512 framebuffer. What those words *mean* is decided by the tpage of
whatever primitive samples them: 4bpp (4 texels per word), 8bpp (2 per word) or
16bpp direct.
"""

from __future__ import annotations

import struct
from dataclasses import dataclass

VRAM_W = 1024
VRAM_H = 512


@dataclass
class ImageBlock:
    size: int
    x: int
    y: int
    w: int
    h: int
    data_off: int   # absolute offset of the pixel bytes in the asset buffer
    is_clut: bool = False


def parse_image_asset(buf: bytes, base: int, limit: int | None = None):
    """Walk one image asset; return the list of blocks it would upload."""
    if limit is None:
        limit = len(buf)
    blocks: list[ImageBlock] = []
    ptr = base + 4
    guard = 0
    while True:
        guard += 1
        if guard > 4096 or ptr + 4 > limit:
            break
        size = struct.unpack_from("<i", buf, ptr)[0]
        ptr += 4
        if size <= 0:
            break
        chunk = ptr
        nxt = ptr + (size & ~3)
        if nxt > limit:
            break
        blocks.extend(_parse_chunk(buf, chunk, min(nxt, limit)))
        ptr = nxt
    return blocks


def _read_block(buf, off, limit, is_clut=False):
    if off + 12 > limit:
        return None
    size, x, y, w, h = struct.unpack_from("<IHHHH", buf, off)
    return ImageBlock(size, x, y, w, h, off + 12, is_clut)


def _parse_chunk(buf, chunk, limit):
    out = []
    if chunk + 8 > limit:
        return out
    flags = struct.unpack_from("<i", buf, chunk + 4)[0]
    pos = chunk + 8
    if flags & 8:
        b = _read_block(buf, pos, limit, is_clut=True)
        if b is None:
            return out
        out.append(b)
        pos = pos + (b.size & ~3)
    b = _read_block(buf, pos, limit)
    # The game guards on the low 16 bits being a positive s16.
    if b is not None and 0 < (b.w & 0xFFFF) < 0x8000 and 0 < (b.h & 0xFFFF) < 0x8000:
        out.append(b)
    return out


def decode_sprite(buf: bytes, img: ImageBlock, clut: ImageBlock | None):
    """Decode one image block into real pixels.

    The block header only gives a VRAM rectangle in 16-bit words; the bit depth
    is not stored with it, because on the real machine it is decided by the
    tpage of whatever primitive samples the region later. INFERRED, not proven
    from code: a chunk that carries a CLUT of 16 entries is 4 bpp and one of
    256 entries is 8 bpp, and a chunk with no CLUT is 16 bpp direct. That is
    the only reading consistent with the CLUT widths actually on the disc, and
    it produces recognisable artwork for every .TMS asset.
    """
    if clut is None:
        bpp, pal = 16, None
    elif clut.w == 16:
        bpp = 4
    elif clut.w == 256:
        bpp = 8
    else:
        return None
    if clut is not None:
        pal = []
        for i in range(clut.w):
            o = clut.data_off + i * 2
            if o + 1 >= len(buf):
                return None
            pal.append(rgb5551(buf[o] | (buf[o + 1] << 8)))

    w = img.w * (16 // bpp)
    h = img.h
    need = img.w * h * 2
    if img.data_off + need > len(buf) or w == 0 or h == 0 or w * h > 1 << 22:
        return None

    out = bytearray(w * h * 4)
    src = img.data_off
    if bpp == 4:
        tbl = [bytes(pal[b & 0xF]) + bytes(pal[b >> 4]) for b in range(256)]
    elif bpp == 8:
        tbl = [bytes(pal[b]) for b in range(256)]
    for y in range(h):
        row = buf[src : src + img.w * 2]
        src += img.w * 2
        if bpp == 16:
            line = b"".join([bytes(rgb5551(row[i] | (row[i + 1] << 8)))
                             for i in range(0, len(row) - 1, 2)])
        else:
            line = b"".join([tbl[b] for b in row])
        out[y * w * 4 : y * w * 4 + len(line)] = line
    return w, h, bytes(out), bpp


def pair_blocks(blocks):
    """Walk a block list into (image, clut) pairs; a CLUT applies to the block after it."""
    out, pending = [], None
    for b in blocks:
        if b.is_clut:
            pending = b
        else:
            out.append((b, pending))
            pending = None
    return out


class Vram:
    """1024 x 512 halfwords, exactly the PS1 framebuffer."""

    def __init__(self):
        self.buf = bytearray(VRAM_W * VRAM_H * 2)
        self.dirty: list[tuple] = []

    def clone(self):
        copy = Vram()
        copy.buf[:] = self.buf
        copy.dirty = list(self.dirty)
        return copy

    def load(self, buf: bytes, blk: ImageBlock) -> bool:
        if blk.w == 0 or blk.h == 0:
            return False
        if blk.x + blk.w > VRAM_W or blk.y + blk.h > VRAM_H:
            return False
        need = blk.w * blk.h * 2
        if blk.data_off + need > len(buf):
            return False
        src = blk.data_off
        for row in range(blk.h):
            dst = ((blk.y + row) * VRAM_W + blk.x) * 2
            self.buf[dst : dst + blk.w * 2] = buf[src : src + blk.w * 2]
            src += blk.w * 2
        self.dirty.append((blk.x, blk.y, blk.w, blk.h))
        return True

    def word(self, x: int, y: int) -> int:
        o = (y * VRAM_W + x) * 2
        return self.buf[o] | (self.buf[o + 1] << 8)


def rgb5551(word: int, semi_as_opaque=True):
    r = (word & 0x1F) << 3
    g = ((word >> 5) & 0x1F) << 3
    b = ((word >> 10) & 0x1F) << 3
    r |= r >> 5
    g |= g >> 5
    b |= b >> 5
    # PS1: an all-zero halfword is fully transparent; the top bit is semi-
    # transparency, not alpha.
    a = 0 if word == 0 else 255
    return r, g, b, a


def vram_to_rgba(vram: Vram) -> bytes:
    """A direct 16bpp view of the whole framebuffer, for the raw VRAM preview."""
    out = bytearray(VRAM_W * VRAM_H * 4)
    b = vram.buf
    for i in range(VRAM_W * VRAM_H):
        w = b[i * 2] | (b[i * 2 + 1] << 8)
        r, g, bl, a = rgb5551(w)
        out[i * 4 : i * 4 + 4] = bytes((r, g, bl, 255 if a else 0))
    return bytes(out)


def decode_texpage(vram: Vram, tpage: int, clut: int) -> bytes:
    """One 256x256 RGBA texture page as a primitive with this tpage/clut sees it.

    tpage bits: 0-3 page X (x64), bit 4 page Y (x256), bits 5-6 abr,
    bits 7-8 colour mode 0=4bpp 1=8bpp 2=16bpp.
    clut: bits 0-5 x/16, bits 6-14 y.
    """
    px = (tpage & 0x0F) * 64
    py = ((tpage >> 4) & 1) * 256
    mode = (tpage >> 7) & 3
    cx = (clut & 0x3F) * 16
    cy = (clut >> 6) & 0x1FF

    pal = []
    if mode in (0, 1):
        n = 16 if mode == 0 else 256
        for i in range(n):
            x = cx + i
            pal.append(rgb5551(vram.word(x, cy)) if x < VRAM_W and cy < VRAM_H else (0, 0, 0, 0))

    # Per-source-byte expansion tables: one VRAM byte is two 4bpp texels or one
    # 8bpp texel, so a whole row becomes a single join instead of 256 lookups.
    if mode == 0:
        rgba = [bytes(c) for c in pal]
        table = [rgba[b & 0xF] + rgba[b >> 4] for b in range(256)]
        row_bytes = 128                      # 256 texels / 2 per byte
    elif mode == 1:
        rgba = [bytes(c) for c in pal]
        table = [rgba[b] for b in range(256)]
        row_bytes = 256
    else:
        table = None
        row_bytes = 512

    out = bytearray(256 * 256 * 4)
    src = vram.buf
    for v in range(256):
        yy = py + v
        if yy >= VRAM_H:
            break
        start = (yy * VRAM_W + px) * 2
        avail = min(row_bytes, (VRAM_W - px) * 2)
        chunk = src[start : start + avail]
        if table is not None:
            row = b"".join([table[b] for b in chunk])
        else:
            row = b"".join([bytes(rgb5551(chunk[i] | (chunk[i + 1] << 8)))
                            for i in range(0, len(chunk) - 1, 2)])
        out[v * 1024 : v * 1024 + len(row)] = row
    return bytes(out)
