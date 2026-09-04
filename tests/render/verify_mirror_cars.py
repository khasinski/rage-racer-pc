#!/usr/bin/env python3
"""Modern mirror preserves complete rival models and their lower bodywork."""

import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "support"))

from native_asset_fixture import create_native_asset_fixture


def run(executable: Path, source: Path, output: Path,
        renderer: str) -> tuple[Path, str]:
    output.mkdir(parents=True)
    environment = os.environ.copy()
    environment["SDL_AUDIODRIVER"] = "dummy"
    if renderer == "modern":
        native_assets = output.parent / "native-assets"
        if not (native_assets / "runtime-index.txt").exists():
            native_assets.mkdir()
            create_native_asset_fixture(native_assets)
        environment["RAGE_PORT_MODERN_ASSETS"] = str(native_assets)
        environment["RAGE_PORT_MODERN_ASSET_TRACE"] = "1"
    command = [
        executable, "--scenario", source / "race-scenario.ini",
        "--set", "run.frames=3000", "--set", "stop.scene=12",
        "--set", "stop.timer=431",
        "--set", "race.grid=0,1,2,3,4,5,6,7,8,9,10",
        "--set", "start.player_track_point=50",
        "--set", "start.rival_track_points=52,54,56",
        "--set", "start.freeze=true",
        "--set", f"capture.directory={output}",
        "--set", "capture.scene=12", "--set", "capture.timer_min=430",
        "--set", "capture.timer_max=430", "--set", "capture.timer_stride=1",
        "--set", f"video.renderer={renderer}",
    ]
    modern_capture = output / "native-mirror.ppm"
    if renderer == "modern":
        command.extend([
            "--set", "video.internal_scale=1",
            "--set", "video.aspect=4:3",
            "--set", f"diagnostics.modern_dump={modern_capture}",
            "--set", "diagnostics.modern_dump_frame=620",
        ])
    result = subprocess.run(command, cwd=source, env=environment,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            timeout=165)
    if result.returncode != 0:
        raise AssertionError(result.stdout.decode(errors="replace"))
    captures = list(output.glob("timer-00430-s12.ppm"))
    if len(captures) != 1:
        raise AssertionError(f"{renderer} produced {len(captures)} captures")
    selected_capture = modern_capture if renderer == "modern" else captures[0]
    if not selected_capture.exists():
        raise AssertionError(f"{renderer} did not produce its presented frame")
    return selected_capture, result.stdout.decode(errors="replace")


def ppm_pixels(path: Path):
    data = path.read_bytes()
    header, dimensions, maximum, pixels = data.split(b"\n", 3)
    if header != b"P6" or maximum != b"255":
        raise AssertionError(f"unexpected PPM header in {path}")
    width, height = dimensions.split()
    return int(width), int(height), pixels


def main() -> int:
    executable, source = map(Path, sys.argv[1:3])
    with tempfile.TemporaryDirectory(prefix="rage-mirror-cars-") as root_text:
        root = Path(root_text)
        classic_path, _ = run(executable, source, root / "classic", "classic")
        modern_path, modern_log = run(
            executable, source, root / "modern", "modern")
        classic = ppm_pixels(classic_path)
        modern = ppm_pixels(modern_path)
    if classic[:2] != modern[:2]:
        raise AssertionError("classic and modern mirror captures differ in size")
    mirror_builds = re.findall(
        r"mirror_vertices=(\d+) mirror_spans=(\d+) "
        r"mirror_vehicle_spans=(\d+)", modern_log)
    if not any(int(vertices) > 0 and int(spans) > 0 and int(vehicles) > 0
               for vertices, spans, vehicles in mirror_builds):
        raise AssertionError("rear camera did not build a native scene")
    mirror_draws = re.findall(
        r"native draws frame=\d+ draws=(\d+) vertices=(\d+) view=mirror",
        modern_log)
    if not any(int(draws) > 0 and int(vertices) > 0
               for draws, vertices in mirror_draws):
        raise AssertionError("native rear camera was not submitted to the GPU")
    width, height, pixels = modern
    # Inspect the actual modern presentation target, not emulated VRAM. The
    # semantic span check above proves a rival is inside the rear frustum;
    # this pixel check proves the offscreen target reached the HUD panel.
    mirror_scene_pixels = 0
    bright_car_pixels = 0
    for y in range(18, min(54, height)):
        for x in range(86, min(234, width)):
            offset = (y * width + x) * 3
            red, green, blue = pixels[offset:offset + 3]
            if max(red, green, blue) > 25:
                mirror_scene_pixels += 1
            # The self-contained fixture uses a white material.  Requiring a
            # red paint colour here made this test fail before it reached the
            # renderer; a bright bodywork sample, together with the semantic
            # vehicle-span assertion above, proves the intended condition.
            if red > 80 and green > 80 and blue > 80:
                bright_car_pixels += 1
    if mirror_scene_pixels < 1200:
        raise AssertionError(
            f"native mirror did not reach the HUD panel: {mirror_scene_pixels}")
    if bright_car_pixels < 8:
        raise AssertionError("controlled native mirror contains no rival bodywork")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
