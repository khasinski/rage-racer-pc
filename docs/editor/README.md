# Rage Racer Editor — plan i wdrożenie

Edytor rozwijamy jako **jedną aplikację** w istniejącym `launcher/`, z edycją
zasobów w projekcie i zapisem dla prawdziwej gry. Plan obejmuje wszystkie
rozpoznane rodziny zasobów; nie wszystkie wymagania są już wdrożone.
[Stan wdrożenia i dowody testów](progress.md) opisują działające funkcje i braki.

- [Pełny plan](plan.md): UX, data hierarchy, source catalog, projekt i zapis,
  runtime integration, auta/tory/audio/menu/save'y, kolejność i koszty.
- [Audyt i notatki](audit.md): fakty z kodu, inventory, istniejące narzędzia,
  brakujące writery/loadery, regresyjne pułapki i decyzje.
- [Kontrakty i backlog](contracts.md): ownership, data boundaries, snapshot, dodatkowe auta/tory i classic adapter.
- [Macierz odbioru](acceptance.md): testy wymagane dla rzeczywistych funkcji.
- [Pomiary lokalnego PAL archive](inventory.json): tylko aggregate metadata/hash.
- [Klikalny mockup](../ux/rage-editor/index.html): reprezentatywne flow w jednej aplikacji.

Najważniejsze decyzje:

1. Wybierać nazwane auto/tor/element — nie anonimowy wpis archiwum.
2. Jeden projekt, draft/history i domyślne Save & apply; import nie tworzy nowego disabled mod.
3. Reuse istniejących C codecs/validators i Electron; wycofać stare ścieżki po coverage.
4. Gameplay/preview/game używają tego samego resolved snapshot. Spec edits nie
   mogą zostać nadpisane późniejszym cars.toml.
5. Track editor obejmuje terrain/placements **i** driveable route, AI/kamery/events.
   Nie deklarować gotowości po samym dopracowaniu auta.
6. Popularne model exchange GLB/glTF + existing OBJ/MTL, PNG, PCM/WAV; importer
   zajmuje się bindingami/material IDs, użytkownik nie naprawia `rage_N`.

Kolejne prace wynikają z braków w macierzy odbioru i aktualnym stanie wdrożenia.
Gotowość wymaga rzeczywistych zmian w grze oraz testów odpowiednich dla każdej
rodziny zasobów.

## Editing a complete car with GLB

In Cars, choose the car and use **Export for editing**. Open the exported
`car.glb` in your 3D editor. It contains body and wheel meshes, wheel instances
and embedded PNG textures. Coordinates currently use native game units, Y-up.

Edit geometry, UVs, vertex colours or base-colour PNGs. Keep the exported custom
properties (`rr_part`, `rr_instance`, `rr_source` and material `rr_material`);
when exporting from Blender, enable **Custom Properties**. Keep both front-wheel
instances linked to the same mesh. Compatible local transforms are supported;
independent edits to the shared front wheels require a future runtime format.
Export a static GLB without skins, animation or compression extensions.

Use **Import edited GLB/glTF** on the same car and disc revision. Invalid input leaves
previous edits intact. The complete import is one undo step. Use **Save & apply**
to publish the project to the game's automatically detected mod directory.
Geometry replacement currently targets the modern renderer. Shader graph/PBR
property changes are not imported. If custom properties were lost or refer to
another source, the importer opens **Assign car parts & materials**. Assign
Body, Rear wheels, Front left wheel and Front right wheel, then choose the
original car materials for the imported materials. The selected target car is
shown in the dialog. Confirm with **Import mapped car** or cancel without
changing the project. Both front wheels must share one mesh. A stale dialog is
rejected if the project changed after you selected the file.

Use **View part** to inspect imported geometry and embedded textures in 3D
without losing the assignments. Drag to rotate, scroll to zoom or choose a
front/side/rear/top view. This inspection shows the imported part before game
material assignments are applied. Choose a target material and use **View target
texture** to inspect its actual car texture; glass and untextured inner surfaces
show a colour swatch. Changing the choice clears the previous thumbnail.

The exported folder also contains three OBJ models and shared PNGs. Import that
folder through **OBJ sets and individual parts → Import OBJ car set folder**.

External `.gltf` imports also work for a complete car or an individual part.
Select the `.gltf` file; keep its BIN and PNG files in the same directory or
subdirectories. Relative paths, spaces and URI-encoded Unicode names are
supported. Import snapshots the referenced bytes into a private GLB, so editing
the source files after selecting them does not change a pending import.
Missing/truncated sidecars are errors. Canonical base64 data URIs also work:
`application/octet-stream` or `application/gltf-buffer` for buffers and
`image/png` or `image/jpeg` for images. They do not need sidecar files. JSON input, including
base64 payloads, is currently limited to 8 MiB; larger assets can use external
BIN/PNG files (up to 128 MiB for the normalised GLB).
JPEG textures also work as local files and embedded GLB images. The compiled
importer converts them to PNG automatically; original JPEG files stay intact.
Supported 8-bit JPEG images are at most 4096 × 4096 and 32 MiB. Geometry and
material assignments use the same importer and history as PNG.
Remote URIs, parent paths, symlinks, animation/skins and
unsupported extensions are currently rejected.
Use PNG/JPEG textures and static GLB/glTF for this workflow. Original files stay intact.

## Rival racing-line hints

In Tracks, expand **Rivals · Racing line**, choose **Forward** or **Reverse**,
then a stretch. Set its minimum/maximum sideways offset and the correction per
simulation tick. These are lateral game units, not heights. The game nudges the
front four rivals only when no nearby car blocks them; equal bounds disable the
nudge. This is one part of rival driving behaviour, not a full replacement AI.

Start/end progress are under **Stretch boundaries**, in internal progress / 16.
Keep entries ordered and non-overlapping. **Save racing line to project** creates
one Undo step; **Save & apply** publishes it for the next race. An unsupported
source table is shown explicitly. Sentinel, unused records and reserved bytes
are preserved. Adding hints, start grids, speed keys and camera editing remain
separate pending work.

## Rival pace keys

In Tracks, expand **Rivals · Pace keys**, choose the direction and a key. Four
basic target fields tune the leading rivals. The game interpolates targets
between progress keys, derives acceleration limits from them and scales later
rivals from rival 4. These values are not kph/mph speed limits.

Progress and slide pitch are under **Progress & slide pitch**. Progress uses
internal track progress / 16; pitch uses 4096 units per turn. Keep authored order:
retail data includes local progress reversals and tables using all 48 slots.
Save to project, Undo and Save & apply work like other track edits. Sentinel -1
and unused slots are preserved; adding/removing keys is not connected yet.
