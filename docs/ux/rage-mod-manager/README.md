# Rage Mod Manager — UX prototype

Open `index.html` by double-clicking it. The prototype is self-contained, works
offline and does not require a build, server, dependencies or access to game data.
HTML/JavaScript here is a disposable design artifact, not the implementation
technology for the application. The production application remains C/C++ and
SDL3/ImGui on Windows, macOS and Linux.

## Review walkthrough

1. In **Źródło gry**, choose the example image. Observe preparation progress,
   cancel once, then repeat and continue to the library.
2. In **Moje mody**, install the example package, toggle it and open its details.
   Use the scenario selector at the bottom left to review an empty library,
   a conflict and an import failure. Resolve the conflict before playing.
3. In **Edytor moda**, select a car, compare the original with the modified
   preview, import a replacement and change its color. Try leaving without
   saving. Review the save/discard/cancel decision.
4. In **Edytor zapisu**, change credits using typing and the step buttons. Enter
   60 seconds to see validation, correct it, try “Brak rekordu” and save a copy.
   Escape restores the numeric field's value from when it received focus.
5. In **Budowanie paczki**, inspect the proposed contents and publication
   boundary. Export is a local operation, separate from publishing.

## Design decisions

- Library and authoring are distinct destinations. Installing a mod should not
  expose archive offsets or require understanding a mesh format.
- Most screen space in the authoring view belongs to the asset preview. The
  asset list and property inspector support that central task.
- Comparison uses the same view for both states. The production viewer should
  synchronize cameras and lighting and support inspecting wheel wells and decals.
- Exact quantities use numeric input and steps. Enums use named selections.
  Times use three labeled numeric segments with inline errors.
- Invalid values are not silently clamped. Save is unavailable while a time or
  credit value is invalid. Missing records are explicit, not a magic number.
- Unsaved changes require a save/discard/stay decision when leaving an editor.
- Progressive disclosure keeps technical details and raw entries out of the
  normal player workflow.
- Focus indicators, labels, keyboard access and responsive layout are included.
  Production accessibility needs native assistive-technology testing as well.

## Prototype boundaries

All data and validation reports are illustrative. Car images are original SVG
schematics for layout review, not actual game models or evidence of mesh quality.
File selection, drag/drop, OBJ conversion, disk persistence, mod loading, undo,
real 3D navigation and game launch are not implemented. Export/build/test actions
describe the expected result rather than creating files or launching processes.
The page does not upload anything, and reloading resets the session.

The profile selector and roughness field illustrate controls; they do not model
the complete underlying profile/material system. The no-record checkbox previews
the disabled time controls; it does not encode a save-format sentinel. Import
progress and errors are separate demo scenarios, not actual parser results.

Before implementation, review the desktop layout, split between player and
author workflows, field behavior, and amount of information shown. The broader
delivery plan is in `../../rage_mod_manager.md`.
