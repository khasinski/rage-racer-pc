#!/usr/bin/env python3
"""What the native sky must keep doing.

The cloud sheet wraps the horizon as a band, the way the game's tile grid
does: sixteen columns round a full turn, two rows deep. It repeats round the
turn and never upwards. Projecting it as a plane overhead tiles it in both
directions and turns the sky into wallpaper, and scaling does not hide that:
the eye reads the repetition rather than the cloud.

The two shader sources are checked against each other on purpose. This backend
compiles MSL on Metal and SPIR-V elsewhere, and the two are separate texts:
editing one and measuring the other wasted a lot of time, and produced changes
that appeared to do nothing.
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


for name, source in (("glsl", glsl), ("msl", gpu)):
    dense = source.replace(" ", "").replace("\n", "")
    require("cloudBand" in dense,
            f"{name}: the cloud sheet must stay a band round the horizon")
    require("fract(atan" in dense or "fract(atan2" in dense,
            f"{name}: the sheet must wrap by heading, so it repeats round "
            "the turn")
    require("cloudCoverage" in dense,
            f"{name}: cloud must fade out rather than fill the sky")

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
for slot, band in ((1, "skyTopColor"), (2, "skyColor"),
                   (3, "skyHorizonColor"), (4, "skyBottomColor")):
    require(f"GameRenderWorldEnvironmentColor({slot}, &camera.{band})" in game,
            f"the sky gradient must take {band} from environment slot {slot}")

# The two shaders must agree on the numbers, whichever one the host compiles.
def constants(text, start, end):
    body = text[text.find(start):text.find(end)]
    body = re.sub(r"/\*.*?\*/", "", body, flags=re.S)
    return re.findall(r"\d+\.\d+", body)


glsl_numbers = constants(glsl, "if (height >= 0.0)", "outColor = vec4(color")
msl_numbers = constants(gpu, "if(h>=0.0)", "return float4(c,1.0)")
require(glsl_numbers == msl_numbers,
        "the GLSL and MSL skies disagree: "
        f"{glsl_numbers} against {msl_numbers}")

if failures:
    for failure in failures:
        print("FAIL", failure)
    raise SystemExit(1)

print("native sky keeps its cloud sheet a band round the horizon")
