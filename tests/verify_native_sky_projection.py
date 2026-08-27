#!/usr/bin/env python3
"""Keep the native sky cylindrical, wrapped and independent of PS1 packets."""

from pathlib import Path


root = Path(__file__).resolve().parents[1]
glsl = (root / "src/port/modern/shaders/native_sky.frag.glsl").read_text()
gpu = (root / "src/port/modern/modern_native_gpu.c").read_text()

for source in (glsl, gpu):
    assert "verticalSlope" in source
    assert "bandCoordinate" in source
    assert "bandOffset" in source
    assert "upperHemisphereCoverage" in source
    assert "cylinderCoverage" in source
    assert "abs(verticalSlope)" not in source

assert "fract(bandCoordinate)" in glsl
assert "clamp(1.0 - height * 2.2" not in glsl

print("native sky wraps its authored cylinder across free cameras")
