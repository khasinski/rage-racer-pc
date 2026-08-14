"""Read a PS1 MODE2/2352 disc image and pull RAGE.BIN / RAGE.STR apart.

The archive format is derived from the game's own loader
(LoadDiscArchiveIndex / LoadAsset):

    RAGE.BIN sector 0 = 135 entries of { u32 sectorOffset, u32 sizeInBytes }
    asset i: LBA = LBA(RAGE.BIN) + sectorOffset, length = sizeInBytes

Nothing here hardcodes a path; the image is always an argument.
"""

from __future__ import annotations

import struct
from dataclasses import dataclass
from pathlib import Path

RAW_SECTOR = 2352
USER_DATA = 2048
HEADER = 24  # sync(12) + address(3) + mode(1) + subheader(8)


class Disc:
    """Sector-addressed view of a MODE2/2352 track."""

    def __init__(self, path: Path):
        self.path = Path(path)
        self.size = self.path.stat().st_size
        if self.size % RAW_SECTOR == 0:
            self.raw = True
            self.sector_count = self.size // RAW_SECTOR
        elif self.size % USER_DATA == 0:
            # Already a cooked .iso.
            self.raw = False
            self.sector_count = self.size // USER_DATA
        else:
            raise ValueError(
                f"{path}: {self.size} bytes is neither a multiple of 2352 nor 2048"
            )
        self._fh = self.path.open("rb")

    def close(self) -> None:
        self._fh.close()

    def __enter__(self):
        return self

    def __exit__(self, *exc):
        self.close()

    def sector(self, lba: int) -> bytes:
        if self.raw:
            self._fh.seek(lba * RAW_SECTOR + HEADER)
            return self._fh.read(USER_DATA)
        self._fh.seek(lba * USER_DATA)
        return self._fh.read(USER_DATA)

    def sector_with_subheader(self, lba: int):
        """(subheader, user data) for a Mode 2 sector, or None past the end.

        The subheader is what says whether a sector is Form 1 (2048 bytes of
        data) or Form 2 (2324 bytes, used for XA audio), so anything walking a
        mixed stream needs it. A cooked .iso has thrown it away.
        """
        if not self.raw:
            data = self.sector(lba)
            return (b"\0" * 8, data) if data else None
        if lba >= self.sector_count:
            return None
        self._fh.seek(lba * RAW_SECTOR + 16)
        sub = self._fh.read(8)
        return sub, self._fh.read(2324 if sub[2:3] and sub[2] & 0x20 else USER_DATA)

    def raw_sectors(self, lba: int, count: int) -> bytes:
        """Verbatim 2352-byte sectors - what an STR demuxer expects to be fed."""
        if not self.raw:
            raise ValueError("raw sectors need a 2352-byte-per-sector image")
        self._fh.seek(lba * RAW_SECTOR)
        return self._fh.read(count * RAW_SECTOR)

    def read(self, lba: int, nbytes: int) -> bytes:
        out = bytearray()
        n = (nbytes + USER_DATA - 1) // USER_DATA
        for i in range(n):
            out += self.sector(lba + i)
        return bytes(out[:nbytes])


# --------------------------------------------------------------------------
# Minimal ISO 9660 directory walk. Enough to find RAGE.BIN and RAGE.STR.
# --------------------------------------------------------------------------


@dataclass
class IsoFile:
    name: str
    lba: int
    size: int


def _both_endian32(b: bytes, off: int) -> int:
    return struct.unpack_from("<I", b, off)[0]


def read_root_directory(disc: Disc) -> list[IsoFile]:
    pvd = disc.sector(16)
    if pvd[1:6] != b"CD001":
        raise ValueError("no ISO 9660 PVD at sector 16")
    root_record = pvd[156 : 156 + 34]
    root_lba = _both_endian32(root_record, 2)
    root_len = _both_endian32(root_record, 10)

    data = disc.read(root_lba, root_len)
    files: list[IsoFile] = []
    pos = 0
    while pos < len(data):
        rec_len = data[pos]
        if rec_len == 0:
            # Padding to the end of the sector; jump to the next one.
            pos = (pos // USER_DATA + 1) * USER_DATA
            if pos >= len(data):
                break
            continue
        rec = data[pos : pos + rec_len]
        lba = _both_endian32(rec, 2)
        size = _both_endian32(rec, 10)
        name_len = rec[32]
        name = rec[33 : 33 + name_len].decode("ascii", "replace")
        files.append(IsoFile(name, lba, size))
        pos += rec_len
    return files


def volume_label(disc: Disc) -> str:
    return disc.sector(16)[40:72].decode("ascii", "replace").strip()


# --------------------------------------------------------------------------
# The RAGE.BIN archive
# --------------------------------------------------------------------------


@dataclass
class ArchiveEntry:
    index: int
    sector_offset: int
    size: int
    lba: int


def read_archive_index(disc: Disc, base_lba: int, count: int) -> list[ArchiveEntry]:
    """Sector 0 of the archive is the (position, size) table the loader reads."""
    toc = disc.sector(base_lba)
    out = []
    for i in range(count):
        off, size = struct.unpack_from("<II", toc, i * 8)
        out.append(ArchiveEntry(i, off, size, base_lba + off))
    return out


# --------------------------------------------------------------------------
# RAGE.STR
#
# RAGE.STR is NOT a RAGE.BIN-style archive: it has no table of contents. It is
# a plain Sony STR stream, so the only way to index it is to walk the sector
# headers. Reading its first sector as a TOC produces numbers that look like an
# index and are not (sector offsets past 2 billion in an 84 MB file).
#
# Each Mode 2 sector's 8-byte subheader gives file/channel/submode/coding, and
# submode bit 5 separates the two kinds that are interleaved 7:1 here:
#     0x48  Form 1, data   -> a video sector, 2048 bytes of frame payload
#     0x64  Form 2, audio  -> XA ADPCM, not part of the video bitstream
# A video sector's payload starts with the STR frame header:
#     u16 0x0160 magic, u16 0x8001 type, u16 sectorInFrame, u16 sectorsInFrame,
#     u32 frameNumber, u32 frameDataBytes, u16 width, u16 height
# A movie is a run of sectors whose frameNumber climbs; where it drops back the
# next movie begins.
# --------------------------------------------------------------------------

STR_MAGIC = 0x0160
STR_TYPE = 0x8001
SUBMODE_FORM2 = 0x20


@dataclass
class StrMovie:
    index: int
    sector_offset: int      # from the start of RAGE.STR
    sectors: int
    frames: int
    width: int
    height: int


def read_stream_index(disc: Disc, base_lba: int, total_sectors: int) -> list[StrMovie]:
    movies: list[StrMovie] = []
    cur = None
    prev_frame = None
    for i in range(total_sectors):
        raw = disc.sector_with_subheader(base_lba + i)
        if raw is None:
            break
        sub, data = raw
        if sub[2] & SUBMODE_FORM2:      # XA audio rides along; skip it
            continue
        if len(data) < 20:
            break
        magic, typ, _sec, _nsec = struct.unpack_from("<4H", data, 0)
        if magic != STR_MAGIC or typ != STR_TYPE:
            continue
        frame = struct.unpack_from("<I", data, 8)[0]
        w, h = struct.unpack_from("<2H", data, 16)
        if cur is None or prev_frame is None or frame < prev_frame:
            cur = StrMovie(len(movies), i, 0, 0, w, h)
            movies.append(cur)
            prev_frame = None
        if frame != prev_frame:
            cur.frames += 1
        cur.sectors = i - cur.sector_offset + 1
        prev_frame = frame
    return movies


def find_file(files: list[IsoFile], name: str) -> IsoFile:
    for f in files:
        # ISO names carry a ";1" version suffix.
        if f.name.split(";")[0].upper() == name.upper():
            return f
    raise KeyError(name)
