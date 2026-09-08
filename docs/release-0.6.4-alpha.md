# Rage Racer 0.6.4-alpha

Standalone game release. No launcher is required or bundled. Modern remains
the default renderer; optional authored replacement cars are disabled in the
release build. No copyrighted game data is included.

Publication approved after manual testing on 2026-09-08. Remaining platform
and performance limitations are documented below.

## Player-visible changes since 0.6.3-alpha

- Enhanced classic rendering: higher internal resolution, true 16:9 horizontal
  coverage and interpolated presentation at the selected FPS target. F10
  switches renderers. PAL/NTSC game logic and physics retain their original
  cadence. See [classic settings and limitations](classic-renderer.md).
- Modern race visibility combines the track's authored visibility regions with
  bounded depth clipping. Static landmarks and moving scenery are published
  independently of the classic renderer's origin-cell gate, reducing whole
  landmark pop-in without exposing all distant geometry.
- Original sky primitives supply the modern sky, with corrected texture-bank,
  palette, horizon and interpolated camera behavior.
- The finish line queues “Finish!” without repeating the final-stretch
  encouragement (GitHub #22).
- Asset, car catalog, upgrade price, default record and render-projection
  validation has been tightened. The engineer shop tests all 19 permitted
  NTSC-U upgrade transactions, including insufficient funds and double-charge
  prevention.
- Race and movie audio lifecycle handling preserves the selected audio routes
  and restores movie audio after race fades.
- Interrupting attract mode before Grand Prix no longer leaves stale resident
  car geometry attached to newly loaded models, textures and wheels.
- Enhanced classic interpolation keeps connected road polygons coherent when
  topology changes, preventing alternating frames with holes in the road.
- NTSC-U speed displays use mph. PAL/NTSC-J retain km/h; rival simulation speeds
  are unchanged and covered by a 300-tick regional invariance regression.
- Save/load no longer waits indefinitely after successfully reading a memory
  card. Pending status probes preserve progress toward completion. Regression
  tests exercise both save and load with the actual asynchronous status machine.

## Performance and stability work

Completed capture frames and their VRAM snapshots are owned independently of
frames under construction. Asset generations and renderer restarts invalidate
retained resources together. Native cars and terrain reuse resident GPU
geometry, geometry preparation avoids repeated work, and compatible draws are
coalesced without changing their order.

Native vertex transfer staging grows with the actual upload instead of cycling
a fixed 112 MB allocation. Offscreen presentation explicitly retires GPU fences;
this fixes the Vulkan buffer accumulation and out-of-memory failure reproduced
on Linux during capture tests. Occluded windows skip unnecessary rendering.
PNG dimensions and projection inputs are checked before allocating or projecting.

These changes do not yet establish that all reported race crashes or frame-time
spikes are fixed. A focused macOS classic reverse Overpass lap averaged 119.78
FPS at a 120 FPS target but still contained an 82 ms interval. Earlier modern
foreground evidence contained a 226 ms interval. Neither is an acceptable basis
for claiming consistently smooth 120 FPS.

## Configuration and compatibility

Use a legally obtained CUE and its BIN tracks, or Track 01 BIN. CHD is also
supported. The game imports native assets automatically after image selection;
no extractor, Python installation or launcher is required. A Track 01-only
image cannot supply audio tracks absent from that image.

Existing configuration names remain valid. `[video] internal_scale`, `aspect`
and `fps` now also apply to enhanced classic. Set `classic_enhancements=false`
for the original classic framebuffer path. Marker capture/history and performance
logging remain disabled in the shipped INI.

Multiple-controller selection, raw wheel axis/button mapping, optional chase
camera lookahead and texture filter values are documented in the
[README](../README.md). Manual-only cars retain their retail transmission
restrictions; GitHub #12 is still a mod/enhancement request.

## Development and packaging

The source also contains asset/mod tooling, a save editor, launcher development
and optional authored cars. These are separate from this standalone game release.
Substantial internal cleanup replaces untyped state and unchecked arithmetic
with typed, bounded interfaces. New runtime fixes and regression checks use C
and CMake; the finish-transition Python test was removed only after its CMake
replacement passed. Four additional source contracts now run in C instead of
Python. The full ClangCL test build has been repaired, and all three release
workflows run the complete unit/functional selection. Existing unmigrated
developer tests remain available.

Release archives must contain the game, default INI, scenario example, license,
README and these release notes. Builds keep the pinned dependency revisions.
Local macOS packages are ad-hoc signed; downloaded Gatekeeper acceptance and
notarization require separate release-signing validation.

## Acceptance evidence

See the [GitHub issue audit](release-0.6.4-issues.md) for every open report and its
remaining evidence. Historical runs are preserved in the
[preparation history](release-0.6.4-preparation-history.md) and
[rendering audit](release-0.6.4-audit.md). Earlier binaries are not evidence for
an unchanged final candidate.

The current macOS Release full build and all 414 unit/functional tests pass.
Targeted ASan/UBSan tests pass. The live finish/menu transition, PAL menu music
tempo, race CD audio, Pegase cabin and clean production PAL startup checks pass.
A three-lap modern reverse Overpass route completes with clean teardown.

Darwine Linux passes 411 of 413 unit/functional checks, with two data-dependent
skips (disc stream table and prebuilt render-stage assets). Its three-lap modern
route and clean Track 01 BIN startup pass. The distributable candidate is rebuilt
in an Ubuntu 24.04 container, matching CI, and passes all nine release regressions;
its maximum required glibc symbol version is 2.38. That binary also passes three modern Overpass laps and clean BIN startup. The host-native glibc 2.43
executable is test evidence only and must not be distributed as the release.

Darwine Windows 11 builds all targets with ClangCL. Its full unit/functional
selection passes 406 of 407 tests with the PAL and NTSC-U images supplied; the
prebuilt render-stage-assets test skips. See [Windows test repair](windows-tests-2026-09-08.md)
and [save/load regression](save-load-settle-2026-09-08.md).
The controlled modern image has the same gray sky as macOS and Linux. Windows
uses Edge SwiftShader software Vulkan in this VM; accelerated Windows rendering,
physical wheel/controller behavior and downloaded application acceptance remain
unverified. A three-lap modern Overpass run at 426x240 completes with clean teardown. The 720p software run was stopped after one lap because rasterization was slow; it is not a successful three-lap result.

Default-driver check: Darwin’s Windows VM reports `No supported SDL_GPU backend found` without the process-only SwiftShader overrides. Test archives keep the normal default config and do not bundle that workaround. Manual Windows acceptance therefore needs a supported GPU backend; the VM’s default launch is a known failed environment check.

CHD is supported, but a complete fresh-package CHD first-run matrix has not
been run. The release workflows rebuild and test the final source without
requiring a player's disc in CI.
