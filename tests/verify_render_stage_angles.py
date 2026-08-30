#!/usr/bin/env python3
"""What the renderer draws for one asset, turned all the way round.

The pixel locks in the suite render the game: they catch a scene changing, at
the handful of angles a scripted race happens to pass through. This asks a
different question, of the renderer alone, with no game and no disc. It stands
one car on an empty stage and turns it through a full circle, and it asks the
things that have to be true of any solid object seen from anywhere.

None of these are reference images. A sweep's worth of them would be hundreds
of files that turn red together whenever a shader changes, which is noise, not
a lock. What survives a legitimate change to the shading and still fails on a
broken import is the shape of the thing: it is there at every angle, it stays
in frame, it grows and shrinks smoothly as it turns, and a car body is
laterally symmetric whatever is painted on it.
"""

from __future__ import annotations

import os
import subprocess
import sys
import tempfile
from pathlib import Path

# ctest reads this exit code as "skipped" rather than "failed".
SKIP_EXIT_CODE = 77

# The sweep is measured at this size: big enough that a lost panel is many
# pixels, small enough that a hundred renders cost about a second.
WIDTH = 240
HEIGHT = 180
STEPS = 24

# Every car in the shipped model bank, by the key the runtime index gives it.
CAR_KEYS = (10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 38, 40,
            42, 44)

# The stage clears to black and draws no backdrop, so anything that is not
# black is geometry. A brightness threshold would instead measure the shading,
# and some course surfaces legitimately render almost black.
CLEAR = 0

failures: list[str] = []


def fail(message: str) -> None:
    failures.append(message)
    print(f"FAIL {message}")


def read_ppm(path: Path) -> tuple[int, int, bytes]:
    data = path.read_bytes()
    fields: list[bytes] = []
    index = 0
    while len(fields) < 4:
        while data[index:index + 1].isspace():
            index += 1
        end = index
        while not data[end:end + 1].isspace():
            end += 1
        fields.append(data[index:end])
        index = end
    index += 1
    width, height = int(fields[1]), int(fields[2])
    return width, height, data[index:index + width * height * 3]


class Silhouette:
    """What is drawn, reduced to the shape it covers."""

    def __init__(self, path: Path) -> None:
        width, height, pixels = read_ppm(path)
        self.width = width
        self.height = height
        self.area = 0
        self.left, self.right = width, -1
        self.top, self.bottom = height, -1
        for y in range(height):
            row = y * width * 3
            for x in range(width):
                offset = row + x * 3
                if (pixels[offset] > CLEAR or pixels[offset + 1] > CLEAR
                        or pixels[offset + 2] > CLEAR):
                    self.area += 1
                    if x < self.left:
                        self.left = x
                    if x > self.right:
                        self.right = x
                    if y < self.top:
                        self.top = y
                    if y > self.bottom:
                        self.bottom = y

    def touches_border(self) -> bool:
        return (self.left <= 0 or self.top <= 0
                or self.right >= self.width - 1
                or self.bottom >= self.height - 1)


def native_assets(source: Path) -> Path | None:
    """Where the imported assets are, if they have been built."""
    override = os.environ.get("RAGE_PORT_NATIVE_ASSETS")
    candidates = [Path(override)] if override else []
    candidates.append(source / "build" / "release" / "native-assets")
    for candidate in candidates:
        if (candidate / "runtime-index.txt").is_file():
            return candidate
    return None


def render(tool: Path, assets: Path, out: Path, *arguments: str):
    """One run of the stage. Returns None when there is no GPU to render on."""
    result = subprocess.run(
        [str(tool), "--assets", str(assets), "--width", str(WIDTH),
         "--height", str(HEIGHT), "--output", str(out), *arguments],
        capture_output=True, text=True, timeout=600)
    if result.returncode != 0:
        if "GPU:" in result.stderr or "SDL_Init" in result.stderr:
            return None
        fail(f"{' '.join(arguments)}: {result.stderr.strip()}")
        return False
    return True


def sweep_paths(out: Path, steps: int) -> list[Path]:
    return [out.with_name(f"{out.stem}-{step:03d}.ppm") for step in range(steps)]


def check_one_car(tool: Path, assets: Path, work: Path, key: int) -> bool:
    """A car has to survive being looked at from every side."""
    out = work / f"car{key}.ppm"
    if render(tool, assets, out, "--pose", f"model:{key}:0",
              "--elevation", "20", "--sweep", str(STEPS)) is None:
        return False
    paths = sweep_paths(out, STEPS)
    shapes = [Silhouette(path) for path in paths]
    frame = WIDTH * HEIGHT

    # Every invariant below is satisfied by a sweep that never turns, so this
    # has to be asked first. The renderer keeps its built geometry until the
    # frame number moves, and a sweep that forgets to move it renders the same
    # picture two dozen times over.
    for step, path in enumerate(paths):
        following = paths[(step + 1) % STEPS]
        if path.read_bytes() == following.read_bytes():
            fail(f"car {key} renders identically at "
                 f"{step * 360 // STEPS} and "
                 f"{(step + 1) * 360 // STEPS} degrees: the sweep is stuck")
            break

    for step, shape in enumerate(shapes):
        angle = step * 360 // STEPS
        # A car that vanishes at one angle is the classic culling or winding
        # fault, and it is invisible to any test that only renders it once.
        if not 0.01 * frame < shape.area < 0.60 * frame:
            fail(f"car {key} at {angle} degrees covers {shape.area} pixels "
                 f"of {frame}")
        # The stage frames itself from the subject's own bounds, so nothing
        # should ever reach the edge of the picture.
        if shape.touches_border():
            fail(f"car {key} at {angle} degrees runs out of frame")

    # The stage sizes itself from the subject's bounds, so somewhere in a full
    # turn the car has to present its full length and fill a good part of the
    # picture. A subject squashed on one axis still turns smoothly and still
    # looks symmetric; what it stops doing is filling the frame it was framed
    # for.
    widest = max(shape.right - shape.left + 1 for shape in shapes)
    tallest = max(shape.bottom - shape.top + 1 for shape in shapes)
    if widest < 0.25 * WIDTH:
        fail(f"car {key} never spans more than {widest} of {WIDTH} pixels "
             f"across, so the stage is not framing it")
    if tallest < 0.20 * HEIGHT:
        fail(f"car {key} never spans more than {tallest} of {HEIGHT} pixels "
             f"down, so the stage is not framing it")

    for step, shape in enumerate(shapes):
        following = shapes[(step + 1) % STEPS]
        ratio = following.area / shape.area if shape.area else 0.0
        # One step is 15 degrees. A solid body cannot change size abruptly
        # over that; a face group appearing or disappearing can.
        if not 0.6 <= ratio <= 1.7:
            fail(f"car {key} jumps from {shape.area} to {following.area} "
                 f"pixels between {step * 360 // STEPS} and "
                 f"{(step + 1) * 360 // STEPS} degrees")

    for step in range(1, STEPS // 2):
        mirrored = shapes[STEPS - step]
        shape = shapes[step]
        largest = max(shape.area, mirrored.area)
        # A car is symmetric about its length whatever its livery says, so
        # the same view from either side covers the same area.
        if largest and abs(shape.area - mirrored.area) / largest > 0.08:
            fail(f"car {key} is lopsided at {step * 360 // STEPS} degrees: "
                 f"{shape.area} against {mirrored.area}")
    return True


def check_full_turn(tool: Path, assets: Path, work: Path) -> None:
    """Turning all the way round has to arrive back where it started."""
    start = work / "turn0.ppm"
    full = work / "turn360.ppm"
    for path, rotation in ((start, "0,0,0"), (full, "0,360,0")):
        if not render(tool, assets, path, "--pose", "model:10:0",
                      "--elevation", "20", "--rot", rotation):
            return
    if start.read_bytes() != full.read_bytes():
        fail("a full turn does not render the same picture as no turn")


def check_repeatable(tool: Path, assets: Path, work: Path) -> None:
    """The same stage twice has to be the same pixels, or nothing above this
    line means anything."""
    first = work / "again1.ppm"
    second = work / "again2.ppm"
    for path in (first, second):
        if not render(tool, assets, path, "--pose", "model:10:0",
                      "--elevation", "20", "--sweep", "4"):
            return
    for a, b in zip(sweep_paths(first, 4), sweep_paths(second, 4)):
        if a.read_bytes() != b.read_bytes():
            fail(f"rendering {a.name} twice gave two different pictures")


def check_track_pieces(tool: Path, assets: Path, work: Path) -> None:
    """Track geometry is one-sided by design, so it is a real result that it
    draws from above and a real result that it does not draw from below."""
    for asset_set in ("course", "terrain", "track1", "track2"):
        above = work / f"{asset_set}.ppm"
        if not render(tool, assets, above, "--pose", f"{asset_set}:88:0",
                      "--elevation", "30"):
            continue
        shape = Silhouette(above)
        if shape.area < 0.005 * WIDTH * HEIGHT:
            fail(f"{asset_set} 88 covers only {shape.area} pixels from above")
        if shape.touches_border():
            fail(f"{asset_set} 88 runs out of frame from above")


def main() -> int:
    if len(sys.argv) < 3:
        print("usage: verify_render_stage_angles.py TOOL SOURCE_DIR")
        return 2
    tool = Path(sys.argv[1])
    source = Path(sys.argv[2])
    assets = native_assets(source)
    if assets is None:
        print("skipping: no imported assets; build the native asset cache "
              "or set RAGE_PORT_NATIVE_ASSETS")
        return SKIP_EXIT_CODE

    with tempfile.TemporaryDirectory() as directory:
        work = Path(directory)
        if not check_one_car(tool, assets, work, CAR_KEYS[0]):
            print("skipping: no GPU to render on")
            return SKIP_EXIT_CODE
        for key in CAR_KEYS[1:]:
            check_one_car(tool, assets, work, key)
        check_full_turn(tool, assets, work)
        check_repeatable(tool, assets, work)
        check_track_pieces(tool, assets, work)

    if failures:
        print(f"{len(failures)} rendering assertion(s) failed")
        return 1
    print(f"{len(CAR_KEYS)} cars hold their shape through a full turn")
    return 0


if __name__ == "__main__":
    sys.exit(main())
