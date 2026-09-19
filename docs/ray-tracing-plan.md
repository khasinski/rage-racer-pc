# Ray tracing plan

## Goal

Add optional ray-traced lighting to the modern renderer without changing game
simulation, asset loading, the classic renderer, or the first-run contract.
The first shippable target is ray-traced sun visibility (hard/soft shadows),
followed by selected reflections. Full path tracing is not an initial goal.

The feature must run in the existing game window, preserve the raster modern
renderer as a supported path, and require no network connection or AI service
from players.

## Architectural decision

The current modern renderer already has the right high-level boundary:
`RenderWorld` contains renderer-neutral cameras, previous transforms, light,
materials and mesh instances. The SDL GPU backend owns buffers, shaders and
submission. Ray tracing should consume `RenderWorld`; it must not inspect PS1
ordering tables, GTE state, VRAM coordinates or gameplay globals.

SDL GPU exposes graphics and compute pipelines, storage buffers and storage
textures, but no portable ray-tracing acceleration-structure API. Therefore:

1. Build the first implementation as a portable compute tracer using a BVH.
2. Keep acceleration structures and tracing behind a small backend interface.
3. Consider Vulkan/Metal/D3D12 hardware RT backends later, after the portable
   path proves the scene contract and visual result. Do not duplicate three
   native backends before that point.

This is a hybrid renderer. Rasterization remains responsible for primary
visibility, transparencies, sky, UI and presentation. Compute rays add effects
whose value justifies their cost.

## Where TypeSafe belongs

TypeSafe's System One model should not make per-frame rendering decisions.
Code owns exact rules, calculations and side effects; narrow semantic judgments
are useful only in the asset-authoring workflow.

An optional authoring pass may propose missing material semantics from structured
asset metadata. Its state contains stable asset and material IDs, source names,
mesh/component context, existing sidecar values, and deterministic texture
statistics. Independent questions can be batched:

- `surface_class` — Choice: road, painted metal, bare metal, glass, rubber,
  water, foliage, emissive, ordinary opaque, or no match;
- `reflection_importance` — Score with concrete levels from none to hero;
- `roughness_band` and `metallic_band` — Scores used to propose ranges;
- `casts_shadow`, `receives_shadow`, `double_sided` — separate Nouls.

Exact facts stay in code. Alpha coverage comes from pixels, instance motion from
the scene, and whether geometry exists from the importer. TypeSafe must not infer
those facts.

Answers produce a review report, never a runtime override. Code validates ranges
and stable IDs, confidence thresholds are calibrated on a hand-labelled Rage
Racer set, and uncertain or conflicting proposals require review. Accepted
values are committed to ordinary material sidecars. Builds, the game and the
asset importer remain fully usable without a TypeSafe key. If this workflow is
implemented in the repository, its client must follow the project's compiled C
tooling constraint and use the current TypeSafe HTTP API; no Python or embedded
interpreter is introduced.

## Stage 0: measurable raster baseline

Before visual changes:

- capture representative fixed frames for every course, reverse routes, replay,
  start grid, tunnel, mirror and each weather/environment state;
- record GPU frame time, upload time, draw count, triangle count and resident
  memory at 720p, 1080p and the current internal-scale presets;
- add a stable camera-cut/history-reset signal to `RenderWorld` snapshots;
- preserve current raster images as the no-ray reference.

Exit condition: repeatable captures and timings on macOS, Linux/Steam Deck and
Windows, with ray tracing disabled.

## Stage 1: renderer-neutral ray scene

Create `src/render/ray/` with no SDL dependency:

- `RayMesh`: indexed local-space triangles and material slots;
- `RayBlas`: immutable BVH per mesh/asset generation;
- `RayInstance`: BLAS handle, current and previous transforms, visibility flags;
- `RayScene`: camera, directional light and bounded instance table;
- `RayTlas`: per-frame instance bounds and BVH;
- `RayHit`: distance, barycentrics, geometric normal, instance and material IDs.

Reuse imported indexed geometry rather than the expanded per-draw vertex stream.
Build BLAS data when a mesh becomes resident and retain it for the asset
generation. Refit or rebuild the small TLAS once per presented frame for cars
and animated scenery. Static course and terrain data must not be rebuilt every
frame.

Add a scalar CPU tracer as the correctness oracle. It is for tests and offline
diagnostics, not gameplay.

Exit condition: CPU tests cover nearest hit, miss, transformed instances,
backface policy, degenerate triangles, large coordinates and generation reset.

## Stage 2: portable compute visibility

Add `modern_ray_gpu.c` beside `modern_native_gpu.c`, with ownership of its own
pipelines, storage buffers and history textures. Upload BLAS data per asset
generation and TLAS/instance transforms per frame.

Extend the raster pass with the minimum G-buffer needed by the first effect:

- linear depth;
- world or view normal;
- material/instance flags where needed;
- motion information reconstructed from current/previous camera and transforms.

Trace one directional-light visibility ray per selected pixel. Start at half
resolution with checkerboard sampling. Bias the origin using geometric scale
and normal; do not reuse PS1 depth bias as a ray epsilon.

The compute output is a visibility texture. The lighting/composite pass uses it
to modulate direct light before fog and post-processing. Alpha-blended surfaces
do not enter the first ray scene. Alpha-masked support follows only after the
opaque path is stable.

Exit condition: ray shadows match the CPU oracle on synthetic scenes and remain
stable through a complete race, replay and renderer toggle.

## Stage 3: temporal accumulation and denoising

Use the previous cameras and instance transforms already present in
`RenderWorld` to reproject visibility. Add:

- depth/normal rejection;
- entity/material rejection for moving cars;
- bounded temporal accumulation;
- edge-aware spatial filtering;
- mandatory reset on camera cuts, scene changes, renderer toggles, asset
  generation changes and resolution changes.

Run the mirror as a separate view and separate history, or leave mirror rays
disabled until that state is implemented. Never share main-camera history with
the mirror.

Exit condition: no ghost shadows during hard replay cuts, car selection,
teleports, start-grid animation or transitions between race and result screens.

## Stage 4: productize ray-traced shadows

Add configuration with explicit capability reporting:

```ini
[modern]
ray_tracing = off       # off, shadows, reflections, full
ray_quality = medium    # low, medium, high
```

Initially ship `off` by default. Unsupported compute features or allocation
failure keep the modern raster renderer active and report the reason; they must
not switch to the classic renderer. Quality controls resolution, ray count and
filtering, not simulation.

Establish measured budgets before choosing release defaults. Suggested starting
targets are 60 fps at 1080p on a representative desktop GPU and a usable
30/40/60 fps quality ladder on Steam Deck. Treat these as gates to measure and
adjust, not assumptions.

Exit condition: feature can be enabled and disabled in-game, survives device
recreation, and passes all existing modern-renderer tests with `off`.

## Stage 5: selected reflections

Reflections need more infrastructure than visibility rays:

- GPU material table with base colour, emissive, roughness, metallic and alpha
  mode;
- texture indirection or an atlas usable by compute hit shading;
- hit UV interpolation and normal transform;
- sky/environment evaluation for misses;
- roughness-aware ray direction and mip choice;
- temporal/spatial filtering with disocclusion handling.

Trace only materials that opt in through reviewed material properties. Begin
with car paint, glass and a small set of wet/metal course surfaces. Keep the
raster result for ordinary diffuse surfaces. Transparent multi-bounce refraction
and fully ray-traced mirrors are separate later work.

Exit condition: reflections improve selected materials without changing opaque
course coverage, car paint selection, fog or UI.

## Stage 6: optional native hardware RT

Only after the compute version is correct and profiled, define a backend-neutral
acceleration API with build/refit/trace/capability operations. Evaluate native
Vulkan ray queries/pipelines, Metal acceleration structures and DirectX Raytracing
behind that interface. Keep the compute BVH as the reference and compatibility
backend.

This stage is optional. It should be justified by measured performance or image
quality, not by the presence of RT hardware.

## Tests and release gates

Unit tests:

- AABB and triangle intersection;
- deterministic BVH build and traversal;
- transform and normal conversion;
- material flag validation;
- TLAS refit/rebuild and asset-generation invalidation;
- history reset decisions.

GPU contract tests:

- CPU/GPU hit agreement for small fixed scenes;
- shadow visibility for opaque, masked and excluded transparent geometry;
- bounded buffers and graceful allocation failure;
- identical shader inputs across SPIR-V and MSL builds;
- no stale history after camera cuts or renderer changes.

End-to-end captures:

- all courses in both directions;
- tunnels, overpasses and long-distance scenery;
- start grid and Reiko;
- rival cars, custom-race rival models and replay cameras;
- rear-view mirror;
- lost-race and other 2D result screens;
- classic/modern toggles with ray tracing enabled.

Performance gates track median and worst-frame GPU time, TLAS build time, memory,
history resets and rays traced. A visual feature does not ship if it reintroduces
the end-of-course slowdown or texture/geometry lifetime bugs fixed in earlier
releases.

## Delivery slices and estimate

For one engineer familiar with this renderer:

1. Baseline and scene contract: 1 week.
2. CPU BVH, BLAS/TLAS lifecycle and tests: 2–3 weeks.
3. Compute shadow prototype: 2–3 weeks.
4. Temporal filter, transitions and cross-platform stabilization: 2–3 weeks.
5. UI/configuration, performance gates and release polish: 1–2 weeks.

A credible portable ray-shadow feature is roughly 8–12 weeks. A convincing
reflection path adds about 4–8 weeks. Native hardware backends would add several
more months and substantially increase cross-platform maintenance.

The first implementation milestone should stop after Stage 2 and answer one
question with measurements: can one half-resolution sun-visibility ray per
pixel fit the target hardware without destabilizing the modern renderer? If not,
we retain the scene/BVH work for offline captures and do not grow the runtime
architecture around an unsuitable effect.

## Prototype status (2026-09-19)

The branch now has the complete Stage 1 data path and an intentionally narrow
Stage 2 experiment:

- deterministic CPU BLAS and TLAS construction and traversal;
- direct import from resident RMESH assets, cached by asset generation;
- deduplicated, std430-compatible GPU BLAS/TLAS/instance buffers;
- nested TLAS-to-BLAS traversal in the shared GLSL source compiled to SPIR-V
  and MSL;
- directional shadow rays from opaque raster fragments;
- `modern.ray_tracing = off|shadows`, defaulting to `off`.

The first implementation flattened visible raster draws and rebuilt their BVH
each frame, costing roughly 4–7 ms on the tested Mac. The instanced path traces
the complete main-pass `RenderWorld`, including off-camera shadow casters. In a
1600-frame PAL Grand Prix smoke with 7,000–8,500 unique mesh triangles, TLAS
construction plus packing measured 0.355 ms median, 0.453 ms p95 and 0.375 ms
mean. The run reached scene 12/timer 101 with a valid Metal capture. Strict C11
compilation also passes for macOS and Zig cross-targets for x86-64 Linux and
Windows.

The GPU cache now retains an unchanged packed BLAS set and uploads only TLAS
nodes, instance indices and transforms on ordinary presentations. A 1050-logic-
frame smoke produced 1487 presentations; 1349 reused static geometry. Dynamic
uploads reached as little as 8 KiB, while a full scene upload reached 652 KiB.

This is not a release candidate. It traces at raster resolution in the fragment
shader. Alpha semantics,
half-resolution compute output, temporal filtering, mirror history, runtime
Vulkan/D3D12 validation and representative GPU timing remain open. The next
performance step is a half-resolution visibility target with GPU timing.
