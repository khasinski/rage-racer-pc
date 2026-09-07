# 120 Hz presentation and CPU geometry checkpoint

The requested target is at least 120 FPS on the user's machine, synchronized
to its display, with unchanged game speed and visual settings. It is not met
yet. Performance work now takes priority over general asset refactoring.

## Display detection and synchronization

KDE reports HDMI-A-4 at 3840x2160/120 Hz. The GPU is an AMD Navi 48, with an
Intel UHD 770 also present. The user's existing modern settings render at
1706x960 (scale 4, 16:9), with filtering, FXAA and vibrant grading retained.
These results do not claim native 4K rendering.

The host-built SDL lacked Wayland and XRandR support and reported 60 Hz.
Reconfigured the existing build-linux-gcc16 in the rage-racer-dev container,
which has the required development dependencies. Its SDL summary now enables
Wayland and XRandR, and the game logs `driver=wayland refresh_hz=120.000`.
Build future changes in that environment to preserve these features:

```sh
podman exec -w /var/home/hasik/rage-racer-pc rage-racer-dev \
  cmake --build build-linux-gcc16 --target rage-racer --parallel
```

Previously `video.fps=vsync` paced the modern renderer to the detected display
rate, while the platform could select immediate presentation because PAL
VBlank runs at 50 Hz. A host presentation request now selects the VSync
swapchain mode independently of the emulated VBlank clock. Classic restores
the platform policy. Intermediate acquisition remains nonblocking. Logs expose
both the display request and the independent logic driver-VSync flag.

## Measurements

PAL Mythical Coast, class 1, one automated lap, original local INI, prewarm on.
All runs completed. FPS below is computed over complete 120-render
reporting windows after excluding the first startup window; the mean is
weighted by their duration. Queue counts measure submitted swapchain images,
not direct scanout instrumentation.

| Local session under build/ | Mean FPS | Slowest 120-frame window | Notes |
| --- | ---: | ---: | --- |
| perf-120hz-reference/20260907-142811-8632f1 | 59.90 | 58.50 | Missing display backends; per-frame tracing enabled |
| perf-120hz-wayland/20260907-143223-72ce97 | 107.17 | 96.13 | Correct refresh; experimental static CPU cache |
| perf-120hz-wayland-reference/20260907-143335-a56420 | 107.22 | 95.40 | Correct refresh; CPU cache disabled |
| perf-120hz-vsync/20260907-143626-704125 | 112.47 | 95.38 | Correct refresh and explicit display VSync; CPU cache removed |
| perf-120hz-vehicle-template/20260907-145229-b8396b | 119.03 | 114.78 | Display VSync and immutable vehicle geometry templates |

These sequential runs establish the former 60 Hz detection problem and current
performance range, not an isolated speedup percentage from VSync. The first
run used heavier tracing. Per-run result.txt records executable/config hashes.
The final VSync run's worst window p95 was 17.385 ms, and its highest mean CPU
geometry preparation was 8.199 ms. The 120 FPS budget is 8.333 ms for the whole
frame, so geometry alone can exhaust it. Many other windows sustain 120 FPS.

Added mean CPU preparation to the low-overhead aggregate profiler, along with
the selected SDL display rate. Fixed the initial queue-count baseline so it
does not include earlier menu presentations. The final executable includes
that reporting fix; the table excludes the affected first window.

## Experiments and regression evidence

A bounded static-triangle CPU cache preserved exact draw dumps and images in
three paired 300-repeat frozen-scene checks. It improved median preparation
only about 4%, and the moving-lap pair above showed no meaningful improvement.
It was removed from the working implementation. The experiment is retained in
/tmp/rage-static-triangle-cache-wip.patch; it is not persistent GPU geometry.
The earlier geometry-pack prototype and user INI remain untouched. Unfinished
importer metadata/image splitting is parked in /tmp/rage-lazy-material-wip.patch.

After rebuilding the game and consumers, offscreen mirror, native-world,
renderer-toggle and timing-restore tests pass. The compiled modern-frame-pacer
test passes after building its previously absent executable. A separate real
Wayland renderer-toggle test passes, checks display VSync with
logic_driver_vsync=0, and verifies restoration after returning to classic.

Next: reduce repeated transformation/expansion and upload of vehicle geometry,
including interpolated frames. Keep the CPU reference path and compare the
same main/mirror views. Validate moving scenes, frame tails and original game
timing; do not infer completion from frozen-scene benchmarks or average FPS.
Windows/macOS synchronization and sustained 120 FPS remain unverified.

## Vehicle geometry templates

The main and mirror paths now share bounded (32 MiB) immutable local vehicle
templates. Index decoding, triangle validation and material grouping happen
once per mesh/submesh and asset set. Instance transforms, lighting state, paint,
fog and decal separation are still evaluated each frame. Source generation
changes and renderer shutdown retire the cache. Unsupported flags, CPU fog,
texture scrolling and allocation failures retain the reference builder.
`diagnostics.modern_uncached_geometry=true` selects that builder for comparison.
This is CPU preparation reuse, not persistent GPU geometry: transformed vertices
are still copied and uploaded each frame.

In a paired frozen race scene (frame 338, 300 preparation repeats), median
preparation fell from 4.570 to 1.599 ms; p95 fell from 4.832 to 1.740 ms.
The PPM image and draw dump were byte-identical. The moving lap above reached
119.03 mean FPS over 34 reporting windows, with a slowest window of 114.78 FPS.
Highest window-mean preparation fell from 8.199 to 3.262 ms. Worst window p95
frame interval remained 15.452 ms, so neither the average nor the window result
establishes the requested 120 FPS floor.

Compiled regression tests compare cached and reference vertices/spans byte for
byte across changing transforms, quaternion rotations, nonuniform scales,
submeshes, paint, fog, decals, invalid triangles, capacity limits, unsupported
flags, frustum culling and cache retirement. Mesh-builder, mirror, native-world,
renderer-toggle and environment-provider tests passed (5/5), as did the model
stage-angle test with imported PAL assets. The mesh suite also passed ASan and
UBSan, including leak detection. Game, smoke, replay and stage targets rebuilt.

Next performance work should reduce remaining frame spikes and per-frame GPU
uploads/transforms without lowering visual settings or changing game speed.

### Upload cost after template reuse

The completed traced lap in
`build/perf-vehicle-upload-analysis/20260907-150024-55415c` uses the same
settings with per-frame tracing enabled. Across 2,469 mirror-active frames,
the combined main/mirror vertex upload averages 18,531,390 bytes per frame
(about 2.22 GB/s at 120 FPS). CPU mapping, copying and recording the upload
average 0.881 ms, with a 3.102 ms maximum. These timings do not measure GPU
execution or PCIe transfer completion. Main geometry preparation averages
1.414 ms, mirror preparation 1.296 ms, and asset warming 0.118 ms.

This identifies duplicate view preparation and expanded vertex uploads as
material remaining costs. The next structural step is GPU-resident local
geometry with per-instance transforms shared by main, mirror and shadow draws.
It must preserve per-camera culling, decal displacement, fog coordinates and
generation retirement, with the current CPU builder retained as a regression
oracle. This trace is diagnostic evidence, not a comparable untraced FPS result
or proof that uploads explain every frame-time spike.

The local-template cache now exposes a borrowed immutable view with stable
identity and array addresses until explicit cache retirement. The live CPU
builder consumes this same interface. Geometry acquisition is independent of
camera, transform, paint and entity state; source span instance fields are not
draw state. This establishes the lifetime contract for GPU buffer ownership,
but does not yet change GPU uploads. Tests verify stable views and unchanged
payloads across moving instances and cache growth, invalid inputs and existing
reference equivalence. Mesh tests, mirror/native-world/renderer-toggle tests
and ASan/UBSan pass after this interface change.

## Shared main/mirror upload ranges

The live renderer now reuses main-view vertex ranges for byte-identical mirror
spans. A bounded hash lookup selects candidates; full payload comparison is
required before sharing, including UV, normal, color, fog and depth attributes.
Unmatched spans are compacted after the main prefix. Draw state and logical
vertex counts remain separate from the physical upload count. Hash collisions
or a full lookup table only reduce sharing. Invalid ranges leave inputs alone.
`diagnostics.modern_unshared_views=true` retains the former upload layout.

The traced completed lap `perf-shared-views-trace/20260907-150714-1e0598`
averaged 9,668,808 uploaded bytes in mirror-active frames, versus 18,531,390
in the earlier trace. This run had severe timing variability (27.24 mean FPS;
88 seconds wall time), so its CPU timings are not evidence of an isolated
speedup. Other desktop workloads were active; they were not stopped.

The subsequent untraced lap `perf-shared-views/20260907-150849-e1e7ac`
completed at 119.21 mean FPS, 114.78 in its slowest 120-frame window, with
14.688 ms worst window p95 and 3.699 ms highest mean preparation. Compared with
119.03 previously, this does not establish an FPS improvement or a 120 FPS floor.
The demonstrated benefit is reduced upload volume, at the cost of comparing
already expanded CPU geometry. This remains per-frame sharing, not persistent
GPU residency or GPU instance transformation.

Paired offscreen captures at race frame 649 (timer 430, frozen scene with mirror)
are byte-identical with sharing enabled/disabled. The compiled range test checks
payload equality, differing UV/fog data, draw-state preservation, in-place
compaction and invalid-range rejection. Mesh, mirror, renderer-toggle and
native-world tests pass, as does the mesh suite under ASan/UBSan.

## Resident vehicle geometry and GPU instance transforms

The default renderer now uploads immutable local vehicle templates once per
asset generation and draws those same GPU buffers in the main, mirror and
shadow passes. Per-draw uniforms carry translation, scale, Euler/quaternion
rotation, fog enablement and decal displacement. Euler operations retain their
original order; normals use rotation only and decal separation remains two
world units along the normalized transformed normal. GLSL is shared by the
main and shadow shaders; SPIR-V and MSL were regenerated with the existing
compiled shader toolchain. Metal execution remains unverified locally.

Local spans reserve diagnostic world-vertex ranges without expanding them on
the CPU. Expansion happens on demand for draw dumps/probes or when a resident
GPU buffer cannot be allocated/uploaded. The existing CPU path still handles
unsupported geometry and CPU fog. `diagnostics.modern_cpu_geometry=true`
disables residency and restores CPU-expanded draws for comparison. Full source
payloads stay within the bounded template cache, with at most 512 resident GPU
buffers to bound driver object overhead; GPU buffers are retired before
those templates on generation change/shutdown. Pending transfer buffers follow
the existing submission lifetime contract, including complete renderer teardown
after a cancelled/failed submission.

In paired frame-649 captures with the mirror active, both the image and main
draw dump are byte-identical between CPU and resident paths. The resident run
recorded 24 initial geometry uploads, reused on subsequent frames. At frame 649
dynamic upload size was 1,193,472 bytes versus 10,054,296 with CPU geometry and
view sharing. Main/mirror preparation in that traced resident frame was
0.829/0.298 ms; dynamic upload recording took 0.144 ms. This is an observed
scene, not a whole-game performance guarantee.

Three completed moving-lap measurements were contaminated by another running
game (`riftbreaker_win`, started at 15:15:25). They cannot establish the speedup
or regression of this implementation:

| Session under build/ | Mean FPS | Slowest window | Notes |
| --- | ---: | ---: | --- |
| perf-resident-vehicles/20260907-151959-5392a4 | 114.80 | 104.57 | Intermediate residency with redundant CPU expansion |
| perf-resident-cpu-reference/20260907-152156-4dedc5 | 117.49 | 108.13 | Residency disabled in the same intermediate build |
| perf-resident-lazy/20260907-152630-f3d5d5 | 113.88 | 107.84 | Final on-demand CPU expansion |

Final run worst window p95 was 19.107 ms and highest window-mean preparation
2.358 ms. No more moving performance runs were launched after identifying the
competing game. Sustained 120 FPS still needs measurement without that load.

The compiled mesh suite checks deferred ranges remain unwritten and reconstruct
byte-identically to the reference, as well as source identity and capacity
failure. GPU depth and shadow-UV tests now exercise both rotation modes with
negative nonuniform scale, translation and nonunit decal normals. Mesh,
fog/depth/shadow-UV/shadow-mask, mirror, native-world, renderer-toggle,
submit-recovery and stage-angle tests pass. The deferred mesh suite also passes
ASan/UBSan with leak detection. Production, smoke, replay and stage targets build.

This completes the first live resident-geometry slice, covering supported
vehicle model banks. Terrain and other unsupported paths still use transient
geometry. The broader architecture roadmap and the 120 FPS target remain open.
