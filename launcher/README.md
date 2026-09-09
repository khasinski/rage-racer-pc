# Rage Mod Manager

Electron desktop application in English for Windows, macOS and Linux. The web
renderer has no Node access; a sandboxed preload exposes specific operations to
the main process. Retail archive decoding, texture extraction, mesh conversion,
save parsing/checksums and material validation use the repository's compiled C
tools. No Python dependency is introduced.

Windows native tools target Windows 10 version 1903 or newer and embed a UTF-8
process-code-page manifest in the supported Visual Studio/ClangCL build. This
keeps Unicode paths from Electron consistent with native file operations.
Windows execution still requires verification on a Windows host.

## Development

From this directory:

```sh
npm ci
npm run build:native
npm test
npm start
```

The native build uses the root CMake project and stages executables in ignored
`resources/bin/`. It defaults to `../build/release`; set
`RAGE_LAUNCHER_BUILD_DIR` to use another CMake build directory. A C compiler,
CMake and the repository submodules are development prerequisites only.

```sh
npm run package
```

Packages the current host platform in `out/`, with the native tools and default
configuration as external resources. The resulting app starts by double-clicking;
players do not run npm or CMake. macOS arm64, Linux arm64 and Linux x64 packaging
and first-launch probes have been exercised locally. The x64 Linux checks ran
in an emulated Debian 12 environment on an ARM host. Windows package verification,
signing and distribution remain outstanding.
Do not publish the bundled game build without the repository's separate rights
audit: the current branch embeds retail-derived car replacements.

`node scripts/probe-package.cjs` opens the host package with a temporary profile
and checks the English empty state, locked tools and availability of all native
executables through the real preload/IPC boundary. It needs Node 22 or newer and
a desktop session. It also requires exit code 0 after a shutdown request, using
Cocoa termination on macOS and CDP Browser.close on other hosts. A timeout fails
the probe before cleanup forcibly stops its isolated processes. It needs
`xvfb-run -a` on Linux CI. It does not require a disc or
replace the separate full import/play test with owned game data.

Add `--preview-regression` to test twenty Original/Modified switches and closing
during delayed texture decoding using synthetic geometry and a one-pixel PNG.
For headless Linux CI, also use `--software-rendering` to explicitly select
SwiftShader for this isolated probe. These flags do not change player settings.

The manually triggered `Launcher cross-platform checks` workflow builds, tests,
packages and probes Windows x64, macOS arm64 and Linux x64. It does not publish
packages. Windows builds use the repository's Visual Studio 2022 ClangCL toolset.
The workflow must run successfully before claiming verification on those hosts.

## Current implementation

- First launch requires a CUE, CHD or Track 01 BIN. The compiled game validates
  the disc and exports RAGE.BIN; the compiled extractor prepares the asset
  library. Completion checks every extracted entry against the manifest. Failed
  or canceled imports cannot replace the previous working selection.
- Settings, mods and assets remain locked without a prepared,
  available game image. Paths and settings are persisted in Electron user data.
- The [integrated save editor](SAVE_EDITOR.md) works without a game image. It
  also creates fresh saves for all three regions. Its C library and regression
  tests belong to the root CMake project; the separate SDL/ImGui app is retired.
- A compact Play screen launches the actual game with the modern renderer.
  Native meshes/textures are generated automatically by the existing runtime C
  importer. There is no classic fallback and no separate player conversion step.
- Settings use the real INI names/ranges behind readable labels. The managed
  configuration preserves other keys/comments. Car names and prologue are
  forced from the detected disc region and are not user-selectable.
- Save discovery, individual files and raw/DexDrive cards use the shared C save
  library. The integrated form edits team names, progression, garages,
  times, controller and audio fields. It writes a separate copy and refreshes
  checksums. Segmented time fields also accept a full `01:40.765` paste into any
  segment; invalid full times leave all segments unchanged. Unknown data is
  retained. Record driver names and cars are editable;
  garage labels and car choices follow the save region, with the loaded disc as
  a fallback for an unknown filename. Transmission and
  ownership use named choices, retaining unknown existing values until edited.
  Team logos have a 64×64 pixel editor, 16-color palette, transparency flag,
  eyedropper, stroke undo and keyboard painting. The C library owns pixel packing
  and palette conversion. Apply changes to the save draft, then save a copy.
- Mods import/export as folders. Raw assets and texture/material overrides can
  be enabled, disabled or removed. File/semantic collisions block launch until
  the user chooses a provider for each overlapping resource. Other changes from
  both mods remain active. Choices are stored with their candidate sets, so a new
  provider requires review. Semantic texture files are isolated per provider.
  Arbitrary executable hooks and symlinks
  are rejected. Material values are validated by the actual renderer parser.
- The library exports all 135 archive entries; importing a replacement creates
  a disabled mod. The original disc and extracted library remain unchanged.
  A material dialog edits shading, transparency, roughness, metallic and color
  values, with a picker that loads each existing material's stored properties.
- Car models starts with numbered player slots 0–31 and a separate rival section,
  filtered by race class and course. The selected car offers a complete body and
  wheel preview. Individual part actions are under Advanced.
  Installed changes lists mods targeting the selected car, with enablement and
  individual mod previews. Raw model/specification changes are listed explicitly
  but are not represented in the 3D preview. Shared rival materials can affect
  multiple cars in the same race bank.
- Export car set writes three OBJ/MTL models (body, front wheels, rear wheels),
  shared PNG textures and car-set.json. Edit those files together in a 3D/image
  editor, preserving rage_* material names, UVs and normals. Import car set reads
  the folder and installs the three models and textures as one disabled mod.
  Currently the set must return to the same slot and disc version; cross-slot
  material remapping is not implemented. Textures shared between parts are
  exported once. Single-mesh import in Advanced does not import textures.
  Car-set PNGs are fully decoded by the game's SDL decoder before installation;
  an undecodable image reports its file name and aborts the entire import.
  Whole-car and individual-part imports also check material texture bindings
  against the selected original part. Unknown texture sources and surface types
  are rejected; deliberately untextured vertex-colour surfaces remain supported.
- Driving & transmission edits rev limit, redline, forward-gear count, AT/MT
  availability (including MT-only), automatic acceleration scale and six pairs
  of AT shift thresholds. Speeds use the in-game speedometer scale; unchanged
  values retain their exact stored representation. Save as mod writes the model
  flag and specification changes into one disabled mod, preserving other bytes.
- Selected-mod previews in My mods include semantic PNG/material overrides,
  with Original/Modified comparison for whole cars. Switching preserves camera,
  zoom, background and wireframe; both versions use the original model's framing
  so geometry scale changes remain visible.
  including material-only mods through Preview appearance. Lighting is simplified;
  raw/legacy texture overrides, nonzero palette variants and dynamic save markings
  are not fully represented in the preview.

## Boundaries and tests

`resources/rage-port.ini` is the distribution configuration template. Native
builds preserve it instead of copying the repository-root INI, which may contain
local development paths or diagnostic settings. Player settings remain in the
application profile.

`main/service.cjs` owns persistence, jobs and child processes. `main/index.cjs`
owns native dialogs and validates IPC senders. Files selected in dialogs remain
in the main process; the renderer receives save handles rather than arbitrary
filesystem access. `main/config.cjs` and `main/mods.cjs` can be tested without a
window. The native helpers are `rage-save-cli` and `rage-mod-cli`.

`npm test` checks configuration preservation, save copies and checksums, native
mesh/material validation, texture decoding, car assembly, parameter editing,
mod conflicts, cancellation and failed profile writes. The native binaries must
be staged first. Additional
manual integration evidence is in ignored `../build/launcher-evidence/`:
actual NTSC-U import (135 entries), Electron settings persistence, editing a
separate save with valid checksums, asset browsing, and launching the modern
game. Linux arm64 and x64 passed all 39 launcher tests, owned-disc car-set
export/import/preview, MT-only/AT readback, and a packaged UI check covering slot
selection, rival filters and combined parameter edits. Linux x64 also passed a
fresh CUE import with all 135 entries. Both packaged UIs ran as a non-root user
under Xvfb with Electron sandboxing enabled in containers permitting Chromium
namespaces. The x64 package is available in `out/Rage Mod Manager-linux-x64/`;
its tests used Debian 12 under x86-64 emulation. These checks do not establish
Windows compatibility, support for older Linux distributions, or complete Linux
gameplay/rendering coverage.

Packaged Play was also exercised on Linux arm64 and x64 with a controlled race
scenario. Both reached the modern renderer, captured a textured race frame at
scene 12/timer 30, stopped at timer 35 and returned to the launcher without a
process error. These Xvfb/llvmpipe checks verify startup and a short rendering
sequence, not hardware performance, audio output or an entire race.

For isolated development checks, `RAGE_LAUNCHER_USER_DATA` selects a separate
profile directory in unpackaged builds only. Packaged builds use the platform
application-data directory unless explicitly given `--user-data-dir=PATH`,
which the package probe uses for isolation. No remote service or telemetry is used.

Exported mod folders include `rage-mod.json` with format version `1`, the display
`name`, and the source disc `region` (`PAL`, `NTSC-U`, or `NTSC-J`). Reimporting
preserves these values and keeps the mod disabled. A mod from another region can
be kept in the library but cannot be enabled for the current disc. Older folders
without this metadata use their folder name and the currently loaded disc region.
This metadata records compatibility; it does not verify the origin of mod assets.
Optional `author` (120 characters), `version` (80) and `description` (1200)
fields are retained on import/export and displayed as plain text in My mods.
Descriptions may contain line breaks; author and version must be single-line.
Use Details in My mods to edit these fields and the mod name. Changes are saved
to the launcher profile and included in future exports; compatibility and asset
files remain unchanged.

Mod metadata may include a stable `packageId` and `requires`, for example
`[{"packageId":"author.base-car","version":"1.0"}]`. The optional version is
an exact match; omitting it accepts any version. Enable matching dependencies
first. Disabling/removing a required mod or changing its required version is
blocked while dependents are enabled. Launch rechecks the dependency graph.
Package IDs appear in Details and survive export; exported older mods receive
their existing installation ID as their initial package ID. Dependencies are
declared in the package metadata and displayed in My mods.

Active selections are now checked by `rage-mod-cli --check-selection`, using
the same C graph engine as runtime manifest ordering. Installed TOML `requires`
also participate, in their own manifest-ID namespace, and mixed dependency
cycles are rejected before composition. The launcher rereads TOML rather than
trusting cached dependency information. Exact JSON versions and disc regions
remain enforced; resource conflict choices remain explicit. The current UI
still displays JSON dependencies only. See `../docs/mod-manifest.md` for limits
and the remaining source-snapshot/provider-stack work.
