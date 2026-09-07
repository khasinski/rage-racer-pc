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
payloads stay within the bounded template cache, with at most 512 resident mesh
entries to bound driver object overhead; GPU buffers are retired before
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

### Avoid redundant geometry state commands

Each render pass now retains its bound geometry buffer and local-transform
uniform state. Adjacent draws with the same transform, fog flag and decal state
reuse the uniform even when material, paint or source mesh changes. World-space
terrain draws share the zero/bypass transform. State resets at each pass, so
main, mirror and shadow cameras cannot inherit an uninitialized binding.

At the same frame 649, main-view local-uniform updates fell from one per draw
(1,086) to 45; mirror updates fell from 353 to 45 and shadow updates from 134
to 44. Geometry buffer binds were 34, 34 and 33 respectively. The image and
draw dump remain byte-identical to the preceding resident implementation.
The mesh suite checks relevant transform-state changes and independence from
material/source identity. Mirror, native-world, renderer-toggle and
submit-recovery tests pass. No new FPS claim is made while another game is
using the machine; this checkpoint demonstrates reduced command volume.

### Reuse terrain input during visibility and draw preparation

Authored terrain quads now carry their six validated indices, four decoded
source vertices and snapped positions from visibility evaluation into vertex
preparation. Visible halves reuse these inputs instead of decoding them again.
This storage belongs to one quad/build invocation; no camera-dependent result
is retained between frames or views. Independent triangle pairs retain their
original behavior, and incomplete trailing quads reset both validity flags.

The existing range regression now also checks distinct UVs, normals and colors
at every corner in both CPU/GPU fog modes. The compiled mesh suite, mirror and
native-world tests pass, as does ASan/UBSan. The frozen frame-649 image and draw
dump match the preceding implementation byte for byte. This removes repeated
input reads; it does not make terrain GPU-resident or establish an FPS increase.

### Resident-budget fallback regression

`diagnostics.modern_geometry_limit` can constrain the resident mesh count
from zero to the unchanged default/maximum of 512. The native-world regression
now runs the same frozen race through normal (512), fully transient (0) and
mixed (1) budgets with residency otherwise enabled. This exercises on-demand
CPU reconstruction, rather than the separately CPU-expanded diagnostic mode.

At timer 430, with the mirror active, all three images and draw dumps are
byte-identical. Upload diagnostics prove both the resident and reconstruction
paths execute in the mixed case and that zero budget creates no resident buffer.
The extended native-world test passes, and production, smoke, replay and stage
targets build. This covers budget exhaustion downstream behavior; it does not
simulate every SDL allocation/map failure inside resource creation.

### Indexed resident geometry

Integrated the existing exact geometry-pack prototype without rewriting its
payload identity or reconstruction contract. Resident meshes are packed once
at upload: only byte-identical complete vertex payloads share an index, and
the index stream preserves original triangle order and draw ranges. Packing
is used only when vertex plus index bytes are smaller than the expanded source;
packing allocation failure or no size benefit retains an unindexed resident mesh.
Both forms share the same main/mirror/shadow draw path and CPU reconstruction.

For the 24 meshes loaded in the frozen scene, 90,399 expanded vertices became
41,818 unique vertices plus 90,399 indices. GPU storage fell from 5,062,344 to
2,703,404 bytes (about 47%). The frame-649 image and draw dump match the prior
unindexed implementation byte for byte. The compiled pack suite checks exact
reconstruction and every payload byte; it also passes ASan/UBSan. Native-world
tests cover normal, zero and mixed residency budgets with the indexed path.
Mirror, renderer-toggle and submit-recovery regressions pass.

Index buffers follow the same generation and submission lifetime as their
vertex buffers. The residency limit now counts mesh entries, each using one
vertex buffer and optionally one index buffer (at most 1,024 buffer objects
for 512 entries). Trace logs distinguish index-buffer binds from vertex binds.
This establishes storage reduction and indexed rendering, not an isolated FPS
increase under the concurrent-game workload.

### Resolve draw materials once per view

DrawSet now classifies each span's material, pipeline, phase and clearcoat
policy once, after all texture upload attempts. Resolving after that complete
upload pass preserves successful retries of a shared material whose earlier
attempt failed. The four ordered rendering phases consume bounded scratch
records instead of repeating texture lookups and classification.

The frame-649 dump contains 1,075 textured main-view spans, so phase-selection
lookups decrease from 4,300 to 1,075; upload/cache checks are additional and
unchanged. No triangle order or material phase changes. Image and draw dump
are byte-identical. Mirror, native-world (including residency budgets),
renderer-toggle and submit-recovery tests pass; production, smoke, stage and
replay targets build. The count reduction is derived from the executed draw
dump and code path; it is not a measured FPS improvement.

## Resident terrain vertex prefix

The world-geometry path now retains exact terrain vertex payloads between
frames in the existing GPU vertex buffer. Per-view visibility still determines
an ordered index stream each frame; neither culling nor triangle order changed.
Non-terrain data, including camera-facing overlays and vehicle CPU fallbacks,
occupies a transient tail. Uploads include only newly retained vertices, that
tail and the current index stream. Stable terrain vertices are not recopied
to GPU after their initial upload.

`diagnostics.modern_world_vertex_limit` controls the bounded world pack, from
zero to the default/maximum of 1,000,000 live vertices. CPU-fog and CPU-geometry
reference modes retain transient uploads. Budget overflow resets the pack and
publishes the entire new prefix; allocation failure or an oversized frame
uses the former transient path. A transient upload invalidates GPU prefix
tracking so the next packed frame republishes it. Asset generation retirement
and renderer shutdown clear this state; failed submission uses the established
complete resource teardown contract.

At frame 649 the pack held 25,917 resident vertices and a 330-vertex transient
tail. Its 21,312 indices plus the changed tail required 103,728 upload bytes,
versus 1,193,472 before terrain residency. The image and draw dump are
byte-identical. That traced frame spent 0.623 ms in upload preparation, including
CPU hashing/packing; the earlier sample took 0.144 ms. Reduced transfer volume
therefore is not evidence of a net frame-time improvement. Avoiding repeated
CPU staging and hashing remains relevant performance work.

The native-world regression now compares full residency, disabled world
residency, a one-vertex budget forcing transient fallback, and a budget forcing
at least ten full prefix rebuilds. All images/draw dumps match with the mirror
active. Mirror, renderer-toggle, submit-recovery and compiled pack tests pass;
production, smoke, stage and replay targets build. No new moving FPS benchmark
was run under the competing-game workload. Terrain visibility, normals and
draw preparation still execute on CPU; this stage makes its GPU vertex storage
persistent, not its entire preparation pipeline.

### Pack world geometry directly from prepared ranges

The packer now accepts borrowed vertex ranges with per-range or per-vertex
retention policy. It preserves the same concatenated index order and validates
every range before insertion. The renderer passes its prepared CPU geometry
directly, removing the intermediate 1,193,472-byte staging copy in the control
scene and the per-vertex retention-mask buffer. Only the final changed vertex
data and indices are written to the GPU transfer buffer. Transient fallback
still gathers the original ranges when packing cannot be used.

Constant retained/transient ranges also skip the irrelevant packing pass.
Range tests cover exact reconstruction, equivalence to contiguous input,
changing transient data, empty ranges, count overflow and rejection of an
invalid later range without partial insertion. The tests pass under ASan/UBSan.
The control image and draw dump match; native-world budget/rebuild regressions
and submit recovery pass. Production, smoke, stage and replay targets build.
The intermediate range implementation recorded 0.518 ms upload preparation
at frame 649 versus the earlier 0.623 ms sample, but these are not isolated
performance measurements and the final range-pass skipping change was verified
for correctness rather than benchmarked. The 120 FPS target remains open.

### Parallelize vertex hash dependency chains

Vertex hashing now mixes the fourteen payload words through four independent
integer chains before the existing avalanche. Exact full-byte comparison still
decides identity; vertex insertion and index order are unchanged.

A local C microbenchmark compiled with GCC 16.2 at `-O2` alternated baseline
and candidate three times. Each run warmed ten frames, then packed 1,500 frames
of 24,576 terrain-like vertices in 96 retained ranges, with a repeated second
view. Both versions produced 8,192 resident vertices and reconstructed every
input vertex exactly. Baseline milliseconds per packing iteration were
0.507368, 0.499908, 0.505696; candidate times were 0.212953, 0.220500, 0.215341.
This synthetic CPU workload improved about 57%; it is not a game FPS result.
The temporary harness is `build/pack-hash-benchmark.c` in this workspace.

The real PAL control image and draw dump remain byte-identical. Frame 649
still uploads 103,728 bytes. Its complete upload preparation measured 0.533 ms,
which does not demonstrate an improvement over the earlier 0.518 ms sample.
Riftbreaker remained active, so no isolated moving FPS comparison was made.
The pack regression passes ASan/UBSan with leak detection. The production and
smoke targets build successfully. Native-world residency/fallback/rebuild,
submission recovery and geometry-pack regressions all pass (3/3).

### Resolve frame meshes once for both cameras

The renderer now resolves resident mesh pointers once per main-scene instance
after world warming has finished. Main-view construction, mirror construction
and completeness checks share that frame-local table. This replaces up to
three provider/cache searches per instance with one, while preserving successful
late retries during warming. The table is rebuilt on every preparation, including
asset-generation changes and resizes. It borrows meshes, reuses its allocation,
is freed at renderer shutdown, and falls back to direct lookup on allocation
failure. Excluded passes do not request resident meshes.

The PAL control image and draw dump match the preceding commit exactly.
At frame 649, traced warm/main/mirror/completeness times changed from
0.111/0.781/0.279/0.065 ms to 0.183/0.669/0.213/0.000 ms. Resolution now falls
inside the warm interval; these individual samples under competing load are
not an isolated performance comparison or proof of 120 FPS.

Production and smoke builds pass with warnings treated as errors. Mirror cars,
renderer toggle cycles, native-world residency/rebuild/fallback and submission
recovery regressions pass (4/4).

An earlier experiment skipping hash lookups for repeated quad corners was
discarded: three alternating synthetic packing runs regressed from
0.214218/0.205670/0.210597 ms to 0.230011/0.226626/0.225684 ms. No such shortcut
remains in the implementation.

### Coalesce adjacent GPU draw ranges

Main and mirror passes now coalesce adjacent spans only when phase, pipeline,
texture, clearcoat policy, instance uniforms, local transform uniforms, vertex
buffer and index buffer agree, and the ranges are contiguous. Triangle order
is preserved, including transparent and decal geometry. No sorting is performed;
shadow submission is unchanged. The semantic draw dump keeps its original spans.
`diagnostics.modern_unbatched_draws=true` retains the individual-command reference.

The real PAL control frame emits 1,072 main and 350 mirror geometry commands,
down from 1,086 and 353. This modest reduction of seventeen commands does not
establish an FPS improvement. The complete image and semantic draw dump match
the preceding commit byte for byte. The native-world regression also compares
batched and unbatched frozen frames, in addition to residency, bounded rebuild
and transient fallback paths. Production and smoke builds succeed; mirror cars,
native-world and submission-recovery regressions pass (3/3). The expanded
native-world regression passes again with the explicit unbatched comparison.

### Locate the remaining legacy scene cost

Optional frame tracing now splits race scene work into cars, environment,
sky, terrain, course objects, scripted scenery, mirror and course scenery.
Both active and paused race paths retain their execution order. With tracing
disabled the existing profiler returns without reading the clock.

In the real PAL frozen control, frame 649 spent 3.497 ms in UpdateEnvironment.
The preceding instrumented run measured sky at 0.004 ms, terrain at 0.245 ms
and mirror at 0.115 ms. This rules out sky geometry as the source of the large
scene interval in that sample. Environment palette interpolation uploads only
sixteen colors, but LoadImage enters GPU_DataWrite, which calls
Psyz_GpuExeque: queued legacy packets are dispatched and flushed there before
the upload. The interval therefore cannot be attributed to palette arithmetic.
The SDL GPU backend's Draw_ExequeSync is a no-op; a GPU fence wait has NOT been
established as the cause. Next profiling should separate queue dispatch from
the texture upload before changing either contract.

Production and smoke builds pass. Control image and semantic draw dump match
the previous commit exactly. This instrumentation establishes a more useful
next target; it is not an optimization or an isolated FPS result. Riftbreaker
was still running when the measurements were taken.

### Isolate legacy per-pixel compatibility work

Temporary C timing around GPU_DataWrite measured frame/timer 430 at 3.839624 ms
for queue execution and 0.004338 ms for the sixteen-color upload. Within queue
execution, dispatch of the same 15,041 GP0 words took 3.821478 ms and its final
flush took 0.009518 ms. This excludes palette generation/upload as the main
cause of the environment interval.

A diagnostic run disabled only the opaque quad compatibility correction calls
inside Draw_PushPrim. Dispatch fell to 0.274891 ms, with a 0.010933 ms final
flush. Those corrections include PS1-versus-modern texture sampling checks for
every pixel of flat-textured quads, not merely filling geometric edge gaps.
Gouraud quads already check only endpoints. The next substantive performance
target is this CPU sampling/coverage algorithm and its interaction with the
modern overlay path. Removing it globally would weaken classic-render fidelity
and is not the proposed fix.

All temporary timing and bypass code was removed; the PSY-Z worktree is clean
and production/smoke binaries were rebuilt with corrections enabled. The raw
local traces are /tmp/rage-palette-split.log, /tmp/rage-queue-split.log and
/tmp/rage-queue-no-gaps.log. This intervention identifies a costly code path,
not an achieved FPS gain; the competing game was still active.

### Reuse sample floors for the instability check

The PSY-Z CPU texture sampler already floors both interpolated UV coordinates
to select texels. It now checks distance to those integer endpoints and their
successors instead of independently calling round twice. Interpolation order,
clamping and the strict 1e-7 compatibility threshold are unchanged.

The compiled reference suite passes 2,014,720 exact sample/flag comparisons.
An added 30,750 cases exercise both UV axes around integers, half-integers,
the tolerance threshold and adjacent representable doubles, including negative
coordinates. The suite also passes ASan/UBSan. The PAL image and semantic dump
match the previous commit byte for byte; production and smoke builds pass.

A GCC 16.2 -O3 C microbenchmark alternated ten-million-sample runs of the old
and new header: baseline 92.360/84.760/86.756 ms; candidate
67.657/68.697/67.435 ms, with identical checksums. This is about 23% less time
in that synthetic sampling kernel, not in the whole frame. Its local harness
is build/sample-round-bench.c. One traced control frame measured environment
work at 3.331 ms versus the earlier 3.497 ms sample; competing load prevents
attributing that difference to this change. The 120 FPS target remains open.
Native-world, renderer-toggle and texture-sample regressions pass (3/3).

### Prepare common correction-pixel attributes before copying vertices

The compatibility pixel emitter now sets UV/color on one local vertex before
replicating it four times, then sets the four positions. It previously copied
the source four times and overwrote the shared attributes separately. Buffer
reservation, topology, ordering and every output attribute are unchanged.

A temporary compiled C harness extracted both emitter implementations and
compared complete vertex/index bytes for 100,000 varying inputs, including
untextured pixels and changing offsets. All matched. Alternating ten-million-
pixel runs at GCC 16.2 -O3 measured baseline 62.393/56.716/56.683 ms and candidate
51.106/50.645/50.762 ms, with equal checksums. The local harness is
build/pixel-template-bench.c; this is a CPU emitter measurement, not FPS.
Production and smoke builds pass, and the PAL image and draw dump match exactly.
Frame 649 environment time was 3.418 ms, which does not establish improvement
over the preceding 3.382 ms sample under competing load.
Renderer-toggle and native-world regressions pass (2/2).

### Solve triangle coverage once per long scanline

Flat-textured quad correction now intersects the three integer edge inequalities
once for each scanline of at least sixteen candidate pixels. Each per-pixel
coverage test then checks the resulting inclusive interval. Integer floor/ceil
division preserves negative intersections, exclusive edges and both windings;
the original predicate removes rejected exact vertices at the endpoints.
Shorter rows and Gouraud endpoint correction retain the point predicate.
UV selection, texture-window comparison and correction pixel emission are
unchanged.

The coverage regression compares the original point algorithm, prepared point
algorithm and scanline algorithm over 2,165,625 samples. It exercises a local
range, a single-pixel clip and reversed/empty bounds for each sample, including
degenerate triangles and exact vertices. ASan/UBSan passes. Production/smoke
builds and native-world, renderer-toggle and coverage regressions pass (3/3).

A local GCC 16.2 -O3 C benchmark, build/coverage-row-bench.c, processed 200,000
rows for each width with equal coverage checksums. Point/row milliseconds were
1.629/2.956 at width 4, 6.842/2.668 at width 16, 22.044/4.021 at width 64 and
76.834/9.904 at width 256. The short-row regression motivated the sixteen-pixel
threshold. This measures only coverage; it is not a whole-frame speedup.
The initial all-row variant recorded 3.473 ms environment work at frame 649,
which does not demonstrate an improvement over the earlier 3.331 ms sample.
The final thresholded build measured 3.448 ms in that interval and reproduces
the PAL control image and semantic draw dump byte for byte. No isolated FPS
comparison was made; sustained 120 FPS remains unverified.

### Recognize quads whose samples always require correction

Temporary per-quad timing identified axis-aligned sky tiles among the expensive
flat-textured quads (for example a 64x128 destination mapping U by 64 and V by
127). One UV axis has integral derivatives even though the other is stretched.
For planes prepared from integer vertices, that axis stays integral at every
integer pixel, so the existing sample-instability predicate always returns true.

The correction path now recognizes this property once for both triangle planes.
It retains the same PS1 span ownership, UV/color interpolation and emitted
correction pixels, but omits coverage and modern-sample comparisons whose result
cannot change the decision. Gouraud quads and fractional mappings use the
existing general path. This is not a bypass of compatibility correction.

The reference sampler suite now performs 2,263,552 exact comparisons and checks
that every positively classified plane really requests correction at each
reference sample. It includes the observed sky mappings, negative slopes and
an explicit fractional-axis rejection case; ASan/UBSan passes. Production and
smoke builds succeed. The PAL control image and semantic dump are byte-identical.
Frame 649 measured environment work at 3.382 ms versus the earlier 3.448 ms
sample, which is not an isolated performance comparison. Temporary per-quad
timing was removed. The remaining path still emits individual compatibility
pixels; this stage removes redundant decisions, not that output volume.
Native-world, renderer-toggle and texture-sample regressions pass (3/3).

### Emit unit-step correction rows as rectangles

Always-corrected flat-textured rows with a positive unit U step and constant
V/color now emit one rectangle per run instead of one per pixel. Runs stop at
triangle ownership boundaries, clipping, UV byte wrap and the original buffer
flush boundary. Fractional/negative slopes, changing V/color and Gouraud retain
individual pixels. The existing shader and vertex ABI are unchanged. Setting
PSYZ_REFERENCE_CORRECTION_PIXELS restores individual correction pixels for A/B
validation.

Saved vertex/index counts are tracked as a logical buffer budget. A run may
compress geometry but cannot postpone the original flush, since subsequent
primitives may sample prior framebuffer writes. Buffer reset clears both real
and saved counts; a failed flush with no room terminates correction rather
than creating a zero-length run.

The compiled span test covers all 256 starting U values and lengths 1..512,
checks generated UV/RGB values and wrap boundaries, and rejects negative or
fractional U steps and varying V/color. Its existing 5,079,264 reference
comparisons still pass, as does ASan/UBSan. The native-world regression now
compares the reference/candidate full VRAM, modern image and semantic draw dump.
Native-world, renderer-toggle, submit recovery and span tests pass (4/4).

Real PAL captures match at native VRAM resolution. During validation of the
row geometry, a temporary probe also compared the complete 4x scaled VRAM
textures: both 33,554,432-byte RGBA files were identical. The probe, temporary
scale override and all temporary GPU capture code were removed before the
final build. Metal hardware remains untested; the 4x check used SDL GPU Vulkan.
An early variant measured 3.256 ms reference versus 2.576 ms candidate in the
environment interval at frame 648, before preserving logical flush budgets.
This is an exploratory sample under competing load, not a measured FPS gain.
The final version preserving flush budgets measured 3.763 ms reference versus
3.990 ms candidate at that frame. This does not confirm a frame-time benefit.
The verified gain is reduced emitted vertex/index volume for eligible runs;
the number and ordering of flushes intentionally remain the same.

### Skip batch VRAM copies when sampled pages remain current

The SDL GPU backend now tracks native VRAM writes in an independent 32x32-word
tile mask. Before a textured batch, it checks every referenced texture page
and indexed palette against this mask. A clean sampling footprint reuses the
existing mirror; any overlap still copies the complete 2 MiB VRAM. Dirty tiles
persist across unrelated batches and readback-cache operations and are cleared
only by a full mirror copy. Texture windows are covered by conservatively
checking whole pages. All existing upload, clear, move, framebuffer draw and
scaled-to-native write observers feed the mask.

This does not restore the old partial dirty-rectangle copy removed in PSY-Z
579f8eb9. Source pages and CLUTs remain intact in the mirror; a dirty sampling
dependency triggers the full original copy. Device creation starts fully dirty,
explicit full snapshots establish coherence, and exposing the mutable live
VRAM handle disables skipping for that device. The snapshot handle is documented
as borrowed for sampling only. PSYZ_REFERENCE_FULL_VRAM_BATCH forces the original
batch-copy behavior; PSYZ_VRAM_BATCH_TRACE reports copied/skipped batch bytes.

The C regression passes 10,660,864 shader-address checks across texture depths,
pages and CLUTs, plus tile retention, clipping and extreme integer inputs.
ASan/UBSan passes. Native-world compares optimized VRAM/images against original
pixel correction and full batch copies and requires actual skipped copies.
That fixture recorded 7 copied and 14,872 skipped textured batches versus
14,879 copied and zero skipped reference batches. These counters exclude the
modern renderer's explicit full-frame snapshots.

Production and smoke builds pass. Renderer toggle, native-world, submission
recovery, retained texture history, sky identity, track texture snapshot and
sample-mask regressions pass (7/7, after building two previously absent test
executables). The real PAL VRAM, modern image and draw dump match exactly.
Frame 648 measured 2.456 ms reference versus 2.433 ms candidate in environment
work; this single pair under competing load does not establish a frame-time
speedup. The proven reduction is in full batch texture-copy commands and bytes.

### Free-GPU moving measurements after the sampling-mask change

The user confirmed the competing game had stopped. Three sequential real-window
PAL class-1/course-0, one-lap autopilot runs completed with the original local
INI, prewarm enabled, Wayland, detected 120 Hz, VSync and a 1706x960 modern target.
The two timing runs disabled per-frame tracing. Metrics below exclude only the
first startup profile window and aggregate the remaining 34 complete 120-frame
windows using total frames divided by their summed durations.

| Batch sampling mode | Mean FPS | Slowest 120-frame window FPS | Worst window p95 ms | Maximum interval ms |
| --- | ---: | ---: | ---: | ---: |
| Dependency mask | 117.804 | 112.010 | 16.154 | 31.821 |
| Original full copies | 118.059 | 112.660 | 15.242 | 27.026 |

Artifacts: `build/perf-120hz-vram-sample/20260907-173916-3487d3`
and `build/perf-120hz-full-copy-reference/20260907-174123-c0dc1f`.
Both use the same executable; the reference enables
`PSYZ_REFERENCE_FULL_VRAM_BATCH=1`. These runs do not demonstrate an FPS gain
from eliminating redundant batch copies. Stable 120 FPS is not achieved.
Reported intervals measure application presentation timing, not physical scanout.

A separate diagnostic lap at
`build/perf-120hz-phase-clean/20260907-174020-ad02cc` identifies
`scene_environment` as the largest measured non-wait game phase: mean 5.226 ms,
maximum 17.370 ms over 853 calls. Terrain averages 0.433 ms. This phase includes
`UpdateEnvironment()` and its palette upload, which also executes queued legacy
GPU commands; the number must not be attributed to palette interpolation alone.
Tracing adds overhead, so this lap is for attribution, not a headline FPS score.
The next performance work should target this synchronous legacy dispatch and
presentation stalls, with unchanged PAL timing and image/VRAM regression oracles.

### Dispatch attribution and consecutive fixed-point texel runs

An opt-in `PSYZ_GPU_DISPATCH_TRACE` diagnostic now reports textured-quad
correction time/calls (including a Gouraud subset), buffer-flush time/calls and
an SDL nanosecond timestamp at dispatch completion. Correction time includes
flushes nested inside correction, so the two timings must not be added.
With the diagnostic disabled no timestamp calls or trace output are issued.

The completed moving diagnostic lap at
`build/perf-dispatch-clean/20260907-174540-b19fa4` recorded 846 dispatches with
more than 500 corrected quads: mean correction time 5.137 ms, mean flush time
0.848 ms and mean 1192 quad calls. This narrows the expensive environment phase
to legacy textured-quad correction; it is not a measurement of palette math.

The correction emitter now also combines fractional fixed-point U steps when
they select exactly consecutive texels. A carry/borrow bound determines the
first pixel where this stops being true. Constant V/color, UV byte wrap,
triangle ownership, clipping and the original logical buffer-flush boundaries
remain required. The exact-unit-step fast path remains in place. The existing
pixel-reference override still disables all combined runs.

One million independent pixel-walk comparisons validate maximal run lengths,
including fractional steps around unity, negative steps/coordinates, clipped
starts, UV wraps and changing V/color rejection. The existing 5,090,587 UV/RGB
comparisons and ASan/UBSan pass. Native-world verifies VRAM, image and semantic
draws against pixel correction/full batch copying. A separate frozen real-PAL
image and draw dump match the pre-change executable exactly
(`/tmp/rage-eager-span.*` and `/tmp/rage-consecutive-span.*`).

The subsequent trace-off real-window lap completed at
`build/perf-consecutive-span/20260907-175146-6721c2`: 117.737 mean FPS,
114.350 slowest-window FPS, 15.528 ms worst-window p95 and 30.411 ms maximum
interval across 33 post-startup complete windows. The same PAL/class-1/course-0,
VSync 120 Hz and 1706x960 settings were retained. This does not establish an
end-to-end FPS improvement over the prior 117.804 mean; the 120 FPS stability
target remains unmet.

### Separating throughput from the game/presentation schedule

With the GPU free, the same production binary and identical graphics settings
completed two further PAL class-1/course-0 laps. Only `video.fps` differed, in
temporary copies of the local INI; the user's original configuration was not
modified. Both used the real Wayland window and retained the 1706x960 target.
The numeric FPS modes request immediate presentation rather than VSync.

| Mode | Mean FPS | Slowest window FPS | Worst window p95 ms | Maximum interval ms |
| --- | ---: | ---: | ---: | ---: |
| Numeric 1000 limit | 644.742 | 407.970 | 7.973 | 26.727 |
| Numeric 120 limit | 117.517 | 113.310 | 15.284 | 28.255 |
| VSync 120 Hz (preceding run) | 117.737 | 114.350 | 15.528 | 30.411 |

Artifacts: `build/perf-throughput/20260907-175406-3116cf` (191 complete
post-startup windows) and `build/perf-120-immediate/20260907-175510-ce2e65`
(33 windows). FPS is weighted by window duration. This is application
render/submission throughput, not a claim of 645 unique physical scanouts on a
120 Hz display, and does not prove absence of individual long frames.

These results change the next optimization priority: bulk rendering throughput
is already several hundred FPS, while both 120 FPS modes miss presentation
slots. `ModernFramePresented` drops elapsed slots on a shared game/render
thread. `ServiceGameFrame` dispatches the scene before entering its presentation
wait; the environment palette upload executes queued legacy GPU commands inside
that uninterrupted work. Optimize this scheduling/dependency path while keeping
PAL/NTSC simulation timing and visual output, rather than reducing graphics
quality or treating VSync alone as the cause.

A temporary attribution-only bypass of textured-quad correction completed
`build/perf-correction-attribution/20260907-175731-10d1fe` with the original
VSync INI: 119.737 mean FPS, 114.970 slowest-window FPS, 14.302 ms worst-window
p95, 32.696 ms maximum interval (34 post-startup windows). This deliberately
omitted compatibility rendering work and is not visually validated or a
shipping solution. It implicates that blocking work in missed presentation
slots but does not explain every outlier. The bypass was removed immediately
after measurement and production/smoke binaries rebuilt from the original
correctness-preserving source.

### Constant-color correction rows and publication boundary audit

Prepared texture triangles now classify equal RGB at their three vertices
once. Constant-color scanlines assign the original RGB endpoints directly;
varying-color triangles keep the original interpolation and conversion order.
This removes repeated zero-gradient RGB interpolation from the blocking legacy
quad-correction path. The frozen independent rasterizer matches 5,220,000 rows,
including flat colors, one-channel differences, equal-Y vertices, reversed
edges and draw offsets. ASan/UBSan and native-world VRAM/image/draw comparison
against the pixel reference pass; production and smoke builds pass.

The game/presentation boundary audit found a prerequisite for cooperative
presentation during scene updates: `CaptureFrameBegin` flips the capture index
and immediately clears that buffer, while `CaptureCurrent` exposes it;
`GameRenderWorldBeginFrame` similarly flips/reset its mutable world and
`GameRenderWorldPresentation` interpolates from that mutable world. The existing
wait-loop presentation is after publication and therefore does not justify
calling it halfway through scene construction. A safe next architectural stage
needs independently retained previous/current published states plus a separate
building state, with matching captured packets and texture revisions. Only then
can additional presentation opportunities be introduced without showing partial
scenes. Do not add a mid-scene present callback against these current APIs.

The trace-off VSync control lap completed at
`build/perf-flat-color/20260907-180249-5a170c`: 117.686 mean FPS,
110.890 slowest-window FPS, 15.371 ms worst-window p95 and 25.476 ms maximum
interval across 34 post-startup windows. The original PAL/class-1/course-0
configuration and 1706x960 target were retained. There is no demonstrated
whole-frame FPS improvement; prioritize the publication/scheduling boundary
rather than treating further scalar raster changes as a solution to 120 Hz.

### Captured-packet publication is separate from construction

`scene_capture` now has three snapshot slots: a private building slot and two
completed frames exposed by `CaptureCurrent`/`CapturePrevious`. Begin-frame
clears only the building slot; end-frame completes the packet walk/metadata and
then rotates ownership. Repeated end-frame calls without a new begin do not
publish another frame. This is a same-thread borrowing contract, not a
thread-safe publication API. Existing live asset identities retain their prior
lifetime requirements.

The opt-in `RAGE_VERIFY_CAPTURE_PUBLICATION` oracle hashes all metadata and
active draw/terrain/packet/face bytes (including live bank identities) before
construction and verifies both published snapshots before rotation. Native-world
now enables and requires this oracle. Renderer toggles, native-world, submit
recovery and retained history pass (4/4). Real PAL frames 384/512/640 verify
publication stability; the frame-430 image and semantic draw dump exactly match
the pre-change capture (`/tmp/rage-published-capture.*`). Production/smoke build.
The extra statically allocated snapshot is 6,621,224 bytes on this toolchain.

This implements the first publication boundary in the actual game/renderer path
without changing frame pacing or claiming an FPS gain. The world-instance buffers
and texture-generation relationship still need equivalent publication ownership
before presentation is allowed during scene construction.

### World publication retains two completed instance buffers

The production world adapter now has separate building/current/previous slots.
`GameRenderWorldEndFrame` publishes after all scene submissions and capture end
in `PortAfterSceneHandler`; current/previous accessors and synchronized
presentation read only completed worlds. Before the first/second publication,
the corresponding accessor returns NULL. Repeated end calls without begin do
not rotate ownership. Starting construction copies the latest completed metadata
into the private building slot and retains that slot's own instance array, so
camera history never comes from an older recycled slot. This remains a
same-thread borrowing API; it does not introduce background rendering.

`RAGE_VERIFY_WORLD_PUBLICATION` checks the full metadata and active instance
bytes of both published worlds across scene construction. Native-world requires
both the capture and world oracles. Renderer toggles, native-world, submission
recovery and retained history pass (4/4), and the frozen real-PAL image/draw dump
match the prior capture-publication stage exactly (`/tmp/rage-published-world.*`).
The production PAL class-1/course-0 lap also completed with both mutation oracles,
original VSync configuration and automatic PAL timing at
`build/verify-published-world-drive/20260907-181156-6674bc`. Its hashing overhead
means this is a lifecycle/route check, not an FPS comparison.

The extra world plus 4096-instance array costs 689,016 bytes on this toolchain.
Texture/asset generation ownership and frame-to-texture association remain to
be addressed before allowing presentation inside scene updates. No pacing or
simulation-speed change is included in this checkpoint.

### The presentation VRAM cache owns its sampled pixels

The frame-keyed VRAM cache previously retained PSY-Z's shared `vram_sample`
handle. That handle is also the backend's batch sampling mirror, so later
legacy commands can change its contents without changing the cached frame key.
The modern renderer now copies the captured RGBA8 VRAM into a private sampling
texture. Its lifetime follows the presentation resource generation, and writes
for subsequent frame keys cycle the destination backing storage. The existing
frame cache still performs capture only once per successfully captured key.

The new compiled GPU regression uploads a known full VRAM pattern, takes an
owned copy, overwrites the source, and reads back both to verify independence.
It also verifies rejection of invalid input without changing saved contents,
refreshing the owned image, and release/recreation. On this machine the GPU test
passes, alongside snapshot-cache, renderer-toggle, native-world, submission
recovery and retained-history regressions (6/6). Production/smoke build; the
real PAL frozen image and draw dump remain identical
(`/tmp/rage-owned-vram.*` versus `/tmp/rage-published-world.*`).

This adds a 2 MiB destination backing image (driver cycling can retain more than
one allocation) and one extra 2 MiB GPU copy per newly captured frame key. The
trace-off original-INI PAL lap completed at
`build/perf-owned-vram/20260907-181826-59ac1a`: 118.375 mean FPS,
114.470 slowest-window FPS, 15.155 ms worst-window p95, 35.854 ms maximum interval
across 34 post-startup windows. This is a correctness/ownership prerequisite,
not evidence that stable 120 FPS has been achieved.

Capture still occurs on first presentation of a frame key. Before introducing
mid-scene presentation, ensure its first sample cannot be taken during a partial
texture update, and guard/retain the matching native material and asset
generations. The mutable PSY-Z mirror is no longer sufficient reason to delay
reuse of an already captured private image.
