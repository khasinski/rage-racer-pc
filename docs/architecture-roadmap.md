# Extensible game architecture: implementation roadmap

Status: active. This is the scope agreed after the performance work, not a
claim that the migration is complete. Each stage needs production integration
and regression evidence; extracting an unused interface is not completion.

## Constraints and future consumers

- Preserve automatic disc import and modern startup in a clean release.
- Keep PAL/NTSC timing, original content, and visual compatibility covered.
- Implement runtime, tools and replacement tests in C/the compiled toolchain.
- Preserve user configuration; do not commit local diagnostic preferences.
- Future launcher: shared configuration/discovery/import APIs must work without
  a game loop or GPU. Do not introduce a second asset-import implementation.
  The `re-asset` launcher is now merged. Continue reconciling its integration
  contract; its Electron UI is an existing consumer of compiled game/tools,
  not a reason to duplicate their parsers in the runtime.
- Future standalone model import/export tool: share formats, validation and
  semantic identifiers without depending on live race globals.
- Future graphics effects and ray tracing: explicit material semantics,
  persistent geometry/instances, and backend capabilities. Keep hardware and
  effect choices out of simulation/content definitions. Ray tracing itself and
  further launcher UI work are not part of this refactor's deliverables.

## Launcher integration checkpoint (2026-09-06)

- Merged `re-asset` including launcher, C mesh/save tools and terrain palette
  fixes; retained compact GPU vertices, snapshot ownership and regional tests.
- Authored/replaced car banks now adopt their own bytes, decoded geometry and
  bounds through `RuntimeCachedMeshAdopt`, instead of copying a live cache
  owner. Resident-only renderer lookups return the prepared replacement bank.
- Mesh overrides join the common C semantic resolver, including exact/base
  precedence and whole-manifest validation, with regression coverage.
- Local optional models remain outside Git. The source-only build and local
  authored-model build both compile. The latter is not a redistributable asset
  package or evidence that the asset-rights audit is complete.
- JSON package and TOML dependencies now use the common compiled selection
  graph, including exact versions, regions and mixed cycles. The launcher
  rereads installed TOML before composition; runtime ordering uses that same
  graph through its stricter legacy manifest-selection adapter.
- Final conflict winner selection now runs in C before package composition;
  stale provider sets invalidate previous choices. The UI summary remains
  advisory, and large requests use bounded stdin rather than argv.
- Package JSON import validation now uses a C schema module and vendored strict
  JSON parser, including owned Unicode fields and the existing launcher limits.
- Remaining integration work: compiled profile mutation/export and resource-claim
  discovery, source fingerprints and atomic source snapshots; separate car
  catalog identity from optional embedded geometry; unify original/generated/
  mod providers. This checkpoint does not complete any roadmap stage.

## Stages and acceptance gates

Real-disc launcher cancellation coverage now cancels on the observable
"Preparing the asset library" boundary after archive read and before extractor
work. PAL/U/J preserve previous state/profile bytes, remove new staging, clear
the job lock and accept the next import. This verifies pre-aborted extractor
handling, not cancellation mid-write or an OS process that ignores termination.

Real-disc launcher tests now inject ENOSPC at launcher.json rename after native
extraction succeeds. PAL/U/J each preserve the previous in-memory state and
exact persisted bytes, remove the failed games staging directory and profile
temporary file, clear the operation lock, and permit the next valid import.
This is a scoped publication fault injection, not actual disk exhaustion or
proof of crash durability between filesystem operations.

Launcher preparation rollback is now exercised with a real native-tool failure:
after each PAL/U/J import a one-sector invalid BIN is rejected. In-memory and
persisted state, previous manifest and games-directory entries remain intact;
the busy lock clears, and a subsequent valid import succeeds into a new owned
directory. All three regional fixtures pass. This covers invalid-source rollback
and retry, not cancellation, disk-full publication faults or packaged launch.

Launcher owned-disc integration now has an opt-in Node fixture using the
existing launcher toolchain, real rage-racer archive extraction and rage-extract
C binaries. PAL/U/J all pass from empty temporary profiles: recognized region,
135 unique archive entries and payload lengths, persisted/reloaded disc state,
ready status and modern configuration. Set RAGE_LAUNCHER_{PAL,NTSC_U,NTSC_J}_CUE
and optionally RAGE_LAUNCHER_BUILD_DIR. The test removes only its own temporary
profiles. No Python is used in this path; packaged UI/double-click and actual
game startup after this preparation are still separate unproven gates.

Post-retry/GP refactor endurance refresh: current full Linux game passes two
class-1 Mythical Coast races per process in PAL/U/J, including three successful
presentation restarts, VRAM oracle agreement, automatic regional timing and
ordered GPU-before-assets teardown with assets_ready=0/meshes=0. Results:
build/regional-races/20260906-131459-0efea9 (PAL, 239.29s),
20260906-131459-0c2fbc (U, 212.32s), 20260906-131459-68e64b (J, 212.37s).
All processes finished successfully. These parallel offscreen/dummy-audio
route-driver runs are not performance measurements, visual assertions or
input/physics replay coverage; no entire roadmap stage is marked complete.

Post-migration checker fixture refresh passes on Linux and Windows ClangCL
Release (Windows 1.05s), including the added area/axis/continuity/wraparound
negative cases. The existing renderer-contract CI workflow now builds/tests
the independent image checker on its Linux/Windows/macOS matrix and watches
its source/fixture/build files. Hosted execution and macOS remain unverified;
the workflow change is local and requires publication to run remotely.

Stage-angle migration complete for this test only: render_stage_angles now
uses CMake plus the C checker, and verify_render_stage_angles.py is removed.
Audit preserves 18x24 quaternion sweeps, strict 1%-60% area, border and 25%/20%
framing limits, 0.6-1.7 continuity, 8% symmetry, five rotation comparisons with
two-channel-level tolerance, exact full turn/repeatability and four track sets
with 0.5% area minimum. Added negative tests cover exact excluded car-area
limits, squashed axes, abrupt area changes and wraparound stuck frames; current
checker passes ASan/UBSan. The renamed production test and checker pass (4.01s).
Malformed output/GPU execution errors now fail explicitly; missing assets still
skip. Other Python helpers and the wider six-stage roadmap remain unfinished.

The stage image checker now has an SDL/GPU-independent CMake contract build.
Strict Linux and Windows 11 ClangCL Release builds pass its fixture (Windows
1.20s), including blank/clipped track silhouettes and exact minimum-area
acceptance versus one-pixel-below rejection. Windows execution covers the CPU
checker/parser, not the stage GPU rendering scenario. Python remains pending
the final coverage audit; no platform-wide renderer claim is made.

Stage checker negative coverage now includes empty/truncated/trailing PPM data,
wrong magic/dimensions/channel maximum, matching rotation images, the allowed
two-level rounding delta, rejected three-level delta and changed silhouette.
Production stream parsing and rotation comparison are shared with the fixture;
strict C11 and full checker ASan/UBSan tests pass with leak detection. The
Python regression remains until the remaining mode/platform checks are done.

Compiled stage orchestration now renders all eighteen quaternion car sweeps,
five Euler/quaternion pairs (including compound rotations), exact full-turn
and repeatability pairs, and four track asset sets. CMake drives the existing
compiled stage and new C image checker; fresh runs pass both this test (4.12s)
and the original Python test (5.20s), plus synthetic checker tests. Artifacts
are retained under build/stage-angles-* for inspection. Missing assets skip;
render/GPU errors fail rather than being broadly treated as no-GPU skips.
Parser/comparison negative coverage and platform verification remain before
removing the Python test.

Stage-angle compiled migration started without deleting the Python regression.
tools/rage_stage_image_check.c implements fixed-size stage PPM validation,
24-view silhouette/symmetry/continuity/stuck-sweep checks, exact comparisons,
rotation pixel tolerance and track framing. Strict standalone C builds pass;
all 18 saved post-snapshot car sweeps pass, and synthetic valid/disappearing/
frozen/clipped/asymmetric fixtures behave as expected. Process orchestration,
CTest registration and complete negative coverage (including parser and other
comparison modes) remain before replacing the existing end-to-end test.

Functional checkpoint: 166 tests selected; 163 passed, two skipped without
data overrides, and shipped_config rejected the user's local marker_capture
setting. The committed HEAD INI passes that same verifier without changing
the user's file. Rerunning stream_table with actual PAL/U CUEs and the existing
render_stage_angles with the local native asset root passes both (5.27 seconds).
Traffic-avoidance fixture hashing now feeds the label and bounded numeric state
separately, eliminating possible combined-line truncation without changing the
expected retail digest; its rebuilt test passes and the warning is gone.
These are functional tests, not the full GPU/e2e matrix or packaged release.

Broad Linux checkpoint: full local build succeeds; all 231 unit-labeled tests
pass (2.04 seconds), including the locally parked geometry prototype fixture.
This is not the full functional/e2e suite or a clean-release build. The build
exposed a memory-card fixture defining g_McMenuPhase as s32 despite the public
MemoryCardPrompt declaration; corrected to the production type, rebuilt, and
memory_card_menu passes (1.02 seconds). A separate traffic-avoidance fixture
format-truncation warning remains and has not been hidden by warning flags.

The CPU asset-session fixture now changes a mesh file under the same path/key:
the active session retains the original bytes and pointer, while shutdown and
reinitialization load the replacement bytes and decoded vertex position. The
Linux fixture passes. This establishes explicit session reload behavior, not
automatic hot reload, source fingerprints or cross-generation GPU retention;
the sanitizer result below predates these new replacement assertions.

Retry ownership fixture now writes and loads a synthetic mesh through the
production asset adapter after recovery. Repeated successful InitRoot retains
the cache-entry pointer, owned byte pointer and contents; double shutdown leaves
zero cached meshes. The fixture passes normally and under the same scoped
ASan/UBSan setup with leak detection. It exercises CPU mesh ownership, not GPU
uploads or resources retained by captured frames across asset generations.

Retry sanitizer checkpoint: the Linux development container compiled the retry
fixture plus modern_assets, platform/config, mod assets, texture patch, track
identity and offline-importer sources with ASan/UBSan (GNU C11, matching the
normal build). Invalid runtime/environment indexes, both retry entry points
and repeated shutdown pass with leak detection enabled and UB halting. Linked
SDL/render/mesh/JSON/miniz static libraries were reused without instrumentation,
so this is scoped asset-adapter evidence, not whole-stack sanitizer coverage.

The retry fixture also covers the normal ModernAssetsInit entry point with a
forced configured root: corrupt index fails, corrected index succeeds without
shutdown, and repeated successful init stays ready. The fixture sets its own
modern-assets environment override to avoid inheriting an unrelated developer
cache. This additional path passes on Linux; no runtime code changed after
the preceding six-test smoke checkpoint.

Retry integration verification: rebuilt Linux smoke passes native_render_world,
GPU submit recovery and default selected-disc startup for PAL/U/J; together
with the retry contract, 6/6 tests pass (7.92 seconds, no skips). The retry
fixture additionally rejects a malformed environment index after loading a
valid runtime index, then succeeds after removing the bad optional index.
This exercises later partial-initialization cleanup, but is not allocator
fault injection, sanitizer evidence or Windows execution.

Failed asset initialization now remains retryable: initialized is published
only after successful source setup, and mod-provider borrowed metadata is
refreshed on each attempt without shutting down importer resources. A new
compiled fixture links production modern_assets.c and offline importer stubs:
invalid index -> corrected index -> retry, successful idempotence, repeated
shutdown, and unavailable importer -> explicit valid root. The original code
failed the corrected-index retry; the fix passes. GPU/disc integration and
Windows execution are still pending for this local change.

Source-identity audit: default ModernAssetsInit uses the live C importer, not
a persistent cache keyed by disc pathname; only forced modern.assets or the
explicit InitRoot API chooses prebuilt resources. Importer entries are keyed
by assetKey/assetSet within a session and released by ModernAssetsShutdown.
Thus adding a pathname/mtime check here would not implement the missing shared
source identity. A concrete lifecycle gap to cover next is failed-init retry:
both asset init entry points set s_initialized before trying their source and
return the cached failure on subsequent calls until shutdown. The toggle path
can call init again, but cannot recover from that cached failure by itself.
Any fix needs a regression with failing source, corrected source, retry, and
owned-resource cleanup; it must not discard resources retained by live frames.

FMV class lookup now clamps to the content table's six entries, not the old
four unique promotion movies. Classes 4/5 therefore consume their own explicit
stream definitions; repeated retail movie selection remains unchanged. Direct
stream/advancement tests include INT32_MIN/MAX and both series and pass with
ASan/UBSan; three class/advance/prize fixtures pass. No new game-level run is
claimed for this lookup-only follow-up.

Reward mapping verification: production prize-entry tests pass ASan/UBSan,
including distinct amounts for every class. Rebuilt Linux smoke passes six
actual-disc scenarios: class-0 promotion and Extra finale for PAL/U/J (34.98
seconds total). These verify the prize-to-FMV flow, movie timing and XA mixer
contribution; exact reward amounts are asserted by the unit fixture rather
than these runtime logs. Full regional class matrix and Windows runtime were
not rerun for this reward-field change.

Prize-screen selection now consumes explicit prizeClass/promotionBonusIndex
fields in the shared class definition, retaining disc-loaded prize amounts and
the existing mutable bonus table. The shared finale has prize row 5 but no
promotion bonus entry. Production prize-entry tests enumerate all six rows
using distinct fixture amounts; rule, advancement and prize-entry tests pass
(3/3). This local follow-up still needs runtime regression and does not expose
external content loading or move the reward amounts out of legacy storage.

Asset-series follow-up verification: the full Linux smoke build exposed a
missing direct stddef.h include in grand_prix_content.h (unit compatibility
headers had supplied NULL transitively). Fixed; the header now passes isolated
strict C11 syntax checking and smoke builds. All three real-disc Extra finale
scenarios pass after rebuilding (PAL/U/J, 20.90 seconds total), including
modern GPU setup, award/ending transition, movie pacing and XA contribution.
This covers the special shared-assets finale, not a new full race/perf matrix.

Asset-series selection now reads each built-in class definition as well: both
menu consumers and scenario setup use the shared adapter, with Extra class 5
explicitly mapped to standard assets. Tests enumerate all twelve class/series
combinations and preserve legacy invalid-input adapter behavior; rule and
advance tests pass, and rules pass ASan/UBSan. This follow-up still needs a
rebuilt game integration run; the 33-case result below predates this field.

Shared GP/FMV definition integration now passes the rebuilt Linux smoke's full
33-case natural award/ending matrix on actual PAL, NTSC-U and NTSC-J CUE/BIN
images (186.75 seconds, three workers, no skips). Each case checks the post-race
award/return scenes, disc-derived movie frame count and sector pacing, positive
XA mixer contribution and session PCM. Smoke SHA-256:
08bae1b823cd57a3399fc4d81036599bf0481cbdfccdf6ee24b75774df1aae3b.
Class-definition/rule tests also pass ASan/UBSan. These offscreen/dummy-audio
results are not an audible playback check, performance benchmark or Windows
game verification, and do not complete the versioned-content stage.

The in-progress shared GrandPrixClassDefinition now also supplies promotion
stream selection to BeginClassFmv. Compiled rule and real stream-selection /
advance-handler tests pass (2/2), enumerating both series, all six definition
entries, direct negative/out-of-range class clamping, repeated upper-class
promotion streams and separate ending selection. This is not yet a regional
disc/audio rerun or an external content schema; those gates remain open.

GP class rules now consume one built-in class-definition table for course
counts, per-series score-record identity, next class, record unlock and finale
flags. The shared Extra finale retains its separate selection/record semantics.
Tests enumerate every class in both series against retail expectations and
retain all eleven record-unlock checks; class/prize tests pass ASan/UBSan.
This is a production data-table consolidation, not yet an external/versioned
content format: asset-series/FMVs and rewards still need common definitions,
validation and full regional award-flow reruns after this change.

Content-rule audit found PrizeForRacePosition subtracting one before validating
the signed position, overflowing on INT32_MIN. It now checks the one-based
range first. Class-progress tests add both signed extremes and pass under
ASan/UBSan with production prize/rule sources. This is input-boundary hardening;
GP course/progression/FMVs still need unified versioned content definitions,
and no data-driven content stage completion is claimed.

Incomplete-presentation policy verification: refreshed Windows ClangCL
world/snapshot contracts pass (0.84 seconds). Rebuilt Linux smoke passes all
five GPU/startup checks: native_render_world, submit recovery and selected-disc
modern startup for PAL/NTSC-U/NTSC-J (7.84 seconds, no skips). This confirms
ordinary tested paths still render; synthetic overflow rejection is covered
by the contracts, not an injected live-game overflow or pixel/performance gate.

Presentation source validation now rejects an overflowed previous or current
world, not just inconsistent instance bounds. Previously previous-frame
overflow could disappear because the game initialized result metadata from
the current frame while borrowing previous vehicles. TryBuild failure feeds
the adapter's explicit incomplete-world signal. Tests cover each source's
overflow independently with unchanged output/count; standalone contracts and
ASan/UBSan world tests pass on Linux. GPU integration rerun is still needed
for this rejection-policy change.

Synchronized presentation exposes explicit success/count via TryBuild. Valid
empty output succeeds with zero count; failure preserves caller output/count.
The game adapter uses this API and marks rejected presentation incomplete with
overflowCount instead of treating it as valid empty data. The count-only API
remains a compatibility wrapper. Standalone world/snapshot and ASan/UBSan world
tests pass; rebuilt Linux smoke passes native_render_world, submit recovery
and selected-disc PAL startup (7.87 seconds). Full regional/performance and
Windows game-adapter reruns remain open.

Synchronized presentation now counts its selected current-static plus
previous-dynamic instances before writing. Insufficient capacity returns zero
without changing the output instead of silently truncating the scene. Tests
cover oversized static output and a mixed previous-vehicle/current-terrain
union whose sources individually fit but whose result does not. Standalone
world/snapshot tests and the world ASan/UBSan fixture pass on Linux. The API
still represents both empty and rejected output as zero; richer error reporting
and current GPU/performance verification remain outstanding.

Post-numeric-change smoke checkpoint: rebuilt current smoke/world/snapshot
targets and passed seven Linux tests in 7.85 seconds: default selected-disc
modern startup for PAL/NTSC-U/NTSC-J, native_render_world, submit recovery,
world interpolation and snapshot ownership. Real regional CUE paths were
provided; no regional cases were skipped. This refreshes GPU/startup evidence
after angle/phase wrapping changes, not full-race pixel/performance evidence
or a rerun of the longer repeated-race matrix.

The standalone renderer gate now also builds the production interpolation,
projection and shadow sources with the full render_world_tests fixture.
Both world and snapshot contracts pass on Linux and Windows 11 ClangCL
(1.14 seconds), including bounded angle/sky-phase wrapping and overflow cases.
Workflow filters cover these added sources/tests. Remote CI, macOS and actual
GPU image/performance verification of the numeric changes remain outstanding.

Periodic sky phase wrapping now uses bounded remainder/correction too, avoiding
the same stalled-subtraction loop on extreme phases. A valid previous phase is
retained for invalid current values. Tests compare 1025 quarter-step phases
against the former bounded-domain algorithm and exercise both signs of 1e20,
FLT_MAX plus infinity/NaN from a valid prior camera. Existing yaw-boundary sky
phase checks still pass. Strict world tests pass; extreme-input checks also
pass ASan/UBSan. This is not comprehensive validation of arbitrary camera
vectors and does not establish a retail flicker cause or full-scene pixels.

Angle compatibility regression compares 11521 quarter-degree deltas across
[-1440,1440] against the former bounded-domain algorithm, including +/-180
ties. All results match. Opposite FLT_MAX endpoints exposed an additional
finite-input subtraction overflow; interpolation now uses a double-precision
remainder only on that overflow path. Both endpoint orders remain finite, and
the full world fixture passes ASan/UBSan. Periodic sky-coordinate interpolation
still uses iterative wrapping and needs a separate bounds/finite-input audit.

Renderer angle normalization no longer repeatedly subtracts 360: for very
large finite floats that subtraction can round back to the same value and
never terminate. Camera-cut detection and interpolation now use fmodf followed
by bounded wrap correction, preserving shortest-path +/-180 semantics. Tests
exercise both signs of 1e20 and FLT_MAX through interpolation and camera updates;
the full render-world fixture passes strict C and ASan/UBSan under a 15-second
timeout on Linux. This is malformed/extreme-input hardening, not an explanation
of previously reported retail track flicker; full GPU/platform reruns are open.

Snapshot ownership/serialization now has tests/render_snapshot_contract, a
standalone build of the production world/snapshot sources and the same full
regression fixture, without SDL/game assets. Release passes on Linux and
Windows 11 ClangCL (0.53 seconds), covering reusable buffers, transactional
replacement, reserved-output preservation and format compatibility. A dedicated
Linux/Windows/macOS workflow is added but not run remotely; macOS remains
unverified. This is not a Windows GPU backend or packaged-game test.

Added tools/rage_snapshot_bench.c, a standalone compiled steady-state CPU probe
for owned-world copies (20000 iterations per size, allocation identity and
copied values checked). Host Linux GCC -O2 measurements across three runs are
recorded as a microbenchmark, not a frame-time gate: 256 instances copy 43008
bytes, 2048 copy 344064 bytes, and 8192 copy 1376256 bytes. First two runs took
0.738-0.808 us, 5.204-5.248 us and 47.338-48.575 us respectively. Reproduction:
cc -O2 -DNDEBUG -std=c11 -Isrc tools/rage_snapshot_bench.c
src/render/render_world_snapshot.c src/render/render_world.c -lm
-o build/rage-snapshot-bench; then build/rage-snapshot-bench.
Warm reusable buffers exclude first-allocation/growth cost and do not measure
cache contention with the game, GPU work or frame-tail latency. Full-game A/B
performance evidence remains required.

Prepared-world image checkpoint: 432 PPMs (18 retail car keys, 24 quaternion
angles each, 240x180/elevation 20) are byte-identical between the earlier local
stage executable and the rebuilt current stage on Linux offscreen. Evidence:
build/owned-world-image-uqV9G0/{before,after}; baseline executable SHA256
440be90a0e60618164598e8f41dd344539e29ab704003eab77c2e5049a74f806,
current 0a4e294e68178986190e5bf7c38d9065e263137c8876a5154adc646b76eab920.
The baseline is an older local binary, not a clean parent-commit A/B build.
This establishes unchanged model-stage pixels only, not full-race sky, mirror,
postprocessing or an isolated performance delta.

The GPU-owned world checkpoint now also passes same-process two-race NTSC-U
and NTSC-J scenarios (212.53/212.58 seconds, run concurrently). Both verify
automatic NTSC 60 Hz base timing, three presentation restarts, VRAM cache
comparisons and ordered teardown. Evidence directories are
build/regional-races/20260906-120632-34d2ba (U) and
build/regional-races/20260906-120632-d62765 (J); both record the same full-game
hash as the preceding PAL pass. This completes this Linux regional regression
matrix, not the presentation stage: image/performance comparisons, physics
replay and Windows/macOS runtime verification remain open.

GPU-owned world integration now passes the full PAL same-process two-race
scenario: modern_repeat_pal completes in 244.63 seconds, with three successful
presentation restarts, VRAM cache-oracle checks and ordered session teardown.
Evidence: build/regional-races/20260906-120143-8c67f1/result.txt and game.log,
including the full-game binary/config hashes. This uses the route autopilot,
not physics/input replay. Offscreen execution does not establish visual
equivalence, audible sound or an isolated performance delta; NTSC and other
platform reruns for the owned-world change remain outstanding.

Reusable snapshot capacity now has explicit grow/shrink/empty/refill and
overlapping-owned-subrange regression coverage. It checks retained allocation
identity after shrinking, independent copied values after source mutation and
clean release. The full snapshot fixture passes strict C and ASan/UBSan/leak
detection on Linux. This verifies buffer lifecycle, not measured frame-time
impact or a real race/menu transition with the new GPU-owned snapshot.

Native GPU preparation now deep-copies renderer-neutral world values into its
own snapshot instead of retaining the producer's world pointer. Cameras/light,
instances and diagnostic history access therefore share the prepared values;
shutdown releases the owner. Copy reuses sufficient instance capacity, including
self-copy, to avoid allocation on steady-size frames. Failed copy invalidates
draw availability/frame identity. Snapshot regression passes under ASan/UBSan;
rebuilt Linux smoke passes native_render_world, modern_submit_recovery and
render_world_snapshot (7.66 seconds). No performance/pixel or Windows gate has
yet been run for this change. Asset IDs still depend on external generations,
so immutable presentation/resource ownership is not complete.

Renderer checkpoint after snapshot ownership changes: rebuilt smoke and snapshot
tests, then passed native_render_world, modern_submit_recovery and
render_world_snapshot on Linux offscreen/dummy audio (7.64 seconds total).
History capture already releases a slot's old world on copy failure, so the
inspection did not establish stale-frame publication there. GPU preparation
still borrows s_world; immutable presentation and generation ownership remain
open. These tests do not establish pixel equivalence, audible sound, performance
or a Windows/macOS runtime pass; native_render_world still uses its existing
Python verifier pending an equivalent compiled replacement.

Frame snapshot writing now exclusively reserves path.tmp instead of truncating
an existing temporary file. A regression preserves sentinel bytes in both the
reserved temporary and final paths after rejection, then verifies successful
write/read after reservation removal. The complete snapshot fixture passes as
a standalone strict C build and under ASan/UBSan/leak detection on Linux. This
does not add unique concurrent writer names, crash recovery, fsync durability
or Windows replacement semantics; it prevents destruction of an existing
staging reservation.

Renderer snapshot read ownership now matches copy ownership: parse into a
private snapshot, replace/release the old owner only on success, and preserve
the previous frame on malformed input. Previously Read zeroed an already-owned
destination and leaked its instances. Replay and test callers now explicitly
zero-initialize destinations. Repeated-read/malformed-replacement regression
passes in CTest and a standalone ASan/UBSan/leak-detection build; snapshot tests
and frame replay build on Linux. This fixes serialized frame ownership, not
remaining live legacy reads or resource-ID generation binding in presentation.

All nine standalone mod contracts now pass under ASan/UBSan with leak detection
and halt-on-UB in the Linux development container (0.17 seconds), including
the native profile and legacy-index writers. RAGE_MOD_SANITIZERS enables this
without ad-hoc compiler flags, and the Linux CI job includes the same variant.
Hosted CI is pending. This is contract-fixture memory coverage, not full-game
sanitizer execution, arbitrary input fuzzing or completion of resource lifetime
gates for the renderer.

Manifest IPC expansion now has an explicit 8 MiB response budget for snapshot
inspection and material-edit refresh. A valid 2048-mesh native manifest below
the 2 MiB input ceiling produces more than the generic 2 MiB response limit
after backingFiles/resourceClaims are included; the new regression verifies
complete tables and derived lists under the explicit budget. All 78 launcher
tests pass on Linux. This probes the native schema limit, not import of 2048
actual meshes or support for arbitrary mesh targets in the current launcher.

Composition file execution now uses one compiled snapshot-copy batch, including
provider-local semantic files, legacy pairs, global winners and the original
archive marker. The launcher retains planning/directories, but no longer calls
fs.copyFile inside composition. The composed batch now shares the native 10000
pair/8 MiB request/1 GiB copied-byte limits across providers (rather than per
package); exceeding them rejects publication. All 77 launcher tests pass on
Linux, including injection of a staged raw-file symlink and cleanup before
publication. Existing raw/semantic/legacy roundtrips still pass. This is not
parent-directory pinning or a complete compiled composition planner.

The legacy index writer now has a compiled standalone regression using its
real stdin path. Invalid later entries leave no output; order/repeated owners,
exclusive creation and readback through the shared parser are verified. All
nine mod contracts pass on Linux and Windows 11 ClangCL (1.70 seconds). The
standalone CI path filters include both profile/index adapters; hosted execution
and macOS remain unverified, and this is not a full Windows launcher run.

Legacy index serialization also runs in compiled tooling: a bounded JSON list
of asset/path pairs is fully checked by LegacyTextureIndexLine before exclusive
UTF-8 output creation. Composition no longer emits index text in JavaScript.
Tests cover order/repeated owners, boundary slots, unsafe/control/NUL paths,
late invalid entries leaving no output and existing-file preservation. All 76
launcher tests pass on Linux with the rebuilt CLI; this new command has not yet
run on Windows. Copy planning/publication and profile persistence still require
compiled migration; no complete roadmap stage is claimed by these adapters.

Profile output now reuses ModFileWriteExclusive and the snapshot module's
UTF-8/UTF-16 target handling, preserving exclusive creation and failed-write
cleanup without a second Windows path converter. The compiled profile fixture
creates and reopens a Polish-character filename and rejects replacement.
All eight contracts pass on Linux and Windows VM (5.28 seconds), and all 75
launcher tests pass with the rebuilt CLI. This verifies native writer paths,
not the full Windows launcher's argument encoding or every other file reader.

Profile writing now separates bounded JSON processing from stdin transport and
owns its parser allocation instead of borrowing the CLI's global manifest.
Table capacities are checked before TOML serialization. A compiled standalone
fixture exercises validation, limits, exclusive creation and parser roundtrip;
all eight mod contracts pass on Linux and Windows 11 ClangCL (5.57 seconds).
All 75 launcher tests pass after staging the rebuilt CLI. Windows fixture paths
are ASCII; Unicode profile output still needs implementation/verification.

Final profile TOML serialization now runs in rage-mod-cli --write-profile-stdin:
bounded strict JSON tables become a private document, validated by the runtime
manifest parser before exclusive file creation. The launcher no longer emits
TOML or runs a separate post-write validation process. Tests cover malformed
tables, unsafe paths, material errors, control/NUL injection, output preservation
and combined-capacity rejection; all 75 launcher tests pass on Linux with the
rebuilt CLI. Windows execution/Unicode output handling remain to verify. File
copy planning and directory publication still belong to the launcher; this is
not completion of compiled profile composition or the unified catalog.

Launch failure cleanup now independently attempts log closure and removal of
the owned composed profile, retaining the original spawn/configuration error.
Previously a rejected log close skipped profile removal and masked that error.
An injected close failure after an actual ENOENT spawn verifies profile removal,
released service state and the original diagnostic. All 74 launcher tests pass
on Linux; cleanup failures are best-effort, not guaranteed filesystem recovery.

Composed profiles now remain under private mod-sources staging until the final
manifest passes the compiled parser, then a same-filesystem directory rename
publishes active-mods. A fault-injection regression checks private placement,
valid bytes and absence of visible active output at publication, rejects the
rename, verifies cleanup and retries successfully. All 73 launcher tests pass
on Linux. This avoids exposing an in-progress tree under the active prefix;
it does not add fsync durability, orphan recovery or cross-process locking,
and the publication coordinator still awaits migration to compiled tooling.

Composition publication regression combines two individually valid 300-material
mods into an invalid 600-entry manifest. The real final parser rejects it; the
test verifies removal of failed output/source staging, byte preservation of a
previous successful profile, and successful retry after disabling one mod.
All 72 launcher tests pass on Linux. Existing cleanup required no code change;
this tests handled validation failure, not process termination/power loss or
atomic publication against another concurrently running launcher.

Compiled-claim lifecycle regression now imports two nonconflicting mods, adds
a conflicting material through the real editor, requires composition rejection,
selects the second provider, reloads the persisted launcher profile and checks
that composition publishes that provider's distinct material value. All 71
launcher tests pass on Linux. The edit path already reparses the manifest, so
no runtime fix was needed; this establishes freshness across edit and reload,
not atomic cross-process profile updates or a packaged Windows launcher run.

Legacy index entries now carry their compiled resourceClaim alongside their
compiled JSON/PNG paths. Fresh composition uses that key both for conflict
grouping and winner-controlled copying; only older cached UI entries reconstruct
it. All 70 launcher tests pass with the rebuilt CLI, including repeated index
owners, boundary slots and actual legacy winner composition. The launcher
still assembles backing-file sets and writes composed profiles; source identity,
atomic snapshots and a unified runtime catalog remain unfinished.

Semantic conflict keys now come from the compiled manifest adapter as
resourceClaims (texture/material/mesh namespaces). Composition reparses its
private snapshots and groups these claims; old cached UI manifests retain a
compatibility reconstruction until refreshed. Tests verify all three namespaces
and the existing fresh-snapshot conflict cases; all 70 launcher tests pass on
Linux with the current staged CLI. Legacy claim discovery and complete catalog
identity/storage unification remain open; this is not Windows runtime evidence.

The compiled manifest adapter now reports backingFiles directly from parsed
texture/mesh entries. Authoritative composition consumes that list when
requesting file dispositions instead of reconstructing references from JSON
tables. Missing manifests explicitly have no semantic backing files. All 70
launcher tests pass with the rebuilt/staged CLI, including mixed texture/mesh
reference output. This is an adapter migration, not a unified resource catalog:
legacy references, semantic conflict keys and the advisory UI remain in JS.

Composition now computes compiled file dispositions once from each validated
private source snapshot, before conflict grouping. Global resource claims and
copy execution consume that same result; the synchronous UI retains its
advisory path-based view. Semantic and legacy reference discovery still live
in the launcher and require migration. All 70 launcher tests pass on Linux,
including a role-versus-path disagreement regression for conflict grouping.

Snapshot opening hardening is in progress: source handles now reject final
symlinks/reparse points and non-regular files before creating an output. Unix
opens are nonblocking so a substituted FIFO cannot wait for a writer. The seven
standalone mod contracts pass on Linux, including new regular-link,
dangling-link, directory and FIFO rejection checks with unchanged byte budget
and no target creation. Windows uses UTF-16 paths and handle attributes; all
seven contracts also pass in the Windows 11 ClangCL VM (4.48 seconds). The
Windows snapshot fixture now also copies between UTF-8 paths containing Polish
characters, verifies bytes and exclusive creation, and checks Unicode target
cleanup on budget failure. All seven contracts pass again on Linux and Windows
(0.66 seconds in the VM). Reparse-point creation remains untested. All 69 launcher tests
pass on Linux after rebuilding and staging the updated CLI. Parent directory
traversal is not pinned; this is not an atomic multi-file snapshot or full race
protection.

Native inventory regression now exercises real directories through depth 8
(accepted) and depth 9 (rejected), plus dangling file symlinks and directory
cycles on Unix. Linux passes all cases; Windows VM passes the directory-depth
and existing Unicode/file-policy cases (0.78 seconds). The Windows fixture
does not create reparse points, so their rejection is not newly proven by this
run. The scan remains susceptible to concurrent filesystem changes between
metadata checks and later file reads; snapshot work is still required.

Launcher inventory traversal is now compiled: `rage-mod-cli --inventory ROOT`
enumerates through SDL and applies the shared file/directory policy before
descending or accepting files. It bounds depth/directories/files/bytes, rejects
symlinks and Windows reparse points, preserves hidden-entry exclusion and emits
sorted accepted paths only after success. Windows root paths use UTF-16 for
attribute checks. The launcher consumes the result instead of walking with
Node fs.readdir/stat. All 69 launcher tests pass after staging the current CLI;
all six standalone SDL/asset contracts pass on Linux and Windows VM, including
an inventory fixture with a Unicode root. Existing/new tests cover forbidden
files, links (Linux), invalid empty directories, hidden entries, ordering and
oversized files. The native scan has an 8 MiB aggregate path budget; it remains
a read-only inventory, not a filesystem-atomic snapshot or source fingerprint.
Resource-claim discovery/profile persistence still need further migration.

Extra GP finale coverage now follows the shared-class rule used by the actual
menu: scenario automation preserves Extra selection but uses
GrandPrixAssetSeries for class-5 assets/records instead of overwriting them
with series 1 every frame. Added class-4 Extra promotion (reused stream 8) and
class-5 Extra ending (stream 10) for all three regions. These six award/audio
cases plus scenario_control and class_progress_rules pass (35.49 seconds).
Both final branches now have regional seeded-award evidence; no physics or
Windows/macOS claim follows from those Linux offscreen tests.

Standard Grand Prix ending now has natural award-entry coverage: class 4 is
finished through the same seeded-progress/finish fixture and production result
flow selects stream 10. The fixture allows 10000 ticks and requires the ending
return scene 33, complete disc-derived 1500-frame timing, positive XA mixer
energy and session PCM. PAL/NTSC-U/NTSC-J all pass (25.39s/20.53s/20.11s).
This covers standard-series final entry, not the extra-series final branch,
stream 9, physics completion, physical sound output or non-Linux platforms.

Promotion award matrix: the natural award fixture now covers classes 0..3 in
both series across PAL/NTSC-U/NTSC-J (24 cases, streams 1..8). All pass with the
CD fade fix: real post-race selection, complete sector-paced movie, positive
movie CD mix energy and nonzero session PCM. Runtime was 101.18 seconds with
four concurrent cases. PAL now also allows 4200 ticks for longer class/prize
paths. This does not cover final-series endings, stream 9 (no recovered natural
selection path), physical audio or Windows/macOS. Earlier-course results and
finish crossings remain explicit smoke fixtures, not physics/input replay.

Class-0 award-entry coverage now runs on all three regions. PAL retains its
2700-tick scenario; NTSC allows 4200 ticks for its longer promotion movie.
Each case checks detected region/base timing, the post-race prize/FMVs/return
path, exact disc-derived stream-1 frames and pacing, positive XA mixer energy
and session PCM. PAL, NTSC-U and NTSC-J all pass locally (9.17s/13.31s/13.58s;
13.59s parallel total). This refreshes regional evidence for the CD fade fix,
not other award classes, final endings, Windows/macOS or physical audio.

Award audio regression found and fixed: tracing now measures the SPU CD mix
energy delta for each XA playback interval under the audio lock. The natural
class-award fixture initially failed with zero movie contribution despite
nonzero whole-session PCM (`class-award-4f61bf737732`). BeginFmv inherited the
race/menu fade's zero CD attenuator. It now cancels that fade and reapplies the
configured CD volume after pausing prior playback, without forcing full volume.
The class-award test now requires nonzero movie CD mix energy, as do direct
FMV tests. All 14 PAL audio/scene/award tests pass after the fix (23.88 seconds);
award evidence is `build/class-award-acdb501f4b85`. This measures the movie's
post-volume CD contribution, not isolated final PCM or audible device quality.
NTSC award cases and Windows/macOS verification remain open.

First natural award-entry regression: smoke-only `hooks.prior_course_wins`
seeds earlier course results while leaving the current course incomplete.
The existing finish hook crosses laps through normal race logic; real results,
prize and class advancement select the movie. `hooks.preserve_fmv` prevents
the test's auto-confirm input from skipping it. The CMake/compiled-checker
`class_award_fmv` fixture passes on PAL class 0: prize scene, FMV, class return,
150 ordered frames at 150 sectors/s, XA start/end and nonzero session PCM.
Evidence: `build/class-award-74501e7b1cc9`. Prior results and finish progress are
synthetic; this is not a physics race. PCM still covers the whole session,
so isolated award audio, other classes/series and NTSC award entry remain open.

Reward-flow automation follow-up: `boot.skip_sequences` no longer injects Start
after a completed race. Scene 5 is shared by opening and reward FMVs, so the
old scene-only condition could skip a class/ending movie during repeat-mode
automation. The smoke build and scenario_control/scenario_after_finish tests
pass (22.66 seconds parallel total). These existing cases verify general
automation, not a completed-class movie; that specific end-to-end test remains
open. The existing smoke finish hook crosses laps through normal race logic
and can be combined with a prior-course progress fixture for that scenario.

Class-award selection coverage now links the real `SelectFmvStream` wrappers
into `advance_grand_prix_class_tests`, replacing mocked BeginClass/EndingFmv.
Every advancing class in both series checks its completed-class stream and
frame count before the live class increments; both final classes select the
ending. Existing invalid-state/progress checks remain. The compiled advance
and FMV request tests pass on Linux. This connects progression to real stream
selection, but BeginFmv remains mocked: natural prize-screen/audio playback
still needs an end-to-end scenario and is not claimed by these unit tests.

The compiled FMV pacing oracle now accepts all 11 retail stream indices, not
only intro/promotion 0 and 5. Every modern_fmv_audio case checks disc-derived
picture sector positions and timing, complete ordered frames, XA completion,
PCM energy, and rejection of a deliberately delayed last picture. Extending
coverage exposed an invalid oracle assumption: PAL ending XA covers 98.026667
seconds while pictures span 100.04 seconds at 150.009996 sectors/s. The pacing
criterion now uses sector distance rather than assumed continuous XA occupancy;
PCM validation remains separate. All 11 cases pass on each local PAL, NTSC-U
and NTSC-J image (21.02s/19.81s/17.91s parallel batches). These are Linux
offscreen direct-stream tests, not natural class-award entry, audible output
quality, hardware audio latency or Windows/macOS evidence.

Broad local checkpoint after default-disc source changes: the complete build
passes. Of 399 tests excluding e2e/endurance/gpu labels, 397 passed, stream_table
skipped for missing default-path data and shipped_config rejected the preserved
local marker_capture=true setting. The committed INI passes the same compiled
policy validator. Rerunning stream_table with its actual disc environment
variables passes on PAL, NTSC-U and NTSC-J. All 68 launcher tests pass. This
build includes the parked local geometry-pack prototype and is not a clean
release build; these results do not replace current Windows/macOS game,
endurance, FMV-audio or display-performance gates.

Default source selection now always uses the selected disc's C importer.
Implicit executable-relative `native-assets` discovery could select a cache
without checking its disc/importer identity and has been removed (including
the macOS relative search). Explicit developer `modern.assets` overrides and
tool `ModernAssetsInitRoot` remain supported. A CMake-driven regression copies
the smoke executable beside an unrelated valid-format cache and requires the
disc importer plus modern GPU initialization. It passes on Linux, as do PAL
and native-world tests exercising explicit cache/mod texture replacement.
This prevents implicit stale-cache selection; source fingerprints and safe
in-session provider switching remain unfinished. Windows/macOS startup has
not been rerun for this change.

The colocated-cache regression now runs for PAL, NTSC-U and NTSC-J using the
same regional disc environment variables as the other integration tests. It
also requires the detected disc region and automatic 50/60 Hz base timing.
All three pass on local real images with Linux offscreen rendering (1.19 seconds
parallel total); missing images are reported as skipped, not successful runs.
This verifies regional startup/source selection, not full FMV or race coverage.

Pack owner discovery now uses the same bounded legacy texture-index parser as
the launcher and runtime, replacing its independent sscanf interpretation.
The entire index is validated before raw assets are opened for edits; invalid
owners, traversal, extra tokens, NULs, oversized lines/files and read failures
abort with a nonzero result. The compiled archive test checks invalid entries
after a valid pending edit and verifies the original bytes remain unchanged.
All five standalone contracts pass on Linux and Windows VM. The patcher still
rereads its input index per asset; this is not protection against concurrent
source edits or a frozen whole-package transaction.

Packed-asset publication now stages each edited raw file with exclusive create,
checks the full write and close, then replaces the original with POSIX rename
or Windows MoveFileEx. Failed staging/publication preserves the original;
pre-existing staging files are not overwritten or cleaned up. Raw read/write
failures return nonzero instead of an apparent successful pack. The compiled
archive fixture verifies a staging collision, unchanged original/reservation,
then successful retry and exact patched bytes. All five standalone contracts
pass on Linux and Windows VM. This is per-file publication, not an all-assets
transaction, power-loss durability guarantee or strict texture-error reporting
(the shared patcher's existing skip-invalid-texture policy remains unchanged).

GPU upload ownership follow-up: the native texture transfer queue now holds
the complete 2048-entry texture cache rather than 256 entries. The old overflow
path waited for GPU idle and released transfers even though their command
buffer had not yet been submitted. Capacity is checked before recording new
copies; overflow no longer retires unsubmitted work. Mip chains exceeding SDL's
32-bit transfer size are rejected before allocation/upload. The smoke target
build and Linux offscreen `native_render_world`, `modern_submit_recovery` and
`modern_region_pal` tests pass. These checks do not exercise a greater-than-256
upload frame or prove pixel correctness; the full GPU ownership stage remains
open.

The runtime now uses the same bounded upload queue exercised by a compiled
fixture: 2048 retained resources, rejected overflow without retirement,
exactly-once drain, cleared slots, repeated drain and reuse. Its capacity is
checked against the texture cache at compile time. The standalone sanitizer
gate passes all four fixtures on Linux; Windows ClangCL Release passes all
five contracts including archive tools. This is CPU ownership coverage, not
a 2048-texture Vulkan stress run or proof of command submission ordering.

Upload retirement is now explicit at the submission boundary: game, render
stage and frame replay call `ModernNativeGpuSubmitted` only after successful
SDL submission. Prepare no longer infers that the previous frame was submitted.
Failed submissions still tear down the speculative renderer cache before reuse;
SDL owns deferred physical destruction after released handles are submitted.
All three targets build and Linux offscreen native-world, submit-recovery and
PAL tests pass. The stage angle sweep was skipped because its configured native
asset cache was absent; this is not new stage image or Windows GPU evidence.

Follow-up image evidence: generated a private PAL CAR/BIG1 test cache using the
existing offline asset-browser extractor (not a new runtime/release dependency),
then reran `render_stage_angles` with `RAGE_PORT_NATIVE_ASSETS` pointing to it.
The test passed without skipping in 5.11 seconds on Linux offscreen: 18 car
rotation sweeps, silhouette checks, rotation-form/full-turn consistency,
byte-identical repeated rendering and track-piece visibility. Generated game
data remains ignored under `build/submission-stage-ArqV5X`. This closes the
missing local stage-image check for explicit upload retirement, not the
compiled extractor migration, long-session GPU stress or Windows image gates.

Current full-game lifecycle evidence after explicit upload retirement:
`modern_repeat_pal` passed in 249.10 seconds after rebuilding `rage-racer`.
Evidence is in `build/regional-races/20260906-102548-11a6f9` with binary/config
hashes. The compiled route driver completed two class-1 Mythical Coast races
through the intervening menus, three presentation restarts, automatic PAL
timing, matching VRAM-cache checks and ordered final GPU/session teardown.
This is Linux offscreen/dummy-audio evidence, not input/physics validation or
automatic detection of flickering textures; NTSC repeat runs have not been
refreshed for this upload-retirement change.

Dependency selection and conflict selection share a bounded NUL-delimited
stdin decoder (8 MiB / 262144 tokens), avoiding Windows command-line limits.
The launcher rejects embedded NULs before encoding. Regression coverage includes
a 128-package dependency chain exceeding 32767 bytes and malformed framing.
The diagnostic argv selection command remains available.

Latest local integration check: full build succeeded; 63 launcher tests and all
seven standalone C contracts passed on Linux. The seven standalone contracts
also passed in the Windows 11 VM (not a full Windows game/Electron run).
The broad Linux suite recorded 395 passes, two skips and one shipping-config
failure caused by the preserved local debug-marker setting. Modern PAL,
NTSC-U and NTSC-J checks passed, as did the PAL two-race/reward/menu lifecycle
check (239.60 seconds). Offscreen/dummy-audio runs do not establish visual
correctness, audible playback or real-display performance.

Import now validates a private copy created through a compiled streaming
copier, then publishes its profile entry; source edits after copying cannot
replace the bytes validated by import. Composition now stages installed packages
and revalidates manifests/claims before dependency and conflict selection; output
uses these same private copies. Compiled resource inventory, source fingerprints
and a runtime provider stack remain incomplete.

Export now uses that compiled copier too, freezes profile metadata at entry,
and validates the generated package JSON against the compiled import schema.
Regression tests cover changed source/profile state, invalid output metadata,
copy failure cleanup and preservation of pre-existing destinations. It retains
unclaimed work-in-progress assets; this is not a runtime-readiness validation or
an atomic filesystem publication protocol. JSON serialization remains in the
launcher. The complete launcher suite passes with 64 tests after this change.

Metadata edits now validate the complete proposed package through the same C
schema before mutating the profile. A bounded `--metadata-stdin` command avoids
temporary files and preserves the file-based import command. UI field checks
remain early diagnostics, not authoritative acceptance; tests demonstrate that
unpaired Unicode surrogates rejected by C leave memory and persisted state
unchanged. Export and editing share the profile-to-package serialization helper.
All 64 launcher tests pass locally; this step has no new Windows GUI evidence.

Startup configuration now has a three-region regression: duplicate classic
renderer entries become modern, the selected disc and composed mod directory
are used, Japanese/international content follows the region, and profile-owned
settings plus unknown INI entries survive configuration generation. Generation
alone does not mutate the source INI. Known launcher settings are taken from
its profile, not imported from arbitrary existing INI values. All 65 launcher
tests pass. Source inspection confirms native importer initialization precedes
modern renderer initialization; this is not a new clean-package startup test.

The package-relative file policy is now shared C code and used before import,
export and composition. Directory traversal, symlink checks and semantic/legacy
resource-claim discovery still live in the launcher; classifying a backing file
does not make it a global override provider.

Directory acceptance now also belongs to this C policy: raw/textures/meshes
roots, safe relative ASCII names and at most eight levels. Inventory validates
even empty directories, which previously could retain unsupported names.
The launcher bounds traversal to 10000 directories in addition to its existing
file/byte limits, then submits the collected directory paths for native checking.
Filesystem traversal and symlink checks remain JavaScript; they have not yet
been replaced with a compiled inventory implementation. All 67 launcher tests
and seven standalone C contracts pass on Linux; the same seven C contracts pass
in the Windows VM. This does not verify the complete Windows launcher.

Legacy texture index lines now share a bounded C parser between the launcher
CLI and `TexturePatchAsset` (game and rage-pack). The launcher no longer carries
its own regex parser; pair existence checks remain there. Entries preserve
order and duplicates, accept ASCII whitespace/CRLF, and reject traversal,
trailing tokens, NULs, out-of-range owners and lines over 510 bytes. The CLI
rejects the whole request on malformed input (partial stdout is discarded by
the process runner); runtime reports and skips malformed lines as before.
The CLI also caps the complete index at 2 MiB. Unicode-only whitespace is no
longer accepted as a launcher-specific extension. All 68 launcher tests, seven
Linux/Windows C contracts and the existing mod-tools texture roundtrip pass.
The existing Python roundtrip has not been removed; its compiled replacement
remains part of the toolchain migration. No new game GPU run was performed.

A compiled `texture_patch` fixture now exercises the production patcher with a
synthetic, valid PNG: 4/8/16-bit edits, an unaligned palette, unchanged repacking,
neighbor nibble/row-padding preservation, owner filtering, malformed/NUL index
entries and out-of-bounds pixel ranges. It compares the entire asset, including
sentinel bytes. Both this fixture and the existing archive roundtrip pass on
Linux. The new test does not yet replace archive extraction/repacking, all PNG
filters or every malformed-sidecar case from the old script, so that script
remains enabled. This new fixture has not yet run on Windows.

Follow-up: the expanded fixture now also covers missing metadata, invalid
depth/dimensions/row storage, missing palettes, excess palette entries and
out-of-bounds palettes, always asserting that rejected input leaves the entire
asset unchanged. A standalone `tests/texture_contract` build runs these same
production sources without SDL or game data. It passed on Linux GCC and in the
Windows 11 VM with ClangCL Release (0.50 seconds). This supersedes the previous
fixture-only Windows evidence gap, not the full-game Windows release gate.

Texture sidecar reading now uses the vendored strict JSON parser instead of
substring searches. Required fields come from the root object and palette fields
from `clut` (the historical flat palette form remains supported). Nested
extension fields cannot shadow texture fields. Duplicate root/palette keys,
trailing data, invalid number types, malformed UTF-8/JSON and files reaching the
4096-byte read limit are rejected before patching. The parser owns no retained
JSON allocations. Regression fixtures cover nested extensions, duplicate keys,
both palette forms and malformed metadata. Game, smoke, pack, stage and replay
targets build with the shared JSON dependency. Linux texture/archive tests and
modern PAL smoke (13.30 seconds) pass; the expanded production patch fixture
also passes on Windows ClangCL Release. This is not a full Windows game rerun.

Texture PNG decoding now receives expected dimensions from validated metadata
and rejects mismatches before pixel allocation/decompression. Asset and palette
bounds are checked before opening the PNG; encoded PNG files are capped at
32 MiB, matching the launcher's PNG validator. Regression fixtures include an
INT_MAX-width image header and an oversized sparse file; rejected inputs leave
the asset unchanged. Linux texture/archive tests and the Windows compiled
texture fixture pass after this change. No frame-rate improvement is claimed;
this bounds avoidable work for invalid mod input, not normal race rendering.

The compiled `mod_archive` test now creates a synthetic three-entry retail
archive, invokes the actual extract/pack executables through SDL process APIs,
checks exported 4/8-bit dimensions and byte-exact untouched repacking, edits one
PNG texel and asserts exactly one expected raw byte changes. A subsequent corrupt
PNG must preserve the last successful edit and unrelated assets. SDL is used
without a window/GPU; no Python or game data is needed by this new test.
Linux `mod_archive`, `texture_patch` and the original `mod_tools` tests pass.
The old script remains: its diagnostic assertions, oversized compressed stream
case and odd-palette archive workflow are not all replaced by this new test.
There is no Windows execution evidence yet for the new archive-process test.

The C archive test now captures combined process diagnostics and verifies clear
messages for wrong-sized and corrupt PNGs, as well as byte preservation in both
cases. It also inserts a byte into the extracted raw asset to make the palette
unaligned, updates its sidecar, paints another pixel and checks the exact packed
result plus untouched neighboring assets. Linux archive/patch/legacy tests pass.
The old script still provides its oversized compressed-stream regression; it
has not been removed, and the C process test still needs Windows execution.

Archive-test migration complete: the C fixture now generates a PNG under 16 KiB
whose stream expands past 1 MiB, rewrites IHDR with valid CRC to the expected
texture size, and asserts the decompression diagnostic plus unchanged assets.
All previous script scenarios now have compiled coverage: exported dimensions,
unchanged repack, isolated edits, wrong-size/corrupt PNG diagnostics, expansion
bounds and unaligned palette edits. Old and new tests passed together on Linux;
both compiled fixtures also passed on Windows 11 ClangCL Release (archive 1.88s),
using the existing guest SDL build. The standalone build exposed and fixed a
missing Windows `<direct.h>` include in rage-extract. Removed only the replaced
`verify_mod_tools.py` script and its CTest registration; `mod_archive` and
`texture_patch` are the compiled gates. Other Python migrations and the broader
architecture/release gates remain incomplete.

The compiled texture/archive contracts now have a standalone CMake driver that
builds SDL and the tools without configuring the game, plus a path-filtered
GitHub Actions matrix for Linux/Windows/macOS. Only required pinned submodules
are fetched. The driver disables X11/Wayland/KMSDRM/audio for these non-graphical
tests, avoiding display-development package requirements. It passed both tests
on Linux and built SDL/tools from the existing guest source in Windows 11,
where both tests passed (2.41s). This verifies the local driver on two systems;
the new workflow has not been pushed/run on hosted CI and macOS is unverified.

Copy roles now also come from C: referenced backing files stay provider-local,
while metadata and unused meshes are omitted from runtime composition without
being removed from the library/export. Reference discovery and resource-key
enumeration remain separate launcher responsibilities to migrate.

The standalone `tests/mod_contract` gate now reuses production sources and
the same compiled tests without SDL/discs. All seven tests passed on Linux and
the local Windows 11/ClangCL VM after the launcher integration. A path-filtered
Windows/Linux/macOS CI workflow is provided; hosted CI and macOS execution are
not established by those local results. This covers mod contracts, not whole
game stability or the packaged launcher shipping contract.

1. **Resource ownership and lifecycle** (in progress)
   - Explicit session, race/content-generation and GPU ownership boundaries.
   - Idempotent teardown and safe failure/retry; no stale cache across repeated
     race/reward/menu/race transitions, including the same track loaded again.
   - Start with the importer's track image snapshot: isolate ownership and
     generation changes from its game/VRAM adapter and test those transitions.
   - Gate: lifecycle unit tests plus repeated real-disc race sequences;
     allocations and borrowed pointers have documented lifetimes.
2. **Unified assets and mods** (in progress; providers not yet unified)
   - Common semantic resolution for original data, generated assets and mods.
   - Versioned manifests, precedence, dependencies/conflict diagnostics and
     invalidation keyed by source/importer/mod identity.
   - Gate: override/conflict/failure tests and clean automatic import startup.
   - Runtime integration checkpoint: a shared allocation-free provider resolver
     now distinguishes missing/ready/error and explicit precedence. Raw asset
     loading uses it for mod-to-retail selection; modern mesh loading and
     resident-only lookup use it for importer/cache selection. Legacy raw
     rejection remains a permitted retail fallback, while a failure in the
     selected mesh source is terminal. Authored mesh replacement still follows
     base resolution. This centralizes selection semantics, not storage or
     identity: session generations, fingerprints, complete mod mesh/material/
     texture layering and a unified resource catalog still need integration.
     Resolver contract tests pass on Linux/Windows; the native loader regression
     and modern PAL/NTSC-U/NTSC-J smoke pass on Linux (14.44s parallel total).
     Smoke, replay and stage targets build. This is not a new Windows runtime
     or visual/performance verification.
   - Material integration: imported/cached definitions and mod/base images now
     use the same provider-selection mechanism. Mod PNG replacement has one
     ownership-transfer path that releases an already generated base image;
     cached pixel I/O remains deferred until a mod image is unavailable.
     Rejected mod PNGs keep the existing base-image fallback policy. Surface
     effects and authored material overrides still run after base resolution.
     Linux tests pass: native_render_world verifies a live GPU draw stream,
     semantic PNG replacement and car paint; modern PAL and material parsing
     regressions pass too. These are not pixel-difference or Windows runtime
     results, and material identity/storage still need the unified catalog.
   - Material lifetime checkpoint: cached sidecar paths no longer point into
     shared global scratch buffers. The public material-loading API requires
     caller-owned path storage and copies paths before releasing sidecar bytes;
     imported material paths acquire the same successful-return lifetime.
     GPU upload keeps that storage on its stack and strips path references when
     caching numeric material properties. Tests cover source mutation, two
     independent materials, rebinding, empty paths and rejected bounds/null
     inputs without damage to existing storage. This does not make all asset
     providers thread-safe or finish immutable presentation snapshots.
     Smoke/replay/stage builds and Linux material, native-world mod/paint and
     modern PAL tests pass (13.70s parallel total); no Windows rerun this step.
   - Material publication is transactional: providers and surface effects build
     a private definition/image, and caller outputs are assigned only after
     owned path copying succeeds. Failure frees intermediate pixels and leaves
     the caller's existing definition, image and path storage unchanged.
     A compiled fault-injection test covers backend failure, path-size failure,
     successful ownership transfer and release counts. The test also passes as
     a standalone C build; Linux material/native-world/PAL regressions pass.
     An ASan/UBSan build could not link because the host sanitizer runtimes are
     missing, so no sanitizer pass is claimed. No Windows rerun for this step.
   - Follow-up verification supersedes that sanitizer gap: the existing Linux
     development container has working runtimes. The standalone material/path,
     transaction and production texture patch fixtures all pass with ASan/UBSan.
     `RAGE_ASSET_SANITIZERS` makes this reproducible in tests/texture_contract;
     the Linux CI matrix now includes that variant (hosted execution pending).
     Material fixtures also join the normal standalone matrix: all four tests,
     including archive roundtrip, pass in Windows 11 Release (1.33s). The image
     value type is split out of modern_assets.h so transactions do not depend on
     the world/cache API. The Linux live native-world regression passes after
     that header split. This is still fixture-scoped, not full-game sanitizer
     coverage or proof of concurrent rendering safety.
   - Provider image results now have an explicit packed-RGBA8 invariant: a
     non-null pixel pointer, nonzero dimensions and an exact overflow-checked
     byte count. The material path checks it before applying surface effects;
     the publication transaction checks it again before exposing the result.
     Tests inject zero dimensions, a short buffer declaration and overflowing
     dimensions and verify cleanup plus unchanged outputs. Three ASan/UBSan
     fixtures pass in the Linux container, all four standalone contracts pass
     in Windows Release, and Linux native-world/PAL smoke passes (14.06s).
3. **Simulation/presentation boundary** (in progress)
   - Complete immutable presentation snapshots; isolate remaining legacy-state
     reads and declare compatibility/VRAM dependencies.
   - Gate: independent main/mirror views, interpolation and regional timing
     tests. No speculative threading before ownership is proven.
4. **Persistent GPU geometry** (in progress; bounded residency prototype parked)
   - Reuse static mesh buffers; update instance/material state separately.
   - Preserve animated UVs, terrain visibility, environmental variants and
     transparent ordering. Expose geometry suitable for future acceleration
     structures without making ray tracing mandatory.
   - Gate: image comparisons plus repeatable CPU/GPU/frame-tail measurements.
5. **Data-driven content** (in progress; built-in regional profiles only)
   - Versioned validated definitions for vehicles, classes, events/rewards,
     FMV sequences and regional profiles; retain original defaults.
   - Gate: retail-equivalent definitions and malformed/extended-content tests.
     New arbitrary tracks also require collision/AI/race-rule support.
6. **Unified regression scenarios** (ongoing across all stages)
   - Repeated races/rewards, all three regions, intro/FMV audio, image probes,
     memory lifetime and performance gates through compiled runners.
   - Add input replays: route driving alone does not test physics/input.
   - Replace existing Python helpers only after equivalent compiled coverage.

## Initial ownership inventory

- `native_asset_importer.c`: session mesh bytes/material keys/bounds; global
  image snapshot and reconstructed track pages keyed by track revision.
- `modern_assets.c`: file cache, index buffers and selected mod manifest;
  currently shuts down the importer too. Ownership is still implicit.
- `modern_native_gpu.c`: device resources, texture lookup/cache, prepared draw
  lists and sky. Track revision already resets texture/sky caches; preserve
  that behavior while replacing hidden ownership, not merely renaming globals.
- `render_world_frame.*`: existing renderer-neutral interpolation boundary to
  extend, not replace with another parallel representation.

Meshes are currently retained across track loads because captured/prepared
frames may still reference them. Do not free them on a scene-ID change without
first defining and testing the last consumer's lifetime.

## Persistent geometry migration boundary

The current `RageNativeDrawVertex` is expanded, world-space presentation data,
not a reusable asset vertex. `RenderBuildNativeDrawsFiltered` rebuilds it for
each camera and `ModernNativeUploadVertices` transfers both lists together.
Keeping that buffer without separating the following dependencies would be
incorrect even when the imported RMESH bytes never change:

| Current operation | Required owner in the persistent path |
| --- | --- |
| Indexed source positions, UVs, colours, authored material flags | Immutable mesh resource |
| Transform, environment light, lighting influence, scroll U | Instance state |
| Material variant, car paint, terrain CLUT selection | Instance/material state |
| Fog colour and reciprocal-depth factor | View state; factor evaluated at the original position |
| Explicit overlay lift toward camera | View-dependent displacement; must not change fog's source position |
| Road-paint lift and terrain boundary snapping | Geometry rules, preserved separately from camera-facing overlays |
| Frustum/backface and authored terrain-quad visibility | View-specific selection, not asset mutation |
| Main/mirror span order | Independent draw lists; preserve transparent ordering |

The next implementation boundary is moving view/instance evaluation out of
stored vertices, then sharing immutable geometry allocations between the two
draw lists. Do not equate reusing a frame's expanded upload with completing
stage 4. CPU-reference tests must remain available while introducing the GPU
path; image comparisons and frame-tail measurements are still required.

## Work log

- Native semantic overrides now use a shared renderer-neutral manifest
  resolver for exact-variant/base precedence and last-assignment precedence.
  Texture and material channels resolve independently; callers request only
  the channel they need, and returned entries retain the manifest owner's
  lifetime rather than borrowing temporary query strings. Linux/Windows tests
  cover precedence, fallback, disabled channels and ownership; Linux provider
  and environment-rendering tests pass. This unifies one selection rule, not
  multiple-mod ordering, dependency resolution or original/generated providers.
- The shipped-configuration policy gate is now compiled C, with fixture tests
  for its four release invariants, disabled boolean spellings, missing values,
  malformed/nonfinite numbers and duplicate settings. Linux and Windows builds,
  self-tests and the committed INI pass. Both old and replacement gates reject
  the preserved local marker_capture=true setting. The obsolete Python checker
  was removed only after this coverage passed. This removes one test dependency,
  not the remaining Python asset-tooling migration.
- The compact renderer completed two class-1 Mythical Coast races in one
  process on each real PAL/NTSC-U/NTSC-J image, with three presentation
  restarts, automatic regional timing, VRAM-cache equality and ordered
  resource/session teardown checks. PAL evidence:
  build/repeated-compact-pal/20260906-071846-38a2f5; NTSC-U/J evidence:
  build/regional-races/20260906-071951-{fea402,01ac4d}. These remain offscreen
  route-driver tests, not physics/input replays or automatic flicker detection.
  The scenarios are registered as modern_repeat_{pal,ntsc_u,ntsc_j}, labelled
  endurance. Completion validation requires the exact race count and the
  intervening menu transitions; four fixture tests cover valid, wrong-count,
  missing-menu and explicit-failure logs.
- Triangle shape evaluation is now shared lazily between flat normals, road
  decal classification and displacement. The raw normal/length stay separate
  from the camera-facing sign; the original flat-normal epsilon and overlay
  zero-length rules are preserved. This is per-triangle CPU preparation, not
  yet persistent metadata. Linux/Windows tests cover reversed winding, cameras
  on either side and exactly in the plane, tiny and degenerate faces, and fog
  at the original position. Four PAL mirror-frame captures remain bit-exact.
- Native rendering now consumes direct 56-byte geometry plus draw-constant
  lighting/environment/shadow-reception uniforms, rather than repeating those
  parameters in 76-byte vertices. Main, mirror and shadow pipelines share the
  compact layout. The CPU-reference builders remain available. Span merging
  compares instance state, and uniforms are updated only on state changes
  within each view. Marker diagnostics read geometry from the compact buffer
  and shading from the owning span.
  The first compact implementation repacked a complete CPU frame during
  upload; its average upload cost rose to 0.3324 ms. Direct compact output
  removes that gather pass: a subsequent 998-frame PAL trace measured 0.1126 ms
  versus 0.1731 ms in the earlier 76-byte trace. These are not interleaved
  benchmarks or whole-frame FPS claims. Frame 1000 uploads 2,112,432 bytes
  instead of 2,866,872 bytes.
  The GPU fixture checks fog and instance outputs in 192 cases on Linux RADV
  and Windows VM SwiftShader. CPU tests compare compact output with the
  expanded oracle for both fog contracts, all five asset sets and changing
  instance state. Four PAL mirror-frame images/world/VRAM captures remain
  byte-identical. Linux regional smoke gates for PAL/NTSC-U/NTSC-J and
  failed-submit recovery pass.
  Windows Release game/smoke builds and the full PAL failed-submit recovery
  scenario also pass on VM SwiftShader, including four submitted-frame
  captures and retained texture history (sampled-vram-f8d334d46e63).
  The slower bounded-residency experiment is separated from the production
  backend; its local algorithm/tests remain available, with the former backend
  preserved under build/indexed-prototype-backend.c. It is not a shipped runtime
  option and does not complete persistent asset-local GPU geometry.
- The shared cached-mesh owner now retains immutable decoded asset-local
  vertices and indices, consumed by existing mesh readers in both the live C
  importer and file-cache path. These arrays contain no instance/view state
  and are released with the asset. Wire bytes remain available for validation
  and serialization; optional allocation failure leaves the validated wire
  reader usable. This adds CPU memory (40 bytes per vertex plus 4 per index),
  not persistent GPU allocation or a measured FPS improvement. Ownership,
  repeated release/re-adoption, wire/decoded equivalence and reopen clearing
  are tested on Linux and Windows. Linux unit/GPU suite: 219 passed; real PAL
  failed-submit recovery passed; four mirror-frame world/VRAM/modern images
  are byte-identical to the expanded reference. Import diagnostics report the
  additional decoded byte count per asset.
- Instance lighting defaults, environment colour and shadow reception are now
  prepared once per visible instance rather than resolved per source vertex.
  Both CPU-reference and GPU builders consume the same explicit instance state;
  this is a CPU-side boundary extraction, not yet an instance GPU buffer or a
  claim of persistent asset-local geometry. Regression cases exercise enabled
  and disabled lighting, negative/zero/fractional/clamped influence, neutral
  versus partially zero environment colours and all five asset sets across
  successive builds. Linux and Windows CPU tests pass. Four PAL mirror frames
  retain byte-identical modern images, world snapshots and sampled VRAM versus
  the expanded-path reference; 192 GPU fog cases also pass on VM SwiftShader.
- Historical bounded-residency experiment (now parked): it retained
  COURSE/TERRAIN vertex identities
  between frames and appends only newly encountered payloads to the GPU's
  resident prefix. Model-bank data is a transient tail, replaced every frame;
  retaining moving opponents had grown the first experiment to 1,681,975
  vertices by frame 1000 despite a stationary player. The CPU arena has a
  two-million-vertex budget, resets before a frame whose worst-case reservation
  exceeds it, and exposes a generation so reset uploads cycle the GPU buffer.
  CPU growth preserves resident indices. Transient overwrites/new residents
  upload after the previous resident prefix; failed submission teardown clears
  all upload counters. Hashing uses complete 32-bit words plus final mixing,
  while exact byte comparison still resolves collisions.
  Tests cover growth/rehash with stable indices, repeated hits, bounded resets,
  and 20 frames of changing transient vertices without resident growth. Linux
  unit and indexed submission-recovery/history tests passed. PAL frame
  1000..1003 world/VRAM/PPM files match the expanded reference byte-for-byte
  (build/geometry-selected-ab/sampled-vram-c5e8142d31d2).
  At frame 1000 the selected arena has 40,120 resident vertices and 16,404
  transient vertices; uploads are 1,246,704 vertex bytes + 150,888 index bytes,
  versus 2,866,872 expanded vertex bytes. Mean prepare_ms over 999 samples:
  expanded 1.1565, retained 1.7862 (p95 1.190/1.877). Evidence:
  build/geometry-selected-performance/{expanded,retained}.log. This improves
  the first indexed prototype but still regresses CPU preparation, so default
  rendering is unchanged. The next bottleneck is per-frame CPU transformation
  and payload lookup; true asset-local geometry with separate instance updates,
  driving/region/image gates and Windows validation remain outstanding.
- Experimental RAGE_PORT_NATIVE_INDEXED=1 packs main/mirror vertices into a
  shared GPU vertex buffer with ordered 32-bit indices. Main, mirror and
  shadow passes use indexed draws; span ordering and CPU diagnostic vertices
  remain unchanged. Exact 76-byte identity includes all attributes; no
  position-only welding. The owned pack grows geometrically, clears published
  counts on failure and is released with presentation resources. Allocation
  failure retains the existing expanded modern path, never classic rendering.
  Unit coverage includes ordered reconstruction, each attribute byte, growth,
  invalid input and release/retry. Real PAL frames 1000..1003 have identical
  world/scene/VRAM/final-image files against the same binary's expanded path,
  with an active mirror (build/geometry-pack-reference/sampled-vram-4571dab9d932
  and build/geometry-pack-final/sampled-vram-fbba2f97d4ca). Indexed-mode canceled
  submission recovery/history also passed on Linux.
  Frame 1000: 37,722 expanded vertices become 21,351 unique vertices and
  37,722 indices, reducing upload from 2,866,872 to 1,773,564 bytes (~38%).
  However, rebuilding the hash pack each frame is too expensive: one paired
  offscreen stationary PAL run measured mean prepare_ms 1.2019 expanded vs
  3.2327 indexed over 999 samples each (p95 1.354/3.358 ms). Logs:
  build/geometry-pack-performance/{expanded,indexed}.log. Thus the prototype
  remains opt-in, not a default performance improvement. Next work must retain
  immutable geometry rather than repack every frame, separating dynamic
  updates and preserving resource-generation invalidation. Windows validation
  and driving/image/performance gates remain outstanding for this prototype.
- Submission ownership prerequisite for geometry reuse: ModernRender previously
  ignored SDL_SubmitGPUCommandBuffer's result, then published success despite
  speculative upload/cache/history state. It now reports failure, destroys
  the presentation resource generation (including native texture/upload state
  and retained history), and lets the next frame rebuild. Callers do not
  advance the rendered-frame/pacer state or publish a stale diagnostic image;
  skip_present prevents the failed modern draw from displaying compat 3D.
  RAGE_PORT_MODERN_FAIL_SUBMIT_FRAME injects one canceled command at a selected
  frame, without claiming real device-loss recovery. modern_submit_recovery
  injects frame 300 at logic-rate presentation and verifies later sampled VRAM and complete retained
  history against real PAL data; Linux recovery/history tests passed 2/2.
  A separate frame-499 injection produced four final PPMs byte-identical to
  the no-failure baseline (build/submit-recovery/sampled-vram-7540e0b49db0).
  An initial vsync test could skip the exact injected frame and correctly
  failed its injection assertion; logic-rate scheduling removes that source
  of test nondeterminism. Windows Release game/smoke builds and the complete
  recovery/history scenario also passed on the VM's Vulkan/SwiftShader with
  the real PAL Track 01 BIN (C:/rage-perf-results/sampled-vram-d307d5b8ad02;
  submit-recovery-result.txt: test_exit=0). Initial Windows harness failures
  were path assumptions, not missing captures: Windows state uses APPDATA,
  not XDG_STATE_HOME. The harness now isolates APPDATA and uses one platform
  marker root for all checks. This is canceled-command recovery, not actual
  device removal; persistent buffer reuse remains pending.
- Fog migration checkpoint: later PAL markers (1000..1003) now also cover an
  active native mirror. Frame 1000 has 24,660 main vertices/1,060 spans and
  13,062 mirror vertices/326 spans. CPU/GPU world, scene, sampled VRAM and
  final PPM files match byte-for-byte for all four frames. Evidence:
  build/fog-mirror-gpu/sampled-vram-6aaae674250b and
  build/fog-mirror-cpu/sampled-vram-2b95d2170e3b. The marker harness accepts
  MARKER_FRAME (default 500) and runs 150 additional frames for capture flush.
  Latest Windows Release game build and 192-case shader probe passed; this
  Windows check uses SwiftShader, not hardware GPU performance or Metal.
  Four Linux offscreen logic-rate runs (GPU/CPU/CPU/GPU, no markers/history)
  measured prepare_ms over 999 samples in frames 1000..1999 each. Means:
  1.1598/1.1512/1.1558/1.1652 ms; p95: 1.190/1.181/1.186/1.199 ms.
  Logs: build/fog-performance-ab/{gpu1,cpu1,cpu2,gpu2}.log. There is no
  demonstrated speedup: GPU-fog preparation was about 0.009 ms slower on
  average in this stationary scene. The change removes a view dependency
  for persistent geometry; it does not implement buffer reuse or prove
  driving frame-time improvement. Wider regional/track/performance gates
  remain part of the overall migration.
- Added opt-in RAGE_PORT_NATIVE_CPU_FOG=1 for whole-frame CPU/GPU fog A/B;
  unset/default uses GPU fog. The reference selects the existing CPU builder
  and passes its fog colour/weight through the shader. Both modes are covered
  by native_fog_gpu (192 cases total on Linux; the earlier 96-case GPU-only
  version was verified on Windows). With video.fps=logic, two PAL smoke runs
  produced byte-identical scene snapshots, world snapshots, sampled VRAM and
  final modern PPMs for all four marker frames 500..503. Prepared frame 500
  has 24,660 vertices/1,060 spans and is complete, but no active mirror.
  Evidence: build/fog-ab-final-gpu/sampled-vram-9f60c42302a3 and
  build/fog-ab-final-cpu/sampled-vram-efd2456a241d. This establishes one real
  scene's full-frame equivalence, not all tracks/regions, mirror rendering or
  performance. Latest A/B shader Windows validation is recorded above.
- Added native_fog_gpu, a compiled GPU readback test using the production
  native vertex shader and a minimal fog-output probe fragment shader. It
  checks all 64 pixels against the CPU fog reference in 96 cases: two camera
  translations/colours, valid/disabled/invalid ranges, fog on/off, and eight
  depths spanning both boundaries. Raster positions deliberately differ from
  fog source positions, exercising the displaced-overlay contract. Linux RADV
  and Windows VM Vulkan/SwiftShader both passed within one RGBA8 UNORM step;
  Windows Release game build and CPU mesh tests passed too. Windows logs:
  C:\rage-perf-results\native-fog-gpu-{build,test,result}.log/txt and
  native-fog-cpu-test.log. CPU mesh tests now use the repository's strict
  warning flags, avoiding clang-cl's different interpretation of bare -Wall.
  This proves the isolated shader output, not full-scene image equivalence,
  varying-depth triangle interpolation, Metal runtime or a frame-time gain.
- In-progress GPU fog migration: the native GPU builder now stores original
  world position plus an enable flag in the existing fog attribute; the vertex
  shader evaluates reciprocal-depth fog using per-view uniforms. No vertex
  size increase. Camera-facing overlay displacement deliberately leaves that
  original position unchanged. CPU-reference builders keep their previous
  colour/weight output. Both SPIR-V and Metal sources were regenerated from
  GLSL (local SPIRV-Cross 83fa691, container glslangValidator).
  Linux render_mesh_build, environment_provider and real PAL history passed
  (3/3); build log build/gpu-fog-build.log. Test additions verify shared fog
  source coordinates, enable/disable and preserved displaced geometry/UVs.
  Cross-run marker images are NOT an equivalence proof: their world/scene
  snapshots differ. Full-scene image comparison and performance measurements
  remain required before shipping this change; isolated GPU readback and
  Windows validation are recorded above.
- Added a shared-asset/two-view reference test for the geometry migration.
  It builds main/rear/main using the same RMESH and instance, with distinct
  cameras, fog colours and aspect ratios. It verifies reciprocal-depth fog,
  identical scroll UVs, opposite camera-facing overlay displacements, fog
  computed before that displacement, repeatable main output and unmodified
  source bytes/instance. Linux render_mesh_build passed. This is CPU-reference
  coverage, not a GPU image comparison or a performance improvement.
- Native world completeness now checks resident meshes, matching the main and
  mirror builders. It no longer retries a failed preparation after geometry was
  built and therefore cannot mark a newly loaded, undrawn mesh as complete.
  Cache fault-injection coverage verifies three resident consumers leave a
  failed read untouched, and a later explicit preparation can recover. This
  exercises cache semantics, not an injected live importer failure. Linux
  cache/provider/history tests passed (3/3), including real PAL history; Windows
  Release game build and cache test passed. Linux build log:
  build/resident-completeness-build.log. Material loading and the larger
  immutable-frame/persistent-GPU migration remain outstanding.
- Separated resident mesh lookup from load/import. RuntimeMeshCachePeek and
  NativeAssetImporterPeek do not read files or game geometry; native main/mirror
  draw builders now use ModernAssetsResidentMeshLookup after explicit WarmWorld.
  The existing loading lookup remains available for callers that deliberately
  prepare assets. Tests assert misses do not call I/O or change counts, resident
  identity is preserved, set/key mismatches stay missing, and teardown removes
  visibility. Linux cache test, file-provider GPU test and real PAL retained
  history scenario passed; build evidence
  build/environment-index-Dgm4lr/resident-lookup-build.log. This isolates the
  draw-builder lookup, not the preceding loading phase or material/VRAM reads.
  Windows Release game/cache-test build and the cache test also passed
  (C:\rage-perf-results\mesh-owner-result.txt: build_exit=0, test_exit=0).
- Diagnostic history now deep-copies the actual prepared render world alongside
  its compatibility scene and retained texture generation. World copies own
  instance arrays, preserve previous state on allocation/input failure, and
  support alias-safe replacement. Unit tests check independence, self-copy,
  invalid capacity and teardown; history integration requires world sidecars
  for every retained frame. Pre-commit full Linux build, all 217 unit/GPU tests,
  render_world_snapshot and retained_texture_history passed (3.63 seconds for
  history). Evidence: build/environment-index-Dgm4lr/precommit-*.log.
  Mesh/material resource bundles and independent complete scene replay remain
  incomplete; a copied world does not itself own referenced asset IDs. Latest
  world-history copy changes have not yet had a Windows build/runtime check.
- Strengthened retained_texture_history beyond file counts: every bank must
  have a matching captured scene filename and all three banks for that exact
  frame/generation; SHA-256 must agree for a given generation/bank across history
  entries. Registered Linux test passed in 3.62 seconds after rebuilding.
  Windows Release game/replay builds and existing rmesh_index test passed with
  the history integration (guest environment-index build/result logs). This is
  Windows compile/link coverage, not a Windows history-capture run. Filename
  association/hash stability do not prove full scene-resource replay equivalence.
- Connected retained importer generations to the 16-slot diagnostic frame
  history. Slots retain only an existing generation matching the prepared GPU
  cache revision, without initiating a live capture; overwrite/history teardown
  releases references. M emits base and two bank raw files with frame/generation
  identity. This supplies retained importer inputs, not complete scene replay.
  Added retained_texture_history CTest, checking 48 full 1 MiB bank dumps plus
  the existing four aligned GPU-VRAM markers. Linux actual PAL integration
  passed at build/sampled-vram-3cb55d1e78ed/. It exercises one generation;
  cross-generation survival is covered by the ownership unit test, not yet an
  end-to-end history transition. Offline file providers return no importer
  generation. Windows build/runtime verification of this integration is pending.
- Expanded generation ownership regression: retain two references, fail the
  replacement read, verify old bank pixels through both references, drop one
  reference, retry successfully, release the owner and verify the final retained
  bank. Invalid page/null handle checks are included. Linux snapshot test passed
  in 0.19 seconds. Windows Release game build and expanded snapshot test also
  passed with the opaque-generation implementation (both statuses explicitly
  checked in C:/rage-perf-results/snapshot-result.txt). Frame-history integration
  and threaded ownership remain incomplete; no whole-stage completion claimed.
- Track snapshots now own an opaque reference-counted generation. Explicit
  single-threaded Retain/Release handles keep published base/bank pixels alive
  across owner replacement and teardown; unretained generations can still reuse
  buffers. The live material decoder retains its input generation around decode.
  Tests retain an old bank, load a distinct generation, destroy the owner and
  verify old pixels before releasing the handle. Linux snapshot test and PAL
  marker scenario passed (0.18/3.06 seconds), build evidence
  build/environment-index-Dgm4lr/retained-generation-build.log. Frame history
  and renderer snapshots do not yet retain these handles; this is the ownership
  mechanism and first consumer, not complete cross-generation frame isolation.
  Reference counts are not atomic and no threaded access is supported. Windows
  validation of the opaque-generation change remains pending.
- Immutable-bank change passed Windows Release game build and the 100-generation
  track_texture_snapshot test; verified both build_exit=0 and test_exit=0 in
  C:/rage-perf-results/snapshot-result.txt (helper itself returns build status).
  NTSC-U/J real-disc short modern scenarios also passed four aligned marker
  captures each on Linux: build/environment-index-Dgm4lr/immutable-banks-{u,j}.log.
  Together with prior PAL, these check integrated startup/capture for all three
  regions, not visual equivalence of bank-dependent track textures or complete
  repeated races. Cross-generation frame resource retention remains pending.
- Began immutable texture views within a content generation: track snapshot
  bank selection now returns one of two complete VRAM images rather than
  copying bank rows over the shared base image. Existing borrowed bank views
  remain unchanged across selections/reacquisition of the same generation;
  the raw snapshot is preserved too. Cost is 3 MiB total versus 1.4375 MiB
  previously per fully populated owner (+1.5625 MiB), while each bank selection
  eliminates a 224 KiB row copy. Tests retain both bank pointers, switch banks
  and check every pixel plus the original base image across 100 generations.
  Linux unit test and real PAL sampled-marker scenario passed (0.18/3.05 sec),
  build evidence build/environment-index-Dgm4lr/immutable-banks-build.log.
  Generation replacement still reuses buffers and invalidates prior borrows;
  frame-owned cross-generation retention and removal of live importer reads
  remain necessary. Windows and repeated-race checks of this change are pending.
- Verified ImportWriteFinish on Windows: Release game build and the expanded
  native_mesh_writer test passed (guest mesh-writer build/test/result logs).
  Linux full default build then passed all 217 unit/GPU-labeled tests, including
  214 unit and three GPU tests, with offscreen Vulkan/dummy audio. Evidence:
  build/environment-index-Dgm4lr/finalize-full-build.log and
  finalize-unit-gpu.log. This does not supersede the previously recorded local
  shipped_config failure or missing full-cache silhouette test, and does not
  mark any whole architecture stage complete.
- Moved second-pass completion into ImportWriteFinish, used by the importer
  before publishing/adopting bytes. Tests now inject increased/decreased mesh
  counts, incomplete vertex/index counts and an invalid current submesh; all
  must reject without touching endpoints. Positive coverage checks trailing
  empty submesh endpoints and repeated finalization, and the existing decoded
  face fixtures now finalize through production code. Removed the last unused
  private integer writer from the importer. Linux new test/build and PAL marker
  scenario passed (3.17 seconds); build evidence in
  build/environment-index-Dgm4lr/writer-finish-build.log. These are direct writer
  fault cases, not concurrent mutation of game-owned source arrays. Windows
  validation of this finalization change remains pending.
- Added positive face-writer round trips through RuntimeMeshOpen/Vertex/Index:
  untextured, scrolling course and environment/near-only terrain with negative
  depth bias. Independent expected values assert PS1-to-native Y/Z signs,
  supplied/default normals, RGBA, half-texel UVs, triangle winding and encoded
  material words. Linux native_mesh_writer passed. Windows Release game build
  and expanded native_mesh_writer test also passed after syncing the extracted
  module/build wiring: C:/rage-perf-results/mesh-writer-{build.log,test.log,
  result.txt}. This closes compiled Windows validation of the extraction,
  not complete source validation, cache export or Windows gameplay coverage.
- Extracted the production face writer and its private data types into
  native_mesh_writer.[ch]; the live importer and compiled native_mesh_writer
  unit test call the same code, without a GPU/game loop in the unit fixture.
  Eight injected cases check mesh overflow/order, vertex/index capacity and
  counter overflow, an unseen textured material, and an extra face after an
  exact-fit write. Rejections must preserve the entire destination and writer
  cursors/counts. First extraction build exposed a missing direct SVec include;
  added game/vector.h explicitly. Linux smoke build, new test and actual PAL
  sampled-marker scenario passed (3.23 seconds for the latter). Evidence build:
  build/environment-index-Dgm4lr/writer-extract-build.log. This exercises face
  rejection branches, not source-memory races or the final mesh-count mismatch
  in the outer two-pass driver. Windows validation of this extraction is pending.
- Hardened the two-pass live mesh writer: each face checks mesh ordering and
  vertex/index capacity before touching output, rejects textured faces whose
  material was absent from the scan, and finalization rejects a changed mesh
  count before filling trailing offsets. Offset address arithmetic uses size_t.
  Previously count mismatches were checked only after writing. This bounds the
  destination; it does not validate every source pointer or make concurrent
  source mutation supported. Fault injection between scan and write remains
  missing, so runtime success is not proof of those rejection branches.
  Linux smoke build and real-disc sampled-marker scenarios passed for PAL/U/J:
  build/environment-index-Dgm4lr/import-write-bounds-{build,runtime}.log and
  import-bounds-{u,j}.log. These are short scenarios, not new complete NTSC races.
- After the shared RMESH header/layout/vertex changes, the rebuilt Linux game
  completed two actual PAL class-1 Mythical Coast races with the intervening
  finish/repeat flow, three presentation restarts, automatic PAL timing,
  matching VRAM cache oracle and checked GPU-before-asset session teardown.
  Evidence: build/performance-drive/20260905-173257-7e3d69/ (result.txt records
  executable/config hashes). This used offscreen Vulkan and route autopilot;
  it is not an input/physics replay, pixel-equivalence test, physical-display
  performance result or new NTSC/Windows repeated-race run.
- Added RuntimeMeshEncodeHeader and integrated it into the live importer,
  sharing version/magic/count encoding with the format implementation. It
  requires the complete declared buffer capacity, writes only the header, and
  leaves all bytes unchanged on failure. Payload population remains the
  caller's responsibility and RuntimeMeshOpen still validates before adoption.
  Tests assert independent header bytes, exact-capacity rejection, unchanged
  payload, rejection of uninitialized payload and a valid empty mesh.
  Linux smoke build, rmesh test and PAL sampled_vram_marker passed
  (mesh-header-build.log, mesh-header-runtime.log under
  build/environment-index-Dgm4lr/). Windows game/replay build and rmesh test
  passed with both layout and header changes; guest vertex-codec build/test
  logs now contain this newer result. No Windows gameplay rerun is claimed.
- Shared RuntimeMeshLayout now computes checked offsets and total wire size for
  both the RMESH reader and live C importer. Removed the importer's duplicate
  size arithmetic. Layout calculation does not allocate or validate content;
  the reader still validates ranges, finite vertices and indices. The offset
  table reader uses size_t rather than a uint32_t loop/multiplication that could
  wrap for huge representable 64-bit buffers. Tests cover the known 216-byte
  fixture, empty layout, null output and maximum uint32 counts without huge
  allocation. Linux smoke build and rmesh test passed in
  build/environment-index-Dgm4lr/mesh-layout-build.log. The test contains a
  32-bit overflow expectation, but this run used a 64-bit host; no 32-bit
  execution is claimed. Complete cache export still remains pending.
- Export-path audit: rage-extract writes raw assets and decoded images but not
  a complete runtime mesh/material cache. The live C importer emits RMESH bytes
  in memory; the old assetbrowser Python pipeline still supplies full cache
  export. It remains in place pending a compiled equivalent and regression
  coverage. No launcher startup/export workaround was introduced.
- Began sharing the native format writer: RuntimeVertexEncode in rage-rmesh
  emits bounded little-endian 40-byte vertex records, preserves all material
  metadata and rejects non-finite fields without modifying the destination.
  The production C importer now uses it instead of its private field writer;
  the reader also explicitly decodes little-endian float bits. Tests compare
  known wire bytes and the existing fixture, size guards, all eight non-finite
  floating fields and UINT32_MAX material. Linux game/smoke builds and rmesh
  test passed (build/environment-index-Dgm4lr/vertex-codec-build.log).
  This is an integrated format primitive, not the missing complete cache
  exporter, model editing tool or persistent GPU geometry implementation.
- Vertex codec integration also passed the actual PAL sampled_vram_marker
  scenario (3.23 seconds, vertex-codec-runtime.log in the same evidence root).
  Added encode/open/decode round trips checking every field and material values
  for untextured, maximum index, UV scroll, near-only terrain, environment CLUT
  and negative depth-bias bits. Linux rmesh and rebuilt render_mesh_build tests
  passed; the latter retains semantic flag handling coverage after decoding.
  Windows Release game/replay build and expanded rmesh test passed, with guest
  evidence C:/rage-perf-results/vertex-codec-{build.log,test.log,result.txt}.
  These checks preserve format/flag semantics, not pixel-equivalent gameplay
  across every region, a big-endian runtime test or exporter completion.
- Environment asset indices now share counted parsing and relative-path
  validation with the native asset boundary. The file provider validates the
  complete optional index before initializing its mesh cache, rejecting duplicate
  keys, invalid/overflowing dimensions, embedded NULs and malformed records with
  a line diagnostic. Sky lookup no longer uses a fixed-size sscanf line buffer.
  The legacy headerless format and missing optional index remain supported.
- Optional environment-index absence now requires successful directory
  enumeration. Existing non-file entries and read failures reject the root,
  instead of silently producing an asset cache without environment data.
  SDL paths remain UTF-8; no errno guessing or parsing translated SDL errors.
  This is startup validation, not an atomic filesystem snapshot or sandbox.
- Added reusable compiled-toolchain integration script
  tests/render/verify_environment_index.cmake, parameterized by replay binary,
  captured world snapshot and output root. Linux offscreen Vulkan passed absent,
  empty, directory and malformed index cases in
  build/environment-provider-762b14e3f55a/. Permission-denied reads and concurrent
  filesystem mutation are not injected by this fixture. It needs a supplied
  snapshot in its initial version.
- Removed that local-snapshot dependency: environment_fixture now writes a
  synthetic camera/world snapshot and 4x4 RGBA texture in C. Registered
  environment_provider with CTest (GPU label), covering missing, empty,
  directory, malformed and valid indices. The valid case must report the
  panorama loaded at 4x4 as well as produce an output image. Linux offscreen
  Vulkan passed all five cases in 0.45 seconds. This is an asset-provider
  regression requiring a working GPU backend, not a retail-image comparison;
  Windows Release build and all five cases also passed through Vulkan/SwiftShader
  in an interactive limited-user task (3.13 seconds). Evidence on the guest:
  C:/rage-perf-results/environment-provider-{build.log,test.log,result.txt}.
  The completed temporary task RageEnvironmentProvider20260905 was removed;
  unrelated tasks and the running VM were preserved. Software Vulkan is not
  evidence for vendor-driver performance or visual retail-game equivalence.
- Broader Linux check initially reported missing unit-test executables because
  previous builds selected individual targets. A full default build succeeded,
  followed by all 213 unit-labeled tests passing. Evidence:
  build/environment-index-Dgm4lr/{full-build.log,unit-suite-built.log}.
  This is the unit-labeled set, not the full regional/e2e acceptance suite.
- Linux's three GPU-labeled tests also passed: environment_provider,
  presentation_device and composite_gpu (gpu-suite.log in the same directory).
  The functional-labeled sweep was not fully green: 161 passed, two skipped,
  and shipped_config failed because the user's local rage-port.ini intentionally
  enables marker_capture. That configuration was preserved; the release-policy
  assertion was not relaxed or bypassed (functional-suite.log).
- The skipped stream_table test was rerun with explicit existing disc paths;
  PAL plus NTSC-U and then NTSC-J passed (stream-pal-u.log, stream-j.log).
  PAL/U compare known offsets/frame counts; J currently checks table shape and
  successful derivation, not an independent full Japanese reference table.
  render_stage_angles still lacks its prebuilt full native asset cache; the
  synthetic sky fixture is not a replacement for the car/track silhouette sweep.
- Expanded rmesh_index tests cover 200 descending environment keys (allocation
  growth/sorting), earliest duplicate row independent of key order, CR/CRLF/LF,
  UINT32_MAX keys, maximum dimensions and counted input boundaries. Linux CTest
  and Windows Release tests passed; Windows game/replay builds also passed.
  Guest evidence: C:/rage-perf-results/environment-index-{result.txt,build.log,
  test.log}. Running the local helper required a process-local execution-policy
  override; the VM's persistent policy was not changed.
- File-provider GPU check in build/environment-index-Dgm4lr/ rendered a valid
  synthetic 1024x512 environment through rage-frame-replay and rejected duplicate
  and oversized entries at line 2. This uses a captured VRAM sheet as test pixels,
  not an authored panorama: it proves provider acceptance/rejection and drawing,
  not visual equivalence, live-importer coverage or complete asset unification.
- Extracted `track_texture_snapshot.*` from the live importer. It owns snapshot
  and bank buffers explicitly, borrows source pointers only during acquisition,
  reuses allocations across generations, and invalidates old content before a
  new-generation read. Teardown is idempotent. Game globals/VRAM calls remain
  confined to the importer adapter; no standalone tool must link them merely
  to test bank reconstruction.
- The C lifecycle test passes 100 generations, independently owned snapshots,
  both banks with mixed resident/shadow rows, undersized source rejection,
  injected read failure/retry, repeated release and same-revision reinitialization.
- Linux game build and 15 selected tests passed. This is a first integrated
  slice, not completion of stage 1: mesh/GPU lifetimes and top-level session/
  race contexts still need migration. The production PS1 read adapter retains
  its existing synchronous behavior; the injected read failure tests the new
  module contract, not newly implemented GPU read-error reporting.
- Windows Release game build and the same lifecycle unit test also passed
  (both exit 0): `build/autopilot-check/windows-snapshot-results/`. This is
  compiled portability coverage, not another Windows full gameplay run.
- The post-refactor PAL class-1 Mythical Coast scenario completed two actual
  races in one process, through the intervening result/reward/repeat sequence:
  `build/performance-drive/20260905-141836-d1f23f/`, exit 0 and
  `autopilot result=complete races=2`. The VRAM cache oracle reported matches
  and no mismatches. This validates the integrated repeat-load path, not every
  visual pixel, audible FMV, other regions or physics/input replay.
- Unified mesh byte/bounds adoption and release in `RuntimeCachedMeshAdopt` /
  `RuntimeCachedMeshRelease`, used by both the live importer and file cache.
  Entries retain the actual provider's release callback/context, rather than
  relying on whichever provider is configured when teardown happens. Rejected
  bytes remain caller-owned; occupied entries cannot be replaced under borrowed
  views. Meshes remain session-resident; no race-time eviction was introduced.
- Ownership tests cover malformed import, occupied-entry rejection, heap-backed
  data released exactly once, borrowed bytes, repeated release and provider
  changes. Linux build and nine selected tests passed. Windows Release game and
  mesh-cache test passed (exit 0): `build/autopilot-check/windows-mesh-owner-results/`.
  A post-change PAL class-1 two-lap route completed with the VRAM oracle enabled:
  `build/performance-drive/20260905-142640-78f23d/`. This is not another complete
  two-race reward/restart test; that preceding integration run predates this slice.
- Teardown audit found that the normal game entry point returns after MainLoop
  without calling ModernAssetsShutdown; the OS currently recovers session mesh
  allocations at process exit. Therefore the route completion above does NOT
  validate explicit whole-session teardown. Next: ordered renderer/session
  shutdown, with GPU consumers retired before releasing borrowed mesh data.
- Added `ModernShutdown`: explicitly called after the game/smoke loop (after
  smoke dumps) and registered once with atexit for failure/exit paths. It waits
  for GPU work, releases modern resources, restores chained hooks, then releases
  session assets. A shutdown-needed guard makes the explicit + atexit pair
  idempotent. The existing backend overlay-destroy hook retires only device
  resources, preserving the content session for device recreation.
- PSY-Z now submits its pending command buffer and waits before invoking device
  destroy callbacks. The modern renderer also invalidates its borrowed VRAM
  snapshot cache when resources are retired; a unit test recaptures the same
  frame number after reset instead of returning a stale GPU handle.
- Six targeted Linux tests passed. PAL normal-route exit passed the lifecycle
  harness gate: `build/performance-drive/20260905-143410-d445c1/`. One resource
  destruction precedes one successful session shutdown. A separate real-window
  WM_DELETE_WINDOW request during native gameplay also exited 0 with the same
  ordered cleanup: `build/autopilot-check/window-close-YBvkUv/`. The local C
  request utility checked the exact target PID before sending the close event.
- Windows Release game and VRAM snapshot/reset test passed after correcting
  that test's existing Unix warning flags for ClangCL (`/W4 /WX`):
  `build/autopilot-check/windows-shutdown-results/`. Windows interactive closing
  was not repeated in this slice. Whole-session in-process restart, failure
  injection around GPU allocation and remaining global state still need tests;
  this does not mark stage 1 or the overall roadmap complete.
- Fixed late attachment to an already-created GPU with a non-initializing
  PSY-Z presentation-device query. Registering an init hook alone would otherwise
  wait for an event that had already happened. The query borrows device/window
  handles through the destroy callback and clears output pointers on failure.
- Split presentation detachment from content-session teardown and added
  `ModernRestartPresentation`. It retains mesh owners and assets while recreating
  presentation resources and hooks. It does not reset game/disc state or start
  a new content session. The opt-in harness `RESTARTS=3` exercises this boundary
  only after submitted GPU work and before the next swapchain command buffer.
- PAL route `build/performance-drive/20260905-144349-aa71f2/` completed with
  three successful presentation restarts, four resource generations, retained
  mesh pointer identity/count and final ordered session cleanup. VRAM oracle
  comparisons matched. No pixel-exact image comparison is claimed for restart.
- Added the compiled `presentation_device` GPU test: initialization, late query,
  invalid outputs, destroy callback lifetime and two create/destroy cycles in
  one process. Seven selected Linux tests passed, including this actual GPU
  test (not skipped). Windows Release build and the device test also passed
  with process-local SwiftShader: `build/autopilot-check/windows-device-results/`.
  The temporary Windows scheduled task was removed after completion.
- Full game-session restart remains unproven: `InitNativeGameData`, scene/input/
  audio state and their reset contracts still need explicit ownership analysis.
  Do not describe successful presentation/device restart as proof of full game
  reset or completion of the architectural migration.
- Audio teardown audit found a concrete dependency-order bug in PSY-Z:
  the PCM dump and mixer mutex were released before the running SDL stream.
  Destroy now quiesces the stream first, then releases callback dependencies;
  init failures (including resume failure) use the same idempotent cleanup.
  The SDL implementation was inspected read-only; no SDL sources were edited.
- SDL_GPU platform shutdown now explicitly destroys the PSY-Z audio owner
  before SDL_Quit, avoiding dangling stream state on later initialization.
  This preserves SPU/disc/sequence state; it is not a whole-session reset.
  Lifecycle calls must be serialized on the main thread, outside AudioLock.
- Added compiled `audio_lifecycle`: a real missing-backend failure followed by
  retry with SDL dummy audio, 40 init/pause/resume/running-destroy cycles,
  repeated destroy, and checks that sample production resumes/stops correctly.
  `presentation_device` now also checks audio across two actual GPU/platform
  reset cycles, including idempotent destruction after backend cleanup.
- Linux game build and seven selected tests passed (audio_lifecycle,
  presentation_device, fmv_audio, audio_settings, modern_vram_snapshot,
  rmesh_cache, track_texture_snapshot). Initial test setup needed rebuilding
  two absent test binaries and reapplying the dummy-driver hint after SDL_Quit.
  This slice has no Windows rerun, audible-output validation, allocator/mutex/
  stream-open failure injection or sanitizer evidence. The alternate SDL_GL
  shutdown path and normal-loop audio shutdown still need lifecycle review.
- Follow-up: normal game/smoke loop completion now explicitly stops audio
  (after smoke diagnostics), SDL_GL also releases its audio owner before
  SDL_Quit, and the SDL audio backend registers one idempotent atexit cleanup
  for early returns. No launcher UI/startup selection flow was changed.
- Added `audio_exit_cleanup`: leaves real dummy playback active when main
  returns, then an earlier-registered atexit observer verifies sample production
  has stopped after backend cleanup. Five targeted Linux tests passed, including
  both audio lifecycle tests and the GPU/platform reset fixture.
- Windows Release game build and both compiled audio lifecycle tests passed
  (exit 0): `build/autopilot-check/windows-audio-results/`. These use SDL dummy,
  not audible hardware. SDL_GL remains source-reviewed, not built/tested here.
- Linux PAL one-lap integration completed with dummy audio actually initialized
  and VRAM/lifecycle gates enabled:
  `build/performance-drive/20260905-145753-dfc288/`. This does not validate every
  FMV, image correctness or full SPU/CD/game-state reset.
- Started the shared mod contract: `[mod] schema_version = 1`, with missing
  version preserving existing version-1 manifests. Unsupported versions fail
  atomically in the semantic parser; duplicate/malformed/overflowing versions,
  embedded NUL and trailing directory paths are rejected. The renderer reports
  a specific unsupported-schema diagnostic instead of accepting a partial
  future manifest. See `docs/mod-manifest.md` for the actual supported subset.
- Three selected Linux tests passed (mod_manifest, mod_asset_fallback,
  rmesh_cache), as did the Linux game build. Windows Release game and expanded
  compiled mod_manifest test passed:
  `build/autopilot-check/windows-mod-schema-results/`.
- Inspection identified the next unification boundary: `mod_assets.c` applies
  raw archive/texture patches independently of the semantic manifest loaded by
  `modern_assets.c`. Invalid semantic manifests currently do NOT disable raw
  replacements from the same directory. A common validated mod owner must gate
  both providers; versioning alone does not complete stage 2. Multi-mod ordering,
  dependencies/conflicts/cache identity remain pending, as do stage-1 session
  state boundaries. No new launcher or import/export UI was implemented.
- Unified the manifest gate in `mod_assets.c`. It owns a copied directory and
  one parsed manifest without SDL/GPU dependencies. The modern renderer now
  borrows that manifest instead of loading/parsing a second copy. Invalid,
  unsupported or unreadable manifests disable raw replacements, legacy PNG
  patches and semantic overrides together; absent manifests preserve raw-only
  compatibility. Reads are bounded to 2 MiB, and failures report diagnostics.
- Linux game build and seven compiled tests passed: parser, legacy fallback,
  valid combined provider, invalid manifest, future schema, semantic-only pack,
  and unreadable manifest (directory in place of the file). Tests exercise both
  first-consumer orders, unchanged destination bytes on rejection, shared view
  identity and no mid-session manifest mutation after editing the file.
- This owner is deliberately process-lifetime, matching current configured-mod
  selection. Renderer teardown only drops its borrow. Explicit session reload,
  multi-mod selection and asset-file content identity are not yet implemented.
  New provider integration has not yet been rerun on Windows or through a
  rendered modded race; preceding Windows schema evidence predates this slice.
- Ported the compiled provider file fixture to Windows using exclusive temporary
  directory creation and a direct configuration stub; production path handling
  is unchanged. After fixing a fixture helper name collision with Win32
  WriteFile, Windows Release game and all seven manifest/provider tests passed:
  `build/autopilot-check/windows-mod-provider-results/`. The same seven pass on
  Linux. This includes rejecting a directory used as mod.toml on both systems.
- The existing native_render_world integration test was run without editing or
  replacing its Python runner. Its first attempt timed out after 105 seconds.
  A retry with an explicit PAL image still waited before gameplay; a debugger
  stack identifies X11_ShowWindow -> SDL_ShowWindow -> InitPlatform, waiting in
  XIfEvent. Evidence: `build/autopilot-check/mod-provider-window-wait.txt`.
  No Xvfb runner is available locally. Rendered-mod integration remains unproven;
  do not count this as a passing visual test or attribute it to the mod loader.
- Added explicit, idempotent `ModAssetsShutdown`. Full ModernAssetsShutdown
  first retires mesh/importer owners and drops the renderer manifest borrow,
  then clears mod selection, parsed content and per-session diagnostics.
  Presentation-only restart does not call this path. The next mod access reads
  configuration/manifest again; this does not undo already-installed game bytes.
- Extended provider scenarios cover double shutdown, valid-to-unsupported
  manifest transitions, repair/retry, zero stale texture entries, disabled-mod
  sessions and configuration re-selection only after shutdown. Linux game build
  and seven tests passed; Windows Release game and the same seven passed:
  `build/autopilot-check/windows-mod-session-results/`.
  Runtime rendering after these changes remains unverified because of the
  previously diagnosed X11 show-window wait. Full game reset, multi-mod
  resolution, dependencies/conflicts and source identity remain open gates.
- Found a usable isolated renderer-validation path without changing SDL or user
  settings: `SDL_VIDEODRIVER=offscreen`. The existing native_render_world test
  passed in 3.21 seconds with an explicit local PAL image. Its assertions prove
  semantic texture override selection, native GPU draw submission and attract
  shadow draws, not pixel-exact output or X11 on-screen presentation. Evidence:
  `build/autopilot-check/mod-render-offscreen.log`. No Python runner was added
  or modified; replacing existing runners with compiled equivalents remains open.
- Compiled presentation_device and composite_gpu tests also passed offscreen
  (not skipped). PAL route `build/performance-drive/20260905-151559-3ed184/`
  completed one lap, three presentation restarts with retained mesh views, VRAM
  oracle checks and ordered shutdown after resource generation four. Audio used
  dummy. This is lifecycle/rendering correctness evidence, not scanout FPS.
- The performance harness now records the requested SDL video driver and
  presentation restart count in result.txt to distinguish future offscreen
  evidence from windowed runs. The X11 ShowWindow wait itself is still unresolved.
- Extracted counted-byte `AssetPathIsRelativeFile` into the compiled shared
  asset library and use it for both mod manifests and runtime mesh-index paths.
  Runtime-index matching records now reject path traversal, absolute/drive
  prefixes, backslashes, control/NUL bytes, empty/dot path components and extra
  fields before returning a file location. This is lexical validation, not a
  symlink sandbox or whole-index duplicate/content validation.
- Linux game build and nine selected tests passed. Index tests cover mesh and
  material paths, counted non-NUL-terminated input and malformed trailing data;
  a cache regression confirms rejected traversal never calls the file reader.
  Windows and rendered-game tests have not yet been rerun for this slice.
- Follow-up verification: Windows Release game and all nine selected
  manifest/provider/index/cache tests passed with the shared path validator:
  `build/autopilot-check/windows-asset-path-results/`. The index fixture now
  selects MSVC warning flags correctly. Linux smoke, frame-replay, render-stage,
  extract and pack targets built successfully. Native-render-world integration
  passed offscreen after rebuilding (3.06 seconds):
  `build/autopilot-check/asset-path-render-offscreen.log`.
- The existing mod_tools regression also passed: synthetic archive extraction/
  packing round trip, edit isolation and invalid-input cases. This does not
  substitute for a new standalone model import/export implementation, nor for
  all retail-region tests. Its existing Python runner was not modified.
- Added `RuntimeIndexValidate` and invoke it before accepting a file-backed
  modern asset cache. It checks the complete current-version index, all record
  fields/set names/paths, hidden NULs and duplicate key/set pairs. Temporary
  identities are sorted once at initialization (not in a render-frame loop),
  then freed. Errors include a line number; allocation failure reports line 0.
  Same key in different asset sets remains valid. Referenced file existence,
  content hashes and the separate environment index are not validated here.
- Linux game/smoke builds and index/cache unit tests passed, including 200
  descending keys (scratch-buffer growth), CRLF line numbering, duplicates and
  malformed records after a valid header. Native-render-world also passed
  offscreen (3.25 seconds). Windows has not yet rerun this full-index slice.
- Windows Release and all nine selected tests now pass with complete-index
  validation: `build/autopilot-check/windows-index-validation-results/`.
  A subsequent defense-in-depth guard validates relative paths at the common
  ModernAssetReadFile entry point as well, covering the environment-index route
  which bypasses the mesh index. It clears failed read outputs. Linux game/smoke
  build and native-render-world offscreen passed (2.96 seconds) after this guard;
  that small subsequent guard is not included in the Windows evidence above.
- Presentation-boundary inventory: most native GPU work consumes neutral
  camera/instances already, but ModernAssetsLoadSkyImage and the live importer
  still expand panoramas using live g_SkyTileMap/g_SkyRowBase. The importer also
  discards sky assetKey and captures live VRAM. A recorded neutral frame alone
  therefore cannot reconstruct this asset independently. Next stage-3 work must
  make the sky layout/source generation explicit without changing the corrected
  screen-space cloud geometry. This is evidence of an open boundary, not proof
  of a current visual regression or authorization to add rendering threads.
- Extracted `RageSkyPanoramaLayout`, a 16-byte resolved tile selection with no
  game-state pointers. Both the live importer and the file-image expansion path
  now resolve the same copied layout before expanding pixels. The pure expansion
  entry point rejects invalid tile indices before changing any destination byte.
  Tests prove changing the source map after capture cannot affect expanded pixels.
- Linux game/smoke builds, sky_panorama_layout and native-render-world offscreen
  passed (2.99 seconds for integration). This is preparation for the actual frame
  boundary, not its completion: capture still happens during asset load. The
  camera/frame must next carry the layout, sky GPU cache identity must include it,
  and snapshot version 6 needs an explicit backwards-compatible extension.
  Live VRAM source ownership remains separate and unproven. No cloud geometry or
  snapshot serialization was changed in this slice; Windows has not rerun it.
- Game-produced cameras now own resolved sky tile layouts. Presentation
  interpolation copies this discrete state and snaps the cloud grid when its
  layout changes. Native GPU sky cache identity includes layout/validity as well
  as asset key and cloud row, and passes the frame's layout to both import paths.
  The neutral layout type is in render/sky_layout.h, not a game-global header.
- Snapshot version 7 serializes the layout for all four current/previous/main/
  mirror cameras. Tests round-trip distinct layouts, reject invalid v7 tiles,
  and reconstruct/read v6 wire data with layout absent. Versions 1-6 retain the
  explicit legacy fallback to current game layout; they are not claimed to be
  self-contained. Current captured layouts do not borrow the live tile map.
- Linux game/smoke/frame-replay/render-stage builds and four selected tests
  passed, including native-render-world offscreen (3.05 seconds). Updating the
  importer signature also required the replay tools' importer stub to match.
  Windows and pixel-exact sky/cache transition tests have not rerun this slice.
  Live VRAM image ownership still prevents fully independent replay; the layout
  boundary alone is not completion of stage 3 or the full migration.
- Windows Release game, frame-replay and render-stage builds now pass with
  frame-owned sky layouts and snapshot v7. Three compiled tests (layout,
  render-world interpolation, snapshot v7/v6 compatibility) pass on Windows and
  Linux. The fixtures now use appropriate MSVC warning flags. Windows evidence:
  `build/autopilot-check/windows-sky-frame-results/`.
- Post-change PAL Mythical Coast class-1 integration completed two actual races
  through the intervening result/repeat path, with three presentation restarts,
  retained mesh owners, VRAM oracle and final ordered resource/session shutdown:
  `build/performance-drive/20260905-153417-6f706a/`. The result records offscreen
  explicitly. Audio was dummy; this is not an audible, pixel-exact, physical
  Windows GPU or X11 scanout test, nor a PAL/NTSC-wide stability claim.
- Replaced separate native sky-cache identity globals with a shared semantic
  `RageSkyTextureIdentity` and a pure comparison used by the actual GPU cache.
  A compiled fixture changes each of the 16 tiles independently with unchanged
  asset/row, and verifies invalidation, symmetry, asset/row changes, copied key
  independence and legacy no-layout behavior. This tests the cache decision,
  not GPU pixel contents after an upload.
- Linux game/smoke build, four selected unit/functional tests and the existing
  native-render-world offscreen integration passed (10.94 seconds). Windows
  evidence above predates this identity-only refactor. VRAM image generations,
  full session reset and the other roadmap stages remain unfinished.
- Extended the actual presentation-device test with an unsupported GPU driver:
  initialization fails, no presentation handles are published, double reset
  cleans up, then two valid device/audio cycles succeed. This exposed a real
  backend bug: destroy callbacks were emitted for failed/uninitialized devices
  and repeated resets. SDL_GPU now emits them only for a successfully published
  device lifetime; internal partial resources are still cleaned independently.
- Regression first failed as expected, then passed after the guard. Linux
  game/smoke builds, audio lifecycle/exit, GPU failure/retry and native-render-world
  offscreen tests passed (4/4). Additional repeated reset after each successful
  device cycle also passed. No SDL library source was edited. This exercises
  device-selection failure, not every allocation/shader/window failure, and has
  not yet been rerun on Windows or the alternative SDL_GL backend.
- Windows verification passed: Release game build, compiled sky-identity test,
  and the real presentation-device failure/retry fixture in an interactive
  limited-user VM task with process-local SwiftShader (exit 0, not skipped).
  Evidence: `build/autopilot-check/windows-device-failure-results/`. The expected
  unsupported-driver diagnostic precedes two successful Vulkan device lifetimes;
  it is intentional fault injection, not an unexplained test error. Removed
  temporary task RageDeviceFailure20260905 after it returned Ready/exit 0.
- Persistent-geometry inventory: ModernNativeGpuPrepare still expands main and
  mirror vertices through RenderBuildNativePassDraws into CPU arrays, and
  ModernNativeUploadVertices copies the combined prepared array into a cycled
  GPU buffer. Asset mesh byte ownership alone does not make this geometry
  persistent. Stage 4 must preserve view-dependent culling, UV/material state
  and shadow/main/mirror consumers while changing this data path.
- Added opt-in performance-trace vertex-upload records: frame identity, main/
  mirror vertex counts, bytes and CPU map/copy/command-encoding time. The normal
  path performs no timestamp/log calls when trace is disabled. These timings
  do not measure GPU execution or scanout and include transfer-buffer cycling.
- PAL Mythical Coast class-1 one-lap measurement:
  `build/performance-drive/20260905-154724-b2fd72/`, offscreen/dummy, trace on,
  complete. 2,064 uploads, no repeated frame identities, all byte counts equal
  (main + mirror vertices) * 76. Total 3,916.60 MiB, mean 1,943.12 KiB per upload,
  max 2,945.74 KiB. Mean CPU upload preparation 0.2351 ms (0.2314 ms excluding
  first ten), max 9.901 ms. Thus skipping unchanged frame IDs would not optimize
  this normal interpolated path: GameRenderWorldPresentation increments its
  serial every presentation. Persistent object-space geometry needs a real data
  path change, not a redundant-upload flag. This trace alone does not establish
  that upload is the principal frame-rate bottleneck.
- Regional post-refactor verification: local NTSC-J (SLPS_006.00) and NTSC-U
  (SLUS_004.03) both completed class-1 one-lap routes with three presentation
  restarts, VRAM oracle and ordered cleanup. Both automatically selected NTSC
  base 60 Hz. Evidence: `build/performance-drive/20260905-155114-563bff/` (J),
  `build/performance-drive/20260905-155116-25a15a/` (U). Offscreen/dummy, not
  audible FMV or pixel-perfect regional equivalence tests.
- The existing J CUE referenced `(v1.1)` filenames absent from its extracted
  directory; first attempt failed disc initialization. Preserved it and wrote
  `build/debug-disc-ntsc-j/region-validation.cue` with matching names, retaining
  track/index layout. U archive BIN/CUE files were extracted into fresh ignored
  `build/region-us-cF3bs1/`; no source images were changed.
- Added optional EXPECT_REGION to the compiled-toolchain CMake route harness:
  validates recognized region, automatic timing-selection log and 50/60 Hz base.
  Positive J gate passed: `build/performance-drive/20260905-155254-f0af80/`.
  Intentionally requesting PAL for U completed the route but correctly failed
  the region gate: `build/performance-drive/20260905-155255-5d5bc7/`. This expected
  negative test is not a game regression. PAL with the new gate is not yet rerun.
- PAL now also passes EXPECT_REGION=PAL, automatic 50 Hz base, one class-1 lap,
  VRAM and lifecycle gates: `build/performance-drive/20260905-155413-bede5c/`.
- Audited existing FMV coverage: fmv_all_audio/ pacing explicitly force classic,
  so they do not prove modern-renderer FMV integration. The existing PAL pacing
  test passed in 57.53 seconds, covering complete decode of stream 5 (promotion)
  and stream 0 (opening), sector-derived picture/XA pacing, nonzero PCM and the
  intended audio tail. Evidence: `build/autopilot-check/pal-fmv-pacing.log`.
  This used offscreen/dummy and the existing Python runner unchanged. It is not
  an audible verification, an all-11-stream rerun or a modern-renderer pass.
- A compiled modern FMV runner can reuse DiscIdentify/DiscStreamTable plus the
  raw-sector reader; keep an independent sector/PCM oracle rather than trusting
  game trace counts alone. Existing Python coverage must remain until the
  compiled replacement covers its complete behavior, including malformed data.
- Added C `rage-pcm-check` and `pcm_metrics` unit coverage: independently reads
  captured little-endian stereo samples, checks complete frames and overflow,
  and compares frame count/absolute amplitude sum with the mixer report. Empty
  or silent capture fails the command-line verifier. Existing Python pacing
  coverage remains unchanged; this is not its complete replacement.
- Added `modern_fmv_audio` CTest scenario (CMake orchestration of compiled game
  and verifier). Checks stream 5's full ordered retail frame sequence (PAL 150,
  NTSC-U/J 300), XA start/end, explicit modern selection and actual nonzero PCM.
  It selects FMV directly, not by completing a class, and does not independently
  derive sector timing or prove audible hardware output/all eleven streams.
- The first BIN run exposed real missing audio: HostOpenDiscImage mounted game
  data but left PSY-Z's CD backend empty. CUE produced sound on the same movie.
  BIN now installs a single data-track sector backend with a separately owned
  FILE cursor for mixer reads; retirement unregisters it under the audio lock
  before closing. No synthetic CUE, launcher flow change or extra user input.
  Track 01 cannot supply separate CD-DA music tracks that are not in that file.
- Linux offscreen Vulkan + dummy audio promotion tests passed for PAL BIN
  (`build/modern-fmv-3197626672ad/`), PAL CUE (`build/modern-fmv-bb76439d3e4c/`),
  NTSC-J BIN (`build/modern-fmv-7413a4797d26/`) and NTSC-U BIN
  (`build/modern-fmv-e07742efc3b1/`). Initial NTSC runs exposed the scenario's
  incorrect PAL-only frame expectation; corrected before the passing reruns.
  Game/smoke/tool builds and five selected tests passed (pcm_metrics,
  audio_lifecycle, audio_exit_cleanup, disc_raw_file, fmv_audio). The raw-file
  test binary initially was absent and was built before rerunning all five.
  Windows portability and broader lifecycle/region/FMVs remain to validate.
- Expanded the compiled modern FMV scenario into eleven independently runnable
  CTest cases. Retail opening expectations are PAL 1800 / NTSC 2160 frames;
  ending is 1500 in each edition. The first complete run is checked because the
  title screen can start the intro again during remaining smoke ticks. The
  initial all-PAL run passed ten movies but overcounted the replayed intro;
  after correcting that test logic its isolated rerun passed in 54.07 seconds.
  PCM checks permit the ending's intentional silent picture tail; nonzero total
  capture is not a proof of uninterrupted audio throughout the entire movie.
- Windows build exposed two existing smoke portability problems: POSIX-only
  setenv and platform_stubs being built once in the shared legacy object target
  without RAGE_SMOKE_TARGET. Use _putenv_s on Windows (preserving an existing
  environment value), and compile platform_stubs separately in each executable
  so the smoke hook fallback and call order use the correct target definition.
  Linux game/smoke rebuilt successfully. Windows Release game/smoke/PCM tool
  builds and pcm_metrics test passed, evidence
  `build/autopilot-check/windows-fmv-pcm-results/`. No Windows GPU FMV run is
  claimed by this build/unit-test result. Existing Python tests remain intact.
- Final Linux PAL all-eleven modern FMV run after the per-executable hook fix
  passed 11/11 in 106.29 seconds (`build/modern-fmv-jXQowZ/all-pal-final.log`).
  Uses offscreen Vulkan and dummy audio, verifies complete first decode and
  captured PCM, not display pixels, wall-clock synchronization, physical audio,
  class-completion transitions, or the full NTSC movie matrices.
- Existing scenario_control runner also passed unchanged after the hook fix:
  both real-menu confirmation and direct race boot reach their synchronized
  stop. Evidence `build/modern-fmv-jXQowZ/scenario-hooks.log` (exit 0, silent on
  success). This retained Python regression has not been replaced or extended.
- Full modern FMV matrices now also passed on Linux using standalone data BIN:
  NTSC-U 11/11 in 117.70 seconds (`build/modern-fmv-jXQowZ/all-ntsc-u.log`),
  NTSC-J 11/11 in 119.97 seconds (`build/modern-fmv-jXQowZ/all-ntsc-j.log`).
  Together with the preceding PAL CUE run, all 33 region/stream combinations
  passed complete first-run decode and nonzero PCM/mixer agreement. These are
  still unthrottled offscreen/dummy tests, not complete class/race transitions
  or independent picture-to-XA timing validation.
- Windows PAL BIN promotion stream 5 passed the actual modern Vulkan runtime
  scenario using process-local SwiftShader in a non-elevated interactive task.
  Verified 150 picture frames, XA start/end and 3,258,368 stereo PCM frames with
  absolute energy 2,759,471,803, exactly matching the mixer. Evidence copied to
  `build/autopilot-check/windows-fmv-pcm-results/` (game.log, pcm-check.log,
  fmv-runtime.log/result). The extra smoke ticks entered attract mode after the
  movie; PCM totals describe the entire process, not isolated soundtrack length.
  Removed only our completed RageFmvRuntime20260905 task; preserved the existing
  unrelated RagePackageValidation task. Full Windows FMV matrix, physical audio
  output and hardware-GPU performance remain unverified.
- Added compiled `rage-fmv-pacing-check`: reads BIN/CUE sectors independently
  of the renderer/decoder, validates ordered STR chunks and retail XA coding,
  matches every reported frame-end sector, checks region-derived 50/60 Hz and
  compares elapsed simulation ticks with XA duration and 150 sectors/second
  (2% tolerance). Trace numbers reject overflow rather than using scanf's
  unchecked integer conversion. Supports representative streams 0 and 5 only;
  CHD and full Python-test equivalence remain out of this tool's current scope.
- Integrated that oracle into modern opening/promotion tests, including a
  negative trace that delays the last frame without changing sector/frame/PCM
  counts. All six region/representative-stream combinations passed both real
  evidence and expected rejection: `build/modern-fmv-jXQowZ/pacing-pal.log`,
  `pacing-ntsc-u.log`, `pacing-ntsc-j.log`. Windows Release tool build passed and
  validated the prior Windows PAL promotion log against its BIN (no new GPU
  playback); `build/autopilot-check/windows-fmv-pcm-results/fmv-pacing-*`.
  Usage and measurement limits are documented in `docs/fmv-regression-tests.md`.
  No claim of physical audio latency, wall-clock synchronization, full malformed
  disc coverage, or overall architecture completion; existing Python retained.
- Disc ownership audit found stale playback across backend replacement:
  Psyz_CdSetDiskPath/SetSectorBackend cleared the track table but left playback,
  prefetched PCM and open CUE track FILE state alive. Added a compiled regression
  using the actual CD backend and paused dummy audio; before-fix evidence
  `build/modern-fmv-jXQowZ/cd-backend-before.log` fails on retained playing state
  and samples appearing before playback is requested on the replacement disc.
- Both mount entry points now serialize with the mixer, stop/close the old
  stream and reset disc-owned buffers, decoder history/filter, XA limit and CD
  counters before replacing metadata, including failed mounts. Mixer volume is
  retained. Documented borrowed callback/user lifetime through unmount return.
  This is a disc backend boundary, not full SPU/game-session reset.
- The C test passes 20 cycles of unmount/idempotent unmount, direct source
  replacement, failed virtual mount and failed CUE path, with distinct sample
  identities, callback counters and no pre-playback stale samples. Four Linux
  lifecycle/audio tests passed (`cd-backend-final.log`); PAL modern promotion
  plus PCM/sector pacing also passed after the backend change (`cd-backend-fmv.log`).
  Paths are under `build/modern-fmv-jXQowZ/`. XA-history-specific failure injection,
  CUE-file-handle release and concurrent active-mixer stress still need dedicated
  coverage; synchronous CD-DA fixture results do not prove those cases.
- Windows Release game/smoke build and the same CD backend lifecycle fixture
  passed: `build/autopilot-check/windows-cd-backend-results/`, both exit 0.
  No Windows real-disc GPU rerun was performed after this backend change.
- Extended the compiled backend fixture with real XA decoding of synthetic
  sectors. Ten replacements use distinct ADPCM data/file IDs after priming the
  old decoder and setting an exclusive one-sector limit and filter. Each new
  source must emit no samples before Play, then produce 6000 stereo frames
  byte-identical to a fresh baseline without inheriting the old filter/limit;
  old-source callback counters stay unchanged. Unmount during XA stops pulls.
  Linux expanded fixture and four-test audio/lifecycle suite passed:
  `build/modern-fmv-jXQowZ/xa-backend-test.log` and `xa-backend-suite.log`.
  This covers decoder/filter/range state with a paused mixer, not concurrent
  mounting against an actively pulling SDL callback or CUE file-handle lifetime.
- Windows Release build and expanded CD-DA/XA lifecycle fixture also passed
  (`build/autopilot-check/windows-xa-backend-results/`, both exit 0). This is
  synchronous decoder/backend coverage, not another GPU movie playback run.
- Added concurrent CD-DA owner retirement coverage to the same compiled fixture:
  the real SDL dummy stream is unpaused and pulls through the SPU/CD backend.
  Twenty successive providers deliberately delay sector reads while the main
  thread requests unmount. Atomic counters prove each provider was read,
  at least one read was observed in flight before unmount, no callback remains
  active on return, and retired providers receive no subsequent reads. Owners
  stay allocated for the whole test so late access reports deterministically.
  Audio sample production must advance. This does not reset/check already mixed
  SPU/device PCM, and concurrent XA mode itself is not exercised in this fixture.
- Linux fixture passed ten consecutive runs (200 concurrent retirements plus
  the synchronous CD-DA/XA checks):
  `build/modern-fmv-jXQowZ/concurrent-cd-repeat.log`, 13.29 seconds. Windows Release
  build and one expanded fixture run also passed:
  `build/autopilot-check/windows-concurrent-cd-results/`, both exit 0. No sanitizer
  evidence or exhaustive scheduling guarantee is claimed by this stress test.
- Started the content boundary with compiled `rage-content` and immutable
  `RageRegionProfile` values for unknown/PAL/NTSC-U/NTSC-J. Boot-family detection
  and runtime timing now consume the same profile data, without SDL, GPU or game
  globals. The serial-family mapping retains the previous nine prefixes and
  case-insensitive recognition. It classifies regions, not game compatibility.
  FMV tables are still derived from the mounted image rather than copied into
  regional defaults. No launcher code or startup selection flow was changed.
- Canonical names select timing; unknown names retain the defensive PAL default.
  The former permissive `NTSC*` string check no longer treats an invented name
  such as `NTSC-invalid` as a recognized 60 Hz profile. Unit tests cover this,
  all prefixes/lowercase variants/truncated strings, missing values and shared
  profile identity. Linux game/smoke/tool build and four selected tests passed
  (`build/modern-fmv-jXQowZ/region-profile-tests.log`). The real-disc test first
  skipped due to its historical default path; rerun with explicit PAL CUE passed.
- Modern promotion PCM + sector-pacing regressions passed after profile
  integration for PAL, NTSC-U and NTSC-J (`region-profile-pal.log`,
  `region-profile-ntsc.log`, `region-profile-j.log`, same evidence directory).
  These profiles are built-in defaults, not completion of versioned external
  content definitions, vehicles/events/rewards, regional asset variants or the
  whole architecture roadmap. Independent oracle expectations remain explicit.
- Windows Release game/smoke/pacing-tool builds and region_profile,
  timing_restore, disc_stream_table tests passed:
  `build/autopilot-check/windows-region-profile-results/`. Corrected existing
  Unix warning flags on the two disc-table tests to `/W4 /WX` under ClangCL;
  otherwise `-Wall` enables unrelated all-warnings diagnostics there. No Windows
  GPU rerun is claimed for this profile slice.
- Presentation dependency audit: ModernNativeGpuPrepare still reads live
  TrackAssetIdentityRevision; NativeAssetImporterLoadSky ignores assetKey when
  decoding current VRAM. Copying just a revision into a frame would not make its
  pixel resources immutable and could label current pixels as an old generation.
  Resource-generation owners must be established before claiming replay isolation.
- Unified the two sky atlas expansion paths behind one pure copied-layout
  implementation: exported 512x128 eight-column strip and decoded 256x256
  four-by-two texture page. The live importer now calls the latter rather than
  maintaining an untested second expansion loop. Inputs are borrowed only during
  the call; source pixels still come from live VRAM through the existing adapter.
- Expanded tests encode local X/Y/tile identity in every RGBA pixel, compare both
  atlas representations and independently assert every output pixel across all
  four retail row-base variants. Invalid tile IDs and undersized source/output
  leave destination unchanged. Linux game build/unit test passed, followed by
  PAL class-1 Mythical Coast one-lap run with panorama loaded 512x256, VRAM oracle
  and lifecycle checks: `build/performance-drive/20260905-164851-101b48/`.
- Windows game/replay/stage builds and three sky/frame/snapshot tests also passed:
  `build/autopilot-check/windows-sky-atlas-results/`. The first invocation pointed
  at an absent helper script; after copying it explicitly, the actual build/test
  result was checked. This is no new Windows GPU run or pixel-exact gameplay
  comparison, and immutable frame-owned texture data remains incomplete.
- Marker/replay audit established that normal modern main-view sky is drawn
  by ModernRenderOverlaySelection from captured packets plus sampled VRAM;
  ModernNativeGpuDraw is called with drawSky=0. Therefore capturing the standalone
  native panorama cache would not capture the main-view sky. The attempted sky
  bundle reported unavailable and was removed, rather than shipped as a replay
  of resources it did not represent (`build/sky-replay-PkLZFH/` investigation).
- Added raw RGBA GPU texture capture and marker `*-vram-sampled.rgba`: exact
  1024x512 RGBA8 texture borrowed from ModernVramSnapshotCache, with a frame-match
  guard. Existing `*-vram.raw` remains the current compatibility readback. GPU
  download now checks fence-wait success before reading either PPM or RGBA data.
- The frame-match guard exposed markers running before the indicated snapshot's
  render submission. M/automatic marker checks now run after 3D rendering;
  passthrough scenes retain diagnostic checks without claiming a modern image.
  Marker info records sampled VRAM frame and whether it matches the scene.
  Before ordering fix: `build/sampled-vram-jJPQb9/game.log` reports unavailable;
  after: `aligned-game.log` and markers 4–7 contain full 2 MiB RGBA captures.
- Added Linux compiled-toolchain `sampled_vram_marker` scenario: isolated state,
  modern class-1 race, four automatic burst captures, no missing/mismatched
  sampled-VRAM diagnostics and exact byte lengths. Passed via
  `build/modern-fmv-jXQowZ/sampled-marker-test.log`. This is no self-contained
  full-frame replay yet: packet replay, native mesh/material resources and ring
  history ownership still need integration. No pixel-exact sky replay is claimed.
- Windows Release game/replay/stage builds and existing three sky/world/snapshot
  tests passed with the diagnostic changes:
  `build/autopilot-check/windows-sampled-marker-results/`. The new marker capture
  integration test currently targets Linux's isolated XDG state layout; no new
  Windows runtime marker capture or injected GPU-fence failure test was run.
- Marker integration now explicitly covers logic, fixed 60 and vsync presentation
  settings instead of inheriting one local INI value. Every capture's info must
  report modernImage=1, the requested FPS setting and identical scene/sampled-VRAM
  frame numbers, in addition to the exact RGBA byte length. All three Linux
  scenarios passed (12 burst captures total):
  `build/modern-fmv-jXQowZ/marker-modes-tests.log`, 10.75 seconds. This verifies
  marker association in those settings, not physical display refresh accuracy,
  interpolated-image pixel equivalence or offline replay of the full scene.
