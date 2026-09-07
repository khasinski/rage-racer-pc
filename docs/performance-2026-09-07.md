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
