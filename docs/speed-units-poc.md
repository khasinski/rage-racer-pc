# Speed units prototype

Branch: `poc/speed-units`. Starting snapshot: `7f3749d82`.

The prototype replaces the baked unit label in the tachometer bitmap at car
initialization, and converts only the speed digits. It does not change engine
RPM, gearing, car specifications, physics or AI. The circular scale is RPM.

## Scope

PAL AGE Erriso (car 0) and Lizard Esperanza (car 3) are supported. Other cars
and disc regions retain their retail units, including the digits. This is a
visual feasibility experiment, not a complete option for all thirteen cars.

Select the mode at startup with `--set poc.speed_units=auto`, `kmh` or `mph`.
The default `auto` performs no bitmap writes and preserves regional rounding.
PAL `kmh` preserves the original `km` / `km/h` artwork. `mph` clears only the
label rectangle and composes a compact pixel glyph from C masks. AGE needs
all fifteen available pixels before the separate gear indicator.

Both Classic and Modern use the patched original 4bpp face. Modern HD image
loading also patches the corresponding car-specific dial replacement, at its
native integer scale, preserving alpha and deriving ink/background colours
from the image. The tested HD assets are Esperanza's 384×376 day/night faces
(`esperanza-dial-day-v2.png` and `esperanza-dial-night-v2.png`). Pack files on
disk are unchanged. Low-memory mode continues to use the patched original.

This prototype has no in-game selector. Changing units requires a restart.
Arbitrary third-party HUD layouts, noninteger scaling, generic HUD keys and
the remaining eleven cars need explicit layout support before release.

## Verification

Build `rage-racer-smoke`, `regional_speed_units_tests`,
`tachometer_needle_tests` and `modern_assets_retry_tests`, then run:

```sh
ctest --test-dir build/release \
  -R '^(regional_speed_units|tachometer|tachometer_needle|modern_assets_retry)$' \
  --output-on-failure
```

The regional test checks conversions and limits, unsupported cars/regions,
unchanged rival state over 300 ticks, exact indexed-pixel patch boundaries,
scaled RGBA patch boundaries and unchanged alpha.

Real GPU captures use `PSYZ_HEADLESS=1 SDL_AUDIODRIVER=dummy`, deterministic
input (`input.state_script=12@177-450:CROSS`), seed 1 and scene 12 timer 430.
PAL `auto` and `kmh` captures are byte-identical for both test cars. The
`kmh`/`mph` runs produce identical final simulation diagnostics. Captures and
logs from this experiment are under `build/reports/speed-units/poc/`.
Modern original, Modern HD, Classic and the class-3 night HD captures have
zero RGB differences outside the tachometer area between unit modes. The
night capture resolves CLUT 13288 and the actual night HD face.

Before release: map the remaining original dials and regional variants,
define HD layout metadata, add the persisted Options selector, invalidate HD
caches when changing units live, and verify replay, retries and car changes.
