# Speed units prototype

Status: the generated-font visual approach was rejected. Its committed
prototype is retained for comparison while the replacement is developed;
see [the retail reference diff workflow](hud-unit-reference-diff.md).

Branch: `poc/speed-units`. Starting snapshot: `7f3749d82`.

The prototype replaces the baked unit label in the tachometer bitmap at car
initialization, and converts only the speed digits. It does not change engine
RPM, gearing, car specifications, physics or AI. The circular scale is RPM.

## Scope

All thirteen PAL cars are supported. Other disc regions retain their retail
units, including the digits, until their artwork can be measured and tested.
This is a visual feasibility experiment with a startup selector.

Select the mode at startup with `--set poc.speed_units=auto`, `kmh` or `mph`.
The default `auto` performs no bitmap writes and preserves regional rounding.
PAL `kmh` preserves the original artwork. Each dial has its own measured
label rectangle, height, italic/bold treatment and spelling. `mph` clears
only that rectangle and composes compact pixel glyphs from C masks.

| Cars | Original | Imperial | Height |
| --- | --- | --- | --- |
| 0, 8 | `km` | `mi` | 7 px, italic |
| 2 | `km` | `mi` | 3 px |
| 4 | `km` | `mi` | 4 px |
| 1, 3, 6, 7, 11, 12 | `km/h` | `mi/h` | 7 px, italic |
| 5 | `km/h` | `mi/h` | 7 px, upright |
| 9, 10 | no label | no label | digits only |

The glyph composer also supports slashless `mph` for future `kmh` layouts;
none of the thirteen measured PAL originals uses slashless `kmh`. Metric
labels are never normalized to a common spelling. Car 2's metal-pattern
background is reconstructed with a matching four-pixel tile, not a flat fill.

Both Classic and Modern use the patched original 4bpp face. Modern HD image
loading also patches the corresponding car-specific dial replacement, at its
native integer scale, preserving alpha and deriving ink/background colours
from the image. The tested HD assets are Esperanza's 384×376 day/night faces
(`esperanza-dial-day-v2.png` and `esperanza-dial-night-v2.png`). Pack files on
disk are unchanged. Low-memory mode continues to use the patched original.

This prototype has no in-game selector. Changing units requires a restart.
Arbitrary third-party HUD layouts, noninteger scaling and generic HUD keys
need explicit layout support before release.

## Verification

Build `rage-racer-smoke`, `regional_speed_units_tests`,
`tachometer_needle_tests` and `modern_assets_retry_tests`, then run:

```sh
ctest --test-dir build/release \
  -R '^(regional_speed_units|tachometer|tachometer_needle|modern_assets_retry)$' \
  --output-on-failure
```

The regional test checks conversions and limits, unsupported cars/regions,
unchanged rival state over 300 ticks, exact indexed-pixel patch boundaries
for all thirteen cars, grayscale shadow selection, and both full-page and
cropped HD surfaces at 1×/4× with both palettes and unchanged alpha.

An opt-in real-game test captures every car with `auto` and `mph` and checks
that final simulation diagnostics match:

```sh
cmake -DGAME="$PWD/build/release/rage-racer-smoke" -DSOURCE="$PWD" \
  -DOUTPUT_ROOT="$PWD/build/reports/speed-units" \
  -P tests/render/verify_speed_units_poc.cmake
```

Optionally pass `-DHD_MOD=/absolute/path/to/pack` to capture Esperanza with HD.
The test needs the legally supplied PAL disc at `disc/PAL/Rage Racer (Europe).cue`.

Real GPU captures use `PSYZ_HEADLESS=1 SDL_AUDIODRIVER=dummy`, deterministic
input (`input.state_script=12@177-450:CROSS`), seed 1 and scene 12 timer 430.
PAL `auto` and `kmh` captures are byte-identical for both test cars. The
`kmh`/`mph` runs produce identical final simulation diagnostics. Captures and
logs from this experiment are under `build/reports/speed-units/poc/`.
Modern original, Modern HD, Classic and the class-3 night HD captures have
zero RGB differences outside the tachometer area between unit modes. The
night capture resolves CLUT 13288 and the actual night HD face.

The expanded run captured all thirteen cars with `auto`/`mph` (26 frames),
with identical simulation diagnostics and zero RGB differences outside the
tachometer. Additional real captures cover the 3px/4px Pegase and Acceron
labels in Classic and Esperanza's night HD replacement. These results are in
`build/reports/speed-units/speed-units-all/`, `speed-units-classic/` and `poc/`.

Before release: map regional variants,
define HD layout metadata, add the persisted Options selector, invalidate HD
caches when changing units live, and verify replay, retries and car changes.
