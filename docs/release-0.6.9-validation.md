# 0.6.9 candidate validation — 2026-10-15

Not published. One visual regression check remains unresolved: `sky_frames`.
The automatic checks below are not a substitute for manual release approval.
All game runs used headless/offscreen rendering and private save/config roots.

## Native tests (source-only release configuration, excluding e2e)

| Platform | Passed | Skipped | Failed |
| --- | ---: | ---: | ---: |
| macOS arm64 | 456 | 1 | 0 |
| Linux x64, Xvfb | 454 | 2 | 0 |
| Windows x64 VM | 444 | 12 | 0 |

Windows has no supported SDL GPU backend in this VM; eight GPU checks were
explicitly skipped. Two other skips are optional legacy Asset Browser Python
checks: the Store alias is not an interpreter. Those legacy tests still run
on macOS/Linux; no legacy implementation or coverage was deleted. The native
editor's compiled GLB/OBJ/texture tests run separately. The remaining Windows
skips are stream_table and render_stage_angles. macOS skips render_stage_angles;
Linux skips it and stream_table.

## Editor and packages

- macOS with actual PAL disc data, source-only game, MCP and packaged editor:
  196 passed, 10 optional skips, zero failures.
- Earlier Linux editor suite: 187 passed, 19 optional skips, zero failures.
- Earlier Windows editor suite: 183 passed, 23 optional skips, zero failures.
- Updated packaged editor: clean and seeded profile boot tests passed on all
  three platforms, including a visible experimental RT setting defaulting off.
- Real compiled MCP: validation, live inspector update, undo/redo, material
  form save and atomic mod apply passed.
- Original geometry: catalogue, all 32 player grade bodies, rival previews,
  GLB/OBJ round trips, repeated exports preserving external edits and published
  mesh-mod game loading were exercised. Source-only builds need no authored cars.

## Game end-to-end checks

25 distinct selected checks passed: initial state; scenario menu/direct launch;
after-finish recovery; two race restarts; GP results/reentry and exit; Time Attack
records; renderer cycles; GP prologue geometry; GP race-intro terrain; menu sweep;
trophies; volume view; boot/title stability; mirrored track; modern feature
frames; C disc-import bootstrap; PAL production/default modern disc selection;
modern FMV audio; FMV frame layout; all-FMV PCM/pacing; and Custom rival replay.

The extended GP results run entered four live races in one process, traversing
replay and prize/results screens between them. Finishes were synthetic, so this
does not establish complete-lap AI, driving or real-time performance correctness.

The new permanent `custom_replay` check chooses CUSTOM and rival model 13 through
normal menu input, then reaches replay in Classic and Modern. It passed on
macOS and Linux. Five replay unit suites now repeat their rival cases in Custom.
Additional isolated game checks wrote and reread a save, then loaded a complete
generated save with all cars/classes; both passed.
The generated PAL sample mod also ran in both renderers with RT off, including
when its configuration key is absent.

## Bugs fixed during this validation

- Custom replay incorrectly used the Time Attack format and skipped rival
  recording, playback, track reconstruction and drawing. Those paths now use
  the existing RaceHasRivals decision, with regression coverage.
- Two new menu/font test targets lacked the Win64 port compile definition.
- The renderer-cycle test incorrectly required real VSync in headless mode.
- Environment-provider GPU checks now explicitly skip when no backend exists.
- The package test read text and readiness in separate UI snapshots, causing a
  false failure during navigation; it now reads them atomically.
- Export failure injection now targets an untouched disc model and covers raw
  as well as authored decoding, rather than an already imported cached mesh.

## Outstanding release gate

`sky_frames` produces 23 captures, but its full-frame digest is 0x0FC5152A versus
0x930DD489. This repeats in builds with and without authored replacements and
with RT both off and full. Its checker hashes whole 320x240 frames, not only sky
pixels. A diagnostic 4:3 run also differs. The expected hash was not changed.
Disabling Classic enhancements also differs (0xF47EB2B9). Review this against
an approved reference before declaring visual tests green.

Only PAL disc data was available for this round. Actual NTSC-U/NTSC-J disc import,
regional music and race behavior remain unverified here (save-file formats for
all three regions were tested). Real audio output, AMD hardware, the reported
Windows second-race textures/performance and full manual race/replay testing
remain release checks. Ubuntu 26 validation builds are not portable Steam Deck
release artifacts. macOS candidates are ad-hoc signed, not notarized.

Evidence and candidate archives: build/reports/0.6.9/.
