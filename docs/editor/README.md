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
