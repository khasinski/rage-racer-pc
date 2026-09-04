#!/usr/bin/env python3
"""A movie plays for as long as its own soundtrack lasts.

RAGE.STR carries no frame rate and the eleven movies do not share one: on the
PAL disc the opening movie runs at twenty-five frames a second and the other
ten at fifteen, and the American pressing encodes most of them at twice that.
What they all share is the disc. At double speed the drive delivers 150 sectors
a second, a frame appears once the sectors carrying it have been read, and the
XA soundtrack is interleaved into those same sectors, so playing the stream at
the rate the drive would deliver it is what holds picture and sound together.

Two things have to be true for that, and this checks both against the disc
rather than against the port:

  * the movie takes as long to play as the soundtrack buried in the sectors it
    plays out of, so the picture cannot run ahead of the sound; and
  * the sectors arrive at the drive's 150 a second.

The first is the one that matters, and it is deliberately not derived from the
trace's own sector column. This file walks the STR chunk headers itself to find
which sector each frame really ends on, and counts the XA sectors between those
positions to get the seconds of sound they carry. Reading the sector range back
out of the trace instead would make the check vacuous: a port that lost a
sector per frame would report a shorter range, the shorter range would hold
proportionally less audio, and the wrong rate would agree with itself. The
number of frames the disc spends per second of its soundtrack is a property of
the disc, and the port has to match it.

It also checks that a movie the game loads assets behind still has sound, which
the opening movie cannot show because nothing loads while it plays.
"""

from __future__ import annotations

import os
import re
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "support"))

from disc_image import SKIP_EXIT_CODE, find_disc, require_disc

SECTORS_PER_SECOND = 150
RAW_SECTOR_SIZE = 2352
ISO_SECTOR_SIZE = 2048
# Mode 2 subheader, then the sector's user data.
SUBHEADER_OFFSET = 16
USER_DATA_OFFSET = 24
SUBMODE_AUDIO = 0x04
STR_MAGIC = 0x80010160
STREAM_COUNT = 11
# Every XA sector on this disc is coding info 0x01: stereo, 37800 Hz, 4-bit,
# which is 2016 stereo frames of sound per sector.
XA_FRAMES_PER_SECTOR = 2016
XA_SAMPLE_RATE = 37800

# Two movies with different rates keep the strict picture-to-XA pacing check:
# one class promotion and the opening movie.  The all-audio mode below visits
# every stream, including the ending's deliberately silent picture tail.
PACING_STREAMS = (5, 0)
ALL_STREAMS = tuple(range(STREAM_COUNT))


def play(executable: str, source_dir: str, stream: int, frames: int,
         pcm: Path) -> tuple[str, bytes]:
    environment = os.environ.copy()
    environment.update(
        SDL_AUDIODRIVER="dummy",
        # Boot/prologue takes about 300 ticks.  The slower 15 fps movies need
        # four ticks per frame on a 60 Hz disc; that upper bound also covers
        # PAL, then leaves transition room after the final movie.
        RAGE_PORT_SMOKE_FRAMES=str(frames * 4 + 900),
        RAGE_PORT_FMV_TRACE="1",
        RAGE_PORT_SMOKE_AUDIO_METRICS="1",
        PSYZ_AUDIO_PCM_DUMP=str(pcm),
    )
    result = subprocess.run(
        [
            executable,
            "--set",
            f"diagnostics.fmv_stream={stream}",
            "--set",
            "video.renderer=classic",
        ],
        cwd=source_dir, env=environment, stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT, text=True, timeout=540,
    )
    if result.returncode != 0:
        print(result.stdout, file=sys.stderr)
        raise AssertionError(f"stream {stream} exited {result.returncode}")
    if not pcm.exists():
        raise AssertionError(f"stream {stream} emitted no PCM capture")
    return result.stdout, pcm.read_bytes()


def cue_track_one(cue: Path) -> tuple[Path, int]:
    """The data track's image file and where it starts in it, following the
    same CUE semantics as DiscCueResolveDataTrack."""
    image = None
    data_track = False
    for line in cue.read_text(errors="replace").splitlines():
        text = line.strip()
        if text.upper().startswith("FILE"):
            quoted = re.search(r'"([^"]+)"', text)
            image = cue.parent / (quoted.group(1) if quoted else text.split()[1])
        elif text.upper().startswith("TRACK"):
            data_track = "MODE1/2352" in text.upper() or "MODE2/2352" in text.upper()
        elif data_track and text.upper().startswith("INDEX 01"):
            minute, second, frame = (int(p) for p in text.split()[-1].split(":"))
            return image, (minute * 60 + second) * 75 + frame
    raise AssertionError(f"{cue} has no 2352-byte data track")


class Disc:
    """Just enough of the disc to find its movies and read raw sectors.

    Where the eleven movies sit inside RAGE.STR, and how many frames of each
    the game shows, is a property of the pressing rather than of the port, so
    this reads both off the disc the same way HostInitDisc does: a movie ends
    where its frame numbering restarts, and those eleven offsets then appear in
    the disc's own boot executable with the frame counts beside them.
    """

    def __init__(self, path: Path) -> None:
        if path.suffix.lower() == ".cue":
            image, self.first_sector = cue_track_one(path)
        else:
            image, self.first_sector = path, 0
        self.file = image.open("rb")
        self.user_offset = 24
        for offset in (16, 24):
            if self.raw(16)[offset + 1:offset + 6] == b"CD001":
                self.user_offset = offset
                break
        self.files = self.read_root()
        self.stream_sector, stream_size = self.files["RAGE.STR"]
        self.stream_sectors = (stream_size + ISO_SECTOR_SIZE - 1) // ISO_SECTOR_SIZE
        self.boot = self.read_boot_name()
        self.offsets = self.scan_stream_starts()
        self.frames = self.read_stream_frames()
        self.spans = tuple(
            (self.offsets[index + 1] if index + 1 < STREAM_COUNT
             else self.stream_sectors) - self.offsets[index]
            for index in range(STREAM_COUNT))

    def raw(self, sector: int) -> bytes:
        self.file.seek((self.first_sector + sector) * RAW_SECTOR_SIZE)
        return self.file.read(RAW_SECTOR_SIZE)

    def user(self, sector: int) -> bytes:
        data = self.raw(sector)
        return data[self.user_offset:self.user_offset + ISO_SECTOR_SIZE]

    def read_root(self) -> dict[str, tuple[int, int]]:
        volume = self.user(16)
        root = struct.unpack("<I", volume[158:162])[0]
        size = struct.unpack("<I", volume[166:170])[0]
        entries: dict[str, tuple[int, int]] = {}
        for offset in range(0, size, ISO_SECTOR_SIZE):
            directory = self.user(root + offset // ISO_SECTOR_SIZE)
            cursor = 0
            while cursor < ISO_SECTOR_SIZE:
                length = directory[cursor]
                if length == 0:
                    break
                record = directory[cursor:cursor + length]
                name = bytes(record[33:33 + record[32]]).decode("latin-1")
                entries.setdefault(
                    name.split(";")[0].upper(),
                    (struct.unpack("<I", record[2:6])[0],
                     struct.unpack("<I", record[10:14])[0]))
                cursor += length
        if "RAGE.STR" not in entries:
            raise AssertionError("no RAGE.STR on the disc")
        return entries

    def read_file(self, name: str) -> bytes:
        lba, size = self.files[name]
        sectors = (size + ISO_SECTOR_SIZE - 1) // ISO_SECTOR_SIZE
        return b"".join(self.user(lba + i) for i in range(sectors))[:size]

    def stream_bytes(self, stream: int) -> bytes:
        """Every raw sector of one movie."""
        first = self.stream_sector + self.offsets[stream]
        self.file.seek((self.first_sector + first) * RAW_SECTOR_SIZE)
        return self.file.read(self.spans[stream] * RAW_SECTOR_SIZE)

    def read_boot_name(self) -> str:
        """SYSTEM.CNF's BOOT line names the executable, which is the serial."""
        text = self.read_file("SYSTEM.CNF").decode("latin-1")
        boot = re.search(r"BOOT\s*=\s*(?:cdrom:)?[\\/]*([^;\s]+)", text, re.I)
        if boot is None:
            raise AssertionError("SYSTEM.CNF names no boot executable")
        return boot.group(1).upper()

    def scan_stream_starts(self) -> tuple[int, ...]:
        """A movie ends where the frame numbering restarts."""
        starts: list[int] = []
        previous = None
        for index in range(self.stream_sectors):
            sector = self.raw(self.stream_sector + index)
            if sector[SUBHEADER_OFFSET + 2] & SUBMODE_AUDIO:
                continue
            body = sector[USER_DATA_OFFSET:USER_DATA_OFFSET + 16]
            if struct.unpack("<I", body[0:4])[0] != STR_MAGIC:
                continue
            chunk = struct.unpack("<H", body[4:6])[0]
            frame = struct.unpack("<I", body[8:12])[0]
            if previous is None or (chunk == 0 and frame < previous):
                starts.append(index)
            previous = frame
        if len(starts) != STREAM_COUNT:
            raise AssertionError(
                f"RAGE.STR holds {len(starts)} movies, expected {STREAM_COUNT}")
        return tuple(starts)

    def read_stream_frames(self) -> tuple[int, ...]:
        """The game's stream table: eleven sector offsets, each beside the last
        frame the game shows of that movie."""
        blob = self.read_file(self.boot)
        wanted = b"".join(struct.pack("<I", o) for o in self.offsets)
        for offset in range(0, len(blob) - STREAM_COUNT * 8 + 1, 4):
            entries = blob[offset:offset + STREAM_COUNT * 8]
            if b"".join(entries[i * 8:i * 8 + 4]
                        for i in range(STREAM_COUNT)) != wanted:
                continue
            return tuple(struct.unpack_from("<I", entries, i * 8 + 4)[0]
                         for i in range(STREAM_COUNT))
        raise AssertionError(f"{self.boot} holds no matching stream table")


def read_stream(disc: Disc, stream: int) -> tuple[list[int], list[int]]:
    """Walks a movie's sectors, chunk header by chunk header.

    Returns where each frame ends, counted in sectors from the start of the
    movie, and a running count of the XA audio sectors passed on the way, so
    the sound carried between any two frames can be read straight off.
    """
    data = disc.stream_bytes(stream)
    count = disc.spans[stream]
    ends: list[int] = []
    audio = [0] * (count + 1)
    chunks = 0
    seen = 0
    in_frame = False
    for index in range(count):
        sector = data[index * RAW_SECTOR_SIZE:(index + 1) * RAW_SECTOR_SIZE]
        header = sector[USER_DATA_OFFSET:USER_DATA_OFFSET + 8]
        video = struct.unpack("<I", header[0:4])[0] == STR_MAGIC
        submode = sector[SUBHEADER_OFFSET + 2]
        audio[index + 1] = audio[index] + (
            1 if not video and submode & SUBMODE_AUDIO else 0)
        if not video:
            continue
        chunk = struct.unpack("<H", header[4:6])[0]
        if chunk == 0:
            in_frame = True
            chunks = struct.unpack("<H", header[6:8])[0]
            seen = 0
        elif not in_frame:
            continue  # a frame already under way when the movie was entered
        seen += 1
        if chunks != 0 and seen >= chunks:
            ends.append(index + 1)
            in_frame = False
    return ends, audio


def trace_run(output: str, stream: int) -> list[tuple[int, int, int]]:
    trace = [
        (int(m.group(1)), int(m.group(2)), int(m.group(3)))
        for m in re.finditer(
            r"fmv frame=(\d+) vblank=(\d+) scene_timer=\d+ sector=(\d+)", output)
    ]
    # The title screen replays a movie, so keep only the first run of it.
    run: list[tuple[int, int, int]] = []
    for entry in trace:
        if entry[0] == 0 and run:
            break
        run.append(entry)
    if not run:
        raise AssertionError(f"stream {stream} decoded no frames")
    return run


def base_hz(output: str, stream: int) -> float:
    """The tick rate the port reports. Tests run the game unthrottled, so the
    vblank count is the only clock there is."""
    reported = re.search(r"base_hz=(\d+)", output)
    if reported is None:
        raise AssertionError(f"stream {stream} did not report its tick rate")
    return float(reported.group(1))


def check_pacing(output: str, disc: Disc, stream: int, shown: int) -> None:
    """The movie must take its soundtrack's own length to play."""
    run = trace_run(output, stream)
    if run[-1][0] + 1 != shown:
        raise AssertionError(
            f"stream {stream} showed {run[-1][0] + 1} frames, expected {shown}")
    vblanks = run[-1][1] - run[0][1]
    if vblanks <= 0:
        raise AssertionError(f"stream {stream} played in no time at all")
    seconds = vblanks / base_hz(output, stream)

    ends, audio = read_stream(disc, stream)
    if len(ends) < shown:
        raise AssertionError(
            f"stream {stream} holds {len(ends)} frames, the game shows {shown}")
    # What the disc spends on the frames after the first one, which is the part
    # of the movie the trace timed.
    first, last = ends[0], ends[shown - 1]
    soundtrack = (audio[last] - audio[first]) * XA_FRAMES_PER_SECTOR / XA_SAMPLE_RATE
    if soundtrack <= 0:
        raise AssertionError(f"stream {stream} has no soundtrack to play against")
    authored = (shown - 1) / soundtrack
    played = (shown - 1) / seconds
    if not 0.98 <= played / authored <= 1.02:
        raise AssertionError(
            f"stream {stream} played {played:.2f} frames a second against a "
            f"soundtrack authored for {authored:.2f}: {shown - 1} frames in "
            f"{seconds:.2f}s where the sound between them lasts "
            f"{soundtrack:.2f}s")

    # And the frames came out of sectors arriving at the rate the drive reads.
    arrived = (run[-1][2] - run[0][2]) / seconds
    if not 0.98 <= arrived / SECTORS_PER_SECOND <= 1.02:
        raise AssertionError(
            f"stream {stream} took its sectors at {arrived:.1f} a second, "
            f"expected {SECTORS_PER_SECOND}")

    # The port's own cursor has to agree with where those frames really end,
    # which is what makes the two rates above answer the same question.
    if (run[0][2], run[-1][2]) != (first, last):
        raise AssertionError(
            f"stream {stream} put frames 0 and {shown - 1} at sectors "
            f"{run[0][2]} and {run[-1][2]}; the disc ends them at "
            f"{first} and {last}")


def check_soundtrack(output: str, pcm: bytes, stream: int) -> None:
    """The movie asked the drive for its soundtrack and got one.

    This used to compare the mean amplitude of everything the mixer rendered
    against a threshold, which separated a working soundtrack from one an
    asset load had paused: 4818 against 1002. It does not separate them any
    more. These runs are unthrottled, so a movie plays perhaps three times
    faster than its own soundtrack and the scene ends while the audio is still
    going; how much of the run is movie therefore depends on how fast the host
    got through the rest, and the two cases now sit at 2442 and 2106.

    Whether the soundtrack survives is answered by playing a movie at its real
    rate and looking: on the throttled build a class ending holds its audio for
    the full 10.5 seconds it lasts. What is left here is the part that stays
    true regardless of speed.
    """
    if "fmv xa start" not in output:
        raise AssertionError(f"stream {stream} never started its soundtrack")
    metrics = re.search(r"audio metrics: frames=(\d+) energy=(\d+)", output)
    if metrics is None:
        raise AssertionError(f"stream {stream} reported no audio metrics")
    frames, energy = int(metrics.group(1)), int(metrics.group(2))
    if frames < 10_000 or energy == 0:
        raise AssertionError(f"stream {stream} rendered no audio at all")
    expected_pcm_size = frames * 2 * 2  # stereo, signed 16-bit samples
    if len(pcm) != expected_pcm_size:
        raise AssertionError(
            f"stream {stream} emitted {len(pcm)} PCM bytes for {frames} "
            f"stereo frames, expected {expected_pcm_size}")
    if not any(pcm):
        raise AssertionError(f"stream {stream} emitted only silent PCM")


def check_soundtrack_tail(output: str, stream: int) -> None:
    if "fmv video end: xa tail continues" not in output:
        raise AssertionError(
            f"stream {stream} cut XA audio at its last displayed frame")


def check_complete_decode(output: str, stream: int, shown: int) -> None:
    run = trace_run(output, stream)
    if run[-1][0] + 1 != shown:
        raise AssertionError(
            f"stream {stream} showed {run[-1][0] + 1} frames, expected {shown}")


def main() -> int:
    executable, source_dir = sys.argv[1], sys.argv[2]
    all_audio = len(sys.argv) == 4 and sys.argv[3] == "--all-audio"
    if len(sys.argv) > 3 and not all_audio:
        raise AssertionError("usage: verify_fmv_pacing.py EXECUTABLE SOURCE [--all-audio]")
    source = Path(source_dir)
    require_disc(source)
    image = find_disc(source)
    if image.suffix.lower() == ".chd":
        print("the disc is a CHD, which this test cannot read sectors out of. "
              "Point RAGE_PORT_DISC_CUE at a cue sheet to run it.")
        return SKIP_EXIT_CODE
    disc = Disc(image)
    print(f"disc {disc.boot}: "
          + " ".join(f"{offset:04X}/{frames}"
                     for offset, frames in zip(disc.offsets, disc.frames)))
    with tempfile.TemporaryDirectory(prefix="rage-all-fmv-audio-") as directory:
        root = Path(directory)
        for stream in ALL_STREAMS if all_audio else PACING_STREAMS:
            shown = disc.frames[stream]
            output, pcm = play(executable, source_dir, stream, shown,
                               root / f"stream-{stream}.s16le")
            check_complete_decode(output, stream, shown)
            check_soundtrack(output, pcm, stream)
            if not all_audio:
                check_pacing(output, disc, stream, shown)
                check_soundtrack_tail(output, stream)
    if all_audio:
        print("every retail FMV fully decodes and emits non-silent PCM")
    else:
        print("representative movies play for as long as their soundtracks last")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
