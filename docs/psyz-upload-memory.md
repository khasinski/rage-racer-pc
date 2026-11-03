# PsyZ texture upload memory audit (2026-11-03)

The PsyZ fork now follows upstream `f8780dc` through fork commit
`8db3cb79c2f3b2835786cdc9000539be813cb9a9`. The game matrix helper is renamed
to `GameApplyMatrixLV` to avoid colliding with upstream's `ApplyMatrixLV`.

## Cause and fix

The SDL GPU backend used one 2 MiB transfer buffer for every texture upload,
including small CLUT uploads. Cycling that buffer while uploads were queued
caused SDL to allocate and retain many full-size backing buffers. Metal
allocation tracing recorded 1,332 allocations of 2 MiB before the fix and 12
after it.

PsyZ commit `8db3cb7` selects a lazily allocated power-of-two transfer-buffer
bucket based on the upload payload, from 1 KiB to 2 MiB. Cycling remains
enabled, and all buckets are released at shutdown. Rendering quality and
shader output are unchanged.

## Measurements

Apple M5 Max, Metal, 1706x960, linear filtering, FXAA and vibrant color;
matching two-lap debug-route smoke runs. Values are median peak macOS process
footprint in decimal GB, including graphics allocations, rather than RSS.

| Assets | Before | After | Reduction |
| --- | ---: | ---: | ---: |
| Original | 3.97 GB | 1.25 GB | 68% |
| HD | 5.09 GB | 2.30 GB | 55% |

A further original-assets run completed three consecutive debug-route races
with a peak footprint of 1.37 GB. This is a bounded stability check, not proof
that arbitrarily long sessions cannot grow memory use.

## Validation

- The new SDK test `gpu::mixed_size_queued_uploads_preserve_gpu_pixels` queues
  a full VRAM image, a tile and 512 distinct palettes before synchronization,
  then compares an actual GPU VRAM capture with the expected full image.
- Six selected SDK upload/display tests and twelve game rendering and
  compatibility tests passed. Existing image references did not change.
- Five failures in the broader SDK move-image/RGB24 test selection also
  reproduce before the memory fix; that selection is not fully green.
- Automated game runs used `PSYZ_HEADLESS=1` and `SDL_AUDIODRIVER=dummy` with
  the real Metal backend.

Local measurement logs and allocation traces are under the ignored directory
`build/reports/memory-audit/`; they are not required to build the game.
