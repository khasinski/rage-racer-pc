#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import argparse
import os
import shutil
import struct
import subprocess
import sys
import tempfile
import re
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
@dataclass(frozen=True)
class Version:
    name: str
    serial: str
    exe_name: str
    sha1: str


VERSIONS = [
    Version(
        name="PAL",
        serial="SCES-006.50",
        exe_name="SCES_006.50",
        sha1="2913e15648eddef40821c5f666460abc04155ee6",
    ),
    Version(
        name="USA",
        serial="SLUS-004.03",
        exe_name="SLUS_004.03",
        sha1="2661e8bf18d209c98fd70d33e18ab88b10abd52b",
    ),
]


def force_symlink(src: Path, dest: Path) -> None:
    src = src.resolve()
    if dest.exists() and dest.resolve() == src:
        return
    dest.parent.mkdir(parents=True, exist_ok=True)
    if dest.exists() or dest.is_symlink():
        dest.unlink()
    os.symlink(src, dest)


def raw2352_to_iso(track_path: Path, iso_path: Path) -> None:
    iso_path.parent.mkdir(parents=True, exist_ok=True)
    with track_path.open("rb") as src, iso_path.open("wb") as dest:
        while True:
            sector = src.read(2352)
            if not sector:
                break
            if len(sector) != 2352:
                raise ValueError(f"{track_path}: partial sector of {len(sector)} bytes")
            dest.write(sector[24 : 24 + 2048])


def extract_from_iso(iso_path: Path, out_dir: Path, *names: str) -> None:
    out_dir.mkdir(parents=True, exist_ok=True)
    cmd = ["7z", "x", "-y", f"-o{out_dir}", str(iso_path), *names]
    result = subprocess.run(cmd, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    # 7z reports "Unexpected end of archive" because the ISO filesystem includes
    # CDDA extents outside track 1. The requested small files still extract.
    missing = [name for name in names if not (out_dir / name).exists()]
    if missing:
        sys.stderr.write(result.stdout)
        raise RuntimeError(f"failed to extract {', '.join(missing)} from {iso_path}")


def psx_header(path: Path) -> dict[str, int]:
    data = path.read_bytes()[:0x800]
    if data[:8] != b"PS-X EXE":
        raise ValueError(f"{path}: not a PS-X EXE")
    return {
        "entry": struct.unpack_from("<I", data, 0x10)[0],
        "text_addr": struct.unpack_from("<I", data, 0x18)[0],
        "text_size": struct.unpack_from("<I", data, 0x1C)[0],
        "stack": struct.unpack_from("<I", data, 0x30)[0],
    }


def sha1(path: Path) -> str:
    h = hashlib.sha1()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def cue_tracks(cue: Path) -> list[Path]:
    names = re.findall(r'^FILE\s+"([^"]+)"', cue.read_text(errors="replace"), re.MULTILINE)
    if not names:
        raise ValueError(f"{cue}: no FILE entries")
    return [cue.parent / name for name in names]


def stage(version: Version, cue: Path) -> None:
    cue = cue.expanduser().resolve()
    if not cue.is_file():
        raise FileNotFoundError(cue)
    tracks = cue_tracks(cue)
    for track in tracks:
        if not track.is_file():
            raise FileNotFoundError(track)
    track01 = tracks[0]

    disc_dir = ROOT / "disc" / version.name
    asset_dir = ROOT / "assets" / version.name
    build_dir = ROOT / "build" / "extract" / version.name

    force_symlink(cue, disc_dir / cue.name)
    for track in tracks:
        force_symlink(track, disc_dir / track.name)

    iso_path = build_dir / "track01.iso"
    raw2352_to_iso(track01, iso_path)
    with tempfile.TemporaryDirectory(prefix=f"rage-{version.name.lower()}-") as tmp:
        extracted = Path(tmp)
        extract_from_iso(iso_path, extracted, version.exe_name, "SYSTEM.CNF")
        asset_dir.mkdir(parents=True, exist_ok=True)
        shutil.copy2(extracted / version.exe_name, asset_dir / "main.exe")
        shutil.copy2(extracted / "SYSTEM.CNF", asset_dir / "SYSTEM.CNF")

    actual = sha1(asset_dir / "main.exe")
    if actual != version.sha1:
        raise RuntimeError(f"{version.name}: SHA-1 mismatch: expected {version.sha1}, got {actual}")

    header = psx_header(asset_dir / "main.exe")
    print(
        f"{version.name}: {version.serial} staged, SHA-1 {actual}, "
        f"entry=0x{header['entry']:08X}, text=0x{header['text_addr']:08X}/0x{header['text_size']:X}"
    )


def main() -> int:
    parser = argparse.ArgumentParser(description="Stage legally obtained Rage Racer disc images")
    parser.add_argument("--pal-cue", type=Path, help="path to the PAL .cue file")
    parser.add_argument("--usa-cue", type=Path, help="path to the USA .cue file")
    args = parser.parse_args()
    requested = [(VERSIONS[0], args.pal_cue), (VERSIONS[1], args.usa_cue)]
    if not any(cue is not None for _, cue in requested):
        parser.error("provide --pal-cue and/or --usa-cue")
    for version, cue in requested:
        if cue is not None:
            stage(version, cue)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
