# Unit labels from retail regional differences

References: PAL `SCES_006.50` and NTSC-U `SLUS_004.03`. The USA developer image
was supplied as `~/Desktop/Rage Racer (USA).7z`; the original archive is untouched.
Thirteen paired VRAM captures confirm identical page/UV layouts and palettes.
Eleven faces differ only around the unit label, while cars 9/10 are identical.
There is no RPM scale or unrelated artwork change.

| Car | Changed pixels | Native difference rectangle x,y,w,h |
| --- | ---: | --- |
| 0 | 52 | 65,6,12,8 |
| 1 | 106 | 68,77,22,7 |
| 2 | 35 | 78,16,13,5 |
| 3 | 120 | 68,7,22,7 |
| 4 | 41 | 66,9,11,6 |
| 5 | 105 | 69,7,20,7 |
| 6 | 134 | 67,78,22,7 |
| 7 | 102 | 68,7,22,7 |
| 8 | 55 | 69,7,12,7 |
| 9, 10 | 0 | none |
| 11 | 99 | 68,7,22,7 |
| 12 | 99 | 55,6,22,7 |

Both CLUTs are identical in every pair. The 3556 bytes of sparse, reversible
encoded deltas are in `src/render/hud_unit_retail_data.h`, with source/target
hashes, not complete retail faces. See [runtime behavior and validation](speed-units-poc.md).

## Reference capture (developers only)

`tools/capture_hud_unit_reference.cmake` runs the game hidden and silent,
with no mods/HD overlays and `poc.speed_units=auto`. For every car it checks
both disc identity and the reported page/UV rectangle, then dumps VRAM.

```sh
cmake -DGAME="$PWD/build/release/rage-racer-smoke" -DSOURCE="$PWD" \
  -DDISC="/path/to/PAL.cue" -DEXPECT_REGION=PAL \
  -DOUTPUT_ROOT="$PWD/build/reports/hud-references" \
  -P tools/capture_hud_unit_reference.cmake
# Repeat with NTSC-U as EXPECT_REGION and the corresponding DISC.
```

These developer commands are not part of player setup. The shipped executable
contains the verified deltas; automatic native import still needs one image.

## Difference generation

```sh
build/release/rage-hud-unit-diff-tool 3 \
  build/reports/hud-references/PAL/car-3.vram \
  build/reports/hud-references/NTSC-U/car-3.vram \
  build/reports/hud-references/car-3.rrhud
build/release/rage-hud-unit-diff-tool --header \
  build/reports/hud-references/PAL build/reports/hud-references/NTSC-U \
  src/render/hud_unit_retail_data.h
```

The tool extracts the 96×94 4bpp face and both 16-colour CLUTs, emits a sparse
XOR patch, and reports changed-pixel bounds and palette bytes. The per-car tool
checks byte-exact forward/reverse application. Reference integration tests
verify the complete generated table against both real source sets. Always
inspect regional differences before treating them as unit-label differences.

`RRHUDD01` contains a car byte, two 64-byte lowercase SHA-256 strings, a
little-endian 16-bit record count and sorted `(offset16, xor8)` records.
Snapshot order: 4512 face bytes, 32 day palette bytes, 32 night palette bytes.
No struct padding/host-endian integers are written. Empty deltas are valid.
Loading rejects bad metadata, duplicate/out-of-range offsets, zero records,
truncation and trailing bytes. Application checks the complete source and
result hashes before touching the destination, and is idempotent in either
requested direction. Source mismatch never changes numeric units.

## Real-game reproduction

`tests/render/verify_speed_units_poc.cmake` accepts `DISC`, `OVERRIDE` (`mph`
or `kmh`), `RENDERER`, `CARS`, `RACE_MODE`, `RACE_CLASS` and optional `HD_MOD`.
With `DIFF_TOOL`, `SOURCE_REFERENCE_ROOT` and `TARGET_REFERENCE_ROOT`, every
runtime dump is checked against the appropriate native retail face/palettes.
Both runs of each pair must have identical final simulation diagnostics.
All launches set `PSYZ_HEADLESS=1 SDL_AUDIODRIVER=dummy` and retain the real GPU.
