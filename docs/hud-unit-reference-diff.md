# Unit labels from retail regional differences

The hand-drawn and recomposed unit-label experiments are superseded by this
approach. Use paired native PAL and NTSC-U assets to preserve the actual
retail spelling, font, placement, antialiasing and palette. No reference NTSC-U
image is currently available locally, so no real regional delta is included
and this module is not yet wired into the game's unit selector.

## Reference capture

`tools/capture_hud_unit_reference.cmake` runs the game hidden and silent,
with no mods/HD overlays and `poc.speed_units=auto`. For every car it checks
both disc identity and the reported page/UV rectangle, then dumps VRAM.
Run it twice, using legally obtained development reference images:

```sh
cmake -DGAME="$PWD/build/release/rage-racer-smoke" -DSOURCE="$PWD" \
  -DDISC="/path/to/PAL.cue" -DEXPECT_REGION=PAL \
  -DOUTPUT_ROOT="$PWD/build/reports/hud-references" \
  -P tools/capture_hud_unit_reference.cmake
# Repeat with NTSC-U as EXPECT_REGION and the corresponding DISC.
```

These are developer tooling commands. Players must continue to need only
one supported legally obtained game image, with automatic native import.

## Difference generation

Build `rage-hud-unit-diff-tool`, then run for each model index 0..12:

```sh
build/release/rage-hud-unit-diff-tool 3 \
  build/reports/hud-references/PAL/car-3.vram \
  build/reports/hud-references/NTSC-U/car-3.vram \
  build/reports/hud-references/car-3.rrhud
```

The tool extracts the 96×94 4bpp face and both 16-colour CLUTs, emits a sparse
reversible XOR patch and reports changed-pixel bounds and changed palette
bytes. It verifies byte-exact forward and reverse application before writing.
Inspect every region difference before calling it a unit-label patch: the
tool does not assume that every regional change is a label. The source and
target SHA-256 identities include the entire face and both CLUTs.

`RRHUDD01` contains a car byte, two 64-byte lowercase SHA-256 strings, a
little-endian 16-bit record count and sorted `(offset16, xor8)` records.
Snapshot byte order is 4512 face bytes, 32 day palette bytes, 32 night palette
bytes. No struct padding or host-endian integers are written. Empty deltas
are valid. Loading rejects bad metadata, duplicate/out-of-range offsets,
zero records, truncation and trailing bytes. Application rejects a wrong
car, modified source or corrupt patch without modifying the snapshot.

## Integration still requiring real NTSC-U evidence

1. Capture and visually compare all thirteen native pairs and both palettes.
2. Review the bounding boxes; confirm no RPM scale or unrelated artwork moves.
3. Include the verified deltas in automatic native asset generation and replace
   the prototype font code. Gate digit conversion on successful asset selection.
4. Restore the requested variant from a canonical source, including retries,
   replay and car changes, with no requirement to provide a second disc.
5. Handle HD faces as image variants; a 4bpp regional XOR cannot be applied to
   an arbitrary RGBA PNG. Preserve renderer choice and verify both unit modes.

`hud_unit_diff_tests` covers exact round trips, palette changes, no-op patches,
full-size patches, idempotence, extraction, format rejection and atomic failures.
Those are synthetic tests; they are not evidence of a PAL/NTSC-U visual match.
