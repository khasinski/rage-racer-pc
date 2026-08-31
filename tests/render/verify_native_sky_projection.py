#!/usr/bin/env python3
"""What the native sky must keep doing.

The cloud sheet is a flat layer overhead, the way the game draws it, not an
image wrapped round the sky: a band expressed in the ray's height squeezes the
whole sheet into a few rows of screen and renders it as a streak. Distance
along the sheet goes as 1/height, which is what gives cloud its perspective
towards the horizon, so the sheet runs past the picture in both directions and
has to tile in both.

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
    require("cloudReach" in dense,
            f"{name}: the cloud sheet is no longer projected as a plane")
    require("1.0/max(h" in dense or "1.0/max(height" in dense,
            f"{name}: cloud distance must go as 1/height, not as a band")
    require("cloudCoverage" in dense,
            f"{name}: cloud must still fade out rather than reach the horizon")

# Nothing may draw cloud below the horizon: the sheet is overhead.
require(re.search(r"smoothstep\(\s*0\.06\s*,\s*0\.16\s*,\s*height\s*\)", glsl)
        is not None,
        "glsl: cloud coverage no longer starts above the horizon")
require("smoothstep(0.06,0.16,h)" in gpu.replace(" ", ""),
        "msl: cloud coverage no longer starts above the horizon")

# The sheet tiles in both axes; clamping either one hides it entirely.
sampler = gpu[gpu.find("s_skySampler = SDL_CreateGPUSampler") - 400:
              gpu.find("s_skySampler = SDL_CreateGPUSampler")]
require(sampler.count("SDL_GPU_SAMPLERADDRESSMODE_REPEAT") >= 2,
        "the sky sampler must repeat in both axes, or the sheet vanishes")

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

print("native sky projects its cloud sheet as the layer overhead it is")
