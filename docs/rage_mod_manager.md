# Rage Mod Manager

Status: product and UX specification. The Electron implementation is in
[`launcher/`](../launcher/README.md); verified behavior and unfinished work are
recorded in [`launcher/IMPLEMENTATION.md`](../launcher/IMPLEMENTATION.md).

The current product direction is a desktop **Rage Racer Launcher** with settings,
an integrated save editor, mod management and an authoring workspace. Review the
new [web launcher project](ux/rage-launcher/index.html) and its
[scope and walkthrough](ux/rage-launcher/README.md). Electron is now the implementation framework;
the previous SDL3/ImGui UI assumption is withdrawn. Existing game and asset
processing code remains in the compiled toolchain.

An offline clickable [UX prototype](ux/rage-mod-manager/index.html) and its
[review walkthrough](ux/rage-mod-manager/README.md) are available for design
review before application implementation. The prototype uses illustrative data.

## Purpose and platforms

Rage Mod Manager is a desktop application for installing mods and authoring
changes to Rage Racer assets. Ship native applications for Windows, macOS and
Linux. The application uses Electron; the web launcher prototype records the
design direction. Reuse the repository's compiled asset-processing libraries.
Do not require Python or a command-line setup step.

A clean installation starts by double-clicking the application. The only game
data requested is a legally obtained CUE or Track 01 BIN. Disc reading and all
required modern asset generation happen automatically, with progress, cancellation
and actionable errors. Starting the game uses the modern renderer.

## Two primary workflows

The default screen is **My mods**. Players should not need to understand an
archive, mesh bank or material identifier to install and play a mod.

1. Select the game image on first launch. Show detected game version and import
   progress; remember the selection.
2. Install a mod by dropping a package onto the window or choosing **Install mod**.
3. Show its name, author, version, description, affected cars/tracks and a preview
   when supplied. Validate compatibility and dependencies before activation.
4. Enable mods with checkboxes. Show conflicts next to the affected mods, naming
   the asset or setting both change. Let the user explicitly choose precedence.
5. **Play** prepares required assets automatically and launches the game with
   the selected profile. Profiles retain enabled mods and conflict choices.

**Create / edit mod** opens a separate authoring workspace. The player-facing
screen must not become an archive editor with an extra launch button.

## Authoring workspace

Use a searchable asset browser on the left, a large preview in the center and
properties on the right. A changes panel lists the complete mod contents.

- Start with categories: Cars, Tracks, Textures, Materials and known Parameters.
  Display recognizable names and thumbnails. Put archive indices, offsets and
  native identifiers in expandable technical details.
- Selecting a car opens its assembled body and wheels, with orbit, zoom and
  front/side/rear views. Include lights and backgrounds suitable for inspecting
  glass, paint and dark wheel wells.
- Provide Original / Modified / Side by side views with synchronized cameras
  and lighting. Show the current mod's contribution separately from the final
  result when other enabled mods also affect an asset.
- Show body, wheels, windows and markings as selectable parts. Preserve explicit
  glass, paint, rubber, metal and decal materials through import/export.
- Include a markings preview that makes the hood logo and windshield lettering
  visible. These can involve runtime composition, so static texture previews
  alone are insufficient to validate them.
- **Export for editing**, **Import replacement**, **Restore original** and
  **Test in game** act on the selected asset. The first mesh exchange format is
  the existing OBJ/MTL workflow. Native RRMESH is generated automatically.
  Blender files remain external authoring documents, not a required runtime
  interchange format. Add glTF only after agreeing on and testing a material
  and coordinate-system mapping.
- Imported meshes get a preview and validation summary before applying:
  materials, missing textures, scale, orientation, wheel placement and geometry
  problems detectable by the importer. Report errors beside the affected part.
- Changes are staged in a mod project; do not modify the user's disc image.
  Undo/redo covers edits. Mark changed fields, support resetting individual
  fields and provide an explicit Save action with an unsaved-change indicator.
- **Build package** shows the exact files and parameter changes to be included.
  Do not include an entire extracted archive or cached original assets by default.

## Shared interaction rules for both editors

| Value | Control | Behavior |
| --- | --- | --- |
| Exact integer, such as credits or retries | Numeric field with − / + buttons | Direct typing, keyboard arrows, meaningful step and visible valid range |
| Known enum, such as gearbox or tires | Named choice or compact list | Show game terminology; put numeric codes in technical details |
| Save slot | List of slots with summaries | Identify team/progress and empty slots; no slider |
| Paint color | Palette swatches | Show selected color and name/index; no numeric slider |
| Duration | Segmented minutes : seconds . milliseconds | Independent numeric segments; no requirement to type punctuation |
| Unusual stored time | Explicit stored-value label | Preserve the original value; expose “No record” only where a sentinel is verified in game code |
| Volume or preview zoom | Slider plus displayed value | Sliders are suitable for continuous adjustment with immediate feedback |
| Unknown byte or bitfield | Advanced numeric/hex or bit controls | Explain that its meaning is unknown; do not invent semantic labels |

Duration controls display, for example, **01 : 40 . 765**, with segment labels
available visually or through tooltips. Tab moves between segments; arrows change
the focused segment. Accept pasting a complete supported game time as a
convenience, but ordinary editing must not depend on text parsing. Seconds range
from 0 to 59 and milliseconds from 0 to 999. Derive minute and total limits from
the actual stored field. Explicitly handle zero, unusual stored values and overflow.

Implementation audit: best-lap, total-time and sector-reference tables do not
have a verified missing-record sentinel. `InitRecordTables` seeds default times;
`RepairRecordTimes` replaces values <= 0 or above `RACE_TIME_MAX_MS` (599999 ms)
with course defaults when a save is loaded. See
[`records.c`](../src/main/PAL/main/race/records.c) and
[`load_save_state.c`](../src/main/PAL/main/save/load_save_state.c).
Do not write zero or -1 as a supposed empty record for these fields. Preserve
unusual source values unless explicitly edited. Ranking and time-attack rows
need their own semantic audit before adding any empty-record control.

Keep a temporary edit buffer: partially typed numbers must not immediately
overwrite the document, be reformatted on every frame or silently clamp to a
different value. Commit valid edits on Enter or leaving the field; Escape
restores the previous value. Invalid values remain visible with an explanation
and prevent saving until corrected or reverted. A committed gesture is one undo
step. Scrolling the page must not accidentally change a value.

Group settings by player intent, not memory layout. Put raw reserved fields and
unidentified bytes under Advanced. Use readable labels, units, adequate field
widths and consistent spacing. Avoid tables whose fields disappear at smaller
window sizes: stack inspectors or use explicit horizontal scrolling where needed.
Support keyboard navigation, visible focus and platform display scaling; status
must not rely on color alone.

## Scope and architecture

Existing foundations include the SDL3/ImGui save editor, C archive extraction
and packing tools, the OBJ-to-RRMESH importer, and texture/material overrides
through `mod.toml`. The current manifest does not provide a general external car
mesh mapping. Add that runtime capability before promising installable car packs;
currently embedded car replacements are not equivalent to a mod-loading API.

Separate reusable compiled libraries for disc/archive access, asset codecs,
mod projects, validation and packaging from the GUI. The game and manager must
use the same import and override behavior. Handle long-running imports off the
UI thread, publish progress safely, and write outputs atomically so cancellation
or failure leaves the previous working project intact.

“Any asset” has two levels: raw export/replacement of archive entries, and
structured editing of decoded formats. Clearly label unsupported formats as raw
data; do not claim every entry has a meaningful visual editor. Build structured
support incrementally. Some game parameters may live in executable code or other
disc data rather than RAGE.BIN; catalogue their actual location before exposing
controls. Modern renderer overrides and repacking a retail archive have different
constraints and must not be presented as interchangeable export targets.

Treat imported packages as data: validate paths, sizes and references, reject
archive path traversal and unsupported executable hooks, and validate everything
before activating a mod. Store user data in platform-appropriate writable
locations, independent of the installation directory.

## Delivery order and acceptance

1. Improve shared field interactions in the save editor: exact numeric controls,
   named choices, palette selection and segmented durations. Verify actual save
   round trips, sentinel preservation, editing cancellation and invalid input.
2. Build the manager shell and first-run CUE/BIN import, read-only asset browsing
   and clear support indicators. Verify clean double-click startup on all three
   platforms without development tools installed.
3. Deliver the first complete authoring loop for cars: original preview, OBJ/MTL
   export/import, separate materials, markings, package creation, installation
   into a clean profile and launching the game. Verify wheels, windows, hood
   logos and windshield lettering in the running modern renderer.
4. Add textures and documented parameter editors, then extend other decoded asset
   families. Verify conflict resolution, disable/uninstall restoring the original
   result, malformed packages and interrupted imports.

## Publication boundary

A separate tool and a mod-hosting site do not establish redistribution rights
for retail-derived models or textures. Audit the tool's code and dependencies
before licensing it. Exported files should record their provenance, and the
package review should identify game-derived content without pretending to give
legal clearance. Independently authored assets and local transformations of the
user's disc data provide useful distribution options. Binary differences alone
are not a guarantee that a package can legally be distributed.
