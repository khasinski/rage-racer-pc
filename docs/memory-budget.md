# Automatic low-memory profiles

The port detects physical RAM with SDL on PC and Android. It also reads
available physical memory on Windows (`GlobalMemoryStatusEx`) and
Linux/Android (`MemAvailable` in `/proc/meminfo`). Swap is not counted as RAM.
An unavailable measurement is reported as zero, rather than treated as an
out-of-memory report.

| Profile | Automatic trigger | Maximum internal scale | Presentation cap | Shadows / SSAO | Replacement textures |
| --- | --- | --- | --- | --- | --- |
| Normal | Above the limits below | User setting | User setting | User setting | User setting |
| Low | RAM <= 4096 MiB or available <= 1024 MiB | 2 (852x480 at 16:9) | 60 FPS | Off | Suspended |
| Minimum | RAM <= 2048 MiB or available <= 512 MiB | 1.5 (640x360 at 16:9) | 30 FPS | Off | Suspended |

Lower user settings and logic-rate presentation are retained. The renderer
choice, brightness, gameplay simulation and race timing are preserved. The
modern renderer continues to use automatically imported native game assets.

Restricted profiles also limit GPU frames in flight to one, synchronize at a
safe presentation boundary to bound cycled buffers, skip speculative texture
and geometry preloading, limit preload upload batches, cap extended draw
distance and disable diagnostic frame history. Geometry capacity is preserved
so the budget cannot silently remove visible scenery.

Installed HD packs and their saved activation preferences are retained.
Texture overrides from both the HD-pack store and legacy `mods.directory`
are suspended before PNG/WebP decoding, including sky, menu and HUD layers.
Ordinary graphics controls show effective capped values; the settings panel
explains that the memory profile is restricting graphics.

## Runtime pressure

`SDL_EVENT_LOW_MEMORY` is handled in an SDL event watch, as required for
Android notifications. The callback only sets an atomic flag. Resource release
and presentation rebuilding happen on the render thread, after submission and
before acquisition of another command buffer. Available memory is also probed
every five seconds on Windows/Linux/Android. Pressure only reduces quality
within the session; it does not repeatedly raise and lower settings when
available memory fluctuates. Starting the game again re-evaluates the machine.

`[memory] profile = auto` is the default. `low` and `minimum` select a fixed
starting restriction, while still responding to further pressure. `normal`
explicitly bypasses automatic restrictions for comparison or manual tuning.
The diagnostic keys `memory_total_mb`, `memory_available_mb` and
`memory_pressure_frame` in `[diagnostics]` support reproducible tests; they do
not constrain the operating system's actual memory allocation.

## Validation (2026-11-04)

Comparable hidden/silent two-lap debug-route runs on Apple M5 Max / Metal:

| Asset setup | Normal peak footprint | Minimum peak footprint |
| --- | ---: | ---: |
| Original textures | 1,252,361,488 bytes | 726,320,280 bytes |
| HD via legacy mod directory | 2,346,370,560 bytes | 752,649,392 bytes |

These are individual macOS process-footprint measurements including graphics
allocations, not Windows RSS measurements. A further minimum-profile run
completed three consecutive races with a 750,077,152-byte peak and a
215,793,664-byte maximum RSS. The two-lap normal and minimum runs finished at
the same simulation frame and player position.

A real-GPU pressure run changed from a 1706x960 modern target to 640x360 after
a synthetic SDL low-memory event during a race and still completed both laps.
Unit tests cover threshold boundaries, unknown measurements, explicit profiles,
thread-delivered pressure, preservation of lower settings and renderer choice,
HD preference retention, and decoder guards. The rendering test exercises both
startup profiles and a live pressure rebuild. Existing brightness/classic and
renderer-switching regressions remain in the selected verification set.

The RAM probes were simulated for the low-RAM classification tests. No physical
2 GiB Windows machine or Android handset was available for this validation;
these profiles reduce allocation demand and do not guarantee that any arbitrary
system-wide memory pressure can be survived. Logs are under the ignored
`build/reports/memory-budget/` directory.
