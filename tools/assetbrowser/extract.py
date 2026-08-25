#!/usr/bin/env python3
"""Rage Racer (PS1) asset extractor.

    python3 tools/assetbrowser/extract.py <disc.bin> -o <output-dir>

Reads the MODE2/2352 track 1 of a Rage Racer disc, pulls the 135-entry
RAGE.BIN archive apart, decodes what it can, and writes a manifest.json plus
decoded models, textures and raw blobs. Nothing is hardcoded to one machine:
the image path is always an argument.

Formats are decoded from the game's own code (see disc.py / models.py /
images.py for the source references), not from header sniffing.

Game data must never enter the repository. The default output directory is
tools/decomp-wip/assets/, which .gitignore already covers.
"""

from __future__ import annotations

import argparse
import json
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import audio  # noqa: E402
import gltf  # noqa: E402
import images  # noqa: E402
import models  # noqa: E402
import png  # noqa: E402
import rmesh  # noqa: E402
from disc import (Disc, find_file, read_archive_index, read_root_directory,  # noqa: E402
                  read_stream_index, volume_label)

ASSET_COUNT = 135
STR_FPS = 15          # the rate the STR header's sectorsInFrame implies, and
                      # what ffmpeg's psxstr demuxer reports for these streams

# Resolved from g_AssetPaths in the PAL executable.
# --exe re-reads them from a real SCES_006.50 if you want to prove it again.
ASSET_NAMES = (
    ["LOGO.TMS", "TITLE.TMS", "RG3.VH", "RG3.VB", "RES.DAT", "CAR.TMS", "SAVE.TMS",
     "SELBGM.BIN", "SELECT.BIN", "OPTION.BIN"]
    + [f"CAR_{c}.{h}"
       for c in ("00", "01", "02", "03", "10", "11", "12", "20", "21", "30", "31",
                 "32", "33", "34", "40", "41", "42", "43", "50", "51", "52", "60",
                 "61", "70", "71", "72", "80", "81", "90", "A0", "B0", "C0")
       for h in ("1ST", "2ND")]
    + [f"GP{n}.TMS" for n in (0, 1, 2, 3, 4, 10, 5, 6, 7, 8, 9, 11)]
    + ["VOICE.BIN"]
    + [f"{course}{cls}.{half}"
       for cls in range(1, 7)
       for course in ("BIG", "MID", "HI", "OVAL")
       for half in ("1ST", "2ND")]
)
assert len(ASSET_NAMES) == ASSET_COUNT

# The car texture page every CAR_xx.1ST uploads: g_CarImageRect at 0x8007C484
# reads {x 704, y 0, w 64, h 256} - one full 4bpp texture page, 0x8000 bytes.
CAR_IMAGE_RECT = (704, 0, 64, 256)
CAR_PAINT_PALETTE_OFFSET = 0x7060
CAR_PAINT_FIRST = (1, 2, 3, 4, 5, 6, 7)
CAR_PAINT_SECOND = (8, 9, 10, 11, 12, 13, 14)
CAR_PAINT_SLOTS_3_A = (1, 0x41, 0xC1, 0x101, 0x181, 0x241, 0x281,
                       0x301, 0x341)
CAR_PAINT_SLOTS_3_B = (1, 0x41, 0xC1, 0x181, 0x241, 0x281, 0x301, 0x341)
CAR_PAINT_SLOTS_4 = (0x141, 0x1C1, 0x201, 0x401)

# Terrain cell pitch in the units the vertex pool is stored in. The grid itself
# is 2048 world units per cell; BuildVisibleCells shifts the cell translation
# left by 2 before handing it to the GTE, so a cell step is 2048 << 2 here.
CELL_PITCH = 2048 << 2


def car_paint_vram_labels() -> dict[tuple[int, int], int]:
    """Map imported palette cells to renderer-neutral paint ramp codes."""
    entries: dict[int, int] = {}

    def assign(start, codes):
        entries.update((start + offset, code)
                       for offset, code in enumerate(codes))

    for start in CAR_PAINT_SLOTS_3_A:
        assign(start, (CAR_PAINT_FIRST[0], CAR_PAINT_FIRST[3],
                       CAR_PAINT_FIRST[6]))
    for start in CAR_PAINT_SLOTS_3_B:
        assign(start + 3, (CAR_PAINT_SECOND[0], CAR_PAINT_SECOND[3],
                           CAR_PAINT_SECOND[6]))
    for start in CAR_PAINT_SLOTS_4:
        assign(start, (CAR_PAINT_FIRST[0], CAR_PAINT_FIRST[2],
                       CAR_PAINT_FIRST[4], CAR_PAINT_FIRST[6]))
        assign(start + 4, (CAR_PAINT_SECOND[0], CAR_PAINT_SECOND[2],
                           CAR_PAINT_SECOND[4], CAR_PAINT_SECOND[6]))
    # These final five-entry gradients are written after every slot loop.
    assign(0x2C1, (CAR_PAINT_FIRST[0], CAR_PAINT_FIRST[1],
                   CAR_PAINT_FIRST[3], CAR_PAINT_FIRST[5],
                   CAR_PAINT_FIRST[6]))
    assign(0x2C6, (CAR_PAINT_SECOND[0], CAR_PAINT_SECOND[1],
                   CAR_PAINT_SECOND[3], CAR_PAINT_SECOND[5],
                   CAR_PAINT_SECOND[6]))
    labels = {}
    base_word = CAR_PAINT_PALETTE_OFFSET // 2
    x, y, width, _height = CAR_IMAGE_RECT
    for entry, code in entries.items():
        word = base_word + entry
        labels[(x + word % width, y + word // width)] = code
    return labels


def names_from_exe(exe_path: Path) -> list[str]:
    """Re-derive the table from a real executable rather than trusting the copy above."""
    d = exe_path.read_bytes()
    if d[:8] != b"PS-X EXE":
        raise ValueError("not a PS-X EXE")
    text_vaddr = struct.unpack_from("<I", d, 0x18)[0]
    base = text_vaddr - 0x800
    ptrs = struct.unpack_from(f"<{ASSET_COUNT}I", d, 0x8007C48C - base)
    out = []
    for p in ptrs:
        o = p - base
        out.append(d[o : d.index(b"\0", o)].decode("ascii", "replace").rsplit("\\", 1)[-1])
    return out


# --------------------------------------------------------------------------


def kind_of(index: int, name: str) -> str:
    if name.endswith(".TMS"):
        return "image"
    if name.endswith(".1ST") and name.startswith("CAR_"):
        return "car_model"
    if name.endswith(".2ND") and name.startswith("CAR_"):
        return "car_pack"
    if name.endswith(".1ST"):
        return "track_images"
    if name.endswith(".2ND"):
        return "track_data"
    return {
        "RG3.VH": "vab_header",
        "RG3.VB": "vab_body",
        "RES.DAT": "raw",
        "SELBGM.BIN": "audio_pack",
        "SELECT.BIN": "select_pack",
        "OPTION.BIN": "option_pack",
        "VOICE.BIN": "audio",
    }.get(name, "raw")


def image_blocks_json(blks):
    return [
        {"x": b.x, "y": b.y, "w": b.w, "h": b.h, "size": b.size, "clut": b.is_clut}
        for b in blks
    ]


class Extractor:
    def __init__(self, disc: Disc, entries, names, outdir: Path, opts):
        self.disc = disc
        self.entries = entries
        self.names = names
        self.out = outdir
        self.opts = opts
        self.cache: dict[int, bytes] = {}
        self.notes: list[str] = []

    def data(self, i: int) -> bytes:
        if i not in self.cache:
            e = self.entries[i]
            self.cache[i] = self.disc.read(e.lba, e.size)
            if len(self.cache) > 8:
                self.cache.pop(next(iter(self.cache)))
        return self.cache[i]

    # -- VRAM contexts ---------------------------------------------------

    def vram_for(self, index: int, kind: str) -> tuple[images.Vram, list[str]]:
        """Rebuild the VRAM a model would be textured from.

        Cars: CAR.TMS (the shared showroom/HUD upload) plus the car's own
        0x8000-byte page. Tracks: CAR.TMS followed by the four image chains and
        the one bare block of the matching .1ST pack. CAR.TMS is loaded during
        boot and remains resident when LoadRaceAssets performs its step-5
        uploads; track model bank 1 deliberately reuses its shared car palettes
        and wheel texture.
        """
        v = images.Vram()
        src = []
        if kind in ("car_model", "car_pack"):
            for blk in images.parse_image_asset(self.data(5), 0):
                v.load(self.data(5), blk)
            src.append("CAR.TMS")
            if kind == "car_model":
                buf = self.data(index)
                io = struct.unpack_from("<I", buf, 0x24)[0]
                x, y, w, h = CAR_IMAGE_RECT
                v.load(buf, images.ImageBlock(0x8000, x, y, w, h, io))
                src.append(f"{self.names[index]} page")
            else:
                hdr = struct.unpack_from("<5i", buf := self.data(index), 0)
                for blk in images.parse_image_asset(buf, hdr[4]):
                    v.load(buf, blk)
                src.append(f"{self.names[index]}[4]")
        elif kind == "track_data":
            v, _alternate, src = self.track_vrams(index)
            return v, src
        elif kind in ("select_pack", "option_pack"):
            buf = self.data(index)
            off = struct.unpack_from("<i", buf, 8 if kind == "select_pack" else 0)[0]
            if kind == "select_pack":
                for blk in images.parse_image_asset(buf, off):
                    v.load(buf, blk)
            src.append(self.names[index])
        return v, src

    def track_vrams(self, index: int, car_pack_index: int | None = None
                    ) -> tuple[images.Vram, images.Vram, list[str]]:
        """Return the two semantic track texture pages used by a race.

        The PS1 swaps the pages a row at a time to fit both in VRAM. Native
        renderers get two ordinary immutable material sets and select one from
        the current track section instead.
        """
        v = images.Vram()
        src = []
        # LoadBootAssets uploads CAR.TMS before the race load. It supplies
        # the shared opponent-car palettes and wheels referenced by model bank
        # 1, so starting from an empty VRAM makes only those materials decode
        # as transparent while track-owned materials still look correct.
        shared = self.data(5)
        for blk in images.parse_image_asset(shared, 0):
            v.load(shared, blk)
        src.append("CAR.TMS")

        # LoadRaceAssets step 3 uploads sub-block 4 of the selected car's
        # .2ND pack before loading the track. Besides cockpit/HUD artwork it
        # replaces the six palettes used by track model bank 1, so omitting it
        # gives every rival the colours left behind by CAR.TMS. There are 32
        # selectable car assets; callers exporting bank 1 provide each one as
        # an ordinary material variant.
        if car_pack_index is not None:
            car_pack = self.data(car_pack_index)
            car_header = struct.unpack_from("<5i", car_pack, 0)
            for blk in images.parse_image_asset(car_pack, car_header[4]):
                v.load(car_pack, blk)
            src.append(self.names[car_pack_index])

        # LoadRaceAssets step 5 then uploads the five track sub-blocks strictly
        # in order, and sub-block 2 goes through UploadImageBlock (one chunk)
        # rather than UploadImageAsset (a chain). Order matters because 3 and
        # 4 are two alternative fills of the same VRAM rectangle:
        # (576,256)-(1023,511).
        first = index - 1
        buf = self.data(first)
        hdr = struct.unpack_from("<5i", buf, 0)
        alternate = None
        for k in range(5):
            blocks = (images._parse_chunk(buf, hdr[k], len(buf)) if k == 2
                      else images.parse_image_asset(buf, hdr[k]))
            for blk in blocks:
                v.load(buf, blk)
            if k == 3:
                # StoreTeamLogoImage snapshots this state as page 1 before
                # sub-block 4 overwrites the resident rectangle with page 0.
                alternate = v.clone()
        src.append(self.names[first])
        assert alternate is not None
        return v, alternate, src

    # -- per-asset ------------------------------------------------------

    def run(self):
        (self.out / "raw").mkdir(parents=True, exist_ok=True)
        (self.out / "models").mkdir(parents=True, exist_ok=True)
        (self.out / "textures").mkdir(parents=True, exist_ok=True)
        (self.out / "images").mkdir(parents=True, exist_ok=True)

        manifest = []
        for i in range(ASSET_COUNT):
            rec = self.one(i)
            manifest.append(rec)
            print(f"[{i:3d}] {rec['name']:<16} {rec['kind']:<13} "
                  f"{rec['size']:>9}  {rec.get('summary','')}", flush=True)
        return manifest

    def one(self, i: int) -> dict:
        e = self.entries[i]
        name = self.names[i]
        kind = kind_of(i, name)
        buf = self.data(i)
        rec = {
            "index": i,
            "name": name,
            "kind": kind,
            "size": e.size,
            "lba": e.lba,
            "sectorOffset": e.sector_offset,
            "confidence": "decoded",
        }
        stem = f"{i:03d}_{name.replace('.', '_')}"
        if self.opts.raw:
            (self.out / "raw" / f"{stem}.bin").write_bytes(buf)
            rec["raw"] = f"raw/{stem}.bin"

        try:
            if kind == "image":
                self.do_image(i, stem, buf, 0, rec)
            elif kind == "car_model":
                self.do_car_model(i, stem, buf, rec)
            elif kind == "car_pack":
                self.do_car_pack(i, stem, buf, rec)
            elif kind == "track_images":
                self.do_track_images(i, stem, buf, rec)
            elif kind == "track_data":
                self.do_track_data(i, stem, buf, rec)
            elif kind == "select_pack":
                self.do_select(i, stem, buf, rec)
            elif kind == "option_pack":
                self.do_option(i, stem, buf, rec)
            elif kind == "vab_header":
                # RG3.VH is the header; its body is the very next asset, RG3.VB.
                self.do_vab(stem, buf, 0, self.data(i + 1), 0, rec)
            elif kind == "vab_body":
                rec["confidence"] = "decoded"
                rec["summary"] = f"VAB body of {self.names[i - 1]}"
                rec["vabBodyOf"] = i - 1
            elif kind == "audio_pack":
                # SELBGM.BIN: StartAudioSlotLoad(1, sub[0], sub[2], sub[1])
                h = list(struct.unpack_from("<3i", buf, 0))
                rec["subBlocks"] = [
                    {"n": 0, "role": "VAB header (VH)", "offset": h[0]},
                    {"n": 1, "role": "sequence data (SEQ)", "offset": h[1]},
                    {"n": 2, "role": "VAB body (VB)", "offset": h[2]},
                ]
                rec["isSeq"] = audio.looks_like_seq(buf, h[1])
                # A SEQ plays these samples at whatever notes the music calls
                # for, so the 0x3C key-on rule does not apply to this bank.
                self.do_vab(stem, buf, h[0], buf, h[2], rec,
                            sequenced=rec["isSeq"])
            elif kind == "audio":
                # VOICE.BIN: { u32 vhSize, u32 vhOffset, u32 vbOffset }
                w = list(struct.unpack_from("<3I", buf, 0))
                rec["subBlocks"] = [
                    {"n": 0, "role": "VAB header size", "offset": w[0]},
                    {"n": 1, "role": "VAB header (VH)", "offset": w[1]},
                    {"n": 2, "role": "VAB body (VB)", "offset": w[2]},
                ]
                self.do_vab(stem, buf, w[1], buf, w[2], rec)
            else:
                rec["confidence"] = "raw"
                rec["summary"] = "not decoded"
        except Exception as exc:  # noqa: BLE001
            rec["confidence"] = "failed"
            rec["error"] = f"{type(exc).__name__}: {exc}"
            rec["summary"] = f"FAILED {exc}"
        return rec

    # -- decoders --------------------------------------------------------

    def do_image(self, i, stem, buf, off, rec, vram_key=None):
        blks = images.parse_image_asset(buf, off)
        v = images.Vram()
        loaded = sum(1 for b in blks if v.load(buf, b))
        rec["imageBlocks"] = image_blocks_json(blks)
        rec["vram"] = self.emit_vram(v, stem)
        rec["sprites"] = self.emit_sprites(buf, blks, stem)
        rec["summary"] = f"{loaded}/{len(blks)} VRAM blocks, {len(rec['sprites'])} sprites"
        if loaded == 0:
            rec["confidence"] = "raw"

    def emit_sprites(self, buf, blks, stem):
        out = []
        for n, (img, clut) in enumerate(images.pair_blocks(blks)):
            got = images.decode_sprite(buf, img, clut)
            if got is None:
                continue
            w, h, rgba, bpp = got
            fn = f"{stem}_p{n:03d}.png"
            png.write_rgba(self.out / "images" / fn, w, h, rgba)
            out.append({"file": f"images/{fn}", "w": w, "h": h, "bpp": bpp,
                        "vramX": img.x, "vramY": img.y})
        return out

    def do_car_model(self, i, stem, buf, rec):
        mo, io = struct.unpack_from("<II", buf, 0x20)
        bank = models.parse_bank(buf, mo, io)
        rec["carFlags"] = {"automaticGearbox": buf[8] != 0, "gears": buf[9]}
        v, src = self.vram_for(i, "car_model")
        rec["vram"] = self.emit_vram(v, stem)
        rec["vramSources"] = src
        rec["textures"] = self.emit_textures(
            v, bank, stem, paint_labels=car_paint_vram_labels())
        rec["model"] = self.emit_bank(bank, stem, textures=rec["textures"])
        rec["summary"] = (
            f"{bank.count} models, {len(bank.vertices)} verts, "
            f"{sum(len(m.faces) for m in bank.models)} faces, "
            f"{len(rec['textures'])} texture pages"
        )

    def do_car_pack(self, i, stem, buf, rec):
        # StartAudioSlotLoad(3, sub[1], sub[3], sub[2]) => VH, VB, table.
        hdr = list(struct.unpack_from("<5i", buf, 0))
        rec["subBlocks"] = [
            {"n": 0, "role": "car spec (SetCarSpec)", "offset": hdr[0]},
            {"n": 1, "role": "engine VAB header (VH)", "offset": hdr[1]},
            {"n": 2, "role": "engine tone table", "offset": hdr[2]},
            {"n": 3, "role": "engine VAB body (VB)", "offset": hdr[3]},
            {"n": 4, "role": "image asset", "offset": hdr[4]},
        ]
        blks = images.parse_image_asset(buf, hdr[4])
        v = images.Vram()
        loaded = sum(1 for b in blks if v.load(buf, b))
        rec["imageBlocks"] = image_blocks_json(blks)
        rec["vram"] = self.emit_vram(v, stem)
        self.do_vab(stem, buf, hdr[1], buf, hdr[3], rec)
        rec["summary"] = (f"5 sub-blocks, {loaded} VRAM blocks, "
                          f"{len(rec.get('sounds', []))} sounds")

    def do_vab(self, stem, vhbuf, vhoff, vbbuf, vboff, rec, sequenced=False):
        h = audio.parse_vab_header(vhbuf, vhoff)
        if h is None:
            rec.setdefault("summary", "no VAB header found")
            rec["confidence"] = "raw"
            return
        rec["vab"] = {k: v for k, v in h.items() if k not in ("vagSizes", "vagOffsets")}
        rec["sounds"] = []
        if not self.opts.audio:
            rec.setdefault("summary", f"VAB: {h['vags']} sounds (not decoded)")
            return
        # A VAG carries no sample rate of its own; it comes from the tone that
        # references it, keyed at the note audio.c always uses. Writing every
        # .wav at 44100 played the 11025 Hz speech four times too fast.
        tones = audio.parse_tones(vhbuf, vhoff, h)
        rates = audio.vag_rates(tones, len(h["vagOffsets"]),
                                None if sequenced else audio.NOTE_KEY_ON)
        rec["rateNote"] = "center" if sequenced else audio.NOTE_KEY_ON
        d = self.out / "audio"
        d.mkdir(parents=True, exist_ok=True)
        for k, (o, n) in enumerate(zip(h["vagOffsets"], h["vagSizes"])):
            chunk = vbbuf[vboff + o : vboff + o + n]
            if len(chunk) < 16:
                continue
            pcm = audio.decode_vag(chunk)
            if not pcm:
                continue
            rate, alts = rates[k] if k < len(rates) else (None, [])
            fn = f"{stem}_s{k:02d}.wav"
            (d / fn).write_bytes(audio.wav(pcm, rate or audio.SPU_BASE_RATE))
            snd = {"file": f"audio/{fn}", "bytes": n, "samples": len(pcm) // 2,
                   "rate": rate or audio.SPU_BASE_RATE,
                   "seconds": round((len(pcm) // 2) / float(rate or audio.SPU_BASE_RATE), 3)}
            if rate is None:
                snd["rateAssumed"] = True
            if alts:
                snd["altRates"] = alts
            rec["sounds"].append(snd)
        rec.setdefault("summary", f"VAB: {h['programs']} programs, "
                                  f"{len(rec['sounds'])}/{h['vags']} sounds decoded")

    def do_track_images(self, i, stem, buf, rec):
        hdr = list(struct.unpack_from("<5i", buf, 0))
        v = images.Vram()
        blks = []
        roles = ["image asset", "image asset", "single image block",
                 "image asset (team-logo)", "image asset (track texture)"]
        subs = []
        for k, off in enumerate(hdr):
            b = (images._parse_chunk(buf, off, len(buf)) if k == 2
                 else images.parse_image_asset(buf, off))
            n = sum(1 for x in b if v.load(buf, x))
            blks += b
            subs.append({"n": k, "role": roles[k], "offset": off, "blocks": len(b), "loaded": n})
        rec["subBlocks"] = subs
        rec["imageBlocks"] = image_blocks_json(blks)
        rec["vram"] = self.emit_vram(v, stem)
        rec["sprites"] = self.emit_sprites(buf, blks, stem)
        rec["summary"] = (f"{sum(s['loaded'] for s in subs)}/{len(blks)} VRAM blocks, "
                          f"{len(rec['sprites'])} sprites")

    def do_track_data(self, i, stem, buf, rec):
        hdr = list(struct.unpack_from("<11i", buf, 0))
        roles = ["camera table", "environment palette table", "environment script",
                 "model bank 1", "track points", "course model bank",
                 "model bank 2", "terrain cell data", "course objects",
                 "track events", "camera table (selected)"]
        rec["subBlocks"] = [{"n": k, "role": roles[k], "offset": o} for k, o in enumerate(hdr)]
        v, alternate, src = self.track_vrams(i)
        rec["vram"] = self.emit_vram(v, stem)
        rec["vramSources"] = src

        rec["banks"] = []
        tex = {}
        for k in (3, 6):
            try:
                bank = models.parse_bank(buf, hdr[k], len(buf))
            except models.ParseError as exc:
                rec["banks"].append({"sub": k, "error": str(exc)})
                continue
            if k == 3:
                # GetCarAssetIndex returns 0..31 and LoadRaceAssets turns that
                # into archive index 11 + value * 2. Track model bank 1 samples
                # palettes installed by that pack, while the remaining track
                # banks only need the two section-page variants.
                car_vrams = [self.track_vrams(i, 11 + variant * 2)[0]
                             for variant in range(32)]
                textures = self.emit_textures(
                    car_vrams[0], bank, f"{stem}_b{k}",
                    variant_vrams=car_vrams,
                    variant_clut_offsets=(0, 1, 2))
            else:
                textures = self.emit_textures(v, bank, f"{stem}_b{k}", alternate)
            b = self.emit_bank(bank, f"{stem}_b{k}", textures=textures)
            b["sub"] = k
            b["textures"] = textures
            rec["banks"].append(b)
            tex[k] = len(b["textures"])
        # Course objects (sub-block 5) and the terrain cells (sub-block 7) are
        # the trackside scenery and the road surface itself.
        try:
            cb = models.parse_course_objects(buf, hdr[5], len(buf))
            textures = self.emit_textures(v, cb, f"{stem}_course", alternate)
            b = self.emit_bank(cb, f"{stem}_course", textures=textures,
                               scrolling_primitives=True)
            b["sub"] = 5
            b["label"] = "course objects"
            b["textures"] = textures
            rec["banks"].append(b)
            rec["courseModelCount"] = cb.count
        except models.ParseError as exc:
            rec["courseModelCount"] = None
            rec["banks"].append({"sub": 5, "error": str(exc)})
        try:
            tb, grid = models.parse_terrain(buf, hdr[7], hdr[8] if hdr[8] > hdr[7] else len(buf))
            # Terrain modes 0/1 select adjacent palette rows at runtime. Bake
            # both rows for both section pages so the native renderer consumes
            # ordinary immutable materials instead of consulting live VRAM.
            textures = self.emit_textures(
                v, tb, f"{stem}_terrain",
                variant_vrams=(v, alternate),
                variant_clut_offsets=(0, 1))
            b = self.emit_bank(tb, f"{stem}_terrain", grid=grid,
                               textures=textures)
            b["sub"] = 7
            b["label"] = "terrain cells"
            b["textures"] = textures
            rec["banks"].append(b)
            rec["cellCount"] = tb.count
        except (models.ParseError, struct.error) as exc:
            rec["cellCount"] = None
            rec["banks"].append({"sub": 7, "error": str(exc)})

        rec["summary"] = (
            "banks " + "+".join(str(b.get("modelCount", "?")) for b in rec["banks"])
            + f", {rec['courseModelCount']} objects, {rec['cellCount']} cells"
        )

    def do_select(self, i, stem, buf, rec):
        hdr = list(struct.unpack_from("<3i", buf, 0))
        rec["subBlocks"] = [
            {"n": 0, "role": "team-logo sample data", "offset": hdr[0]},
            {"n": 1, "role": "course model bank", "offset": hdr[1]},
            {"n": 2, "role": "image asset", "offset": hdr[2]},
        ]
        v = images.Vram()
        blks = images.parse_image_asset(buf, hdr[2])
        for b in blks:
            v.load(buf, b)
        rec["imageBlocks"] = image_blocks_json(blks)
        rec["vram"] = self.emit_vram(v, stem)
        bank = models.parse_bank(buf, 0xC, hdr[0])
        rec["textures"] = self.emit_textures(v, bank, stem)
        rec["model"] = self.emit_bank(bank, stem, textures=rec["textures"])
        rec["summary"] = f"{bank.count} models, {len(blks)} VRAM blocks"

    def do_option(self, i, stem, buf, rec):
        imgoff = struct.unpack_from("<i", buf, 0)[0]
        bank = models.parse_bank(buf, 4, imgoff)
        rec["subBlocks"] = [{"n": 0, "role": "model bank (at +4)", "offset": 4},
                            {"n": 1, "role": "free space / image buffer", "offset": imgoff}]
        v = images.Vram()
        rec["textures"] = self.emit_textures(v, bank, stem)
        rec["model"] = self.emit_bank(bank, stem, textures=rec["textures"])
        rec["summary"] = f"{bank.count} models, {len(bank.vertices)} verts"

    # -- emitters --------------------------------------------------------

    def emit_bank(self, bank, stem, grid=None, textures=None,
                  scrolling_primitives=False):
        j = models.bank_to_json(bank)
        if grid is not None:
            # track/visible_cells.c:214-226 is the authority. The grid word at
            # (31 - sy) * 32 + sx holds the cell index in its low 10 bits, with
            # 0x3FF meaning "no geometry here" (bits 10..15 are the region id;
            # there is no rotation or mirror flag). The cell's translation is
            #
            #     vec[0] = ((sx << 11) - (camX - 1024)) << 2
            #     vec[2] = ((sy << 11) - (camZ - 1024)) << 2
            #
            # and that vector is what the GTE adds to the cell's own vertices.
            # The << 2 is the whole story: the grid pitch is 2048 in the world
            # units the camera and the track points use, but the vertex pool is
            # in GTE units, which are four times finer. Spacing cells 2048 apart
            # in vertex units piles them six deep on top of each other.
            # Confirmed against the data: every one of BIG1's 9065 terrain face
            # centroids lies within +/-4095.5 of its cell origin, i.e. exactly
            # half of 8192 - and only 25% of them fall inside +/-2048.
            j["cellSize"] = CELL_PITCH
            j["placements"] = [
                {"cell": grid[row * 32 + col] & 0x3FF,
                 "x": col * CELL_PITCH + CELL_PITCH // 2,
                 "z": (31 - row) * CELL_PITCH + CELL_PITCH // 2}
                for row in range(32) for col in range(32)
                if (grid[row * 32 + col] & 0x3FF) != 0x3FF
            ]
            j["regions"] = [w >> 10 for w in grid]
        p = self.out / "models" / f"{stem}.json"
        p.write_text(json.dumps(j, separators=(",", ":")))
        gltf_path = self.out / "models" / f"{stem}.gltf"
        gltf.write_bank(gltf_path, bank, textures)
        rmesh_path = self.out / "models" / f"{stem}.rmesh"
        rmesh.write_bank(rmesh_path, bank, textures, scrolling_primitives,
                         terrain_primitives=grid is not None)
        materials_path = self.out / "models" / f"{stem}.rmat"
        has_paint = any("runtimePaintMask" in texture
                        for texture in (textures or []))
        material_rows = ["# rage-rmat v5\n" if has_paint
                         else "# rage-rmat v4\n"]
        for index, texture in enumerate(textures or []):
            variants = texture.get("runtimePixelVariants", [
                texture["runtimePixels"],
                texture.get("runtimePixelsAlt", texture["runtimePixels"]),
            ])
            suffix = (f" | {texture.get('runtimePaintMask', '-')}"
                      if has_paint else "")
            material_rows.append(f"{index} {' '.join(variants)}{suffix}\n")
        materials_path.write_text("".join(material_rows))
        return {
            "file": f"models/{stem}.json",
            "gltf": f"models/{stem}.gltf",
            "runtimeMesh": f"models/{stem}.rmesh",
            "runtimeMaterials": f"models/{stem}.rmat",
            "modelCount": bank.count,
            "vertexCount": len(bank.vertices),
            "normalCount": len(bank.normals),
            "faceCount": sum(len(m.faces) for m in bank.models),
            "modelFaceCounts": [len(m.faces) for m in bank.models],
        }

    def emit_vram(self, v, stem):
        if not v.dirty or not self.opts.vram:
            return None
        rgba = fast_vram_rgba(v)
        p = self.out / "images" / f"{stem}_vram.png"
        png.write_rgba(p, images.VRAM_W, images.VRAM_H, rgba)
        return {"file": f"images/{stem}_vram.png", "rects": v.dirty}

    @staticmethod
    def apply_texture_window(rgba, window, pixel_size=4):
        if window is None:
            return rgba
        width_u, width_v, off_u, off_v = window
        out = bytearray(len(rgba))
        for y in range(256):
            source_y = (y % width_v) + off_v
            src = (source_y * 256 + off_u) * pixel_size
            tile = rgba[src:src + width_u * pixel_size]
            row = tile * (256 // width_u)
            dst = y * 256 * pixel_size
            out[dst:dst + 256 * pixel_size] = row
        return bytes(out)

    def emit_textures(self, v, bank, stem, alternate_vram=None,
                      variant_vrams=None, variant_clut_offsets=(0,),
                      paint_labels=None):
        pairs = {(f.tpage, f.clut, f.texwin)
                 for m in bank.models for f in m.faces if f.uv}
        pairs = sorted(pairs, key=lambda p: (p[0], p[1], p[2] or (0, 0, 0, 0)))
        out = []
        for n, (tp, cl, window) in enumerate(pairs):
            rgba = self.apply_texture_window(images.decode_texpage(v, tp, cl),
                                             window)
            fn = f"{stem}_t{n}.png"
            png.write_rgba(self.out / "textures" / fn, 256, 256, rgba)
            raw_fn = f"{stem}_t{n}.rgba"
            (self.out / "textures" / raw_fn).write_bytes(rgba)
            item = {
                "file": f"textures/{fn}",
                "runtimePixels": f"textures/{raw_fn}",
                "width": 256, "height": 256,
                "tpage": tp, "clut": cl,
                "texwin": list(window) if window is not None else None,
                "bpp": {0: 4, 1: 8, 2: 16, 3: 16}[(tp >> 7) & 3],
                "pageX": (tp & 0xF) * 64, "pageY": ((tp >> 4) & 1) * 256,
                "clutX": (cl & 0x3F) * 16, "clutY": (cl >> 6) & 0x1FF,
            }
            if paint_labels is not None:
                paint = self.apply_texture_window(
                    images.decode_texpage_labels(v, tp, cl, paint_labels),
                    window, pixel_size=1)
                if any(paint):
                    paint_fn = f"{stem}_t{n}.rpaint"
                    (self.out / "textures" / paint_fn).write_bytes(paint)
                    item["runtimePaintMask"] = f"textures/{paint_fn}"
            if alternate_vram is not None:
                alternate = self.apply_texture_window(
                    images.decode_texpage(alternate_vram, tp, cl), window)
                alt_fn = f"{stem}_t{n}_alt.rgba"
                (self.out / "textures" / alt_fn).write_bytes(alternate)
                item["runtimePixelsAlt"] = f"textures/{alt_fn}"
            if variant_vrams is not None:
                variants = []
                unique = {rgba: item["runtimePixels"]}
                for variant, variant_vram in enumerate(variant_vrams):
                    for clut_offset in variant_clut_offsets:
                        pixels = self.apply_texture_window(
                            images.decode_texpage(
                                variant_vram, tp, cl + clut_offset), window)
                        path = unique.get(pixels)
                        if path is None:
                            variant_fn = (
                                f"{stem}_t{n}_v{variant}_c{clut_offset}.rgba")
                            (self.out / "textures" / variant_fn).write_bytes(
                                pixels)
                            path = f"textures/{variant_fn}"
                            unique[pixels] = path
                        variants.append(path)
                item["runtimePixelVariants"] = variants
            out.append(item)
        return out


def write_runtime_index(out: Path, records: list[dict]) -> None:
    """Write the native renderer's compact asset lookup table."""
    rows = ["# rage-rmesh-index v1\n"]

    def add(asset, asset_set, model):
        if not isinstance(model, dict) or "runtimeMesh" not in model:
            return
        rows.append(f"{asset} {asset_set} {model['runtimeMesh']} "
                    f"{model.get('runtimeMaterials', '-')}\n")

    for rec in records:
        if not isinstance(rec.get("index"), int) or rec["index"] >= ASSET_COUNT:
            continue
        add(rec["index"], "model", rec.get("model"))
        for bank in rec.get("banks", []):
            sub = bank.get("sub") if isinstance(bank, dict) else None
            asset_set = {3: "track-model-1", 6: "track-model-2",
                         5: "course", 7: "terrain"}.get(sub)
            if asset_set is not None:
                add(rec["index"], asset_set, bank)
    (out / "runtime-index.txt").write_text("".join(rows))


def have_ffmpeg() -> bool:
    import shutil
    return bool(shutil.which("ffmpeg"))


def extract_movies(disc, str_file, movies, out: Path, want_video: bool) -> list:
    """Index every RAGE.STR movie, and decode it if ffmpeg is on the PATH.

    The index is derived here (see disc.read_stream_index); only the pixels are
    handed off. MDEC video plus XA ADPCM audio is a commodity decode that
    ffmpeg's psxstr demuxer already does correctly, and it wants the verbatim
    2352-byte sectors, which is why the .str carve is raw.
    """
    recs = []
    ff = have_ffmpeg()
    d = out / "fmv"
    if want_video and ff:
        d.mkdir(parents=True, exist_ok=True)
    for m in movies:
        rec = {
            "index": m.index,
            "lba": str_file.lba + m.sector_offset,
            "sectorOffset": m.sector_offset,
            "sectors": m.sectors,
            "frames": m.frames,
            "width": m.width,
            "height": m.height,
            "seconds": round(m.frames / float(STR_FPS), 1),
            "bytes": m.sectors * 2352,
        }
        if want_video and ff and disc.raw:
            stem = f"fmv{m.index:02d}"
            tmp = d / f"{stem}.str"
            tmp.write_bytes(disc.raw_sectors(str_file.lba + m.sector_offset, m.sectors))
            # Frame 1 of most of these is a fade in from black, so the poster
            # comes from a fifth of the way in.
            poster = d / f"{stem}.png"
            seek = max(0.0, m.frames / float(STR_FPS) / 5.0)
            _run(["ffmpeg", "-v", "error", "-y", "-ss", f"{seek:.2f}", "-i", str(tmp),
                  "-frames:v", "1", str(poster)])
            mp4 = d / f"{stem}.mp4"
            # +faststart moves the moov atom to the front. Without it a browser
            # has to fetch the whole file before the first frame appears.
            _run(["ffmpeg", "-v", "error", "-y", "-i", str(tmp),
                  "-c:v", "libx264", "-preset", "veryfast", "-crf", "24",
                  "-pix_fmt", "yuv420p", "-c:a", "aac", "-b:a", "96k",
                  "-movflags", "+faststart", str(mp4)])
            tmp.unlink(missing_ok=True)
            if mp4.exists():
                rec["video"] = f"fmv/{mp4.name}"
            if poster.exists():
                rec["poster"] = f"fmv/{poster.name}"
        elif want_video and not ff:
            rec["note"] = "ffmpeg not on PATH, so only the index was built"
        print(f"[str{m.index:2d}] {m.width}x{m.height} {m.frames:5d} frames "
              f"{rec['seconds']:6.1f}s  {'decoded' if 'video' in rec else 'indexed'}",
              flush=True)
        recs.append(rec)
    return recs


def _run(cmd) -> bool:
    import subprocess
    try:
        return subprocess.run(cmd, capture_output=True).returncode == 0
    except OSError:
        return False


class _Slice:
    """A read-only window onto an open file, for a 206 response body."""

    def __init__(self, fh, length):
        self.fh, self.left = fh, length

    def read(self, n=-1):
        if self.left <= 0:
            return b""
        if n is None or n < 0 or n > self.left:
            n = self.left
        data = self.fh.read(n)
        self.left -= len(data)
        return data

    def close(self):
        self.fh.close()


_LUT = None


def fast_vram_rgba(v: images.Vram) -> bytes:
    """Same as images.vram_to_rgba, table-driven so a full page is not a minute."""
    global _LUT
    if _LUT is None:
        _LUT = bytearray(65536 * 4)
        for w in range(65536):
            r, g, b, a = images.rgb5551(w)
            _LUT[w * 4 : w * 4 + 4] = bytes((r, g, b, 255 if a else 0))
    lut = _LUT
    src = v.buf
    out = bytearray(len(src) * 2)
    for i in range(0, len(src), 2):
        w = (src[i] | (src[i + 1] << 8)) * 4
        out[i * 2 : i * 2 + 4] = lut[w : w + 4]
    return bytes(out)


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("image", type=Path, help="Track 01 .bin of the disc (MODE2/2352) or a .iso")
    ap.add_argument("-o", "--out", type=Path, default=None,
                    help="output directory (default tools/decomp-wip/assets)")
    ap.add_argument("--exe", type=Path, default=None,
                    help="SCES_006.50 / SLUS executable, to re-derive the name table")
    ap.add_argument("--no-raw", dest="raw", action="store_false",
                    help="skip the verbatim per-asset dumps (they feed the hex view)")
    ap.add_argument("--no-audio", dest="audio", action="store_false",
                    help="skip decoding VAG sounds to .wav")
    ap.add_argument("--no-vram", dest="vram", action="store_false",
                    help="skip the 1024x512 VRAM previews (much faster)")
    ap.add_argument("--no-fmv", dest="fmv", action="store_false",
                    help="index the RAGE.STR movies but do not decode them "
                         "(decoding needs ffmpeg on the PATH)")
    ap.add_argument("--only", type=str, default=None,
                    help="comma-separated asset indices or name substrings")
    ap.add_argument("--serve", type=int, nargs="?", const=8000, default=None,
                    metavar="PORT", help="serve the output directory when done")
    args = ap.parse_args(argv)

    if args.out is None:
        args.out = Path(__file__).resolve().parents[2] / "tools" / "decomp-wip" / "assets"
    args.out.mkdir(parents=True, exist_ok=True)

    with Disc(args.image) as disc:
        label = volume_label(disc)
        files = read_root_directory(disc)
        rage_bin = find_file(files, "RAGE.BIN")
        rage_str = find_file(files, "RAGE.STR")
        entries = read_archive_index(disc, rage_bin.lba, ASSET_COUNT)

        names = ASSET_NAMES
        if args.exe:
            names = names_from_exe(args.exe)

        # Self-check: the archive must tile its own file exactly.
        end = max(e.sector_offset + (e.size + 2047) // 2048 for e in entries)
        if end * 2048 != rage_bin.size:
            print(f"warning: archive ends at sector {end}, file is "
                  f"{rage_bin.size // 2048} sectors", file=sys.stderr)

        ex = Extractor(disc, entries, names, args.out, args)
        if args.only:
            wanted = set()
            for tok in args.only.split(","):
                tok = tok.strip()
                if tok.isdigit():
                    wanted.add(int(tok))
                else:
                    wanted |= {i for i, n in enumerate(names) if tok.upper() in n.upper()}
            manifest = []
            for i in sorted(wanted):
                (args.out / "raw").mkdir(parents=True, exist_ok=True)
                (args.out / "models").mkdir(parents=True, exist_ok=True)
                (args.out / "textures").mkdir(parents=True, exist_ok=True)
                (args.out / "images").mkdir(parents=True, exist_ok=True)
                r = ex.one(i)
                manifest.append(r)
                print(f"[{i:3d}] {r['name']:<16} {r['kind']:<13} {r['size']:>9}  "
                      f"{r.get('summary','')}", flush=True)
        else:
            manifest = ex.run()

        movies = read_stream_index(disc, rage_str.lba, (rage_str.size + 2047) // 2048)
        # --only is the fast iteration path; decoding 4.5 minutes of video does
        # not belong in it.
        streams = extract_movies(disc, rage_str, movies, args.out,
                                 args.fmv and not args.only)
        # The movies are not RAGE.BIN assets, but the browser's list is the only
        # place a user can get at anything, so they join it as pseudo-assets
        # rather than being named in the header and then existing nowhere.
        for s in streams:
            manifest.append({
                "index": 1000 + s["index"],
                "name": f"RAGE.STR movie {s['index']}",
                "kind": "fmv",
                "size": s["bytes"],
                "lba": s["lba"],
                "sectorOffset": s["sectorOffset"],
                "confidence": "decoded" if s.get("video") else "raw",
                "summary": (f"{s['width']}x{s['height']}, {s['frames']} frames, "
                            f"{s['seconds']}s at {STR_FPS} fps"
                            + ("" if s.get("video") else " (not decoded)")),
                "fmv": s,
            })
        doc = {
            "disc": {
                "image": str(args.image),
                "label": label,
                "files": [{"name": f.name, "lba": f.lba, "size": f.size} for f in files
                          if not f.name.startswith("\x00") and not f.name.startswith("\x01")],
            },
            "archive": {"name": "RAGE.BIN", "lba": rage_bin.lba, "size": rage_bin.size,
                        "entries": ASSET_COUNT},
            "streams": streams,
            "assets": manifest,
        }
        out = args.out / "manifest.json"
        out.write_text(json.dumps(doc, separators=(",", ":")))
        write_runtime_index(args.out, manifest)

        # The page itself lives in the repo; only its copy sits next to the data.
        page = Path(__file__).resolve().parent / "browser.html"
        if page.exists():
            (args.out / "index.html").write_text(page.read_text())

        print(f"\nmanifest -> {out}")
        by_kind = {}
        for a in manifest:
            by_kind.setdefault(f"{a['kind']}/{a['confidence']}", 0)
            by_kind[f"{a['kind']}/{a['confidence']}"] += 1
        for k in sorted(by_kind):
            print(f"  {k:<30} {by_kind[k]}")

    if args.serve is not None:
        serve(args.out, args.serve)
    return 0


def serve(root: Path, port: int) -> None:
    import functools
    import http.server
    import os
    import re
    import socketserver

    class RangeHandler(http.server.SimpleHTTPRequestHandler):
        """SimpleHTTPRequestHandler plus byte ranges.

        The stock handler answers 200 with the whole body to every request, so
        a <video> cannot seek and the hex view's Range hint is ignored. It is
        also single-threaded, which stalls the page whenever a movie is
        streaming; ThreadingTCPServer below fixes that half.
        """

        protocol_version = "HTTP/1.1"

        def send_head(self):
            rng = self.headers.get("Range")
            m = re.fullmatch(r"bytes=(\d*)-(\d*)", rng.strip()) if rng else None
            if not m:
                return super().send_head()
            path = self.translate_path(self.path)
            if os.path.isdir(path):
                return super().send_head()
            try:
                f = open(path, "rb")
            except OSError:
                self.send_error(404, "File not found")
                return None
            size = os.fstat(f.fileno()).st_size
            start, end = m.group(1), m.group(2)
            if start:
                first = int(start)
                last = int(end) if end else size - 1
            else:                       # "bytes=-N" is the last N bytes
                first = max(0, size - int(end or 0))
                last = size - 1
            last = min(last, size - 1)
            if first > last or first >= size:
                f.close()
                self.send_response(416)
                self.send_header("Content-Range", f"bytes */{size}")
                self.send_header("Content-Length", "0")
                self.end_headers()
                return None
            f.seek(first)
            self.send_response(206)
            self.send_header("Content-type", self.guess_type(path))
            self.send_header("Accept-Ranges", "bytes")
            self.send_header("Content-Range", f"bytes {first}-{last}/{size}")
            self.send_header("Content-Length", str(last - first + 1))
            self.end_headers()
            return _Slice(f, last - first + 1)

        def log_message(self, *a):
            pass

    handler = functools.partial(RangeHandler, directory=str(root))
    socketserver.ThreadingTCPServer.allow_reuse_address = True
    socketserver.ThreadingTCPServer.daemon_threads = True
    with socketserver.ThreadingTCPServer(("127.0.0.1", port), handler) as httpd:
        print(f"\nbrowser -> http://127.0.0.1:{port}/    (ctrl-c to stop)")
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print()


if __name__ == "__main__":
    raise SystemExit(main())
