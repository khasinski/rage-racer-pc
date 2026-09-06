# Rage Racer Launcher — web UX project

Open **index.html** in a desktop browser. Keep `launcher.css` and `launcher.js`
beside it. No server, build, network connection or game image is required.

This supersedes the mod-manager-only prototype as the proposed application
structure. The Electron implementation now lives in
[`launcher/`](../../../launcher/README.md). This folder remains the illustrative
web prototype; its simulated interactions are not implementation evidence.

## Screens and review route

1. **Play**: a compact launch panel, settings summary, sample save, active mods and
   shortcuts to authoring. Play displays the launch handoff; it starts no process.
2. **Settings**: image quality, HUD/camera, controls and advanced settings.
   Change a value, save it, return home and reload the page: committed settings
   survive in browser local storage. Leaving a dirty form offers save, discard
   or stay. Defaults require confirmation.
3. **Save editor**: exact credit input, segmented lap time, missing-record state
   and gearbox selection. Invalid seconds disable saving. This is a control
   study, not the full existing save editor or actual save-file persistence.
   The sample missing-record toggle is not a verified game-format operation;
   the production editor preserves unusual values instead of inventing a sentinel.
4. **My mods**: install, enable/disable, details and uninstall. Use the scenario
   selector to review conflicts or an empty library. Home reflects active mods.
5. **Mod editor**: inherited authoring prototype with asset search, OBJ/MTL
   import/export interactions, schematic comparison and material colors.
6. **Build package**: review package contents and provenance before local
   export. No automatic publication.
7. **Game source**: simulated CUE/CHD/BIN choice, preparation, cancellation and errors.
   The game-data import remains automatic in the proposed shipping flow.

## Settings mapped to the repository

Labels, defaults and ranges come from the root `rage-port.ini`. The prototype
exposes a deliberate subset rather than claiming to cover the entire file:

| Category | Source keys |
| --- | --- |
| Image | `video.internal_scale`, `aspect`, `fps`, `texture_filter`, `post`, `grading` |
| HUD/camera | `hud.anchor`, `show_lap_times`, `show_time_limit`, `camera.chase_turn_lookahead`, `modern.mirror_distance` |

| Controls | `input.analog`, `wheel`, `steering_linearity`, 16 keyboard button mappings |
| Advanced | `video.draw_distance`, `timing.standard`, `diagnostics.performance`, `marker_capture` |

Numeric fields validate without silently replacing invalid input. Time and
gamepad editors use example data; configuration fields use actual INI semantics.
The presentation-rate menu offers common values from the broader supported
range. Keyboard binding capture demonstrates the interaction, not exhaustive
browser-to-SDL key translation or duplicate-binding validation. Full wheel axis
calibration and other analog response settings need a subsequent UX pass.

**Preview configuration** shows the mapped values and can download a sample INI.
It is intentionally marked as partial and must not replace the full game config.
Production configuration writing must preserve unknown keys and validate with
the same compiled configuration code as the game. There is no launch-volume
setting in the current INI, so the design does not invent one.

## State and limitations

- Browser local storage persists applied settings and basic mod/source readiness.
  Clear site storage to reset; editor drafts and illustrative records are not
  durable files. The demo begins without a game image. Settings, saves, mods and
  authoring are disabled until preparation succeeds. Cancellation and import
  failure keep them disabled.
- Original vector illustrations show composition and layout; they are not game
  screenshots or redistributed original models/textures.
- Archive access, actual asset conversion, save checksums, game launch, package
  validation, material physics, and 3D navigation remain simulated.
- No IPC, Node access, uploads, remote fonts or third-party runtime libraries.
  Production Electron would require a separate narrowly scoped native bridge;
  the existing C/C++ import and game logic should remain separately testable.
- Settings belong to the launcher profile. Gameplay modifications belong to mod
  projects. The prototype has one named profile; profile creation is not built.
- Android is outside the current design scope. Desktop layouts are the priority.

## Next design decisions

Review the home screen's balance between launching and editing, discoverability
of advanced options, the control-binding interaction, and whether the mod editor
should occupy a separate window. Then design the complete save browser and wheel
calibration before connecting real data. Framework selection and packaging are
separate from this UX artifact.

## Review verification

Checked in headless desktop Chrome using Playwright on 2026-09-05:

- Invalid resolution scale blocks saving; valid applied values update home and
  survive reload. Discard restores the previous committed value.
- Keyboard capture updates the INI preview; the sample INI downloads correctly.
- Reset defaults, time validation and unsaved-change navigation work.
- Unresolved mod conflicts block the home-screen launch action.
- Home, settings, mods, workshop, saves and package screens fit a 1024px viewport
  without page-level horizontal overflow. No browser JavaScript errors occurred.

PNG previews `01-home.png` through `06-package.png` were captured from the
prototype. These checks validate browser interactions, not native packaging,
actual game behavior or Windows/Linux platform integration.

## English revision and first-run design

The interface, dialogs, validation messages, accessible labels and downloaded
INI comments are in English. The large home illustration has been removed.

First launch shows one task: select a CUE, CHD or Track 01 BIN. All data-dependent
sections remain locked until preparation succeeds. The demo button simulates
a North American disc; it does not detect a real file. The scenario selector
can explicitly simulate a prepared installation for reviewing later screens.

Car names and prologue follow the loaded disc version. They are no longer
editable settings and the sample INI does not export content overrides. The
source screen displays the detected edition as read-only information. The
prototype storage version changed to ensure older prepared-demo state does not
skip this new first-run experience.
