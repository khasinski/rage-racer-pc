# Linux profiling: Mythical Coast (2026-09-05)

## Final measured outcome

The final clean PAL class-0 tour completed with modern 1706×960 / 16:9 and
unchanged image-quality settings, without per-frame tracing or cache-oracle
readbacks. Discarding the first three 120-render batches as in the baseline:

| Metric | Before | Final |
| --- | ---: | ---: |
| Mean rendered frames/s | 47.57 | 59.81 |
| Whole-tour CPU time | 40.43 s | 19.90 s |
| Whole-tour wall time | 40.53 s | 40.63 s |
| Mean batch p95 interval | 25.92 ms | 25.34 ms |

This is about 26% higher render throughput and 50% less CPU time, not a faster
simulation or lower image quality. The final steady-window maximum interval
was 39.90 ms: pacing tails remain, so this is **not** a claim of perfectly
uniform 60 Hz scanout. GPU hardware timing is not measured by CPU submit times.
The large measured tunnel material-upload and row-readback stalls were removed
as detailed below. Results are individual controlled runs, not confidence
intervals or a guarantee for other hardware.

Final evidence: `build/performance-drive/20260905-130406-603cda/`.
PAL repeat-race, NTSC-U/J cache-oracle checks, targeted Linux/Windows tests and
the Windows software-Vulkan route run are detailed below and in the separate
Windows validation report. Further frame-pacing refinement and Windows hardware
profiling remain outside what these results establish.

PAL, class 0, course 0, modern 4x (1706x960), 16:9, linear, FXAA,
vsync on a 60 Hz display. Existing user configuration retained. Grading was
requested but its setup failed; audio initialization also failed on this host.
These results do not cover audio cost or successful colour grading.

### Follow-up with working Vulkan grading

The composite pipeline supplied only Metal shader code, so grading could not
initialize on Vulkan. It now supplies compiled SPIR-V as well as MSL. An actual
GPU render/readback test checks eight colours against the vibrant formula and
opaque alpha; it passed on Linux and Windows (SwiftShader). The selected Linux
suite including this fixture passed 23/23 tests.

A subsequent complete PAL tour with grading successfully enabled measured
59.72 rendered frames/s, 19.93 s CPU / 40.68 s wall time and mean batch p95
26.22 ms, using the same startup exclusion. This single run shows no material
throughput regression, but does not establish uniform frame pacing. Evidence:
`build/performance-drive/20260905-131446-a10fbf/`. Audio remains unverified.

### Remaining pacing: phase-correlated trace

A fresh post-grading PAL tour with phase tracing completed in
`build/performance-drive/20260905-132323-0e5c2a/`. From logic frame 500 onward,
109 of 1,424 rendered intervals exceeded 25 ms. Over 573 logic ticks,
`draw_sync` averaged 8.92 ms (maximum 22.06 ms); `texture_swap` averaged
0.022 ms (maximum 2.09 ms). The wait phase averaged 31.10 ms, but includes
intermediate rendering, so it must not be interpreted as idle CPU time.

For example, the 25.447 ms rendered interval tagged frame 500 overlaps the
next tick's 14.467 ms DrawSync before presentation resumes in the wait phase.
The corresponding examples at frames 502 and 508 contain 13.226 and 15.844 ms
DrawSync respectively. This identifies uninterrupted compatibility-rendering
work on the presentation thread as a remaining pacing constraint, not another
100 ms tunnel texture upload. Trace overhead and other phases also contribute;
these overlaps alone do not prove that moving DrawSync would be safe or fix all
tails. Any scheduling change must preserve VRAM dependencies and PAL/NTSC logic.

### Prepare textured-triangle ordering once

A fresh CPU sample (`20260905-132552-2a4d77/cpu.data`, 1,425 samples, none
lost) attributed 23.44% self samples to Draw_PushPrim and 7.02% to
PsxTextureTriangleSpan. Its stable sort copied and reordered the same vertices
for every scanline. Sorting now happens once per triangle; the scanline helper
uses the prepared vertices, preserving interpolation and rounding order.
The new C oracle compares every valid span field and coverage against the old
implementation: 5,220,000 scanlines matched on Linux and Windows. Both game
builds passed, as did 12 selected Linux tests. A sanitizer build was attempted
but the host lacks libasan/libubsan; no sanitizer pass is claimed.

Two post-change traced PAL tours completed (573 measured ticks each):

| Run | Mean DrawSync | Max DrawSync | Whole-tour CPU | Mean batch p95 |
| --- | ---: | ---: | ---: | ---: |
| Before, `20260905-132323-0e5c2a` | 8.922 ms | 22.063 ms | 19.97 s | — |
| After, `20260905-132928-1878d9` | 5.818 ms | 22.792 ms | 16.02 s | 22.96 ms |
| Repeat, `20260905-133035-3516cb` | 8.487 ms | 20.264 ms | 19.84 s | 25.95 ms |

The repeat does not reproduce the large first-run improvement. Treat this as
removal of redundant CPU work with exact-output coverage, not a proven 35%
whole-path speedup. The first after-run overlapped a Windows VM build, another
reason not to treat these as isolated confidence-controlled measurements.
All paths above are beneath `build/performance-drive/`.

An isolated `-O2` C microbenchmark alternated both algorithms three times over
256 deterministic triangles × 256 scanlines × 1,000 repetitions. Reference
CPU times were 1.255 / 1.252 / 1.252 s versus prepared 0.491 / 0.491 / 0.491 s
(about 2.55× faster for this helper); aggregate checksums matched. This excludes
the GPU and the rest of DrawSync, and is not a game-FPS prediction. Local
reproducer: `build/autopilot-check/texture-triangle-bench.c`, compiled against
the frozen test oracle and production header.

### Prepare camera transforms per native draw build

The same CPU sample attributed 8.07% self time to RenderWorldToView, which
renormalized the camera quaternion or evaluated Euler trigonometry per point.
Native culling and fog now share a caller-owned prepared camera snapshot for
each draw build. Main/mirror views and successive interpolated frames do not
share mutable cache state. Euler rotations remain separate Z/Y/X operations
to preserve rounding. Invalid quaternion and null-input behavior is retained.

The new C test matched 256,000 view-coordinate and fog results exactly against
the existing implementation on Linux and Windows. Both game builds passed;
17 selected Linux tests passed, including mesh building, mirror pass, world
snapshots, GPU composite and compatibility raster oracles.

The first post-change tour (`20260905-133659-057fcc`) completed but overlapped
test/build activity and is not used for performance comparison. A second tour
(`20260905-133805-1a16b6`) completed without concurrent agent builds/tests:
mean native prepare time after frame 500 was 2.593 ms versus 3.014 ms in the
preceding `20260905-133035-3516cb` run (about 14% lower in this pair).
Whole-tour CPU was 19.33 s / 40.56 s wall; steady render rate 59.81/s,
mean batch p95 25.26 ms, maximum interval 40.16 ms. These remain single-run
comparisons and do not prove uniform 60 Hz scanout. Relative to the original
47.57/s / 40.43 s CPU baseline (40.04 user + 0.39 system), the accumulated changes retain approximately
26% higher throughput and 52% lower CPU time with working Vulkan grading.

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

## Follow-up: pacing and first-use texture work

The presentation deadline now persists across logic ticks, with a short bounded
sleep in the wait hook. Rendering is no longer restricted to only part of the
PAL logic wait. A callback skip flag avoids submitting a duplicate presentation
at the normal end-of-tick call. PAL/NTSC logic deadlines remain separate.
The new `queued_fps` counter counts submitted swapchain images, **not physical
scanout**. Exclude its first batch, which includes startup submissions.

With these changes, clean runs render and queue approximately 58–60 frames/s.
One-tour CPU time fell from 40.04 s to 19.58 s (wall time 40.73 s).
This does not establish uniformly spaced frames: batch p95 intervals still
frequently range from 22 to 30 ms.

Cache-miss tracing identified 169 alternate-bank 256×256 RGBA textures at the
tunnel: 42.25 MiB base pixels, 56.11 MiB with mipmaps, excluding driver overhead.
The burst spent 35.00 ms decoding, 55.53 ms building mipmaps and 19.27 ms
uploading. This identifies the port's measured stall, not the original PS1
game's historical slowdown.

Bounded prewarming now prepares the opposite course/terrain bank as materials
become known, starting during the intro: at most two misses per render and a
1.5 ms soft budget (a single load can exceed that budget). It is not a full
course-wide preload and does not preload car palette variants. Import now
selects the bank encoded in the requested material variant rather than relying
on the live bank. `diagnostics.texture_prewarm=false` permits an A/B run.

At logic frame 653 / point 180, submit-stage cost fell from 110.835 ms to
0.486 ms; the corresponding interval fell from 131.189 ms to 22.545 ms.
Full decoded RGBA hashes matched for all 670 course/terrain material keys
present in the no-prewarm reference (zero mismatches or missing keys).
PAL class 1 completed two races with the intervening reward/menu transition;
NTSC-J completed two tours with automatically detected 60 Hz / 30 Hz logic.
These are route-driver checks, not manual input or audio verification.

Evidence: `pacing1-clean/`, `pacing2-clean/`, `warm1-clean/`,
`warm-off-verify-clean/`, `warm-on-verify3-clean/`,
`perf-pal-repeat-clean/`, `perf-ntsc-j-clean/` under
`build/autopilot-check/`.

Two intervening warm verification attempts timed out before entering the race,
inside X11 window mapping. Showing the window before claiming it for GPU
presentation resolved subsequent launches; failed attempts are not benchmarks.

## Follow-up: compatibility raster CPU work

A 15-second user-CPU sample across the repeated-race run (562 samples, none lost)
attributed 27.05% self samples to `ModernTextureSample` and 13.88% to
`ModernTriangleContains`. This includes menu/transition work and is not an
isolated steady-race sample. Compatibility raster work remains necessary for
correct PS1 texture sampling; it has not been disabled.

UV plane derivatives are now prepared once per triangle instead of recomputed
for each pixel. Arithmetic order, clamping and integral-boundary correction
remain unchanged. The compiled `texture_sample` test compares 2,014,720
samples against the previous implementation, including degenerates, reversed
winding, sprite UVs and translated triangles. All comparisons pass.

Two clean one-tour runs after this change used 17.27 s CPU / 40.16 s wall and
20.74 s CPU / 40.55 s wall (`sample-cache1-clean/`, `sample-cache2-clean/`).
The variability does not establish an end-to-end speedup from UV caching alone;
it needs a controlled A/B measurement. Frame-time tails remain and need further
work. The 17 targeted timing, raster, sky and route tests pass. UBSan verification could not
link because the host's libubsan installation is missing its referenced library.

The unit-sprite path additionally skips coverage/interpolation checks only for
positive axis-aligned 1:1 UV mappings. For such sprites both original triangle
planes always request integral-boundary correction; uncovered pixels also
always request correction. Pixel emission, ordering, clipping, UV wrapping and
colours are unchanged. Non-unit/wrapped mappings retain the general path.
The C test checks 8,652,800 reference samples and 65,536 perturbed mappings.
`unit-sprite1-clean/` completed one tour in 40.72 s wall / 20.69 s CPU.
This still overlaps the earlier run variation; there is no demonstrated
end-to-end speedup from this shortcut yet.

The latest traced run (`sample-cache-trace-clean/`) still has 30–42 ms
intervals whose recorded preparation and submit stages total only 3–10 ms.
The next measurement must account for work between native renders (including
compatibility ordering-table processing and logic) and scheduling, rather than
assuming the remaining gaps are native GPU draw cost.

## Profiler usage and remaining verification

### Whole-loop phase attribution

Opt-in `frame-phase` records now split service setup, scene dispatch, native
publication, DrawSync, texture swap, deadline wait and presentation/OT enqueue.
Both these records and `modern-frame` include SDL nanosecond timestamps.
The deadline-wait phase includes intermediate renders; it must not be added
to their costs as if they were disjoint. Log writing adds diagnostic overhead.

In `phase2-clean/`, 572 ticks after frame 500 gave:

| Phase | Mean wall time | Maximum |
| --- | ---: | ---: |
| Scene dispatch | 1.091 ms | 3.463 ms |
| Native publication | 0.067 ms | 0.189 ms |
| DrawSync | 9.982 ms | 28.238 ms |
| Texture swap | 0.096 ms | 21.373 ms |
| Present and enqueue ordering tables | 0.488 ms | 16.664 ms |

This localizes most between-render blocking to DrawSync, not game physics or
native publication. In this port, DrawSync calls `psyz_sync`, which executes
`Psyz_GpuExeque`: dispatching queued PS1 packets, flushing geometry and the
backend sync. Its name alone is not evidence of waiting for GPU completion.
The next optimization target is that queue-processing path. The isolated
texture-swap and presentation outliers also remain to be explained.
Both phase-traced route tours completed; the main-loop behavior test still
passes. No synchronization semantics changed for this instrumentation.

### Prepared PS1 scanline samples

A fresh 20-second user-CPU profile (`queue-profile-clean/cpu.data`, 4,934
samples, zero lost) attributed 24.63% self samples to `Draw_PushPrim`, 8.31%
to triangle coverage and 6.20% to PS1 texture-span construction. Annotated
assembly showed divisions in the inlined per-pixel sampling path.

PS1 16.16 starting values and UV/RGB steps are now prepared once per scanline
rather than recalculated for every pixel. The compiled `texture_span` test
matches the previous implementation on 5,079,264 samples, including zero-width
spans, negative screen positions, reverse UV/RGB slopes and UV wrapping.
Pixel selection and compatibility correction emission remain unchanged.

In `span1-clean/`, the same 572-tick window after frame 500 reduced mean
DrawSync wall time from 9.982 to 8.393 ms (about 16%). Maximum remained
27.630 ms versus 28.238 ms, so this does **not** resolve frame-time tails.
The traced tour completed in 40.80 s wall / 19.90 s CPU. This is one before/after
pair and still needs repeated measurements. Nine targeted regression tests
passed after the change.

Triangle coverage now also prepares winding, edge coefficients and top-left
inclusivity once per triangle. Integer edge evaluation and exact-vertex
rejection are preserved. The `triangle_coverage` test compares 2,165,625 cases
against the old predicate: exhaustive small-coordinate triangles and samples,
plus signed 16-bit coordinates, vertices, edge midpoints and random pixels.
All cases match.

`coverage1-clean/` completed the same traced tour in 40.94 s wall / 20.19 s
CPU. Over the same 572 ticks, mean DrawSync was 7.690 ms and maximum 21.971 ms.
These compare with 8.393 / 27.630 ms immediately before this change. The
whole-process CPU time did not improve in this individual run, so this remains
phase-local evidence, not proof of an overall speedup or smooth scanout.
The next profile target outside compatibility raster work is repeated native
mesh-bounds calculation: the last sample attributed 4.86% self CPU samples to
`RuntimeMeshBounds`, which currently scans mesh indices on every query.

### Immutable mesh bounds

Live-imported and file-cached meshes now prepare per-submesh bounding spheres
once at load time. Storage is owned by the corresponding asset cache and freed
on release/shutdown. The bounded mesh reader remains allocation-free and
supports uncached caller-owned bytes; its optional cache requires immutable
bytes and is cleared by every reopen, including failed opens. Allocation
failure retains the original calculation path. Unusable/empty bounds preserve
the conservative no-cull result.

Tests cover exact cached/uncached equality, invalid bounds, invalid indices,
insufficient cache capacity, reopening and repeated cache release.
`bounds1-clean/` completed the traced tour in 40.63 s wall / 19.86 s CPU.
After frame 500, mean native preparation was 2.823 ms versus 3.619 ms in
`coverage1-clean/` (about 22% lower in this pair). Mean interval was 16.773 ms,
but the maximum was still 52.763 ms. This improves a repeated geometry cost;
it does not establish that the remaining stalls have been eliminated.

### Track-page readback stalls

The remaining tunnel outlier in `bounds1-clean/` was not a preload miss:
frame 654 spent 16.859 ms swapping track texture rows after a 16.094 ms
DrawSync, producing a 52.763 ms native-render interval. The original row-swap
loop calls StoreImage/DrawSync/LoadImage/DrawSync for each row. SDL GPU
StoreImage downloaded and fence-waited even when reading untouched uploaded
texture data.

The backend now retains exact uploaded 16-bit words with per-pixel validity.
Every marked GPU-written region invalidates the CPU read cache, including
draws, clear, move and RGB24 upload; valid LoadImage data restores its region.
StoreImage still flushes pending primitives first, uses the cache only when
the entire requested rectangle is valid, and otherwise performs the original
GPU readback. Successful readbacks can populate the cache. Backend teardown
invalidates all entries. Storage costs 1.5 MiB.

`PSYZ_VERIFY_VRAM_READ_CACHE=1` forces GPU readbacks even on cache hits and
compares exact 16-bit words. The PAL tour `vram-cache-verify-clean/` completed
with 514 matching reads and zero mismatches. The compiled cache test covers
row reads, partial invalidation, unaffected neighbours, reset and invalid
rectangles; all 22 targeted regressions pass.

In the fast-path run `vram-cache-fast-clean/`, frame 654's texture swap took
0.912 ms instead of 16.859 ms. This addresses the second measured tunnel cost;
it is not a claim that all frame-time tails are gone.
The fast-path run completed both route tours successfully.

The profiler is opt-in: `diagnostics.performance=true`
reports 120-render batches; additionally enabling `performance_trace=true`
reports per-render timing and current player track point. The first interval
is startup-relative and should be excluded from steady-state analysis.
Texture tracing includes decoded RGBA checksums and adds CPU overhead.

Still required: address frame-time tails, verify ordinary player input and
audible playback with working audio, successful colour grading, and broader
platform/region coverage. The measurements above do not prove those properties.

## Combined-change regional checks

With all current cache/pacing changes and the Windows portability fix:

- PAL class 1: two complete races through the intervening reward/menu flow
  (`build/autopilot-check/perf-final-pal-repeat-clean/`).
- NTSC-U (`SLUS_004.03`): two tours, automatically selected 60 Hz / 30 Hz
  logic, 1,026 exact CPU-cache/GPU readback matches, no mismatches.
- NTSC-J (`SLPS_006.00`): the same two-tour check, also 1,026 matches and
  no mismatches.

The NTSC checks used the new tracked `tools/performance_drive.cmake` harness,
which records executable/configuration hashes, isolates logs/state and verifies
route completion and cache-oracle output. Evidence sessions under
`build/performance-drive/`: `20260905-124829-abdedf` (U) and
`20260905-125017-ddb1c5` (J). Both rendered in modern 1706×960.
These validate regional operation and cache coherence, not NTSC performance:
forced GPU oracle readbacks deliberately retain the slow path.

Reproduce with a legally obtained local image:

```sh
cmake -DGAME="$PWD/build/rage-racer" -DDISC="/absolute/path/game.cue" \
  -DCONFIG="$PWD/rage-port.ini" -DLAPS=2 -DVERIFY_VRAM=ON \
  -P tools/performance_drive.cmake
```

Omit `VERIFY_VRAM` for a timing run; use `TRACE=ON` only for phase diagnosis.
The harness preserves quality settings but explicitly selects modern and
disables marker/history/RenderDoc overhead. It does not modify the supplied INI.
Windows results are recorded separately in
`windows-performance-validation-2026-09-05.md`.
