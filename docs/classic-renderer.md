The classic renderer supports higher internal resolution, a wider field of
view, and interpolated presentation. Select it with F10 or `[video] renderer =
classic`. Modern remains the shipped default and retains its native asset
import requirements.

For 1080p, 16:9 and a 120 FPS presentation target:

```ini
[video]
renderer = classic
classic_enhancements = true
internal_scale = 4.5
aspect = 16:9
fps = 120
texture_filter = nearest
post = none
grading = off

[hud]
anchor = edges
```

`internal_scale` accepts 0.5–16. At 16:9, 3 produces 1280×720, 4.5 produces
1920×1080, and 9 produces 3840×2160. Resolution is independent of window size.
`aspect = 4:3` (also the current `auto` behavior) retains the original field of
view. `16:9` adds horizontal coverage while retaining the vertical projection;
it does not stretch the 4:3 framebuffer. The mirror retains its original clip
rectangle. HUD elements can remain centered with `hud.anchor = center`.

`fps = logic` presents the original game cadence. A number selects a presentation
target; `vsync` follows the monitor. PAL gameplay remains 25 logic ticks per
second and NTSC remains 30, regardless of the presentation target. Input and
physics are not advanced by intermediate presentations.

Classic uses the actual emitted GP0 polygons, including the original road
subdivisions, affine texture coordinates, CLUTs, texture windows, translucent
polygons, and ordering-table order. It does not render native replacement
meshes or use a depth buffer to reorder the world. The completed VRAM texture
snapshot is retained across intermediate frames. Higher resolution rasterizes
these polygons at the selected target size; source textures retain their
original resolution.

Interpolation matches source faces and emitted subdivision children between
completed logic frames. Camera cuts, ambiguous repeated instances, changed
subdivision layouts/materials, and large projection jumps are deliberately
held at the logic cadence. Sky motion is interpolated separately; HUD counters
and texture animation remain tied to game logic. This is conservative screen
interpolation, not a new high-rate simulation or a promise that every polygon
moves smoothly. Original visibility distance and PS1 geometric precision still
apply. Menus, FMV and interlaced screens retain the compatibility presentation.

For the original framebuffer path, set `classic_enhancements = false`. This
also disables classic widescreen and interpolation. Enhanced rendering is not
pixel-identical to PS1 rasterization (for example, output color precision and
raster coverage differ).

Regression coverage is implemented in C and CMake:

- `classic_motion`: packet correspondence, reordered OTs, subdivision changes,
  ambiguous instances, camera movement/cuts, source immutability and mirror sky
  classification.
- `modern_frame_pacer`: fixed PAL/NTSC logic cadence at multiple display targets.
- HUD placement tests: widescreen anchoring and the unenhanced fallback layout.
- `classic_presentation_standard`, `classic_presentation_wide`, and
  `classic_presentation_high_fps`: production executable, target dimensions,
  capture overflow checks and distinct vertex hashes within a logic tick.
  Set `RAGE_PORT_DISC_CUE` to a legally obtained disc image to run these tests.
  They render offscreen and do not measure swapchain/display performance.

`diagnostics.classic_trace = true` records interpolation fractions, matched
packet counts and projected-vertex hashes. Existing `diagnostics.modern_dump`
and `capture.path` controls also capture enhanced classic output; their names
are retained for compatibility. Marker history remains a modern-only diagnostic.

Validation on 2026-09-08: Release builds and the three production presentation
checks passed on macOS arm64 (Metal), Linux x86-64 on Darwin (Vulkan), and
Darwin's Windows 11 VM (ClangCL, SwiftShader software Vulkan). Five motion,
pacing and HUD tests passed on all three platforms. macOS ASan/UBSan also passed
those five tests and the 1080p interpolation run, which recorded 1,264 distinct
presentations between logic ticks. The software Windows run verifies rendering
and interpolation, not accelerated display performance.

A focused macOS reverse Overpass lap at 1080p with a 120 FPS target averaged
119.78 rendered FPS, with a 10.25 ms p95 interval and an 82.00 ms maximum interval
near point 454. The shared presentation hitch remains unresolved. An additional
background run still encountered long presentation waits before SDL reported
the window as minimized; after minimization, game logic continued to the
requested stop point. These enhancements do not establish that the earlier
framerate/stability issue is solved.

Offscreen captures now explicitly retire their GPU submission fence. SDL's
Vulkan backend otherwise retains completed buffers when a window is claimed
but swapchain submissions are skipped; the original Linux runs exhausted GPU
memory. The corrected path completed all three repeated presentation checks.
