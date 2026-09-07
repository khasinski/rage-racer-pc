# 0.6.4-alpha stabilization

Status: candidate preparation, not ready to tag or publish.

Scope agreed with the user: fast, stable release before new mod-facing features
and major rendering changes. Keep existing visual quality and PAL/NTSC speed.
The broader architecture roadmap remains open after this stabilization boundary.
Do not replace the published 0.6.3-alpha tag or packages.

## Evidence and remaining gates

- Version: CMake and all three release workflow defaults now select 0.6.4-alpha.
- Linux compiled build: full GCC16 build passed in `rage-racer-dev`, including
  previously unbuilt tests (`/tmp/rage-064-full-build.log`).
- Portable suite: 403 unit/functional tests ran. Initially 400 passed, two skipped
  for missing explicit data paths, and shipped_config rejected the developer's
  local INI with marker capture enabled. The tracked release INI passed the
  same validator separately, without modifying the user's configuration.
  Both skipped tests passed when supplied the legal PAL image and imported
  render-stage assets. Logs: `/tmp/rage-064-portable-tests.log` and
  `/tmp/rage-064-data-tests.log`; tested tracked INI: `/tmp/rage-064-shipped.ini`.
  This is not yet proof of clean-package startup.
- Renderer regression evidence for the preceding transfer-boundary commit:
  main-loop ordering, native-world image/VRAM/publication oracles, renderer
  toggles, submission recovery and retained history passed. Real PAL frozen
  image/draw dump matched. See `performance-2026-09-07.md`.
- Moving stability: candidate b574ee682 completed three PAL class 1/course 0
  races, three presentation restarts and lifecycle verification successfully
  (`build/stability-064/20260907-183419-ac60d5/result.txt`). This run does not
  automatically assert visual correctness. Longer coverage across tracks,
  scene transitions and supported disc regions remains required.
  The actual Linux c7b5f7e69 package additionally completed a PAL class 1/course 1
  race with one renderer restart, retained meshes and ordered lifecycle teardown
  (`build/stability-064-course1/20260907-185726-60eafe/result.txt`). It ran with
  offscreen video/dummy audio; this checks route stability, not monitor pacing,
  audible output or visual correctness.
  The same package also completed class 1/course 2 and class 5/course 3 races,
  each with one verified presentation restart and lifecycle teardown. The
  course 3 race included six route-driver laps. Results:
  `build/stability-064-course2/20260907-190207-5c756c/result.txt` and
  `build/stability-064-course3/20260907-190436-9358d4/result.txt`. Combined with
  course 0 evidence this exercises all four PAL courses, not all car/class
  combinations or player-controlled physics.
  A freshly rebuilt GCC16 RelWithDebInfo game with ASan/UBSan completed one PAL
  class 1/course 0 lap and lifecycle teardown with leak detection and both
  sanitizers configured to halt on errors. No sanitizer error was emitted;
  exit status was zero (`build/stability-064-sanitized/20260907-190033-69a74f`).
  Runtime libraries were copied from the build container into the ignored
  build directory for this test; release packages do not require them. This
  instruments the configured game/host targets, not every third-party library.
- Performance: roughly 645 application FPS at numeric 1000 cap demonstrated
  rendering headroom, but stable VSync 120 FPS is still unproven. Validate
  frame intervals and outliers on the fixed candidate, not only mean FPS.
- Follow-up performance blocker: packaged c7b5f7e69 and the local GCC16 binary
  both fell to about 20 FPS in Wayland/VSync tests later on the same machine.
  The original profiling harness reproduced it and completed its lap
  (`build/perf-064-control/20260907-185137-c34596`). A sampled packaged main
  thread waited in Wayland explicit-sync image acquisition, beneath
  SDL_AcquireGPUSwapchainTexture. This does not establish a CI compiler defect;
  the desktop subsequently reported ScreenSaver.GetActive=true. Treat these as
  locked-desktop samples, not foreground release performance; repeat after
  unlocking. The profiling harness now rejects a known locked desktop before
  starting the game. A lock occurring during a run still requires discarding it.
  The same packaged c7b5f7e69 binary completed a clean-state Track 01 BIN lap at
  numeric 120 (immediate presentation): 117.828 FPS mean, minimum window 113.630,
  worst window p95 15.272 ms, maximum interval 51.086 ms, excluding the first
  startup window (`/tmp/rage-064-package-c7-immediate`). It logged complete
  lifecycle teardown. This is not validation of foreground VSync pacing.
- Packaging: verify clean CUE and Track 01 BIN flows, automatic native asset
  generation and modern startup in the actual packaged artifact.
  Linux artifacts b574ee682/c7b5f7e69 started with isolated empty XDG config/state
  directories using only explicitly supplied PAL CUE or Track 01 BIN paths.
  The importer and modern renderer initialized; the BIN scenario reached the
  race and imported native meshes. These timed probes are not full successful
  route tests or proof of interactive file-picker/double-click behavior.
- Platforms: b574ee682 passed Linux, Windows and macOS release builds, sanitizers,
  compiled texture/archive and mod contracts, and renderer snapshot contracts.
  Launcher CI passed on macOS but exposed two build defects: clang-cl ignored
  save-generator quote include options, and Ubuntu 22.04 required the GNU
  feature macro for RenderDoc's RTLD_DEFAULT lookup. Both have targeted fixes;
  rerun launcher CI and release builds on the corrected candidate. Local full
  build and save-generator, release-package and main-loop regressions passed.
  Hardware/runtime coverage must be distinguished from builds.
  c7b5f7e69 subsequently passed all three release builds and Linux/macOS launcher
  checks. Windows launcher checks exposed two further portable-tool defects:
  POSIX mkdir in the save generator (fixed in 2786f1a3a), then legacy Winsock
  typedef collisions in the mod CLI (fixed in f3d3b5543). Local tool builds and
  related regression tests passed. Launcher matrix 34168199514 passed on all
  three platforms at f3d3b5543, including packaged first-launch probes and native
  integration. This launcher probe does not include a legally supplied game disc.

Release only after the relevant evidence is recorded for the exact candidate;
no tag or public release has been created by this preparation step.
