# Implementation status

Active objective: implement the agreed desktop Electron launcher in `launcher/`.
This file records evidence and remaining work; it is not a completion claim.

## Verified locally

- Real Electron 44.2.0 window with sandboxed renderer and validated IPC.
- CMake builds and stages the game, extractor, packer, OBJ converter and shared
  C save/manifest helpers. No new Python implementation or runtime dependency.
- Real NTSC-U CUE import validates region and all 135 extracted archive entries.
- Persistent readable settings, locked first launch, native file dialogs.
- Save file opened and edited through the Electron UI; separate output has valid
  checksums. A negative special time survives the C JSON boundary.
- Native raw/DexDrive cards are covered by generated fixtures with unrelated
  nonzero data. Electron DexDrive flow selects an entry, edits it and saves a copy
  while preserving the wrapper and unrelated bytes.
- Team-logo editor supports painting, palette colors/transparency, eyedropper,
  keyboard painting and stroke undo. Electron palette/paint/undo/apply/save flow
  passes native readback; invalid logo data is rejected before output is written.
- Raw asset export and replacement imports; folder mods and C-validated material
  editing. Composition/conflict tests leave source folders unchanged.
- Actual game process launches in the modern renderer; log reports the live C
  importer. Relaunch locking was tested after lifecycle changes.
- Local macOS arm64 package builds, with native resources outside ASAR.
- Packaged macOS application tested (`app.isPackaged=true`) with a fresh
  --user-data-dir: all native tools found, locked empty state, CUE import of all
  135 entries, and packaged game launched with the live importer in modern mode.
  Evidence: packaged-profile-vJwYUE/game.log and packaged-ready-confirmed.png.
- Ten focused Node/native integration tests pass. Electron UI flows verified
  separately through Playwright using isolated user data.

Evidence is in ignored `build/launcher-evidence/` at the repository root.

## Required follow-through

- macOS packaged first-run flow is verified; repeat on Windows/Linux.
- Review remaining advanced save controls.
  Logo, record names/car selections, named garage cars, transmission and ownership
  choices are implemented. Electron save-copy and card-entry flows verify correct
  checksums, an unchanged original and preservation of unrelated card data.
- Finish the car workshop: textured OBJ/MTL/TGA/RRMESH export, OBJ/RRMESH import,
  textured part preview and regional names are integrated. Imported geometry
  has been verified in the modern runtime. Base assembled-car preview is
  integrated; previewing active texture/material overrides remains unfinished.
- Expose the remaining useful settings and deliberate game-parameter mod edits;
  maintain the boundary between launcher configuration and mod content.
- Mod conflicts have per-resource provider selection. Legacy PNG/JSON bundles
  are selected together per archive asset and their indices are merged. Verify
  additional legacy texture targets in the game; composition and migration are covered by
  regression tests. Semantic textures are namespaced per provider, and a changed
  candidate set invalidates stale choices.
- Verify Windows/Linux packaging, native tool dependencies, and first-run flow.
- Continue auditing duplicate actions and unavailable source files. Native close
  prompts have been verified in Electron for dirty settings and pending logo
  edits, with both Keep editing and Discard and close. C importer cancellation
  preserves the prior selection; mod cancellation removes staging data and
  releases the job lock. Commit phases disable cancellation. An aborted child
  must exit before staging cleanup, even when it handles SIGTERM and exits zero.
- Audit package rights/signing before any publication. No publication is currently
  authorized or performed. The current game binary embeds existing car assets.

## Architectural facts

Native staging no longer copies the repository-root INI over the distribution
template. The launcher uses its checked-in resources/rage-port.ini, preventing
developer-local paths and scripted diagnostics from entering future packages.
The current template was checked for those settings; a full native staging run
passed and preserved the template byte for byte.

Visible product naming now follows the requested Rage Mod Manager name:
window, header, application menu, package productName and packaged bundle.
The npm identity, executable and bundle identifier stay stable. Default user
data explicitly retains the existing Rage Racer Launcher directory, while
development overrides and --user-data-dir remain honored. Real Electron checks
verified the new window/app name and unchanged default profile path. The renamed
macOS package passed the first-run probe. Current output:
`launcher/out/Rage Mod Manager-darwin-arm64/Rage Mod Manager.app`.

Native command output is now buffered as bytes and decoded only after process
completion, preserving UTF-8 split across pipe chunks. Exceeding the configured
output limit terminates the child and reports a specific error after close,
instead of returning a truncated JSON suffix. Regression checks split UTF-8,
exact-limit success and overflow rejection. All 27 launcher tests pass locally.

Material commit retains the previous manifest until profile persistence
succeeds. On a caught commit failure, it restores the prior file by rename
(without requiring a fresh copy allocation), restores in-memory metadata, or
removes the new manifest when none existed. Regression tests cover both cases,
unchanged profile bytes and cleanup. This covers handled I/O failures, not
process-crash recovery across the two-file commit. All 26 tests pass locally.

Preview now preserves native RRMESH per-vertex RGBA values in a separate colors
array, including across assembly instances. WebGL applies the runtime's bounded
2x color modulation for textured faces and direct color for untextured geometry.
This restores authored trim/wheel colors that the prior preview omitted.
Regression coverage checks a colored OBJ through native conversion and preview,
and color preservation during assembly. An Electron whole-player-car preview
was rechecked; all 25 tests pass.

Settings persistence restores the last saved in-memory settings when writing
the profile fails. Regression coverage injects ENOSPC, checks unchanged disk
bytes and snapshot values, retries, and reloads the successful update. All 25
launcher tests pass locally.

My mods now offers whole-car previews for installed body replacements. The
service loads each neutral assembly part from that mod when replaced and from
the base otherwise. The preview labels its source and explicitly uses base-disc
textures; material/texture overrides remain outside this inspection path.
Regression coverage verifies replacement body bytes are used alongside base
wheel geometry and rejects unknown mod IDs. A real Electron session opened an
imported Erriso whole-car preview through IPC; screenshot is
`build/launcher-evidence/electron-mod-whole-car.png`. All 24 tests pass.

Full registry assembly audit: all 188 body variants (32 player, 156 rival)
were loaded through the real service/native preview pipeline using the local
NTSC-U image. All assembled vertices were finite, indices stayed in bounds,
and every rubber/metal texture source had a nonempty alpha atlas. No failures.
Evidence: `build/launcher-evidence/assembly-audit-2dpBE1/results.json` and
`check-all-assemblies.cjs`. This checks data completeness, not visual wheel fit
or opacity at every UV coordinate. Only the previously documented sample cars
have visual inspection. The current packaged macOS first-run UI probe also
passed. README now reflects implemented model, assembly and parameter tools.

Rival whole-car preview is available on each body-family row. Native
`--rival-layout FILE BODY` validates the track pack and selects the correct
suspension row for bodies 0,5,10,15,20,25,30. The missing rival-wheel palette was
located in palette-only records in boot asset 005, which PNG sidecars omit.
Preview reconstruction now replays that complete image chain before other
uploads. The C helper's -4 upload mode reuses its bounded image-chain reader.
A synthetic palette-only chain verifies decoded wheel texels and truncated-chain
rejection. The assembled bank-88/body-0 Esperanza was visually verified in
Electron with textured wheels; screenshot is electron-whole-rival.png under
build/launcher-evidence. Other variants need broader visual coverage. All 24
launcher tests pass. The macOS package includes the new native helper and UI.

Whole-car preview foundation: native `rage-mod-cli --car-layout FILE` reads
signed front-wheel offsets from the serialized player model header and emits
the neutral assembly used by GameRenderWorldSubmitCarAssembly: body part 0,
rear-wheel pair part 3, and two part-2 front wheels with the opposite side rotated
180 degrees around Y. It validates header boundaries and avoids host pointer
layout assumptions. All 32 local NTSC-U player banks were read successfully;
the regression covers negative offsets, opposite-side placement and malformed
headers. The layout converts suspension offsets into imported mesh coordinates
(four times game units, with Y/Z inverted), matching the runtime's 0.25 scale.
`Preview car` is now exposed in player groups. One bounded service operation
loads the three unique parts and joins four instances, preserving UVs, normals,
surface tags and distinct per-part texture sources. The geometry regression
checks opposite-side transforms, indices and texture isolation. All 22 tests
pass. A real Electron preview of player bank 10 was visually inspected after
correcting the coordinate conversion; evidence is
`build/launcher-evidence/electron-whole-car.png`. Other player variants still
need assembled visual coverage, and rivals use a separate format. This is the
base model, without enabled mod overrides.

Mod enablement and removal now use the operation lock and restore in-memory
state if profile persistence fails. Export holds the same lock and cancellation
removes partial output. Regression tests inject ENOSPC at persistence, verify
unchanged profile bytes and installed files, then retry successfully. Export
cancellation preserves the installed source. Operation setup is inside its
try/finally so a failed initial state notification also releases the lock.
All 20 launcher tests pass locally.

Material editing starts with a picker when the mod already contains materials.
Each edit form loads the chosen entry and keeps its ID read-only; Add material
is separate and rejects a duplicate ID in the form. This fixes the former
datalist behavior, which changed the target ID without loading its properties.
An Electron test edited the second of two distinct materials and verified that
its values were loaded and the first material remained unchanged.

Material editing now holds the service operation lock through native validation
and commit. Validation receives cancellation; commit disables cancellation before
changing the installed manifest. A regression cancels the operation, attempts a
concurrent mod toggle, and checks the original manifest, in-memory values, job
lock and temporary-file cleanup. All 18 launcher tests pass locally.

Car models now groups the complete 1140-part registry into 56 collapsible
player/rival bank groups, sorted by player/rival and bank number. Every preview,
export and replacement action is retained. Expanded groups survive state
updates, including preview loading. There is no Find/search control. A real
Electron test counted all groups and parts, opened a textured preview, returned
to the expanded group and saved a parameter mod. Screenshot:
`build/launcher-evidence/electron-car-groups.png`.

Form errors in an open editing dialog now appear as a focused inline alert;
the form and its draft values remain available for correction. A real Electron
test bypassed only HTML's minimum constraint to provoke native rejection of a
zero rev limit, verified the draft survived, then saved 8500 successfully.
Screenshot: `build/launcher-evidence/electron-form-error-preserved.png`.
Save controls now label the controller/neGcon/audio fields in English and use
named choices for Stereo/Mono and the Extra Grand Prix unlock state. Unknown
stored enum values retain the existing preservation option. All 17 tests pass.

Legacy texture composition uses the native loader's index format (asset number
and JSON path). Import rejects missing PNG/JSON pairs and invalid paths/indices.
Conflicts use `legacy-textures:asset-N`, selecting one provider's entire texture
set for that asset, including when providers renamed their files. Composition
copies matched pairs to provider-specific filenames and emits one merged index.
Semantic references to the same PNG remain independent. Installed indices are
read on startup to upgrade older profiles; unreadable bundles disable mods and
surface a startup error. The regression verifies paired bytes, unrelated assets,
semantic references, old-profile migration and rejected imports. All 17 tests
pass locally. Runtime verification found that texture-only composition also
needs the original `raw/asset_000.bin` recognition marker. Composition now adds
it for legacy textures, with a regression assertion. The native loader logs
successful texture patching once per asset. A separate real NTSC-U profile
combined two texture-only mods; the game logged asset 0 patched from one image,
used the modern renderer and reached race scene 12 timer 140 normally. Asset 6
was not loaded in that scenario. Evidence is under
`build/launcher-evidence/legacy-runtime-iDTbze/`. The initial harness checked
stdout instead of the redirected diagnostic log; the log was subsequently
checked directly and the harness corrected. The staged macOS game and launcher
package include this fix.

Raw mod inventory now rejects asset indices above 134, matching the archive's
135 entries. Asset replacement holds the operation lock before staging, and
installs its final name in the same commit as its files. It rejects replacement
while the game runs, supports cancellation without leaving partial mods, and
passes the AbortSignal to native manifest validation. Regression coverage checks
range boundaries, cancellation cleanup, persisted names and source preservation.
All 16 launcher tests pass locally.

Cross-platform build preparation: build-native.cjs now explicitly selects the
existing Windows ClangCL/VS2022 toolchain. A manual launcher-check.yml matrix
builds native helpers, runs Node/native tests, packages and opens the empty-state
UI on Windows/macOS/Linux (Xvfb on Linux). It publishes no artifacts. This file
has not been pushed or run remotely, so Windows/Linux remain unverified.
scripts/probe-package.cjs uses Node's built-in WebSocket/CDP and a temporary
profile, with no local Playwright dependency. Its actual packaged macOS UI check
passed. The first cleanup exposed an orphaned Electron helper holding inherited
pipes open. The probe now creates its own Unix process group (Windows uses
taskkill /T), waits for parent exit rather than pipe closure and stops only that
owned group. A subsequent local probe completed with exit code zero.

Workshop display names now follow disc region using the same compiled name
tables as the save editor. rage-save-cli car-names exposes those tables without
requiring a save file. Catalog labels and newly imported mod names retain part
suffixes while replacing the car name (e.g. Erriso -> Alouette for NTSC-J).
Stable model keys remain unchanged. The regression covers all 13 cars in all
three regions and wheel suffixes; it uses native tables, not a Japanese disc
fixture. Twelve launcher integration tests pass.

OBJ exports now include decoded TGA texture pages and map_Kd references in
model.obj.mtl. The C texture helper writes uncompressed top-origin BGRA TGA;
the palette regression checks dimensions, channel order and alpha flags.
Real exports of car.player.12.part.0 and car.rival.88.part.0 produced 17 and 5
material texture references respectively, all resolving to complete files under
build/launcher-evidence/textured-export-e9vxhf/. Glass remains a simple opaque
material, matching the inspection preview. The README in each export states
that OBJ replacement imports geometry/material IDs, not edited texture pixels.
Opening these exports in an external 3D editor has not yet been verified.

Latest user direction: remove Find from Car models and ensure models have
textures. Search UI/handler were removed. Native car export reports authored
source/page/CLUT mapping; native preview includes UVs. preview-textures.cjs
collects bounded VRAM uploads and the C helper decodes indexed texture pages.
Player atlases come from serialized car images; rival atlases require full
track texture image chains, including image entries omitted by PNG extraction.
WebGL draws material groups with decoded textures, with simplified lighting and
opaque dark glass. Base disc textures also accompany imported mesh previews;
active texture/material overrides and custom team artwork are not yet reproduced.
Electron visual checks: electron-textured-preview.png (Erriso) and
electron-rival-textured.png (Esperanza rival). texture-banks.json records
nonempty decoded textures for part zero in all 56 banks, not exhaustive visual
inspection of every part. Eleven integration tests pass, including native
palette decoding and rejecting truncated uploads. Preview remains per part.

Packaged first-run audit: current macOS package launched with app.isPackaged and
an isolated --user-data-dir. Import persisted an NTSC-U disc and 135-entry
manifest in packaged-profile-uLmMoX. The Playwright wait after import hung and
was stopped after the importer exited, before packaged game-launch verification.
Follow-up on packaged-profile-vJwYUE confirmed the renderer remained responsive:
direct CDP evaluation returned the ready home screen and a successful snapshot.
The Playwright wait was bypassed on the same live page, Play was clicked, and
snapshot reported running=true/error=null. The child executable path was inside
the app bundle; its log confirmed active=modern and the live C importer. Thus
the full macOS packaged flow is now verified, despite the harness wait issue.
Only the owned test game and launcher processes were then terminated. Packaging
checks all six native tools and the base config, rather than only rage-racer.

Car workshop integration in progress: the C manifest parser accepts `[meshes]`
with relative `meshes/*.rmesh` paths. The modern authored-car path resolves
`car.player.<bank>.part.<submesh>` and `car.rival.<bank>.part.<submesh>` overrides
and applies the existing per-car material map. Replacements must contain exactly
one mesh. Parser tests cover lookup, duplicate precedence, traversal and wrong
extensions. The launcher manifest CLI, package inventory, import validation,
export and composition now preserve meshes. The native CLI validates single-part
RRMESH files before installation. Mesh conflicts use per-part provider selection
and provider-specific paths. A service test converts two OBJ fixtures using the
real converter, imports them, edits a material without losing mesh references,
resolves the conflict, verifies the selected file bytes and exports a package.
It also rejects a corrupt mesh. This test found and fixed an existing export
failure caused by copying onto an already-created directory with errorOnExist.
All ten launcher tests pass. The game now exposes a headless authored-car
catalog and source RRMESH export after disc identification. Launcher IPC and
Car models screen expose catalog search and folder export (OBJ + MTL + RRMESH).
A real NTSC-U image produced 1,140 unique part keys; exporting the first body
produced all three model files plus editing instructions. Exports explicitly
represent authored base models, before active overrides. Part names currently
use authoring labels and need regional display-name mapping. The Car models
screen now imports OBJ/RRMESH replacements through a native file dialog, checks
the target against the game catalog, converts OBJ with rage-mesh-obj and installs
a disabled named mod. Conversion and installation share one cancellable job;
staging is removed on completion or failure. Verified in real Electron using
the exported Erriso OBJ and a separate profile: exactly one disabled mod points
to car.player.12.part.0. All ten launcher tests remain passing. Native preview
JSON now feeds a WebGL geometry viewer from either a base part or an installed
mod, including disabled mods. Rotation, zoom, wireframe and reset are available.
Real Electron rendered the imported body and wireframe without renderer errors;
visual inspection caught and corrected the vertical axis. A service regression
asserts preview coordinates come from the selected mod and rejects an unknown
provider. Preview uses neutral surface colors, not game textures or lighting;
it currently shows one part rather than an assembled car. Runtime now applies
explicit mesh overrides even when modern.authored_cars=0; only unmodified parts
follow that switch. Two real game runs entered the race with the launcher-
imported Erriso body, one with the switch off and one on. Both logged the exact
provider path and 854 triangles for car.player.12.part.0 and exited successfully.
Evidence: runtime-car-0/game.log and runtime-car-1/game.log under the evidence
directory; verify-car-runtime.cjs records the invocation. The regular game uses
capture.path together with stop.scene/stop.timer, whereas capture.directory
belongs to the smoke executable and captures the classic framebuffer.
Modern captures were subsequently verified using the regular game. The external
camera capture modern-car-chase-1/race.png shows the imported Erriso rear body,
glass, decals and surrounding track rendered in modern mode. Its game.log records
the installed mod path. This proves this one round trip, not fleet-wide visual
coverage. Canonical OBJ export after re-import exactly matches the original
position, normal, UV, color, face and material-assignment lines. Scripts and
images are retained in build/launcher-evidence/.
Screenshots: build/launcher-evidence/electron-car-catalog.png and
build/launcher-evidence/electron-car-import.png.

The runtime importer is lazy: preparing the library does not precompute every
GPU mesh. Launch invokes the existing automatic C importer in modern mode. Do
not promise a prebuilt cache or add a separate player command. Car names/prologue
are derived from disc region internally, never exposed as user selectors.

Do not mark the active goal complete until the agreed implementation and its
verification are complete. The first functional app is concrete progress, not a
substitute for the remaining requested workflows.
## Car parameter editor

`rage-mod-cli --car-spec read FILE` reads the player car's second asset pack
(model bank + 1). `--car-spec write FILE OUTPUT KEY VALUE ...` edits revLimit,
redline, topGear and automaticAccelerationScale into an exclusively created
copy. The C implementation uses GameCarSpec field offsets, validates the pack
boundaries and field storage ranges, and preserves every unrelated byte.

Verified with all 32 player specification packs from the local NTSC-U image.
The regression test checks exact byte preservation, original/previous-output
protection, malformed offsets and rejected edits. All 14 launcher tests pass.
Car models exposes Parameters on each player body row. A numeric form reads
base-disc values and saves changed fields through bounded IPC into a new,
disabled raw mod. It uses the existing import, conflict and composition path.
The service regression checks composition bytes, invalid edits, running-game
rejection and staging cleanup. A real Electron session edited Erriso's rev
limit to 8500 and verified the installed mod with the native reader; screenshot
is `build/launcher-evidence/electron-car-parameters.png`.
Existing mod values are not loaded into this base-variant editor. Other car
specification fields and non-car game parameters remain outside this form.

Profile write serialization (2026-09-05): settings and mod-priority writes now
hold the same non-cancellable operation lock as other profile mutations, including
rollback on failed persistence. Settings cannot be saved while the game runs;
the UI disables Save settings in that state. A gated persistence regression test
checks competing settings, priorities, mod enablement and launch requests during
both writes. All 28 launcher tests pass. macOS arm64 package rebuilt.

Mod library model controls (2026-09-05): each mod now has a collapsed Models
section, grouped by player/rival model bank, with a bounded scrolling area.
Materials, Export and Remove remain in the main row. Expanded sections survive
service updates while opening native textured previews. Verified in Electron
with an imported mesh mod: initial collapse, expansion, full-car preview through
IPC and expansion retained after closing. Screenshot:
build/launcher-evidence/electron-mod-models-collapsed.png. macOS package rebuilt.

Save paint controls (2026-09-05): both garage paint fields now use 18 indexed
radio swatches matching g_PaintColorTable in host_state_menu.c. Native keyboard
radio navigation, visible focus and selected borders are supported. Out-of-range
stored colors remain unchanged unless a swatch is selected. Actual Electron flow
verified mouse selection, ArrowRight, Save a copy, native readback of color 9,
valid checksums and byte-identical source. Evidence: verify-paint-ui.cjs and
electron-paint-palette.png under build/launcher-evidence. Swatches represent the
menu palette; they are not a rendered-car lighting preview.

Record-state audit (2026-09-05): inspected InitRecordTables/RepairRecordTimes in
src/main/PAL/main/race/records.c and its call during LoadSaveState. Best lap,
total and sector references are seeded with course defaults; <=0 or >599999 ms
is repaired to defaults on game load. No distinct missing-record sentinel was
established. The illustrative UX requirement is corrected to prohibit writing
an invented empty marker. Existing signed-value preservation remains intentional.
Ranking/time-attack row empty semantics are not established by this check.
Also corrected obsolete documentation saying Electron was undecided/unimplemented.

Game exit handling (2026-09-05): release the process reference immediately on
close, independently of asynchronous log/composed-mod cleanup. Cleanup and
notification rejection cannot become an unhandled EventEmitter promise.
Nonzero exit and signal termination report both game-process.log and game.log;
a successful spawn clears only the prior game error, preserving startup errors.
Regression runs a missing executable followed by an actual failing child process,
checks unlock/retry, error notification and process log output. All 30 tests pass.

In-app diagnostics (2026-09-05): View logs on Play and error banners opens a
read-only, escaped, selectable view of game-process.log and game.log. The fixed
IPC operation accepts no file path and reads at most the last 64 KiB per file;
missing/empty files have explicit messages. Native-service bounded-tail tests
and an actual Electron open/close flow pass; all 31 launcher tests pass.

Keyboard binding capture (2026-09-05): keyboard settings now open a capture
dialog instead of requiring SDL names to be typed. Browser physical codes map
to SDL scancode names for letters, digits, arrows, navigation, punctuation and
F1–F24; unsupported codes request another key. Escape cancels without editing.
Tests compare common names against the bundled SDL table. Actual Electron
capture, save and Escape cancellation pass; all 32 tests pass. Modifier/keypad
capture and duplicate-binding guidance remain to be implemented.

Extended key capture (2026-09-05): added left/right Shift, Ctrl, Alt and GUI,
keypad digits/operators/Enter, NumLock and CapsLock. SDL names verified against
the bundled keymap. Config validation now accepts the keypad multiplication
asterisk. Real Electron Right Shift and NumpadMultiply capture, save and Escape
cancellation pass. All 32 tests pass. Modifier keys represent individual physical
buttons; combinations/chords are not a runtime binding format.

Shared keyboard assignments (2026-09-05): Controls now lists duplicate bindings
(case-insensitive) with every affected PlayStation button. Capture updates this
notice immediately; intentional duplicates remain saveable. Binding buttons also
have the IDs referenced by their labels. Actual Electron test assigns Q to UP,
checks UP/L1 notice, reassigns K, checks notice disappears, and saves successfully.

Save-copy serialization (2026-09-05): native save writes and the final rename
now hold the operation lock. Competing save writes and game launch are rejected
until completion. The native write's JSON result is parsed before committing and
returned directly, avoiding an unlocked second read. Integration coverage checks
concurrent requests, native readback equivalence and byte-identical source;
all 33 tests pass. Electron paint/select/save-copy flow rechecked successfully.

Model inspection views (2026-09-05): Front, Side, Rear and Top camera buttons
preserve current zoom and wireframe mode. Actual Electron imported whole-car
preview checked in all four views. Visual inspection corrected front/rear yaw
and top pitch to match imported coordinates. Screenshots: car-view-*.png under
build/launcher-evidence. Existing drag, keyboard orbit and Reset view remain.

Save source aliases (2026-09-05): save-copy checks filesystem device/inode
identity inside the operation lock before native editing. Different path strings
that identify the original through a directory symlink/junction or hard link are
rejected. Regression uses real links and a missing native-tool directory to
verify rejection precedes editing, preserves source bytes and creates no temp
file. All 34 tests pass locally. This does not claim protection against external
processes changing filesystem links concurrently after the check.

Linux ARM64 verification in progress (2026-09-05): isolated Docker container
rage-launcher-linux-check, Node 22 / Debian bookworm, sources copied into /work;
host repository mounted read-only. GCC 12 exposed enum-comparison warnings
promoted to errors in scene/font static assertions and knockback threshold.
Explicit integer comparisons preserve checks and behavior without disabling
warnings. Rebuild currently compiling modern_assets.c; no successful full Linux
build, test run or package probe claimed yet. Logs: linux-build*.log in evidence.

Linux ARM64 build result (2026-09-05): full native staging passes under GCC 12
following the four explicit enum comparison fixes. All 34 launcher tests pass
on Linux, and Electron 44.2.0 linux-arm64 package builds. Evidence:
linux-validation.log. First-launch Xvfb probe is not passing: initially Chromium
requires root-owned 4755 chrome-sandbox in this container. Configuring that only
inside the test container advances to execvp '/work/launcher/out/Rage' failure,
apparently involving spaces in the helper path; investigate before claiming
Linux launch support. Container rage-launcher-linux-check remains available.
Probe now includes child signal and captured diagnostics after close instead of
losing the native reason on an early exit. No sandbox disabling was introduced.

Linux startup diagnosis (2026-09-05): copying the package to /work/linux-package
removes the truncated execvp path failure. It then reaches Chromium namespace
creation, rejected by default Docker permissions (Operation not permitted).
Preparing a second local test image from the existing container so namespace
support can be tested without disabling the application sandbox. docker commit
session remains active; do not rerun setup/build just because this is slow.
Evidence: linux-direct.log and linux-container-image.txt.

Linux ARM64 first-run UI now verified (2026-09-05): same packaged application
passes the Xvfb startup probe in rage-launcher-linux-sandbox, a local test
container with SYS_ADMIN and seccomp=unconfined to permit Chromium namespaces.
Confirmed again after restoring chrome-sandbox to its original node ownership
and 0755 permissions. No application sandbox flags were disabled. The original
package path containing spaces works with namespace sandboxing: the earlier
path failure was specific to the fallback setuid attempt under restrictive
Docker settings. Evidence: linux-sandbox-probe.log. This proves empty-profile
UI/native-tool discovery on Linux ARM64, not disc import/gameplay, Linux x64 or
Windows. Both test containers remain available; source host was not mounted in
the second container.

Model preview backgrounds: Dark/Light selection changes only the inspection
background and retains camera, geometry and textures. Real Electron imported
car preview exercised Light with front/side/rear/top views and visual inspection.

Linux ARM64 owned-data integration (2026-09-05): LauncherService with the actual
packaged Linux native executables imported the local NTSC-U CUE, validated 135
entries, reported ready, and launched the bundled game. Generated config forces
modern rendering; game.log confirms the live C importer while the child is
running. Test stopped that child after verification. Xvfb and dummy audio were
used. Evidence: linux-owned-check.cjs/log and linux-game.log; container profile
/work/linux-owned-profile-rwIguy. This is service/native integration plus the
separate packaged first-run UI probe; it does not claim a clicked Electron disc
chooser flow or completed gameplay on Linux. Windows and Linux x64 unverified.

Linux clicked-flow limitation discovered (2026-09-05): actual packaged Electron
UI imported the CUE and reached ready (profile /work/linux-electron-profile-DQwEeL).
Play launched the child but it later SIGSEGV'd after repeated Invalid GPU device
errors in this Xvfb container. Earlier live-importer log checks only demonstrated
initial startup, not successful rendering; do not treat them as gameplay proof.
Installing Mesa Vulkan drivers in the test container before repeating. Playwright
adds --no-sandbox to its own launch; the separate direct package probe is the
sandbox-enabled evidence. Playwright's RAF wait stalled despite ready UI; CDP
inspection of the same session confirmed ready and clicked Play. Retry uses
interval polling and a sustained process/GPU-error check. First session stopped.

Linux clicked import/Play after Vulkan installation (2026-09-05): same packaged
Electron session /work/linux-electron-profile-xYa29r imported the CUE and showed
ready. Playwright waiting stalled; an independent CDP connection inspected this
same window and clicked its Play button. After 15 seconds snapshot.running was
true, error null, the live C importer was logged and no Invalid GPU device errors
occurred. Mesa Vulkan drivers resolved the previous container GPU failure.
Evidence: linux-cdp-finish.cjs/log and linux-electron-vulkan-game.log. The CDP
verification completed; owned game and Electron processes were then stopped.
This verifies clicked import/launch and sustained startup, not a full race or
visual rendering quality. Playwright launch uses --no-sandbox; the separate
packaged startup probe already passed without that flag.

Material override precedence (2026-09-05): runtime inspection found that authored
surface defaults overwrote explicit mod roughness/metallic and altered tint after
parsing the mod. Base material loading now leaves overrides to the final resolve:
surface defaults first, explicit mod properties last, for both imported and cached
sources. AuthoredCarSurfaceResolve is shared compiled logic; tests cover every
surface, preserved texture/paint-mask paths, unchanged no-mod glass behavior and
transactional rejection of invalid properties. render_material ctest passes.
Game build in progress. Preview material mapping/effects remain unfinished; this
fix addresses the game-side behavior before implementing matching preview logic.

Material precedence build follow-up (2026-09-05): macOS game build and native
staging completed; all 34 launcher tests pass and macOS package rebuilt. Linux
source copy updated with resolver/runtime/tests and current renderer UI; matching
Linux build running (linux-material-order-build.log). Its prior packaged binary
does not yet contain this material-order fix. One prematurely started overlapping
macOS staging invocation was stopped; final staging ran after the game build
completed and passed.

Preview material identity groundwork (2026-09-05): rage-mod-cli --car-materials
player|rival FILE scans serialized model banks in model/face order, assigning
slots to first-seen page/CLUT pairs as the live importer does for model banks.
Bounded C decoder validates pack/header/stream extents and primitive strides.
Player model-size metadata is at +24, model offset at +32; rival primary model
block is runtime header entry 3. Synthetic player/rival tests cover repeated
pairs/order and truncated face runs. All 35 launcher tests pass. Real NTSC-U
raw assets decode successfully for all 56 player/rival banks (material-slot-banks.json).
This exposes identity data only; connecting mod properties/textures to the
preview and auditing effects of runtime CLUT remapping remain outstanding.
Linux material-order game build completed successfully; its staging/package
still need refreshing with the new material decoder and subsequent changes.

Semantic material IDs (2026-09-05): optional bank argument on --car-materials
uses the runtime AssetMaterialId function to emit canonical IDs, including track
course/class and model-bank-1 naming for rivals. Invalid bank/type combinations
are rejected. Tests cover player and rival IDs and rejected banks; all 35 tests
pass. Sample mapping audit of body/front/rear parts for player banks 10/72 and
rival banks 88/134 finds every authored page/CLUT pair in the raw material table.
Evidence: check-material-mapping.cjs and material-mapping-*/results.json.
This is matching base-disc identity evidence; arbitrary raw mods/runtime palette
variants still require their own handling when preview overrides are connected.

Mod material preview (2026-09-05): selected-mod part/whole-car previews now map
exported source page/CLUT pairs through native material tables to canonical mod
IDs (variant 0 override before base). Whole assembly carries overrides alongside
remapped textures, including original wheel geometry alongside replaced bodies.
WebGL applies tint, emission, lit/unlit, alpha modes and an inspection-only
roughness/metallic highlight. Explicit mod texture images and raw replacement
material tables are not included yet; pixels still come from the base disc.
Real Electron test edited car.1.material.0 to an unlit red tint and opened the
imported car: only the corresponding panels changed; screenshot inspected at
electron-mod-material-preview.png. Assembly regression checks remapped material
identity. Remaining limitations include exact game lighting/transparency ordering,
other palette variants and previews for material-only mods in the library UI.

Semantic PNG preview overrides (2026-09-05): preview material resolution also
selects textures by canonical ID, preferring variant 0 over base. Installed-file
inventory/path checks, PNG signature/dimensions and 32 MiB/4096-per-axis preview
bounds precede transport. Browser PNG decoding feeds WebGL textures; whole-car
assembly retains replacements. Power-of-two images repeat; other sizes clamp
for WebGL1 completeness. Corrupt PNG decode errors propagate through awaited
preview mounting. Real Electron mod import/whole preview verified a generated
solid turquoise texture replacing the targeted panels; screenshot inspected at
electron-mod-texture-preview.png. Native-backed tests check player/rival variant
selection and missing installed texture rejection. Raw/legacy texture overrides,
nonzero palette variants, glass/decal texture processing parity and material-only
mod preview entry points remain unfinished.

Car workshop redesign (2026-09-05): player slots 0–31 are numerically sorted,
with a separate rival section filtered by class/course. Complete-car actions
replace the part table as the main workflow; individual meshes are Advanced.
Car sets contain body/front/rear OBJ+MTL files, shared native-generated PNGs and
car-set.json, and import transactionally as one disabled mesh/texture mod.
The current format requires the original slot and region (no cross-slot material
remapping). Real owned-disc export/import/preview passed player bank 12 and rival
bank 88, with 12 and 4 semantic PNGs respectively. Evidence: verify-car-set.cjs,
car-set-x3vqGx/result.json. Synthetic native-backed regression checks three-part
installation, failed mesh rollback and rejection of wrong-slot/path escape input.

Transmission editing now reads/writes CarModelAsset.transmissionAvailable at
model byte 8 (zero = MT-only), plus all six GameCarSpec.shiftPoints pairs at
0x120. Native copies preserve unrelated bytes and use exclusive output creation.
The service groups edits across both raw packs in a single mod. Regression checks
source preservation and composed output for MT-only + rev limit + AT threshold.
UI converts thresholds using the tachometer's 160/1168 scale with two decimals;
a numerical test verifies round-trip preservation of every signed 16-bit value.
39 launcher tests pass. Actual Electron UI verified slot selection and parameter
controls; screenshots electron-car-workshop.png and electron-car-transmission.png.
Final workshop UI check (2026-09-06): workshop-cdp.cjs passed in an isolated
Electron profile after slot sorting, rival class/course filtering and speedometer
conversion. It selected player slot 1, saved MT-only plus a changed AT upshift
through the form, and verified one new mod containing both raw packs. The rival
class-6 selector showed four model entries. Updated screenshots include
 electron-car-workshop.png, electron-car-transmission.png, electron-car-rivals.png.
Initial navigation buttons now start disabled until the renderer has loaded its
state/listeners, preventing early clicks from being lost during startup.
The macOS arm64 package was rebuilt with these changes.

Linux workshop validation (2026-09-06): synchronized current C and launcher
sources into rage-launcher-linux-sandbox without copying macOS native binaries.
The first transfer included AppleDouble metadata files; removed those test-copy
sidecars after the compiler identified ._car_knockback.c. Native rebuild passed.
39 tests pass with one corrected scheduling assumption: a deliberately failing
child may already have reported its new error when launch() resolves, so the
retry test asserts replacement of the previous error instead of requiring null.
No runtime behavior was changed for that test correction.

Owned-data checks exported/imported complete player bank 12 and rival bank 88
sets and previewed 12/4 semantic PNG textures, then wrote MT-only and upshift2
into the two raw packs and read them back. Original parameters remained intact.
The rebuilt Linux arm64 package passed both empty-profile startup and the real
workshop UI flow under Xvfb as user node, without --no-sandbox. The test container
permits Chromium namespaces (SYS_ADMIN, seccomp unconfined); no desktop-distribution
sandbox claim follows beyond that environment. UI saved MT-only + AT changes as
one two-file mod and selected class-6 rival models. Screenshot inspected.
Evidence: linux-workshop-{build,tests,owned,package,probe,ui}.log and
linux-car-{workshop,transmission,rivals}.png in build/launcher-evidence.
Windows and Linux x64 remain unverified; no full Linux race/rendering claim.

Car-set PNG decode validation (2026-09-06): rage-mod-cli --png bounds image
files to 32 MiB/4096 per axis, then loads them through SDL_LoadPNG, matching the
modern runtime's decoder. Car-set import validates the staged copy and names
undecodable input files in the error; failed imports remove all staging output.
The regression now includes an intact PNG signature/IHDR without image data,
which the former header-only validation accepted. The mod count stays unchanged.
Both macOS arm64 and Linux arm64 pass 39 tests. Owned-disc player/rival complete
sets still export/import/preview successfully. Both packages were rebuilt and
the packaged native helpers decoded exported 256x256 images successfully.
Evidence: png-validation-{build,tests,owned,package}.log and Linux equivalents
in build/launcher-evidence. SDL is already part of the repository's compiled
runtime; macOS dependency inspection found only system libraries/frameworks.

Linux x64 validation (2026-09-06): created an isolated linux/amd64 Node
22.23.2-bookworm container, rage-launcher-linux-x64, with the same native/windowing
dependencies used for ARM64. Verified process.arch=x64 and uname=x86_64. Copied
source assets and code without host binaries or AppleDouble metadata, installed
Electron dependencies and completed a fresh GCC 12 native build (including the
game, SDL, generated car meshes and all launcher tools). This runs under x86-64
emulation on the ARM host, not on physical x64 hardware.

All 39 tests pass. A fresh profile imported the owned NTSC-U CUE and verified all
135 archive entries, then exported/imported/previewed a complete car with PNGs.
Combined MT-only/upshift edits read back correctly and original parameters stayed
unchanged. Profile: /tmp/rage-owned-x64-Y9J4rU inside the test container.
The x64 package passed its empty-profile startup probe and real workshop UI test
as user node under Xvfb, with sandbox enabled (container permits Chromium
namespaces). The UI test exercised 32 slots, class-6 rivals and a two-file
MT-only/AT mod save. Screenshot inspected. No full gameplay claim.

Evidence: linux-x64-{dependencies,npm,build,tests,owned,package,probe,ui}.log and
linux-x64-car-{workshop,transmission,rivals}.png in build/launcher-evidence.
Package copied to launcher/out/Rage Mod Manager-linux-x64. Windows remains
unverified. Older Linux distributions and physical x64 runtime graphics need
separate coverage.

Material binding checks (2026-09-06): reproduced acceptance of a car-set OBJ
using rage_63 although the selected part only provides texture source 0. Native
--mesh now returns the low-word materials actually used by indexed vertices,
retaining existing vertex/index counts and rejecting invalid vertex references.
The car import service compares these against the original part's exported
material metadata before installing either a complete set or an individual mesh.
Unsupported surface encodings and unavailable source slots report the affected
part/file. Explicit untextured slot 65535 remains valid (e.g. wheel wells).
Temporary base meshes are removed before mod inventory/install and on failure.

Regression covers the previously accepted source 63, unsupported surface 384,
individual-part import and legitimate untextured geometry. Owned player/rival
complete sets still export/import/preview with 12/4 PNG textures. All 39 tests
pass on macOS arm64, Linux arm64 and Linux x64. Corresponding native helpers and
packages rebuilt. Evidence: material-bindings-{build,tests,owned,package}.log and
rage-launcher-linux-{sandbox,x64}-bindings-{build,tests,package}.log.
Windows environment inspection found UTM VMs for Android/Linux/Haiku/Ubuntu,
no Windows VM or Wine/Windows compiler. Asked for an available Windows test host
or runner; no Windows verification claim.

Packaged Play/render validation (2026-09-06): clicked the real Play button in
isolated Linux profiles, using the default modern settings (scale 4, 16:9,
vsync) and diagnostic direct-race setup for class 0/course 0/car 3. No mods were
enabled. First x64 harness timed out before its 120-second capture deadline;
the traced retry captured a valid image after roughly 450 seconds but its
separate 20-second wait for global frame 300 was insufficient. These runs are
not recorded as clean-exit passes. Thread CPU percentages were cumulative;
a later sample showed idle intervals, so they do not establish continuous busy
shader compilation or a renderer deadlock. The runs eventually progressed.

ARM64 and x64 final tests instead used stop.scene=12, stop.timer=35, following a
modern dump at timer 30. Both captured the race and exited normally; the Electron
snapshot reported no error. The x64 final test captured after about 120 seconds.
The environment is headless Xvfb with software Vulkan/llvmpipe, and x64 runs under
emulation. No physical audio device was available. This proves packaged Play,
live native import, a short modern-rendered sequence and process cleanup; it is
not an audio/performance/full-race audit. Captured PNG inspection shows textured
road/environment, authored cars/wheels and HUD. No production code changed.

Successful profiles (inside respective containers):
ARM64 /work/build/launcher-evidence/appearance-cdp-KO8RpG
x64 /work/build/launcher-evidence/appearance-cdp-WCJWvA
Evidence: linux-arm64-play.log, linux-x64-play-bounded.log,
linux-{arm64,x64}-race.png/.ppm, linux-{arm64,x64}-game.log. Earlier timeout logs
remain as linux-x64-play.log and linux-x64-play-trace.log. All bounded game/UI
processes from these tests terminated; containers remain available.

## Windows Unicode path preparation

Native helpers use narrow main/stdio paths without an existing UTF-8 process
manifest. Added packaging/windows/utf8.manifest to the seven launcher build
executables under MSVC (including ClangCL). This uses the Windows 10 1903+
activeCodePage setting documented at
https://learn.microsoft.com/en-us/windows/apps/design/globalizing/use-utf8-code-page.
The car-set integration test now runs conversion, material inspection and PNG
decoding beneath a directory containing spaces, Polish/Japanese text and emoji.
All 39 tests pass on macOS; CMake Release configuration also passes (log:
build/launcher-evidence/utf8-configure.log). This does not verify Windows manifest
embedding or execution: both remain pending an actual Windows build/test host.

## Segmented time clipboard input

Added full minutes:seconds.milliseconds paste to any time segment. Valid input
updates the three fields together through the existing draft input handler;
invalid formatted input reports an error without changing the draft. Plain
numeric pastes retain native segment behavior. Parser tests cover millisecond
precision, optional spaces, invalid seconds, ambiguous fractions and the signed
32-bit maximum boundary. All 40 launcher tests pass on macOS and renderer syntax
checks pass.

Rebuilt the macOS arm64 package and exercised the actual system clipboard with
Cmd+V in its isolated Electron window. Pasting 02:03.004 into the seconds field
updated all segments; pasting 01:60.000 reported an error and retained the prior
values. Save a copy produced native readback 123004 with valid checksums and an
unchanged original. The harness only substituted file-dialog selections; the
clipboard, UI input, preload/IPC and native save writer were real. Evidence:
build/launcher-evidence/verify-time-paste.cjs and time-paste-rG7eKs; package build
log time-paste-package.log. The isolated application exited after verification.

## Mod identity metadata

Optional author, version and description now survive folder import, stored
profile and export/reimport. My mods displays escaped plain text, retaining
description line breaks and wrapping long text. Bounded metadata rejects wrong
types, control characters and oversized fields before installation. Existing
metadata without these optional fields remains valid. All 40 tests pass on
macOS, including identity roundtrip and invalid-field rejection; renderer syntax
check passes.

Packaged macOS UI verification found long descriptions made mod rows too tall.
Descriptions now use a native collapsed disclosure. A repeat verified the
author/version text, literal HTML-like description content (no injected image),
expand/collapse and visible Export button without page horizontal overflow at
1000px window width. Screenshot visually inspected:
build/launcher-evidence/electron-mod-metadata.png. Harness:
verify-mod-metadata.cjs; latest profile mod-metadata-ZGKmtn. Both runs passed UI
assertions but hung during Playwright shutdown; their identified test processes
were explicitly terminated. This is not evidence of a clean application exit.

## macOS packaged shutdown investigation (open)

The hang is not confined to Playwright: a child-process/CDP harness sent a Cocoa
NSRunningApplication termination request to the exact isolated package PID, but
the process remained alive for 60 seconds. A separate fresh-profile launch with
neither inspector nor remote debugging also remained alive 20 seconds after
the same request. Both harnesses terminated their own process groups afterward.
The initial CDP Cmd+Q attempt was inconclusive because synthetic key events do
not establish native menu dispatch. No production fix has been applied yet.

Evidence: build/launcher-evidence/verify-clean-quit.cjs,
verify-quit-no-debug.cjs and quit-sample.txt. The sampled main thread repeatedly
sat in Electron Framework -> usleep/nanosleep; stripped symbols do not identify
the actual cause. Next isolate launcher lifecycle code from Electron/package
behavior and verify the termination API result before changing production code.

Isolation: a minimal Electron 44.2.0 script (no launcher imports) that creates
one hidden BrowserWindow and calls app.quit reproduces the hang after logging
before-quit, will-quit and quit. --disable-gpu does not change it. A no-window
control exits with code 0. Scripts are in build/launcher-evidence/quit-minimal/;
each runner kills its own isolated group after a bounded timeout.
This isolates the observed failure from launcher lifecycle code. Upstream issue
https://github.com/electron/electron/issues/52582 reports matching symptoms and
its author reports resolution after upgrading physical hardware to macOS 26.6.
Our machine runs 26.3, whereas that report reproduced on 26.4.1, so identical
root cause is an inference, not proven. Read-only API comments were saved in
electron-quit-comments.json. No OS update or forced-exit workaround was applied.
Clean shutdown still needs verification on an unaffected/updated macOS host.

The checked-in package probe now verifies process exit code 0 after shutdown,
with a three-second window settling period. macOS uses PID-specific Cocoa
termination and checks that the request was accepted; other hosts use CDP
Browser.close. The existing workflow automatically runs this stronger gate.
Latest local runs passed on macOS arm64 and Linux arm64 (sandbox container).
No production shutdown change was made: the later macOS passes conflict with
earlier repeated hangs, so the environmental failure remains intermittent or
otherwise incompletely characterized, not fixed. CDP Browser.close also passed
on macOS but is intentionally not used there as a native-quit substitute.

## Car workshop installed changes

Added slot-local installed mod list with existing enablement IPC and per-mod
whole-car previews. It matches exact player banks, individual rival body groups
and shared semantic material banks; raw model/specification mods are listed but
explicitly excluded from preview claims. Region mismatches disable activation
and preview. Tests verify rival group isolation, wheel-only changes, shared
materials and raw specifications. All 41 tests pass on macOS; renderer syntax
passes.

Rebuilt macOS arm64 package and verified through its real CDP/IPC UI: slot 0
has no related test mod, slot 1 shows the imported material mod, enabling it
updates service state, and Preview mod opens the model canvas. Inspected the
workshop screenshot at build/launcher-evidence/electron-car-installed.png.
Harness verify-car-installed.cjs; passing profile appearance-cdp-oZgbbo. An
initial harness run used an incorrect descendant-canvas selector and timed out;
corrected to canvas#model-preview. Both isolated process groups were stopped by
the harness. This test verifies opening the preview, not pixel-level appearance
or graceful shutdown; those require their separate evidence.

## Mod details editor

My mods now has a Details form for name, author, version and description, with
explicit Save/Cancel and existing unsaved-modal close protection. A narrow IPC
method validates fields, rejects compatibility/asset edits, holds the operation
lock, prevents writes during gameplay and restores previous details if profile
persistence fails. Export uses the edited profile identity. All 42 tests pass
on macOS, including disk-failure rollback, compatibility preservation and export
readback. Renderer syntax passes.

Packaged macOS form verification passed: edited all four fields, saved through
real IPC, reopened and read back the author, canceled a subsequent name edit,
and checked description persistence in launcher.json. Screenshot inspected at
build/launcher-evidence/electron-mod-details.png. Harness verify-mod-details.cjs,
profile appearance-cdp-ypiUXh, build log mod-details-package.log. The harness
terminated its own isolated process group after the UI assertions.

## Linux refresh after workshop and details forms

Synced current main/renderer/tests/scripts to the existing isolated Linux ARM64
and x64 containers, retaining their own native binaries. All 42 integration tests
pass on each architecture. Rebuilt both desktop packages, then ran the updated
first-run and process-exit probe as non-root under Xvfb with sandboxing enabled:
both passed, including exit code 0. Logs: linux-{arm64,x64}-ui-tests.log,
linux-{arm64,x64}-ui-package.log and linux-{arm64,x64}-ui-probe.log under
build/launcher-evidence. These empty-state probes do not repeat the new form
interactions on Linux; those were exercised separately on packaged macOS.

## Whole-car original/modified comparison

Selected-mod whole-car previews now fetch the original and modified assembled
geometry, then offer a toggle preserving yaw/pitch/zoom, background and wireframe.
Both use the original geometry's center/scale to avoid hiding imported size
changes. Remounting releases prior WebGL buffers/textures/shaders and the resize
observer. Packaged macOS test switched Modified -> Original -> Modified after
choosing a side view/light background and checked the actual WebGL view uniform
remained identical. Screenshot inspected: electron-car-compare.png; harness
verify-car-compare.cjs; profile appearance-cdp-4BOn2e. A final visual refinement
adds active-button styling, mode text in the title and control spacing; this
small follow-up has not yet been repackaged. Existing preview limitations
(dynamic markings, raw/legacy textures and exact runtime lighting) still apply.

Follow-up packaged verification includes the mode-title/active-button styling.
Disposal now explicitly releases the old WebGL context using WEBGL_lose_context
when available, in addition to deleting its resources. Twenty consecutive
Original/Modified switches passed in packaged macOS: actual camera uniforms
remained identical and each current canvas retained a live context. Evidence:
verify-car-compare.cjs, appearance-cdp-jLrzNX, car-compare-repeat-package.log and
updated electron-car-compare.png. This bounded check does not claim a long-run
GPU memory profile.

## Preview close/decode race

A packaged regression scenario delayed Image.decode during a comparison switch,
closed the dialog, then released decoding. Before the fix the completed decode
reopened the dialog (assertion failed). Remounts now retain their originating
preview identity and discard completion/errors if that preview was closed or
replaced. The same packaged scenario passes after the fix (profile
appearance-cdp-Au1hSU; verify-preview-close-race.cjs). The normal 20-switch
comparison test also passes (appearance-cdp-B4ryZC). Native decode timing is not
changed: only the test delays browser image decoding deterministically.

The regression is now reproducible without private game data via the checked-in
scripts/probe-preview.cjs and probe-package.cjs --preview-regression. The manual
CI matrix runs it. Packaged checks pass on macOS arm64 and Linux arm64/x64:
20 switches preserve the actual camera uniform, then a delayed PNG decode cannot
reopen the closed preview. The Linux Xvfb environments lacked default WebGL;
the explicit probe-only --software-rendering option selects SwiftShader, and
both pass including clean process exit. Default player launch flags are unchanged.
Logs: linux-{arm64,x64}-preview-software-probe.log. Prior failed default-WebGL
logs remain as linux-{arm64,x64}-preview-probe.log. This is software-rendered
component coverage, not a hardware performance or game import test.

## Mod dependency enforcement

Added stable packageId/requires metadata with bounded identifiers and optional
exact version requirements. Import retains dependencies while leaving mods
disabled; export preserves package identity. Activation, disabling/removal of
providers, identity-version edits and final composition validate enabled
dependencies, with rollback on invalid changes. My mods displays requirements
and Details shows the package ID. All 43 tests pass on macOS, including missing
dependency activation, blocked removal/disable/version edit, metadata export
and final composition rejecting invalid persisted state. Version ranges and a
graph-editing UI are not implemented; the documented contract uses exact
versions or any version. Latest UI changes await packaged verification.

## Packaged dependency UI verification

The earlier local harness imported the base mod through a second
LauncherService after the packaged application had started, so the running
process never saw it; that file was not evidence. The rewritten
build/launcher-evidence/verify-mod-dependencies.cjs prepares the profile first:
it imports the addon (requires core 1.0) and then core, both disabled, persists
the profile, and only then starts the packaged macOS app with that
--user-data-dir. Through the real preload/IPC boundary it checked that My mods
shows "Requires: core (1.0)", that enabling the addon alone opens the standard
error dialog reading "addon requires core version 1.0 (enable a matching mod
first)" and leaves the checkbox cleared, that enabling core and then the addon
leaves both checked with no dialog, that disabling core while the addon depends
on it is refused with the same message and core stays enabled, and that the
persisted launcher.json agrees with the UI. The app exited with code 0 through
Cocoa termination. Screenshots: electron-mod-dependencies-list.png,
electron-mod-dependencies-error.png, electron-mod-dependencies-enabled.png;
profile dependency-ui-8NSlLK. On the same package
`node scripts/probe-package.cjs --preview-regression` passed (first-run UI,
twenty preview switches, close/decode race, clean shutdown) and `npm test`
passes. The package predates no source change, so it was not rebuilt. Linux
packages still lack this verification.

## Requirement status in the mods list

My mods now says, next to each requirement, which installed mod satisfies it:
"satisfied by <name>" for exactly one enabled match, "enable <name> first" when
a matching mod is installed but disabled, "not installed" when nothing matches,
and "multiple enabled copies" when activation would be refused. A mod whose
package ID is shared by another installed mod also shows a duplicate warning
before anyone tries to enable it. The renderer helper mirrors the main-process
matching (same package ID, same region, exact version when one is required,
never the mod itself); tests/mod-dependency-display.test.cjs covers those
rules and `npm test` passes 44/44. The packaged macOS harness now also asserts
"enable core first" before and "satisfied by core" after the base is enabled,
and the preview-regression probe passes on the repackaged app
(profile dependency-ui-VfwV5P, updated screenshots). Decision for v1: exact
version or any version remains the whole contract; semver ranges and a graph
editor stay out of scope.

## Linux refresh with dependency support

Both containers received the dependency and display changes by copying the
changed files into /work/launcher (there is no host mount). Running the
package and probe through `docker exec` as root fails twice over: Chromium
refuses to start as root without --no-sandbox, and a package created by root
is unreadable for the `node` user (spawn EACCES). Run every step as `node`
(`docker exec -u node`) and, if a directory was already created by root, hand
it back with chown. With that, `npm test` passes, `npm run package` rebuilds
`/work/launcher/out/Rage Mod Manager-linux-{arm64,x64}`, and
`xvfb-run -a node scripts/probe-package.cjs --preview-regression
--software-rendering` passes first-run UI, twenty preview switches, the
close/decode race and clean shutdown on both architectures. These are
software-rendered, emulated-x64 checks, not hardware or gameplay tests.

Both Linux packages were then rebuilt as `node` with the requirement-status
display (app.asar 2026-09-06 00:08 container time, arm64 and x64) and passed
the same Xvfb software-rendering probe again. The x64 package was copied from
rage-launcher-linux-x64 to launcher/out/Rage Mod Manager-linux-x64 on the host;
the arm64 package stays in rage-launcher-linux-sandbox. Logs:
linux-arm64-dependency-refresh.log and linux-x64-dependency-refresh.log in
build/launcher-evidence.
