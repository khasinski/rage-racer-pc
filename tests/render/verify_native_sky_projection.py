#!/usr/bin/env python3
"""What the native sky must keep doing.

The cloud sheet wraps the horizon as a band, the way the game's tile grid
does: sixteen columns round a full turn, two rows deep. It repeats round the
turn and never upwards. Projecting it as a plane overhead tiles it in both
directions and turns the sky into wallpaper, and scaling does not hide that:
the eye reads the repetition rather than the cloud.

The shader is checked as source, not as a picture, because it is cheap and it
says which line is wrong. There used to be a second check here holding the
hand-written Metal copy to the same numbers as the GLSL; both formats are now
translated from this one source, so there is no second text to disagree with.
"""

from pathlib import Path
import re


root = Path(__file__).resolve().parents[2]
glsl = (root / "src/port/modern/shaders/native_sky.frag.glsl").read_text()
gpu = (root / "src/port/modern/modern_native_gpu.c").read_text()

failures = []


def require(condition, message):
    if not condition:
        failures.append(message)


dense = glsl.replace(" ", "").replace("\n", "")
require("cloudBand" in dense,
        "the cloud sheet must stay a band round the horizon")
require("fract(atan" in dense or "fract(atan2" in dense,
        "the sheet must wrap by heading, so it repeats round the turn")
require("cloudCoverage" in dense,
        "cloud must fade out rather than fill the sky")

# The sheet repeats round the horizon and never upwards. Tiling it vertically
# as well turns the sky into wallpaper, which no scaling hides: the eye reads
# the repetition rather than the cloud.
sampler = gpu[:gpu.find("s_skySampler = SDL_CreateGPUSampler")]
sampler = sampler[sampler.rfind("s_sampler = SDL_CreateGPUSampler"):]
require("address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT" in sampler,
        "the cloud sheet must repeat round the horizon")
require("address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE" in sampler,
        "the cloud sheet must not repeat upwards, or the sky becomes "
        "wallpaper")
# Sampled a texel at a time, as the original does: smoothing softens every
# cloud edge and reads as a stretched, low-resolution sky.
require("min_filter = SDL_GPU_FILTER_NEAREST" in sampler,
        "the cloud sheet must be sampled without smoothing")

# The gradient reads the sky's own slots, and the dark band belongs below.
game = (root / "src/port/render_world_game.c").read_text()
for slot, band in (("ENV_SKY_TOP", "skyTopColor"),
                   ("ENV_SKY_MIDDLE", "skyColor"),
                   ("ENV_SKY_HORIZON", "skyHorizonColor"),
                   ("ENV_SKY_BOTTOM", "skyBottomColor")):
    require(f"GameRenderWorldEnvironmentColor({slot}, &camera.{band})" in game,
            f"the sky gradient must take {band} from {slot}")

# Both formats must be built from this source, and both must be present, or a
# host quietly loses its sky.
shaders = root / "src/port/modern/shaders"
for generated in ("native_sky_frag_spv.h", "native_sky_frag_msl.h"):
    require((shaders / generated).exists(),
            f"{generated} is missing: run build_spirv.sh")
# The header carries the Metal source as bytes, so read it back out.
metal = bytes(int(byte, 16) for byte in re.findall(
    r"0x([0-9a-fA-F]{2})", (shaders / "native_sky_frag_msl.h").read_text()))
metal = metal.decode("utf-8", "replace")
require("fs_native_sky" in metal,
        "the Metal sky must keep the entry point the renderer asks for")
require("cloudBand" in metal.replace(" ", ""),
        "the Metal sky must be the one translated from this GLSL")

if failures:
    for failure in failures:
        print("FAIL", failure)
    raise SystemExit(1)

print("native sky keeps its cloud sheet a band round the horizon")
