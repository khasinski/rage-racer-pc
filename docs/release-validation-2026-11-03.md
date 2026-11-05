# Integration validation — 2026-11-03

This session covers the upstream PsyZ integration and the current native menu,
brightness and saved-settings changes. It does not mark Texture Refresh 0.1
accepted: normal-physics day/night laps, the native pack chooser and clean
double-click startup on all three systems remain separate observations.

## Repaired behavior

- Double-rate physics blended the wheel-speed snapshot loaded at a gear
  change. Preserve this discrete initial condition while continuing to blend
  continuous motion. At the existing comparison point, position error fell
  from `(425,39)` to `(136,17)` world units; the original 300-unit tolerance is
  unchanged. The unit fixture covers shifts on both half-step parities.
- The always-open native main menu once again preloads and starts idle race
  demonstrations. Opening/exiting transitions cannot trigger that load.
- Publishing a decoded movie now includes its black bars on both display
  pages. Pixels from the preceding scene can no longer remain in the bars.
  Real-disc FMV layout, in-process decoding without ffmpeg and audio/video
  pacing checks pass with the PAL CUE.
- Direct Extra GP boot no longer carries the menu's series bits into the
  track-pack slot. It now performs the same handover as normal car selection.
  This fixes Classic scenarios waiting at the frontend and the shared Extra
  finale. The loader retains its strict slot/bank validation. Start, class 4
  and finale scenarios verify both the slot and the selected asset series.
- Linux rejects a directory named `mod.toml`. Opening a directory can succeed
  on Linux; treating its apparent zero length as an empty manifest incorrectly
  activated its raw overrides. The existing unreadable-provider test covers
  this case.
- Explicit `RAGE_PORT_PROFILE_ROOT` isolates configuration, saves, audio cache
  and packs on every desktop platform, excluding old portable-save roots.
  Smoke executables allocate an independent temporary profile unless this
  override is explicitly supplied.
- CD track-transition logs remain available in Release builds, so replay and
  race audio tests can verify the selected tracks as well as mixed PCM energy.

## Test migration and image review

Menu tests now operate through the native menu, without a Press Start phase.
Prologue, vehicle and Grand Prix captures use scene timers instead of absolute
frame numbers. Held test input supports `scene@first-last:BUTTONS`; an end-to-end
check observes real accelerator input during the range and its release after
the range. The sky sequence now uses this input to avoid steering at a different
race location when asset loading takes a different number of frames.

Atlas tests use the retained wordmark instead of the removed Press Start sprite.
The mirrored crest fixture checks the new location and shading above the menu.
The old `ui.title.*` row override test verifies that retired overrides cannot
change the native text menu, including a clean/reference image comparison.
No missing UI coverage was replaced by a skipped test.

Classic course captures were compared with archived images matching the old
hashes before updating references:

| Capture | Changed pixels / 76,800 | Maximum channel difference / 255 |
| --- | ---: | ---: |
| GP 0 | 2 | 9 |
| GP 1 | 4 | 9 |
| GP 2 | 1 | 8 |
| GP 3 | 0 | 0 |
| Extra GP 0 | 14 | 8 |
| Extra GP 1 | 1 | 8 |
| Extra GP 2 | 4 | 9 |
| Extra GP 3 | 1 | 9 |

The seeded neutral-gamma modern capture is byte-identical to the stored
pre-rebase binary's capture, SHA-256
`6d3c229980970314a2c984771cc8f9f3c21bc662ed7c96fb1ba7125d3c28a64b`.
A separate neutral reference remains in the modern feature test. Modern's
100% brightness reference includes the intentional 1.15 baseline; Classic's
brightness invariance remains covered with grading enabled and disabled.
All 23 sky captures were reviewed as a contact sheet after changing input
synchronization. These scripted captures do not establish a complete driven lap.

## Local evidence

- `build/reports/release-gates-final-console.log`: 517 passed, 64 skipped, zero
  failures out of 581 registered tests before the additional real-disc fixes.
- `build/reports/release-gates-verified-pal-console.log`: full PAL-enabled gate,
  551 passed, 29 skipped, one temporary-directory fixture failure.
- `build/reports/release-gates-pal-last-rerun.log`: the remaining fixture passed
  after assigning its profile outside the directory checked for FMV leaks.
  Combined final coverage is 552 passed, 29 skipped, no unresolved failures.
- `build/reports/release-gates-fmv-fixed.log`: real-CUE movie checks.
- `build/reports/release-gates-extra-gp.log`: Classic presentation and asset checks.
- `build/reports/classic-reference-diff.json`: old/new hashes and pixel differences.
- `build/reports/sky-review-contact.png`: reviewed scripted sky sequence.

The default suite's skips include regional media and real-disc checks whose
default nested disc path is absent here. Additional PAL checks use an explicit
legal CUE; skipped tests are not counted as passes.

## Packaged startup

A staged macOS `.app` generated its native assets and menu PCM in two empty
profiles, once from CUE and once from Track 01 BIN. The BIN run used the package
directory as its working directory. Both entered the Metal modern renderer and
produced the same frame hash:
`383458ca2079eaf04a607ef090ff35e31d090658ce7eed2e16f5584cdbc83d67`.
These launches supplied the disc through CLI configuration. They do not prove
Finder double-click or native selection acceptance.
The final ad-hoc-signed local validation archive is
`build/release-validation/Rage-Racer-validation-macos-arm64.zip`, SHA-256
`366bf6b8afef9e76671b8cf4c63d5d34d748bbb2f64454120d22efcaad477a4a`.
It is a validation package, not a notarized public release.

`release-validation.yml` builds and tests macOS arm64, Windows x86-64 and Linux
x86-64 with read-only repository permissions and uploads validation artifacts.
It has no release-publication step. Isolated snapshots are published on
`validation/release-20261103`; the original working tree and its index are not
replaced by those snapshot commits. Linux is also built locally in an Ubuntu
24.04 x86-64 container. Software Vulkan/container performance is not hardware
performance evidence. The previously used bazzite/Windows VM host was unreachable.
An additional packaged Linux/Vulkan runtime probe under x86-64 emulation on
Apple Silicon exited with SIGSEGV, including with restricted software vector
width. A diagnostic preload captured a program counter outside the executable;
gdb could not read registers through this emulation environment. The cause is
unresolved: this is neither a passing Linux runtime result nor proof that the
native Linux build has the same failure. Native Linux GPU validation is required.
Independent composite and SSAO Vulkan GPU tests passed in the same emulated
environment, so the failure has not been reduced to a general Vulkan startup
failure. Logs are in `build/reports/linux-package-*.log` and
`build/release-validation/linux-runtime/`.

## Performance scope

The CMake harness supports Linux and macOS and forces hidden, silent execution.
Per-frame diagnostics include the scene timer, allowing startup/intro stalls
to be separated from settled driving. Runs use the existing debug route for
repeatable rendering load; its completed laps do not satisfy normal-physics
acceptance. Headless rendering intervals are not display/scanout FPS.

Final Apple M5 Max/Metal measurements at 1706×960, linear filtering, FXAA and
vibrant grading used three alternating original/HD pairs, two route laps per
run. No other game runs or container builds were active. Settled measurements
start at race scene timer 250; each case contributes 3,735 intervals.

| Metric | Original | HD 7.232 |
| --- | ---: | ---: |
| Median of three p99 measurements | 5.89 ms | 6.66 ms |
| Range of p99 measurements | 5.33–7.10 ms | 6.64–6.99 ms |
| Maximum settled interval | 14.28 ms | 16.59 ms |
| Settled intervals over 16.67 ms | 0 | 0 |
| First measured texture preload | 1.21–1.99 s | 5.99–6.31 s |
| Maximum interval including intro | 181.19 ms | 177.39 ms |

The recurring intro spikes are at scene timers 8, 60 and 80, coinciding with
additional material preloads. Moving these preloads before the intro and
reducing the initial HD decode/preparation cost are the next performance
targets. This session measured those costs; it did not eliminate them. SDK
clock time to the first race image is recorded separately and must not be
mistaken for complete process-start or disc-picker latency. OS file caches
were not flushed. Each process used a fresh profile.

The tested smoke executable SHA-256 was
`a74d263d40a41bcdca7c4ed6ca439ec1cf63d9952818cf32bbc172c9d5b99e46`.
Raw logs, settings hashes and completion evidence are under
`build/release-validation/performance/final-{original,hd}/`; per-run quantiles
and spike positions are in `build/reports/performance-review.json`.

## Follow-up: smoke memory footprint

The reported approximately 4 GB is macOS physical footprint, including Metal
resources in unified memory. In the original-texture probe, `footprint`
attributed approximately 2.90 GB to IOAccelerator graphics and 0.92 GB to owned,
unmapped graphics memory. Ordinary malloc allocations were much smaller. This
was not just a large reserved virtual address range.

An Objective-C diagnostic hook recorded Metal buffer and texture allocations
without changing SDL. It found 1,332 allocations of 2 MiB buffers. The legacy
VRAM upload transfer was sized for the entire 1024×512 RGBA backing image even
when `LoadImage` uploaded only a small CLUT. SDL's buffer cycling preserved each
in-flight upload and then retained those backing stores for reuse, holding
approximately 2.6 GiB in this one size class.

PsyZ commit `8db3cb7` replaces that amplification with lazy power-of-two upload
size classes, 1 KiB through 2 MiB. Transfers still cycle to preserve queued
copies. Resource teardown releases every size class. Full-page initialization
and movie upload retain their existing dedicated transfer.

| Measurement | Before | After |
| --- | ---: | ---: |
| Original textures, peak physical footprint | 3.97 GB | 1.25 GB |
| HD, peak physical footprint | 5.09 GB | 2.30 GB |
| 2 MiB backing allocations in the diagnostic trace | 1,332 | 12 |
| Combined size of those allocations | 2.79 GB | 25.2 MB |
| Three consecutive races, original textures | not measured | peak 1.37 GB |

The original and HD comparisons use the same settings and two route laps as the
earlier measurements, with no diagnostic injection during footprint runs.
The reductions are approximately 68% and 55%. Three full debug-route races
completed in the stability run; this does not assert normal-physics acceptance
or rule out every possible longer-term leak.

Validation passed 12 targeted Rage tests covering exact classic/modern image
references, brightness invariance, mirrors, renderer restarts, VRAM caching,
image upload and FMV. Six selected SDK tests also passed. The added GPU regression
queues a full VRAM upload, a size-boundary-crossing tile and 512 CLUT uploads
before synchronizing, then compares actual GPU VRAM against the complete expected
image, bypassing the CPU readback cache. Five broader SDK display/RGB24 image
checks failed identically in before/after probes; no claim of full SDK GPU parity
is made.

Evidence is retained under `build/reports/memory-audit/`, including `after-*`,
`stability-footprint.txt`, `metal-*-trace.log`, `render-tests.log`,
`compat-tests.log`, `psyz-upload-regression.log` and the baseline comparison logs.
