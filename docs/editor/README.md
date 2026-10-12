# Rage Racer Editor — decyzje do wdrożenia

Ta runda jest analizą i planem. Docelowy produkt ma być **jedną aplikacją**,
opartą na istniejącym `launcher/`, z project-based editing wszystkich rozpoznanych
rodzin zasobów. Nie powstał kolejny działający editor ani kolejny backend gry.

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

Kolejny konkretny implementacyjny krok po przeglądzie: wspólny C Catalog dla
prawdziwego auta + całej Mythical Coast, Project deltas i end-to-end resolved
parameter/texture loop. Potem pierwsza structured track modification przed
rozbudowaniem reszty formularzy.

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

Use **Import edited GLB** on the same car and disc revision. Invalid input leaves
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

The exported folder also contains three OBJ models and shared PNGs. Import that
folder through **OBJ sets and individual parts → Import OBJ car set folder**.
