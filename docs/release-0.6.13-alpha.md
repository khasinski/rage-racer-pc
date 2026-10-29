# Rage Racer PC 0.6.13-alpha

This alpha updates the automatic-transmission shift points for the remaining
cars that retail sold manual-only (`PEGASE`, `HIJACK`, and `BULSHADE` /
`TEMPEST`) in `cars.toml` and `cars.ntscj.toml`, and fixes the Linux release
and sanitizer builds.

## Changes

- `PEGASE` Grade 0 and Grade 1 (`car_02_g0`, `car_02_g1`), `HIJACK` Grade 0 and
  Grade 1 (`car_06_g0`, `car_06_g1`), and `BULSHADE` / `TEMPEST` (`car_11_g0`)
  now use powerband-aligned automatic `shift_points` in `cars.toml` and
  `cars.ntscj.toml`:
  - First-gear downshift thresholds are set to `0` to match the retail automatic
    and `@SeanArchibald`-contributed setups.
  - Upshift thresholds sit at the upper end of each car's high-torque plateau
    so `PEGASE` no longer shifts at 6,650 RPM and drops into its 5,374 RPM
    torque dip on every gear change.
  - Downshift thresholds are scaled below the preceding gear's upshift speed
    with proper hysteresis, fixing the inverted Gear 1/Gear 2 threshold on
    `HIJACK` Grade 1 (`car_06_g1`).
- Linux GCC `-Werror=type-limits` and `-Werror=format-truncation=` warnings in
  `src/render/car_paint.c`, `src/port/native_asset_importer.c`, and
  `src/port/mod_assets.c`, as well as `-lm` linking for `overlay_model_tests`,
  are fixed so Linux release and sanitizer workflows build cleanly.

## Still open

The Windows second-race render report is not confirmed fixed. Per-action
controller remapping is still the `rage-port.ini` bindings.
