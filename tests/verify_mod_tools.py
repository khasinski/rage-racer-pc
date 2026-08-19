#!/usr/bin/env python3
"""rage-extract and rage-pack round trip, on a synthetic archive.

The archive is built here rather than read from a disc, so the test carries no
game data and can run anywhere. It holds the same structures the real one does:
an index of sector offsets and sizes, and image assets made of [size][payload]
links whose payload is a word, a flags word, an optional palette block and a
pixel block.

What is asserted is the property a modder depends on. Packing an extract nobody
edited must not change a byte, an edit must land exactly where it was made and
nowhere else, and bad input must be refused with a message that says what is
wrong instead of writing something broken.
"""

from __future__ import annotations

import struct
import subprocess
import sys
import zlib
from pathlib import Path
from tempfile import TemporaryDirectory

SECTOR = 2048
ENTRIES = 135


def block(x: int, y: int, w: int, h: int, payload: bytes) -> bytes:
    body = struct.pack("<IHHHH", 12 + len(payload), x, y, w, h) + payload
    return body


def image_asset(clut: list[int], words: int, rows: int, pixels: bytes) -> bytes:
    """One image asset: a leading word, one link, then the terminator."""
    clut_block = block(0, 500, len(clut), 1, b"".join(struct.pack("<H", c) for c in clut))
    pixel_block = block(64, 0, words, rows, pixels)
    payload = struct.pack("<II", 0, 8) + clut_block + pixel_block
    return struct.pack("<I", 0) + struct.pack("<i", len(payload)) + payload + struct.pack("<i", 0)


def build_archive(path: Path) -> dict[int, bytes]:
    """A 4bpp asset, an 8bpp asset, and one entry that is not an image."""
    assets: dict[int, bytes] = {}

    clut16 = [0] + [((i * 2) | ((i * 3) << 5) | ((i) << 10)) & 0x7FFF or 1 for i in range(1, 16)]
    # a row is `words` VRAM words wide, so 4bpp packs two texels per byte
    pixels4 = bytes(((y + x) % 16) | (((y + x + 1) % 16) << 4)
                    for y in range(32) for x in range(32))
    assets[0] = image_asset(clut16, 16, 32, pixels4)

    clut256 = [0] + [((i % 31) | ((i // 8) << 5) | ((i % 17) << 10)) or 1 for i in range(1, 256)]
    pixels8 = bytes((y * 3 + x) % 256 for y in range(24) for x in range(32))
    assets[1] = image_asset(clut256, 16, 24, pixels8)

    assets[2] = b"not an image at all, just bytes" * 4

    index = bytearray(ENTRIES * 8)
    body = bytearray()
    cursor = ENTRIES * 8
    cursor += (-cursor) % SECTOR
    for number, data in sorted(assets.items()):
        start = len(index) + len(body)
        pad = (-start) % SECTOR
        body += b"\0" * pad
        offset = len(index) + len(body)
        struct.pack_into("<II", index, number * 8, offset // SECTOR, len(data))
        body += data
    path.write_bytes(bytes(index) + bytes(body))
    return assets


def read_png(path: Path) -> tuple[int, int, list[tuple[int, int, int, int]]]:
    data = path.read_bytes()
    assert data[:8] == b"\x89PNG\r\n\x1a\n", f"{path} is not a PNG"
    at, idat, w, h = 8, b"", 0, 0
    while at + 8 <= len(data):
        length = struct.unpack_from(">I", data, at)[0]
        tag = data[at + 4:at + 8]
        body = data[at + 8:at + 8 + length]
        if tag == b"IHDR":
            w, h = struct.unpack_from(">II", body)
        elif tag == b"IDAT":
            idat += body
        elif tag == b"IEND":
            break
        at += 12 + length
    raw = zlib.decompress(idat)
    rows, stride = [], w * 4
    previous = bytearray(stride)
    for y in range(h):
        filt = raw[y * (stride + 1)]
        line = bytearray(raw[y * (stride + 1) + 1:(y + 1) * (stride + 1)])
        for i in range(stride):
            a = line[i - 4] if i >= 4 else 0
            b = previous[i]
            if filt == 1:
                line[i] = (line[i] + a) & 0xFF
            elif filt == 2:
                line[i] = (line[i] + b) & 0xFF
            elif filt == 3:
                line[i] = (line[i] + ((a + b) >> 1)) & 0xFF
            elif filt == 4:
                c = previous[i - 4] if i >= 4 else 0
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                line[i] = (line[i] + (a if pa <= pb and pa <= pc else b if pb <= pc else c)) & 0xFF
        rows.append([tuple(line[x * 4:x * 4 + 4]) for x in range(w)])
        previous = line
    return w, h, [pixel for row in rows for pixel in row]


def write_png(path: Path, w: int, h: int, pixels: list[tuple[int, int, int, int]]) -> None:
    raw = b"".join(b"\x00" + bytes(c for pixel in pixels[y * w:(y + 1) * w] for c in pixel)
                   for y in range(h))

    def chunk(tag: bytes, body: bytes) -> bytes:
        joined = tag + body
        return struct.pack(">I", len(body)) + joined + struct.pack(">I", zlib.crc32(joined) & 0xFFFFFFFF)

    path.write_bytes(b"\x89PNG\r\n\x1a\n"
                     + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0))
                     + chunk(b"IDAT", zlib.compress(raw))
                     + chunk(b"IEND", b""))


def main() -> int:
    extract, pack = Path(sys.argv[1]), Path(sys.argv[2])
    with TemporaryDirectory(prefix="rage-mod-") as directory:
        root = Path(directory)
        archive = root / "RAGE.BIN"
        build_archive(archive)
        mod = root / "mod"

        run = subprocess.run([extract, archive, mod], capture_output=True, text=True)
        if run.returncode != 0:
            raise AssertionError(f"rage-extract failed: {run.stdout}{run.stderr}")

        textures = sorted((mod / "textures").glob("*.png"))
        if len(textures) < 2:
            raise AssertionError(f"expected the two image assets to decode, got {textures}")
        w, h, _ = read_png(mod / "textures" / "asset_000_00.png")
        if (w, h) != (64, 32):
            raise AssertionError(f"4bpp texture decoded as {w}x{h}, expected 64x32")
        w8, h8, _ = read_png(mod / "textures" / "asset_001_00.png")
        if (w8, h8) != (32, 24):
            raise AssertionError(f"8bpp texture decoded as {w8}x{h8}, expected 32x24")

        before = {p.name: p.read_bytes() for p in (mod / "raw").glob("*.bin")}

        run = subprocess.run([pack, mod], capture_output=True, text=True)
        if run.returncode != 0:
            raise AssertionError(f"rage-pack failed: {run.stdout}{run.stderr}")
        after = {p.name: p.read_bytes() for p in (mod / "raw").glob("*.bin")}
        drifted = [name for name in before if before[name] != after[name]]
        if drifted:
            raise AssertionError(f"packing an unedited extract changed {drifted}")

        # An edit lands where it was made, and nowhere else.
        target = mod / "textures" / "asset_001_00.png"
        w8, h8, pixels = read_png(target)
        painted = pixels[:]
        colour = next(p for p in pixels if p[3] == 255)
        for y in range(4, 12):
            for x in range(5, 20):
                painted[y * w8 + x] = colour
        write_png(target, w8, h8, painted)
        run = subprocess.run([pack, mod], capture_output=True, text=True)
        if run.returncode != 0:
            raise AssertionError(f"rage-pack failed on an edit: {run.stdout}{run.stderr}")
        after = {p.name: p.read_bytes() for p in (mod / "raw").glob("*.bin")}
        changed = [name for name in before if before[name] != after[name]]
        if changed != ["asset_001.bin"]:
            raise AssertionError(f"an edit to one texture changed {changed}")

        # A PNG of the wrong size is refused, and nothing is written.
        guarded = {p.name: p.read_bytes() for p in (mod / "raw").glob("*.bin")}
        write_png(target, 8, 8, [(0, 0, 0, 255)] * 64)
        run = subprocess.run([pack, mod], capture_output=True, text=True)
        message = run.stdout + run.stderr
        if "the size is fixed" not in message:
            raise AssertionError(f"a mis-sized PNG was not reported clearly: {message}")
        if {p.name: p.read_bytes() for p in (mod / "raw").glob("*.bin")} != guarded:
            raise AssertionError("a refused PNG still changed the asset")

        # A PNG that is not a PNG is refused by name.
        target.write_bytes(b"certainly not an image")
        run = subprocess.run([pack, mod], capture_output=True, text=True)
        message = run.stdout + run.stderr
        if "is not a PNG" not in message:
            raise AssertionError(f"a corrupt PNG was not reported clearly: {message}")

    print("mod tools round trip, edit isolation and refusals all hold")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
