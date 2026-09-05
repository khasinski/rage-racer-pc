# Linux profiling: Mythical Coast (2026-09-05)

PAL, class 0, course 0, modern 4x (1706x960), 16:9, linear, FXAA,
vsync on a 60 Hz display. Existing user configuration retained. Grading was
requested but its setup failed; audio initialization also failed on this host.
These results do not cover audio cost or successful colour grading.

## Debug overhead

Same route driver, speed 6000, one tour in each fresh process. No M captures
during measurement. Discarded the first three 120-render batches (startup/intro);
the remaining 11 batches in each run gave:

| Configuration | Render rate | Mean CPU submit-stage time | Peak process RSS |
| --- | ---: | ---: | ---: |
| Marker capture/history and RenderDoc disabled | 47.57/s | 0.60 ms | 79 MiB |
| Marker capture + 16-frame history | 47.20/s | 0.92 ms | 181 MiB |
| History + RenderDoc + Vulkan synchronization validation | 35.42/s | 13.11 ms | 377 MiB |

Rates are rendered-frame cadence, not independently measured scanout FPS.
Submit-stage CPU time includes material preparation/uploads, command recording,
post-processing and history copying; it is **not GPU execution time**. RSS
excludes a full accounting of VRAM. This is one run per variant, not a benchmark
confidence interval. The driver bypasses player physics; ordinary gameplay may
have additional costs. Existing probe settings were unchanged across variants.

Evidence (local ignored build directory): `build/autopilot-check/perf-clean/`,
`perf-history/`, `perf-vulkan/`; each contains `game.log` and `time.txt`.
The local reproduction launcher is `build/autopilot-check/profile-runs.cmake`.

## First tunnel and CPU sampling

Two route-driver tours without marker capture/RenderDoc/validation, with
`diagnostics.performance=true` and `diagnostics.performance_trace=true`.
Sampled the process with `perf record -e cpu-clock:u -F 99 --call-graph dwarf,8192`.
Per-frame tracing and sampling add overhead; this is a separate diagnostic run.

Near the first tunnel, logic frame 653 / track point 180:

- 129.50 ms between rendered frames;
- native world preparation 1.36 ms;
- overlay build 0.005 ms;
- submit stage 109.26 ms;
- 248 instances, unchanged from adjacent frames.

On the second tour, the same broad region (points 220 through 140) peaked at
25.85 ms between frames. This supports investigating first-use material/cache
work or allocation/driver stalls, but does **not** identify the exact resource
or establish that it is the same cause as the original game's slowdown.

CPU profile: 6K samples, no lost samples. `Psyz_VideoVSync` accounted for 31.96%
self / 71.48% inclusive samples. Most of the inclusive difference is clock
querying in the frame-wait loop. `ModernFrameWaitTick` overlaps this call tree;
its percentage must not be added to the VSync percentage. This is substantial
busy-wait CPU consumption, not evidence that rendering itself needs that CPU.
The pacing path also limits intermediate presents near the next PAL VBlank.
The observed ~47/s cadence alternates roughly 16.7 and 25 ms intervals.

Evidence: `build/autopilot-check/perf-tunnel/game.log` and `cpu.data`.

## Next work

1. Split submit-stage timing into material load/mipmap/upload, draw recording,
   and command submission; log material identity on slow cache misses.
2. Investigate deadline-based sleeping/presentation separately from PAL/NTSC
   logic timing. Verify game speed, input, sound and frame pacing before changing
   the synchronization contract; reducing CPU usage alone need not raise FPS.

No timing or asset-loading optimization was applied based solely on these
measurements. The added profiler is opt-in: `diagnostics.performance=true`
reports 120-render batches; additionally enabling `performance_trace=true`
reports per-render timing and current player track point. The first interval
is startup-relative and should be excluded from steady-state analysis.
