# 0.6.4-alpha release audit

Audit baseline: `03719b1e084ade4d3fca128752816ef8240924c2`, clean main game,
with optional authored cars disabled as in the release workflows. No launcher,
no re-asset modifications. No publication is authorized by this audit.

This is an open verification ledger, not a release approval. Absolute claims
of no bugs or best-ever performance require evidence; green unit tests alone
cannot establish them.

## Release cleanup update (2026-09-08)

Manual acceptance found two release-blocking rendering regressions: wrong car
bodies after interrupting attract mode and intermediate-frame road holes in
enhanced classic. [Causes, fixes and regression evidence](render-regressions-2026-09-08.md)
are recorded separately. The original test archives remain baseline artifacts;
they are not approved for publication.

The subsequent [darwine validation report](validation-darwine-2026-09-08.md)
records Linux/Windows regional renderer checks, an additional offscreen GPU
queue fix, and remaining Windows driver limitations. The full Windows test
build is subsequently repaired; see [Windows test suite repair](windows-tests-2026-09-08.md).

The current standalone release preparation is summarized in
[release notes](release-0.6.4-alpha.md) and the complete
[open-issue audit](release-0.6.4-issues.md). The prior foreground profiling block
below is historical; its measured stalls remain unresolved, and no new result
is presented as a solution to the original pop-in/performance goal.

This cleanup found and fixed the duplicate final-stretch cue (#22), the missing
HUD preview presentation stub, GCC fixture lifetime/interposition failures, and
ClangCL warning-option handling for record defaults. Enhanced classic matching
lives in the shared port presentation layer; the existing architecture boundary
check remains unchanged and passes. The CMake finish/menu transition test passed
before its old Python implementation was removed.

Current evidence is under `build/release-064-candidate/`. The macOS full build,
413 portable checks, targeted sanitizers, live audio/texture/finish/startup tests
and three-lap modern Overpass route pass. Darwin Linux passes 411 portable checks
(one additional prebuilt-cache check skipped), its route and BIN startup pass,
and an Ubuntu 24.04 container rebuild passes nine release checks. Windows passes
the game build and nine release checks; modern screenshot comparison passes on
software Vulkan. See the release notes for the final route/package results.

Release workflows now package the standalone game only, include documentation,
and run the newly relevant regressions. No public tag or release has been made.
Physical display pacing, accelerated Windows crash confirmation, download
acceptance and outstanding enhancement requests remain explicit acceptance gaps.

## Historical foreground profiling checkpoint

- Foreground profiling is blocked on an uninterrupted single-game window.
  Three consecutive goal turns encountered the same condition: a second
  build/release game and/or loss of foreground during the required route
  interval. `foreground-end-sampled` lost focus/was occluded at frame 1038,
  before the targeted end section. Another build/release process appeared.
  The sampling watcher eventually started at frame 2971, so `end-sample.txt`
  does NOT cover the intended first-lap 1720..2140 interval; reject it for
  attribution of the prior point-433 foreground stall. The test game was
  stopped; both driver and sampler handles are terminal. The user's other
  game was left running. On resume, the sampling watcher must reject late
  starts instead of accepting any frame above its threshold.
  Remaining requirements: identify/fix the measured foreground presentation
  stall, then verify complete Overpass forward/reverse runs for remaining
  pop-ins and absence of unwanted geometry. No arbitrary renderer changes
  are justified as a substitute for the missing foreground evidence.
- Foreground slowdown is confirmed after both landmark fixes. In
  `single-window-shuttle-performance`, first-lap end frames 1801..2101 have
  1,206/1,206 samples with SDL input focus. Submission intervals average
  117.37 FPS, p95 10.208 ms, but include one 226.045-ms gap at frame 1967,
  point 433. The corresponding `presentation-stall frame=1968` is 225.918 ms
  with flags 0x20002620 before/after. This is not explained solely by an
  occluded/inactive window. Need a sample covering this precise foreground
  interval; prior nextDrawable stack samples were taken while inactive.
  The second lap lost focus/was occluded and is not a complete rendering
  benchmark (logic completed, presentation stops at frame 2738). Game exited
  0 with clean teardown. Driver initially rejected 446266 versus exact 446400
  units; its 4000u/s NTSC route validation now allows one 134-unit logic step,
  while still requiring explicit two-lap completion. No complete-goal claim.
- Moving shuttle native publication now also precedes the classic origin-cell
  visibility gate, with hidden classic GTE/environment state preserved.
  Release and ASan/UBSan shuttle/static scenery tests pass. `native-shuttle-
  {extra-gp,grand-prix}` both completed 93 captures and clean teardown. Each
  direction has 92/93 byte-identical images versus the static-landmark fix;
  the remaining frames (reverse 842, forward 1070) have small differences,
  inspected in paired images without an obvious new artifact. This is a
  bounded producer fix, not proof that every route pop-in is eliminated.
- After the landmark publication fix, `landmark-end-depth32768` completed
  a reverse tour with 86 consecutive end-section captures and clean teardown.
  Seven shared frame/camera comparisons against the 16k candidate have
  normalized RMSE 0.00012–0.0081; the major whole-landmark disappearance is
  resolved by publication, not by increasing the global far plane. The global
  32k range remains diagnostic only because earlier forward runs exposed
  unwanted terrain. Current Release build includes per-frame window flags;
  no foreground FPS claim is supported yet.
- `landmark-performance-reverse` was stopped before route completion because
  another game instance was running concurrently; it is not a valid FPS
  comparison. Repeated ~1-second stalls occurred during countdown at point
  465, with SDL flags 0x20002020 (neither focused nor marked occluded).
  A 3-second sample captured 2457/2511 main-thread samples in nextDrawable.
  The compiled read-only macOS window inspector reports both game windows
  on-screen, neither application active/hidden, and identical window bounds.
  SDL's occluded flag alone does not cover this measured situation. User
  scheduling for a single foreground-game benchmark is pending. Future
  benchmark-frame records include window_flags for focus/occlusion auditing.
- Synchronized reverse captures (`logic-offscreen-depth-{16384,32768}-reverse`)
  completed, 93 images each, clean teardown. All 93 prepared GPU cameras match
  except far plane. End-straight pairs barely differ (normalized RMSE below
  0.0012), yet the finish landmark appears abruptly between frames 1478/1490.
  `logic-tower-probe` identifies the left tower at pixel 775,380 in frame 1490
  as COURSE mesh 57, entity 0x30000, not terrain. Its native instance is absent
  at 1478. The producer `DrawStaticScenery` previously gated native publication
  on the classic TrackCellVisible origin-cell test. A candidate now publishes
  the landmark independently and keeps that gate only for classic submission.
  Hidden classic draws preserve GTE/environment state. Release and focused
  static_scenery/native_visibility tests pass. Forward/reverse visual runs
  in `native-landmark-{extra-gp,grand-prix}` completed with 93 matched captures
  each and clean teardown. Reverse frame 1478 now includes both towers and
  the finish hall; the synchronized baseline lacks the whole landmark.
  Forward frame 578 still has no confirmed floating-terrain fragment; the
  full forward contact survey found no obvious new artifact at survey scale.
  The unchanged 16k clipping limit and other visibility gates still need
  evaluation; these runs do not prove all pop-ins or slowdown fixed.
- Added actual `nativeCamera` to capture metadata and an explicit dump-only
  `diagnostics.modern_dump_offscreen` mode. A 3-second sample of the stopped
  windowed logic diagnostic captured 2400/2575 main-thread samples in
  CAMetalLayer nextDrawable -> semaphore_timedwait. Unlike earlier samples,
  this directly captures swapchain blocking, but is not a foreground race
  profile. The offscreen comparisons use video.fps=logic and make no FPS claim.
- Native vertex staging now grows to the required upload size rather than
  cycling a fixed 112,000,000-byte transfer buffer. GPU geometry capacity and
  vertex/index contents remain unchanged. `transfer-sized-reverse` completed
  one full tour and clean teardown: 4,709 uploads, median 15,096 bytes, maximum
  271,396 bytes; staging grew once to 524,288 bytes. Release build and focused
  native_visibility/render_geometry_pack tests pass. This is not a validated
  FPS improvement: the run included occlusion and a second game process.
  Window transitions 0 -> 1 -> 0 were exercised; presentation resumed, but
  the transition into occlusion itself still had a 1,006-ms presentation call.
  Visible presentation stalls up to 73 ms also remain. The separate
  `transfer-sized-visual-reverse` tour completed with 93 matched captures and
  clean teardown. Comparisons with the older default reverse captures are
  not pixel-identical: frame 578 is close (normalized RMSE 0.0045), while
  frame 1322 differs in visible camera position despite matching logic-camera
  metadata (RMSE 0.1685). Interpolated render frames are not synchronized by
  the current dump harness; these images do not establish pixel equivalence.
  A synchronized A/B capture is still required.
- Rejected the optional straight-region union experiment after the user saw
  bad geometry in `overpass-straight-reverse`. Forward and reverse diagnostic
  tours completed (93 matched captures each), but completion is not visual
  acceptance. Removed the experiment and its dedicated API/tests from live
  source; archived the four source snapshots in
  `build/release-audit-064/rejected-straight-visibility`. Exact offending frame
  remains unconfirmed. Pop-in and foreground slowdown are still unresolved.
- Static course backface-culling experiment with region masks disabled and
  far=32768 completed a forward tour (`overpass-static-facing-forward`), but
  frame 578 shows an even larger floating terrain section. Rejected; the
  diagnostic switch was removed from source. This does not justify disabling
  region masks. The earlier offending marker-12 triangle belongs to terrain
  mesh 63/cell 13,18, material 69, flags=69 (not terrain-near-only).
- Window-flag profiling completed (`presentation-window-flags`): its >20-ms
  presentation calls have flags 0x20002024, including SDL_WINDOW_OCCLUDED.
  A candidate now skips swapchain presentation for occluded/minimized windows,
  preserving simulation/input and allowing explicit screenshot diagnostics to
  render offscreen. `occluded-present-check` completed a visible reverse tour,
  exit 0; initialization still has 101/64/32-ms calls with input-focus flags
  0x20002220. Occlude/uncover transition verification remains open, and this
  candidate is not proof that the reported visible race slowdown is solved.
- Focused pop-in/slowdown goal: `reverse-presentation-profile` completed a
  full reverse tour, exit 0. Long pauses are measured inside
  `Psyz_VideoPresentIntermediate`, including 782 ms at frame 334 and roughly
  999/999/1004 ms at 1590/1591/1593 (point 309), not just the finish straight.
  The same finish-section 799-ms pause did not repeat in this run; there were
  roughly 20-ms present calls at frames 1988/1990. Two macOS sample artifacts
  were collected (`end-sample.txt`, `stall-sample.txt`); sampling began after
  the one-second pauses, so it does not identify their blocking stack.
  The end sample reports 7.5 GiB physical footprint, peak 7.8 GiB, and mainly
  frame-pacer sleep; the machine has 64 GiB RAM. High footprint warrants
  investigation but is not proof of memory pressure causing the pauses.
  SDL Metal calls `nextDrawable` even from nonblocking swapchain acquisition.
  Added before/after SDL window flags to future stall logs to distinguish
  occlusion from stalls in an unobscured window. Build passes; fix still open.
- User also observed pop-in and slowdown in `reverse-performance-clean`,
  which completed 223200 units, exit 0, without dumps/history/capture.
  Its final 300 logic frames (1802..2102) contain a 798.812-ms submission gap
  at frame 2046, point 454; also 34.128 ms at 2043 and 44.279 ms at 2047.
  Median=8.332 ms, p95=10.937 ms; average=110.48 submissions/s, so the average
  hides a severe outlier. The affected 120-presentation window reports
  render_fps=65.01 but build=0.009/submit=0.869/prepare=0.375 ms averages,
  suggesting the dominant pause is outside those measured renderer sections.
  Foreground sampling covered much, but not all, of the race; do not attribute
  the stall to either geometry or window occlusion without more evidence.
  Timing now optionally logs intermediate-present calls exceeding 20 ms to
  locate that gap. This is instrumentation, not a performance fix.
- The user saw drastic end-of-track FPS loss during `reverse-popin-trace`.
  That diagnostic starts synchronous full-resolution GPU readback and PPM
  output every three logic frames at scene timer 1400. It must not be used
  as performance evidence. Its process exited 0 without a completed-tour
  record. A separate `reverse-performance-clean` run disables dumps, marker
  history and capture, retaining only timing. Result pending validation.
- At experimental far=32768 the user completed another Ghepardo/class-4
  reverse tour with no observed bad geometry, but reported conspicuous pop-in
  near the end (`user-overpass-reverse-32768`, 223200 units, exit 0).
  A class-1 forward capture tour at the same depth completed cleanly, but
  frame 578 again contains small suspended pieces above the old-town roof.
  Thus increasing the global cutoff alone is not an accepted fix.
  `diagnostics.native_visibility_trace` now optionally logs logic frames,
  camera position and cell-mask additions/removals to correlate pop-in with
  region changes. Normal visibility behavior is unchanged by this diagnostic.
- User-observed Ghepardo/class-4 Overpass tours completed in both directions,
  each 223200 route units, exit 0 (`user-overpass-markers` and
  `user-overpass-reverse-markers`). User did not observe the earlier distant
  fragments, but reported one fleeting black sky square in forward and an
  objectionably short draw distance with many pop-ins. Visibility acceptance
  therefore remains open: the 16384 cutoff is not an accepted final solution.
  Forward's manual marker plus 16 history frames were preserved in
  `user-overpass-markers/captures`; no claim that they caught the sky square.
- Division audit found the GPU camera/probe paths computed reciprocal FOV
  without the CPU projection's input checks. Snapshot floats are read verbatim,
  so replay can supply zero/nonfinite FOV. `RenderPerspectiveScales` now rejects
  invalid/unrepresentable scales; draw setup validates scales and depth terms
  before GPU uploads/render-pass creation, and the probe uses the same guard.
  `render_world` passes in Release and ASan/UBSan, with bitwise old/new equality
  for 24 valid FOV/aspect combinations and floating-point exception checks for
  invalid inputs. Sanitized game and Release game/replay/stage builds pass;
  local test app refreshed and ad-hoc signature verified. Evidence:
  `macos/projection-guard-*`. This is malformed-camera robustness, not evidence
  of a cause of ordinary race FPS loss or resolution of distant geometry.
- Texture memory audit found mod PNG dimensions were checked only after decode
  and RGBA conversion. `ModernLoadPNG` now reads a bounded PNG header from the
  same seekable stream before decoding, enforcing the existing 16384 limit.
  CgBI-before-IHDR is supported; full image validation remains SDL's job.
  ASan/UBSan `modern_png` and `modern_assets_retry` pass, including complete
  valid images, zero/oversized dimensions on both axes, truncated/invalid
  chunks, nonzero stream offsets, and 100 repeated preflight/decoder failure
  pairs with unchanged SDL allocation count. This limits rejected oversized
  images; it is not a global budget for all accepted textures.
  Sanitized game and Release game/replay/stage builds pass. The local test app
  was refreshed and its ad-hoc signature verified. Evidence: `macos/png-guard-*`.
- User reports further distant geometry after the previous visibility work.
  Treat visual acceptance as failed/pending reproduction, not resolved by the
  three sampled improvements. Location/direction of the remaining fragment
  has been requested. No game process was running during follow-up inspection;
  the exact executable used for that observation has not been established.
  Repository and saved user INI files contain no visibility/depth override.
- Native race visibility now defaults to authored camera regions with a
  16384-unit far plane and GPU depth clipping. Detailed visual/transition
  acceptance remains open. The rejected route-distance filter was removed;
  its sources and investigation patch are archived under `rejected-route-filter`.
- Baseline physical forward/reverse tours are recorded, including the shared
  finale's forced reverse. Full visual acceptance of all routes is incomplete.
- The combined authored-region + 16384-unit clipped-depth candidate completed
  all eight physical-direction tours (574 captures). All contact surveys were
  reviewed; detailed acceptance and transition checks remain open. It clears
  the confirmed old-town, reverse floating-tunnel and reverse fragmented-building
  views without losing the opening arch or the sampled bridge/hillside terrain.
- All 19 permitted upgrade payment/grade transitions passed ASan/UBSan checks,
  including insufficient funds and rejected model requests. Actual menu/package
  testing remains open.
- The first 0.6.3 timing attempt was discarded: auto selected PAL (25-Hz
  driving), and foreground monitoring observed Firefox covering the game.
  A short exclusive foreground window has been requested for comparable FPS.
  No performance improvement has been claimed. Both comparison builds now use
  Release flags. Default-visibility ASan gameplay completed two tours cleanly.
  A local standalone macOS bundle is staged, ad-hoc signed and verified; actual
  clean-user launch/disc-picker validation and distribution signing remain open.
- Expanded sanitizer instrumentation now covers project renderer/asset libraries
  and test translation units. Of 411 portable tests, 406 passed initially and
  all five failures passed after fixture/source-layout corrections. An updated
  two-tour macOS game run completed in `asan-full-course1-forward`: 446400 route
  units, exit 0, renderer teardown assets_ready=0/meshes=0, no ASan/UBSan
  diagnostics. User still observed unwanted distant Overpass geometry in this
  default-visibility run; this is not visual acceptance. No game remains running.
- Catalog/loader tests now link the production catalog and its production
  tables. All 32 retail variants and 19 permitted upgrade previews passed,
  including an explicitly instrumented ASan/UBSan executable.
- Game rebuilt after moving the unchanged catalog tables to
  `src/port/host_state_car_catalog.c`. Five relevant tests passed: car catalog, car
  prices, model loading, buffer bounds and record defaults.
- macOS audit, memory/division review, actual fresh-profile/package tests and
  controlled performance comparison remain open. Linux and Windows on darwine
  have not started, as requested.

## Required scope

1. Finish macOS validation first. Then use `ssh darwine` for Linux and its
   Windows VM. Do not substitute the previously attempted Bazzite host.
2. Drive all four courses forward and reverse. Record class/environment,
   disc region, binary/configuration hashes, route completion, and regular
   screenshots at a calibrated spacing of a few tens of metres. Review all
   images, including long straights, crests, switchbacks, tunnels, rear-facing
   scenery and visibility transitions. Include mirror/camera coverage.
3. Preserve the distant view along the opening straight while eliminating
   inappropriate geometry revealed from behind. Change visibility dynamically
   if evidence requires it; repeat affected routes after changes.
4. Audit texture/model allocation, size arithmetic, ownership, cache eviction,
   generation changes, failed uploads, teardown/restarts, and retained history.
   Exercise actual races under ASan/UBSan as well as malformed asset tests.
5. Audit variable divisors and signed division overflow, especially car/track
   geometry and projection. Add boundary regressions for findings.
6. Verify every car/upgrade price, model/grade indexing, affordability and
   purchase transitions against original data. Verify fresh-save Time Attack
   lap/total/sector/ranking defaults for every course/direction and region.
7. Compare 0.6.3-alpha and the fixed candidate on identical hardware, settings,
   routes and unlocked foreground display. Measure frame-time distribution,
   outliers and memory, separately from screenshot/trace runs. Include ordinary
   driving and race/menu/restart transitions; the route driver bypasses physics.
8. Test the standalone game package, clean CUE/BIN selection and automatic native
   startup on supported platforms. Publish neither launcher nor extra tools.

## Evidence gathered so far

- macOS release-equivalent RelWithDebInfo build completed in
  `build/main-064-game`. This is independent of the previous re-asset build.
- A compiled C probe read the legal local Track 01 BIN through the production
  ISO reader. The image is **NTSC-U**, executable `SLUS_004.03` (571392 bytes),
  despite its generic local filename. All 32 car-price values matched once at
  executable offset 0x73284; eight default lap values matched at 0x6d954 and
  eight total values at 0x6d974. Probe/source/output are in
  `build/release-audit-064/retail-tables*`.
  This confirms raw tables for this image, not yet grade indexing, purchase
  behavior, production initialization or regional equivalence.
- Existing `record_defaults_tests` supplies its own default arrays; it is not
  sufficient evidence for the production initialized state. Existing car-price
  tests check endpoints and overlap, not every original price/model mapping.
- First macOS course-0 forward drive and portable suite started. Evidence lives
  in `build/release-audit-064/macos/`. Capture cadence currently uses 12 logic
  frames at 6000 track units/second; physical spacing still needs calibration.
  These are instrumented visual runs, not performance measurements.

## Outstanding

All requirements remain open until their full scope is checked. Do not infer
completion from a running process, an image count, or a completed autopilot lap.
No Linux or Windows testing on darwine has started; macOS comes first.

### First macOS checkpoint

- Portable suite: **409/409 passed**, 173 functional + 236 unit tests, log
  `build/release-audit-064/macos/portable-tests.log`.
- Strengthened tests now link production initialized lap/total defaults and
  assert every car-price entry against the verified original sequence.
  `record_defaults` and `car_prices` passed after these changes. The format
  string remains a test fixture; model/grade mapping is still pending.
- Forward course 0 completed one full tour and clean teardown, 64 screenshots.
  Course 1: 93 screenshots; course 2: 87 screenshots, also complete/clean.
- Course 0 contact sheets were inspected as a first pass. Tunnel exit near
  the waterfront and distant scenery need full-resolution comparison and
  precise location capture; **visual correctness remains unproven**.
- The sequential remaining-routes CMake runner is active; inspect its actual
  process/session and `macos/routes.log` before restarting anything. A separate
  ASan/UBSan game build is underway in `build/main-064-asan`.
- The initial course-0 runtime log was written through the default macOS log
  path and copied to `course-0-forward/runtime.log`. Subsequent runs have
  explicit per-run log paths. macOS ignores XDG overrides; do not claim those
  variables isolate its user settings. Clean-profile/package tests remain open.

### Overpass City investigation

User confirms at least three distinct locations; resolving just one is not
sufficient. Course 1 forward contact sheets (93 images) have been inspected as
an initial survey. Candidate locations on the baseline NTSC-U drive:

- frames 578/590: disconnected elevated structure seen above the town descent;
- frame 1046: solid black rectangle at a rocky bend;
- frame 1418: exposed overhead structure at the late tunnel exit.

These are suspicious observations, not yet classified root causes. Compare
full-resolution captures and original renderer/source geometry before deciding
whether to cull or repair them. Additional defects may exist. Other candidate
views must not be discarded just because these three match the user's count.

Tracked default `video.draw_distance=1` requests retail reach in the legacy
capture path, but does not limit the native semantic world (see below).
Baseline route completion at that setting does **not** satisfy the requested
long opening-straight view. Extended-distance and eventual dynamic-visibility
runs are required. The old `draw-distance-track-reach` branch contains a related
experiment; inspect its assumptions rather than cherry-picking blindly (its
cell-distance calculation uses `long`, whose Windows width differs).

ASan/UBSan macOS game build completed in `build/main-064-asan`; runtime sanitizer
coverage has not yet started. Remaining baseline route process is still active.

Full-resolution follow-up at course-1 frame 1046 shows the black rectangle is
inside a freestanding sign/screen frame with two posts. It must be investigated
as screen content/orientation, not assumed to be a stray clipping plane. The
three observed candidates are not proof that the three user-reported extended
visibility defects have been identified.

### Follow-up checkpoint

- Baseline sequencer completed all eight requested series/course combinations.
  Actual autopilot logs confirm `series=1` for reverse courses 0–2. Course 3,
  class 5, instead logs `series=0`: `GrandPrixAssetSeries` deliberately uses the
  shared finale assets. Its directory name is **not evidence of reverse
  coverage**; a physical reverse inspection of that course is still needed.
- Two course-1 forward tours under ASan/UBSan completed with exit 0 and clean
  renderer teardown. No sanitizer diagnostics appeared in the run logs at
  `macos/asan-course1-forward/`. This is limited to the route driver and this
  configuration; it does not establish gameplay physics or leak coverage.
  Startup was heavily delayed while the game was covered by another application;
  a process sample showed waiting in Metal `nextDrawable`. This run is unsuitable
  for performance comparison. Bringing the test process forward let it progress.
- Repeated screenshot diagnostics now optionally write `.info.txt` files
  (`diagnostics.modern_dump_info=true`) carrying camera pose and draw state.
  The game rebuilt successfully. Paired Overpass reach 1/4 runs are in progress
  under `macos/overpass-reach-*`; inspect completion and metadata before reuse.
- Enlarged frame 578 clearly shows disconnected textured structures above the
  town at baseline reach 1. Frame 1418 shows repeated overhead beams at the
  tunnel exit; classify it by comparison, not appearance alone.

### Native visibility finding

`PortAfterSceneHandler` calls `GameRenderWorldPublishCourseObjects` and
`GameRenderWorldPublishTerrainGrid`. They publish every static object and every
nonempty terrain cell, deliberately bypassing the original region/scan mask.
The native camera has a fixed far plane of 262144 game units. The configured
draw-distance multiplier is consumed by `ModernDepthLimit` in legacy geometry
capture; it does not constrain these native static submissions.

This corrects the earlier assumption that setting 1 restores retail visibility.
At matching frame 578, camera 20524,5490,15726 and identical view matrix, both
reach 1 and 4 show the same disconnected structures above the town. The capture
face count changes (425 to 500), but the native static geometry defect persists.
Fix visibility in the native world/draw preparation path, preserving the long
straight and rear view; do not rely solely on the legacy depth multiplier.

### Region visibility experiment

Two opt-in investigation switches in `render_world_game.c` are not a finished
shipping fix: `diagnostics.native_region_visibility` filters native static
terrain/objects by the authored region bitset without imposing the short scan;
`diagnostics.native_static_backfaces` tests static-object triangle facing.
Both default off. The latter uses CPU triangle filtering and needs performance
review if retained; it did not remove the remaining small fragments at frame 578.

- Region-only Overpass forward: 93 captures, full tour, clean teardown.
  All seven contact-sheet pages were inspected. The arch at the end of the
  opening straight stays visible. Most disconnected structures at frame 578
  disappear, but several small fragments remain. This is partial improvement,
  not proof of correctness. The black screen at 1046 remains, and the overhead
  beams at 1418 still require classification. Rival-car occlusion at frame 890
  prevents assessing the underlying road from that image.
- Region plus static backfaces: full tour, 93 captures, clean teardown; frame
  578 inspected and still has the small fragments. Full review pending.
- Metadata initially described the newest published native world, one logic
  frame ahead of the snapshot in some captures. Corrected `WriteSceneInfo` to
  use `ModernNativeGpuPreparedWorld`, matching its world/draw dumps. Earlier
  `nativeWorld` counts describe the published frame explicitly named in them;
  do not treat them as exact per-pixel evidence for the screenshot.
- Next reproduction uses the existing marker/probe capture at frame 578 and
  pixel 860,217 to identify a remaining fragment. Script/output:
  `macos/overpass-probe*`; check actual process and completion before restarting.

The compiled retail probe additionally verified all 13 model base-index bytes
against a unique sequence at SLUS_004.03 offset 0x6c974 and all 13 unlock-base
bytes at 0x6c984. This verifies the production table contents; complete purchase
and upgrade transitions remain open.

### Remaining fragment identified

Marker 12 at frame 578, probe 860,217: one hit, terrain asset set 2, key 98,
mesh 63, source entity 131661 (= terrain cell 13,18), material 69, view depth
about 22451. The simultaneous compat image lacks the fragment. Marker bundles
12–15 were copied into `macos/overpass-probe-4/markers/`, including scene/world,
draw lists, VRAM and both renderer images. The probe run reached these captures
but then **timed out at 240 seconds**; it is not a completed route/sanitizer test.

Foregrounding through System Events was not reliable and reading its window
state ultimately reported missing assistive access. Do not infer benchmark
readiness from a successful `set frontmost` command. This also qualifies the
earlier note attributing progress to bringing the process forward: causation was
not established. The observed Metal wait and occlusion are enough to exclude
these runs from comparative FPS claims.

The next opt-in experiment, `diagnostics.native_terrain_backfaces`, applies the
existing triangle-facing rule to terrain. It defaults off, as do the two other
investigation switches. The binary rebuilt successfully. Its route is launched
by `macos/overpass-terrain-facing.cmake`; inspect the process and output before
continuing. No investigation switch is yet a validated shipping solution.

### Facing experiment rejected; physical reverse support

The terrain-facing run finished (93 images, clean teardown), but frame 578
lost almost all valid terrain. Source review explains why: native terrain
already uses `TerrainQuadIsHidden`, opposite imported winding to course objects,
and preserves a twisted authored quad when either triangle faces the camera.
The extra course-style triangle rule was invalid. Removed both static and
terrain facing investigation switches, and discarded the unbuilt alternate
winding experiment. No change remains in `src/render/render_mesh_build.c` or
`src/render/render_world.h`. Only the region-visibility investigation remains,
disabled by default.

The remaining mesh-63 face has no near-only material flag. Its world vertices
are around x=26745, y=-799 to -1061, z=-37304, while the camera is around
20524,-5490,-15727. The correctly facing quad belongs to a distant part of the
course. Blindly adding more triangle culling is not the next fix; native
visibility must exclude this disconnected distant section while keeping the
continuous opening straight. Existing branch `draw-distance-track-reach`
contains a nearest-route-point experiment, but its `long` distance arithmetic
and pointer-only cache identity need replacement before reuse.

Added `autopilot.direction` (-1 forward, +1 reverse, 0 follow series), without
mutating race series/assets or opponents. Start logs name the actual direction;
tour logs include cumulative track units. Five driver tests pass, including
opposite initial movement with the same race series and two complete ring
tours. The game rebuilt. The missing physical reverse inspection of the shared
finale is now running via `macos/finale-reverse.cmake`, output directory
`macos/course-3-physical-reverse/`. This is an inspection against opposing
traffic on the shared finale assets, not a claim that retail offers a separate
reverse finale race. Check completion, direction and images before crediting it.

### Route-window candidate: useful result, fails full visual review

- Physical finale reverse finished with direction=1, series=0, 103800 cumulative
  route units, 43 captures and clean teardown. All three contact pages reviewed;
  no obvious broken road was observed in this preliminary survey. Ordinary
  gameplay and full-resolution transition review remain distinct requirements.
- Implemented allocation-free `native_track_reach` map (1024 cell progress
  labels), bounded source count/segment lengths, double-precision squared
  coordinate distances (avoids signed-square overflow), uint64 route sums,
  and cyclic separation. Native integration invalidates on track asset revision,
  pointer or count; nearby cells remain available. Config switch
  `diagnostics.native_route_reach` is zero/off by default and currently requires
  the region-visibility investigation switch.
- Unit test passed with assertions enabled; separately compiled ASan/UBSan test
  also passed. Covers ring wrap, threshold edges, rebuilding changed data,
  INT_MIN/INT_MAX positions, invalid counts/lengths, and missing inputs.
- Reach 32000: full Overpass forward tour, 93 captures and clean teardown.
  Frame 578 is clear of the identified disconnected objects and terrain fragment;
  frame 470 retains the far arch. However, review of all six contact pages shows
  missing landscape around frames 974/986 and a sky hole beneath the bridge
  around 1070. This candidate **fails visual acceptance**. Whole-cell nearest
  route-point filtering also removes legitimate landscape from other sections;
  it must not be enabled as a release fix in its current form.
- The seven-other-route candidate sequencer was deliberately stopped (owned
  CMake PID 44132) once that regression was identified. The course-0 child had
  completed immediately before cancellation: 64 captures and clean teardown.
  Process inspection confirms no candidate game child remains. Course-0 visual
  review is still pending. Do not restart this rejected configuration just to
  finish the matrix.
- Source/binary/config hashes for this candidate were saved in
  `macos/route-reach-candidate.sha256`. Geometry visibility remains unresolved;
  distinguish landscape from disconnected track geometry before further broad
  route validation. The default game behavior still excludes these opt-in
  investigation filters.

### Retail model-header and loader verification

A compiled C probe reads the RAGE.BIN index directly through the production
disc/ISO/archive readers and samples byte 0x0a from all 32 car headers. Evidence:
`build/release-audit-064/retail-car-headers.c` and `.csv`. Original NTSC-U headers
permit 19 upgrades and disable upgrading at every catalog model boundary.
The car-select entry gate checks this loaded `upgradesAvailable` flag and the
class unlock level. No normal gameplay cross-model upgrade was demonstrated;
do not infer one solely from the deliberately broad GetCarAssetIndex helper.

Replaced the loader test's fake `model * 10 + grade` lookup with the production
catalog. Replaced impossible test grades with real ones. Added exhaustive
original-variant and permitted-upgrade selection checks, inactive slot checks,
paint eligibility and unchanged save grade during preview. Original catalog
bytes are now defined once in `host_state_car_catalog.c`, with the same alignment and
contents, and linked by both game and tests; the catalog test compares them
against verified original byte sequences.

At the time of this initial check, RAGE_ENABLE_SANITIZERS did not instrument this unit target:
its generated flags were inspected. Therefore the sanitizer evidence is from
the separately compiled `car_model_loading_sanitized` binary, built explicitly
with `-fsanitize=address,undefined -fno-sanitize-recover=all`, not from merely
running an ordinary test under a directory named asan. Exit 0; log stored beside
the binary. Actual file loading/serialization remains stubbed in this selection
test; existing asset validation tests and gameplay runs cover separate layers.
Cash deduction and full interactive purchase flows remain open audit items.

### Expanded sanitizer coverage and fixture corrections

The CMake sanitizer option now instruments the native renderer, simulation,
asset/content libraries and test sources in addition to the game objects.
Generated compile flags were checked; the complete sanitizer build succeeded.
The first portable run passed 406/411 tests. The remaining five passed in
`macos/sanitized-fixture-retest.log` after these corrections:

- Give the scripted-draw fixture a real ordering table, including offset 0x2be.
- Reset paint and spinning-scenery per-frame recording buffers between frames.
- Use unsigned modular arithmetic in the motion fixture's 15-bit random source.
- Name the extracted catalog source `host_state_car_catalog.c` so the existing
  audio source-layout validator recognizes its host-state-only role.

These are fixture/build corrections, not five demonstrated gameplay defects.
Unchanged motion/script digests passed. Initial full-suite output is preserved
in `macos/sanitized-portable-suite.log`; no sanitizer failures are being waived.

### Native camera depth clipping investigation

The fixed-depth diagnostic `diagnostics.native_far_plane=20480` completed a full
Overpass tour (`overpass-depth-20480`) but retained the bad frame-578 geometry.
Inspection found the native geometry pipeline zero-initializes SDL rasterizer
state and never sets `enable_depth_clip`. The installed SDL_gpu.h explicitly
defines false as depth clamp and true as depth clip. Thus adjusting camera depth
terms alone did not clip triangles in cells intersecting the camera volume.

Native geometry now enables depth clipping. Repeats with diagnostic depths of
20480 and 16384 both completed (93 captures each, clean teardown/exit 0).
20480 removes part of the bad old-town geometry; 16384 clears it in frame 578.
Frame 470 retains the distant opening arch; frames 974, 1070 and 1418 retain
mountains, terrain beneath the bridge and tunnel-exit beams respectively.
The 93-frame contact survey was reviewed at overview scale. Full-size frame 590
also has clear sky above the road. Complete per-frame and reverse validation
is still required before accepting this fixed-depth candidate.
Paired comparisons are stored in `overpass-depth-clip-16384`. This is evidence
for those specific views, not complete acceptance of every route/direction.
Default camera distance remains 262144. Dynamic visibility and all-platform
validation remain open. No game is left running at this checkpoint.

Reverse at 16384 completed 93 captures and clean teardown, but FAILED visual
acceptance: frame 566 shows a detached curved tunnel above the road; frame 890
shows fragmented buildings above the hillside. Frame 530 also has a black
upright polygon requiring identification. User independently reported remaining
fragments during this run. Evidence: `overpass-depth-clip-16384-reverse/suspects.jpg`.
The next experiment combines authored region visibility with the 16384 depth
limit (`overpass-region-depth-16384-reverse`), without the rejected route filter.

That combined reverse experiment completed 93 captures and clean teardown.
Full-size frames 566 and 890 now remove the detached tunnel and fragmented
buildings. The full contact survey has no obvious road holes at overview scale.
A dedicated probe at reverse frame 530 identifies the black upright polygon as
terrain mesh 177, cell entity 131388, material 33, at view depths 4721–5091.
It is also present in the simultaneous classic image (`overpass-reverse-board-probe/compat.jpg`),
so it is not evidence of distant geometry revealed only by the native renderer.
Marker bundles 16–19 and probe output are preserved in that run's `markers/`.
The combined forward repeat completed in `overpass-region-depth-16384-forward`
(93 captures, clean exit/teardown). Landmarks 470, 578, 590, 974, 1070 and 1418
preserve the opening arch, clear old-town sky and retain the hillside/bridge
terrain. Validation of the other three courses in both physical directions is
running through `region-depth-routes.cmake`; default shipping behavior has not
yet switched to this candidate.

### All permitted tune-up transaction checks

The engineer-shop fixture now exercises all 19 permitted NTSC-U model/grade
upgrade transitions with the verified retail price sequence: one credit short,
exact funds, rejected model request, successful countdown, payment on screen
exit, Time Attack grade update and no double charge on a subsequent frame.
Model/index lookup and loading remain stubs in this fixture; separate production
catalog/loader tests cover that boundary. This is not an interactive package test.
ASan/UBSan passed both ordinary CTest and explicit trace-output mode; the latter
also checks the correction that clears the output FILE pointer after fclose.
The historical 108864-state digest is unchanged. Evidence:
`macos/retail-upgrade-transactions.log`, `macos/retail-upgrade-trace-mode.log`.

### Complete combined-visibility capture matrix

The six runs in `region-depth-routes.cmake` all completed and verified their
actual physical direction, nonempty matched screenshot/metadata counts and
clean renderer teardown. Counts: course 0 forward/reverse 64/64; course 2
87/87; finale 43/43. Combined with Overpass 93/93, total 574 captures.
Full contact surveys were reviewed for the six additional runs. The suspected
opening beneath the rock in course-2 forward frame 1058 and foreground grass
in reverse frame 614 are also in baseline images; paired comparisons are saved.
They are not regressions caused by this visibility candidate, but regular
physics/camera testing is still needed to explain the grass view.

### 0.6.3 comparison build preparation

Detached worktree: `build/release-audit-064/baseline-063`, tag 0.6.3-alpha,
commit `ae4ef100a`. Original submodules were initialized at their tag revisions.
RelWithDebInfo SDL3 GPU game built successfully in `build/baseline-063-game`.
For equal route sampling, copied current debug_route/debug_autopilot, added the
same player-update/scenario hooks, omitted GPU-capture initialization, supplied
the old API's missing bounded integer parser and equivalent track-index wrap,
and used the original active-race phase value 2. This backport lives only in the
comparison worktree; renderer and asset implementation remain the tag versions.
Benchmark runs must record this qualification and confirm matching route/timing
before drawing any frame-rate conclusion.

### Default visibility implementation and verification

Promoted the combined region/depth candidate to normal race behavior. The region
lookup now lives in `native_visibility.c`, with ASan/UBSan checks for camera-cell
boundaries, direct versus reversed grid rows, region 31 and invalid 32/63,
out-of-range cell indices, missing tables and non-finite camera coordinates.
Non-race camera setup retains its previous far range. Diagnostic overrides are
available for comparisons. The rejected route-reach implementation and test were
removed from the live source tree after archiving them and their integration patch.

`asan-default-visibility` completed two full Overpass tours using default settings
(446400 units, exit 0, assets_ready=0/meshes=0 on teardown, no sanitizer errors).
`default-visibility-tests.log` passed the new region test, geometry construction,
geometry packing and all engineer-shop checks.

### Frame timing and local package preparation

Both versions have the same optional `benchmark-frame` timestamp line after GPU
command submission, controlled by performance + frame_timing. This is passive
timing instrumentation in addition to the baseline's route-driver backport;
no baseline rendering algorithm was ported. `frame-intervals.c` is a compiled
analysis utility that accepts only a completed second tour and computes interval
distributions after excluding the first warm-up tour.

Discarded `benchmark-063-a`: timing auto was PAL in 0.6.3, and repeated foreground
checks showed Firefox. The run was stopped and is not a performance result.
The comparison script now forces NTSC and uses Release builds for both versions.
Awaiting a four-minute unobscured-game window requested from the user; this does
not block independent audit work.

Built `build/release-064-package` with Release, authored cars OFF and SDL3 GPU.
Staged standalone app plus normal bundle resources under
`build/release-audit-064/macos-package`, signed ad-hoc and verified with strict
codesign. Its dependency list contains only system dylibs/frameworks. The compiled
release-package and visibility checks passed. This does not prove downloaded
Gatekeeper acceptance, notarization, disc-picker operation or clean-user startup.
No game or build remains running at this checkpoint.
