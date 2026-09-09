# Feedback after 0.6.4-alpha

Work is on `fix/post-064-feedback`. The published 0.6.4-alpha archives,
including their `docs/` directories, remain unchanged.

## Classic stuttering: interpolation coverage fixed locally

The enhanced classic renderer could submit intermediate frames while most course
polygons remained at the logic cadence. `KeepConnectedSurfacesCoherent` propagated
a hold through a connected projected surface whenever a packet had no safe match.
This prevented the road holes fixed before 0.6.4, but made interpolation coverage
collapse as subdivision and visibility changed during driving.

The local fix matches an emitted child through its stable captured parent face
when a changed subdivision has no stable allocation offset. It retains the
parent's exact emitted SXY corners in both frames and moves old children through
the parent triangles. A component is held only when neither an exact child nor a
valid parent mapping exists. This preserves the previous GP0 packet stream and
its material, does not mix geometry from two layouts, and keeps shared parent
edges coherent.

A macOS arm64 Metal run used the NTSC-U image, Overpass City extra-GP/reverse,
class 4, car 9, autopilot speed 4000, 1920x1080 and a 120 FPS presentation target.
It stopped at race timer 450. The first visible-window run became occluded at
frame 265, so a second run explicitly rendered offscreen for complete geometry
measurements. This is not a monitor/swapchain smoothness measurement.

The offscreen trace recorded 449 race logic frames. Before the fix, across the
moving section (capture frames 400–668), only 41,428 of 270,187 course/terrain
packet occurrences were eligible for interpolation: **15.33%**. With parent-face
motion, 214,713 of 270,187 are eligible: **79.47%**. The direct-child matcher still
finds 149,616 candidates in that section; parent matching covers changed layouts.
The complete race trace also had render intervals of 8.356 ms median,
15.244 ms p95 and 62.333 ms maximum. CPU/GPU submission timings and actual display
cadence still require separate investigation; these measurements do not assign
every hitch to the matching algorithm.

Evidence: `build/post-064-feedback/stutter/{runtime,offscreen}.log`.
New `classic-motion` diagnostics report polygon, exact-child candidate,
projectable parent, accepted and course counts once per captured frame when
`diagnostics.classic_trace=true`. C regression coverage verifies the counters,
the parent-motion fallback, coherence propagation and reset.

The existing high-FPS production check passed after the change and recorded 961
distinct interpolated presentations. It does not measure actual monitor cadence;
the separate 62.333 ms maximum offscreen render interval remains a performance
investigation. Removing the coherence guard alone would reintroduce holes and is
not an acceptable fix.

## Quick direction reversal in car select: fixed locally

After the first car swap, reversing direction rebased the turntable immediately
next to the opposite swap threshold. The next frame completed that swap, and a
short held direction could request another car immediately afterwards. Both car
select and the shop use the same helper.

The helper now adds/subtracts a full revolution where necessary, preserving the
visible orientation while giving the reversed animation its full travel.
`menu_turntable` reproduces left/right and right/left six-frame presses with
immediately available assets. It failed before the fix and passes afterward.

The car select/shop exhaustive sweep hashes were updated only after a comparison
of old and new implementations normalized the animation angle modulo 600000.
All other recorded state/calls retained identical hashes: 1246984149 for select
and 3565153949 for shop, across 1,433,600 and 5,898,261 states respectively.

## Replay sky at hard camera cuts: fixed locally

The overlay and classic presentation paths replay the previous capture. They
previously used `current->previousCamera` to interpret its sky packets. A hard
camera cut intentionally resets that history to the new camera, so the old sky
packets were projected with the new shot's grid for one presentation.

Both paths now use the actual previous published world as the packet source and
the current/interpolated world only as the target presentation camera. When the
sky asset or cloud row changes, the source is deliberately unavailable rather
than borrowing a mismatched camera. The compiled source contract covers this
cut-specific ownership rule.

## Preparation and configuration boundaries

Mesh import from a live disc no longer runs in `ModernNativeGpuPrepare`, which
also runs for every high-FPS presentation. It is now performed once after a
completed logic frame has published its immutable render world. GPU preparation
uses resident meshes only; a cache miss cannot make repeated VSync frames decode
and build the same source mesh.

Visible material images now follow the same lifecycle. `ModernAssetsPrepareWorld`
scans the published mesh ranges, resolves the exact texture variant and car-paint
colours, and retains a bounded 32 MiB CPU cache. The first draw copies an already
prepared image instead of reopening a disc/cache file or decoding a mod PNG.
Mip construction and GPU transfer remain in draw submission because SDL requires
the active command buffer for those operations. The native fixture logged its
mod override before GPU pipeline creation; its later draw-side material load was
0.002 ms.

The native no-config defaults now match the released first-run preset: modern,
4x internal scale, 16:9, VSync, linear filtering, FXAA and vibrant grading.
The launcher test compares every launcher-visible setting against both first-run
INI templates, so a future default change must update all supported entry
points together.

The high-FPS presentation clock is now an isolated native component. It owns
the completed-logic-frame timestamps, interpolation fraction and presentation
deadline, while `modern_renderer.c` supplies only SDL time and display interval.
Its unit regression covers duplicate observation of a frame, first-frame
fallback, interval clamping, deadline advancement and suspend-sized gaps. This
keeps timing changes independent from GPU resource and overlay changes.

Prepared material ownership is likewise isolated in `modern_prepared_materials`.
The component owns the immutable CPU image copies, 128-entry/32 MiB budget,
path-copy lifetime and fallback when a private copy cannot be made. Asset import
now only builds a material transaction and asks that component to reuse or
retain it; GPU upload remains in the native GPU module. The game, both headless
render tools and the isolated asset retry contract build against the split.

Prepared mesh lookup is now isolated in `modern_prepared_meshes`. It snapshots
all main-pass resident meshes once for the owned render-world instance array,
then supplies the same immutable lookup to main and mirror geometry builds.
The cache validates that an instance belongs to its world before indexing and
owns growth/release itself, removing the native GPU module's unbounded raw
pointer/capacity pair. Its regression covers both passes, world replacement,
growth, out-of-world lookup rejection and release.

Texture cache identity is isolated in `modern_texture_index`. It owns the
fixed-size open-addressed mapping for the full semantic texture key: asset
bank/key, material, variant and both car-paint fields. GPU texture creation
and upload still remain in the native GPU module. The regression covers
duplicate insertion, distinct variant/paint identities, a deliberate hash-slot
collision, capacity rejection and clear, so a future mod key change cannot
silently bind an otherwise valid but wrong texture.

Sky-packet reprojection is isolated in `modern_sky_reprojection`. The overlay
builder chooses the published source camera and presentation camera; the module
then transforms points and reconstructs smooth cloud quads without retaining
either camera. Its unit coverage includes a changed grid, a degenerate source
grid and the 31-to-0 tile-column wrap that previously caused a full-tile jump.

GP0 environment-state interpretation is isolated in `modern_overlay_state`.
It owns E1–E5 state, VRAM-page clipping and conversion of an overlay scissor to
the presentation target. The regression retains the zero-height mirror slide
case `(86,240)-(233,239)`, so it remains empty rather than becoming a full-page
flash after future overlay refactors.

The transient overlay geometry store is now isolated in
`modern_overlay_batches`. It owns vertex/span allocation, contiguous-batch
coalescing, pass/layer boundaries and capacity failure without writing past its
fixed GPU upload budget. `modern_renderer.c` now only translates captured GP0
packets and submits those batches. The unit test covers coalescing, quad and
triangle layout, pass/layer split, capacity rejection and release/reset state.

## Mod and tooling contract review

Mod manifests already have the appropriate stable boundary: schema version 1,
semantic lowercase IDs, explicit requirements, deterministic dependency order,
and semantic texture/material/mesh keys. Existing compiled manifest and package
tests cover version rejection, identity collisions, missing requirements and
cycles. No new parallel identifier scheme was added.

There is no Python component in the shipped game, asset-import path, launcher,
or release package. The only Python test dependency left in the active CTest
suite belongs to the standalone Asset Browser developer tool. Replacing it
requires a compiled replacement with equivalent coverage before removal under
the project migration rule. The largest remaining maintainability targets are
`modern_renderer.c` and `modern_native_gpu.c`; their next split should isolate
material preparation/upload from GPU draw submission.

The macOS build-launcher guard is now a CMake test rather than a Python runner.
It retains its checks that the familiar build path is a symlink to the current
bundle executable and that the target is runnable. This removes one active
release-platform Python requirement without changing the shipped application.
The RETIRE chase-camera smoke scenario has likewise moved to CMake with the
same environment, timeout and log assertions; it passed in 6.99 seconds here.
The custom player/rival track-point start smoke scenario is also CMake now,
preserving its five state/log assertions and passing in 1.58 seconds.
Time Attack through results and record-name entry is now covered by the same
scenario/input script in CMake rather than Python; it passed in 14.41 seconds.
The manual-only direct-boot car scenario has moved too, retaining its checks
for forced transmission setup and an actually moving race car; it passed in
5.03 seconds.
The mirrored-course regression is now a CMake runner as well. It creates an
isolated asset-link fixture, drives normal and mirrored courses with the same
left input, and verifies steering sign plus main-view faces; it passed in
26.22 seconds here.

`ppm_capture_check` is a compiled image assertion helper shared by the boot
copyright frame, classic Lakeside waterfall and Age Pegase cabin regressions.
Those three runners now use CMake and the same exact dimensions, regions and
pixel thresholds as their Python predecessors. They passed in 0.39, 4.25 and
7.02 seconds respectively.
The same helper now covers Trophy View's OPTION texture-page assertion and the
two-frame title-artwork/one-pixel logo UV check. Their CMake runners passed in
2.38 and 1.43 seconds.
The international and Japanese prologue captures also use the checker now,
while their runners retain raw-input navigation, primitive-buffer rejection and
the Japanese text-option assertion. They passed in 10.17 and 9.13 seconds.
The classic race sky digest now runs through the compiled checker too. Its
baseline changed from `aa27f132` to `b51dbd37` specifically because the
hard-camera-cut fix now uses the previous published sky grid; the 23-frame
capture passed in 8.79 seconds.
Modern baseline/enhanced feature frames are now a CMake runner as well. It
uses a deterministic logic-rate presentation plus fixed random/input state,
checks the PPM dimensions/non-empty content in compiled code, and retains the
full SHA-256 references in CMake. The old high-FPS capture timing produced
different frames between identical runs; the revised runner passed twice in
11.54 and 11.64 seconds.
Scene-capture determinism is now checked by a compiled trace parser. It
compares both full traces, validates every row, rejects every overflow, and
keeps the thresholds for race models, terrain cells, skipped 3D packets and
faces. The two-run CMake scenario passed in 19.13 seconds.
The Grand Prix intro terrain regression is also CMake/compiled now. It retains
the capture metrics for car, road and HUD plus the log checks for the retail
record, time text and LOD shift; it passed in 9.85 seconds.
The engine-audio smoke runner is now CMake too, retaining its pitch-update
floor and nonzero input/output/tail reverb checks; it passed in 11.33 seconds.
The broader VAB audio-output runner is CMake now too. It retains mixer/SEQ
metrics, all four stereo voice snapshots and the retail SPU RAM body check;
it passed in 4.22 seconds.
The PAL/NTSC menu-music-tempo regression now also runs without Python. Its
CMake runner performs both frame-identical routes and verifies equivalent
sequence-event counts at the respective game rates; it passed before the
follow-up tempo correction below.
The Grand Prix start regression is CMake/compiled as well. It keeps the
classic 4:3 framebuffer on purpose, so its pixel checks continue to cover the
retail HUD, road, mirror, sky and Overpass geometry while the separate layout
tests cover widescreen placement. Its LOD, sequence-shutdown and engine-audio
checks also remain; it passed in 12.77 seconds.
The opening-FMV frame runner no longer needs Python either. Its CMake form
uses the common compiled PCM and PPM checkers for the centered 320x192 picture,
audio-frame count and corruption check. It correctly reports `SKIP` on this
host because no owned disc image is configured; a full pass still requires one.
The complete frontend/OPTION menu sweep is a CMake runner now. It preserves
the exact set of twelve frontend screens and eleven OPTION modes, the 3D-face
assertion for controller options, and the primitive/sanitizer failure checks.
The eight-course Grand Prix/Extra Grand Prix matrix is CMake too. It writes
each isolated scenario, checks the selected series/course state and compares
the classic PPM capture with the existing SHA-256 golden reference.
Race CD-DA switching and Grand Prix results are now CMake runners as well.
They retain their disc-aware skip behavior and assert the prologue/race/replay
track transitions, mixed audio and result-screen progression.
FMV pacing is also fully native now: the sector/XA oracle gained a strict
picture-duration-versus-XA-duration mode for the two representative movies,
while the all-stream runner keeps complete decode and PCM coverage. The former
shared Python disc helper was removed after every consumer moved to CMake.
The memory-card roundtrip is likewise CMake now, retaining platform-specific
card locations, existing/empty LOAD GAME cases and a generated complete save.
Window lifecycle control moved to a strict C test on Linux hosts that provide
`xdotool`; it still performs three resizes and two fullscreen toggles.

The only Python left under `tests/` belongs to the Asset Browser developer
tool's own GLTF/sky fixtures. It cannot be deleted until that tool has a
compiled replacement; the game runtime, importer, release checks and all game
integration tests no longer depend on Python.

The optional host-code `lint` target no longer needs Python either. Its CMake
script reads `compile_commands.json`, selects the `src/port` and `src/render`
C sources, and invokes the existing `clang-tidy` and `cppcheck` checks. An
isolated macOS configure with lint enabled completed successfully.

The obsolete Python scenario wrapper was replaced with the compiled
`rage-scenario` tool. It validates the same race, grid and starting-position
arguments, verifies its selected game binary and forwards only `--scenario`
and `--set` to the game, so runtime configuration remains the sole scenario
parser. CTest covers a full dry run and malformed-grid rejection.

The old Python disc staging helper is now the compiled `rage-stage-discs`
tool. It resolves the data track with the native CUE parser, reads the ISO
directly from 2352-byte sectors without `7z`, stages the cue and every declared
track, validates the known regional PS-X EXE SHA-1/header, and writes the same
`assets/<region>/main.exe` and `SYSTEM.CNF` outputs. Its compiled validation
test covers SHA-1, PS-X EXE rejection and digest mismatch; the CLI usage gate
is also registered in CTest.

Three small Ruby renderer source contracts now run inside the compiled
`source_contract_tests` binary: native pointer rejection for the former PS1
frame offset, typed ownership of display/mirror/tachometer frame state, and
the required `DrawSync(0)` before all smoke-capture readback paths. This keeps
the checks active on hosts without Ruby and leaves the larger optional visual
comparison tools for a separate compiled migration.

The Grand Prix and Time Attack progress-layout contract also moved from Ruby
into that binary. It checks the typed save-state declarations, field layout and
all restore/write paths for the selected series and player money, so a future
save-format refactor fails consistently on every supported build host.

The audio-state layout contract is compiled as well. It retains the checks for
the shared `CdlGetlocP` response, typed race-car and sector-record tables,
camera-path aliasing, pause snapshots and the direct music-channel reset. None
of those source-level regression checks now depends on Ruby.

The menu UI-script contract is compiled too. It inventories every
`RunTimedDrawScript` use and requires a native command array, native alias or
pointer declaration for each script, preventing serialized PS1 command layouts
from being passed to native pointer-sized code.

Depth-probe clipping and screen-space triangle intersection now live in the
standalone `modern_depth_probe` module instead of the GPU renderer. Its unit
test covers a near-plane crossing, complete rejection, a central screen hit and
a miss; all game and render-tool targets build it from the same source.

Material-to-shader uniform conversion is likewise isolated in
`modern_material_uniform`. Its test pins lit, unlit and automatic shading,
surface parameters, alpha mode and the clearcoat flag before draw submission.

The Grand Prix exit smoke route no longer uses Ruby. Its CMake runner preserves
the native save-prompt script assertions, scripted input, 70-second game
timeout, scene-exit check and sanitizer/runtime-error rejection. The route
completed successfully in 39.81 seconds on the host smoke build.

The mirror-entry smoke route is also native now. A compiled manifest/PPM
checker accepts both historical and extended capture manifests, requires the
seven consecutive zero-height-clip timers and rejects a blue-flooded main
scene. Its malformed-manifest cleanup path also releases the manifest handle
once only. The complete route passed in 41.18 seconds.

Mod texture-patch announcement state now belongs to the same resettable asset
session as archive override announcements. Ending a session clears both, so a
subsequent selected mod receives accurate diagnostics instead of inheriting
suppressed messages from the prior mod.

The merged launcher/save-editor package was also run with its native save-format
gate and Node suite: 82 tests passed and three disc-image integration tests were
skipped because no owned PAL, NTSC-U or NTSC-J image was supplied. The suite
exercises save creation/editing, mod import/export/composition, cancellation,
profile rollback and launch error handling through the single launcher project.

After building the complete CMake tree, the host unit suite passed 249/249 in
4.20 seconds on the already-built host tree. Running CTest after only selected application targets is not a
valid substitute: CTest still registers every unit executable and correctly
reports unbuilt targets as `Not Run`.
`cmake --build build --target check-unit` now performs that complete build and
then runs the labelled unit suite as one reproducible gate; it passed 249/249
after the final source-contract migration.

## Remaining feedback

- PAL course/car select sequence tempo: fixed. The 60 Hz compensation added in
  `929db08c` emitted 60 sequencer ticks in 50 PAL game frames, speeding only
  the SEQ events by 20% while preserving sample pitch. Menu SEQ now advances
  once per game frame under both standards. The PAL/NTSC regression and VAB
  output check record 124 notes for the scripted PAL route.
- Widescreen FMV zoom: recorded as a future option; not implemented.
- Documentation in the ZIP: retained at the user's request.
