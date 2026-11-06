# Retail speed unit switching prototype

Branch: `poc/speed-units`. The hand-drawn/recomposed font experiment is replaced
by verified retail PAL ↔ NTSC-U bitmap deltas. No glyph masks or invented unit
spellings remain. Each car uses exactly its original regional artwork.

Use `--set poc.speed_units=auto`, `kmh` or `mph` at startup. `auto` preserves the
loaded disc. All thirteen PAL and NTSC-U cars support both forced units;
NTSC-J and unknown layouts retain their native units until separately verified.
The two dials without a label remain unlabeled. The circular scale is RPM.

The 13 sparse XOR deltas total 3556 bytes, including full-face/palette SHA-256
identities. They are embedded in the executable; a player needs only their own
single CUE/Track 01 BIN. No second image, patch file, conversion command or
renderer fallback is needed. Changes affect only presentation, never physics.

At car initialization the port reads the 96×94 indexed face and both CLUTs,
selects the requested retail variant, then writes it back only if conversion
was necessary. Repeated initialization accepts an already converted target.
Wrong car/layout, modified texture or corrupt delta leaves the snapshot and
displayed units native. Numeric conversion becomes effective only after
successful artwork verification, preserving the retail US rounding order.

Classic and Modern's original UI consume the exact patched VRAM. HD faces
retain the pack's surrounding artwork/background and alpha while the unit
region is composed from the verified target's palette-indexed pixels. A
one-native-texel halo removes the old label's HD antialiasing. This is image
composition, not XOR against a PNG. Incompatible sizes and shared generic
HUD keys are rejected for a converted dial; the same renderer then draws
the verified native face. Third-party layouts need explicit layout metadata.

This remains a startup-only PoC. A persisted Options selector, live HD cache
invalidation and replay/car-change acceptance remain release integration work.

## Verification

Build `rage-racer-smoke`, `regional_speed_units_tests`, `hud_unit_diff_tests`,
`hud_unit_retail_tests`, `tachometer_needle_tests` and `modern_assets_retry_tests`.

```sh
ctest --test-dir build/release \
  -R '^(regional_speed_units|hud_unit_diff|hud_unit_retail|tachometer|tachometer_needle|modern_assets_retry)$' \
  --output-on-failure
```

`hud_unit_diff` covers reversible patches, both CLUTs, strict parsing, wrong
sources and atomic failures. `hud_unit_retail` checks all embedded deltas and
measured pixel counts. With captured references it also verifies every car
in both directions, already-selected variants, repeated application, rejection
without digit changes, both HD addressing forms/scales/palettes, alpha and
pixel boundaries:

```sh
build/release/tests/render/hud_unit_retail_tests \
  build/reports/hud-references/PAL build/reports/hud-references/NTSC-U
```

Real hidden/silent GPU validation captured all 13 PAL cars in Modern and all
13 NTSC-U cars in Classic, each in native/forced units. Converted faces and
both palettes exactly match the opposite retail reference. Final simulation
diagnostics match within each pair. Esperanza HD was also checked in daylight
and the actual class-3 night environment (CLUT 13288).

Reports: `build/reports/hud-references/runtime/`, `runtime-usa/`, `hd-day/`,
`hd-night/`. Reproduction and provenance: [reference diff workflow](hud-unit-reference-diff.md).
