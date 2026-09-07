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
- Moving stability: multi-race/restart exercise in progress. Longer coverage
  across tracks, scene transitions and supported disc regions remains required.
- Performance: roughly 645 application FPS at numeric 1000 cap demonstrated
  rendering headroom, but stable VSync 120 FPS is still unproven. Validate
  frame intervals and outliers on the fixed candidate, not only mean FPS.
- Packaging: verify clean CUE and Track 01 BIN flows, automatic native asset
  generation and modern startup in the actual packaged artifact.
- Platforms: candidate-wide GitHub checks plus Linux, Windows and macOS release
  builds still required. Earlier green CI for 35715956d does not certify the
  later changes. Hardware/runtime coverage must be distinguished from builds.

Release only after the relevant evidence is recorded for the exact candidate;
no tag or public release has been created by this preparation step.
