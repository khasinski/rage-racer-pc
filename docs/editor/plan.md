# Jeden Rage Racer Editor — pełny plan

**Status: plan do przeglądu, nie opis gotowych funkcji.** 2026-10-11.
Baza kodu: `main` / `5b3749725` z lokalnymi zmianami. W tej rundzie zmieniamy
wyłącznie dokumentację i mockupy; ostatnie eksperymentalne zmiany produktu
pozostają do osobnej oceny.

[Audyt kodu i danych](audit.md) · [Pomiary PAL](inventory.json) ·
[Klikalny mockup](../ux/rage-editor/index.html) · [Macierz odbioru](acceptance.md) ·
[Kontrakty i backlog techniczny](contracts.md)

## 1. Cel i granica „jednego narzędzia”

**Jeden Rage Racer Editor** pozwala przygotować grę, wybrać ustawienia,
zarządzać save'ami i modyfikacjami, edytować auta, tory, obrazy i audio oraz
sprawdzić rezultat w prawdziwej grze. Edycja odbywa się w projekcie, nie przez
zarządzanie dziesiątkami anonimowych podmian archiwum.

- Jedna instalowana aplikacja, jedna nawigacja, jeden projekt, jeden pipeline
  importu/walidacji/zapisu. Gra pozostaje binarką działającą również samodzielnie.
- `launcher/` zostaje bazą. Electron/UI i istniejące C biblioteki/worker procesy
  zostają; brak nowej aplikacji C++, Zig, serwera web albo frameworka UI.
- Kompilowane helpery mogą istnieć jako prywatny backend i test tools. „Jedno
  narzędzie” nie wymaga połączenia wszystkich procesów w jedną wielką binarkę.
- Przetwarzanie płyty, binarnych struktur, siatek, materiałów i save'ów pozostaje
  w C/istniejącym compiled toolchain. JavaScript prowadzi UI, pliki projektu i joby.
- Blender / edytor obrazów jest **opcjonalnym** zewnętrznym narzędziem dla
  modelowania szczegółowej siatki. Bez niego można zmienić parametry, materiały,
  tekstury, położenie obiektów, drogę i kamery. Nie budujemy drugiego Blendera.
- „Pełny editor” w tym planie oznacza wszystkie **rozpoznane znaczeniowo** rodziny
  zasobów gry, nie dowolne reserved bytes ani automatyczne odgadnięcie znaczenia
  nieznanego formatu. Nieobsługiwany element ma nazwę/context, stan read-only i powód.

Definicja sukcesu: laik wybiera **auto lub tor**, widzi je od razu, zmienia
rozpoznawalną rzecz, zapisuje, uruchamia grę i otrzymuje ten sam wynik. Nie musi
znać banku, CLUT, numeru assetu, `mod.toml`, RRMESH ani CLI.

## 2. Docelowa nawigacja i hierarchia

Główne sekcje: **Game, Cars, Tracks, Interface, Audio, Saves, Projects & mods**.
Advanced diagnostics/raw data są w menu narzędzi i details, nie obok auta na równych prawach.

**Game:** duży Play, wybrane źródło/wersja, aktywny zestaw, ustawienia Graphics /
Audio / Controls / Camera. Missing assets importują się automatycznie, postęp
w jednym miejscu. Save editor dostępny bez płyty. Pozostałe sekcje mają sensowny
empty state prowadzący do wyboru CUE/BIN (obsługiwany CHD zostaje).

**Cars:** wyszukiwana lista nazw i miniaturek. Rodzina auta ma wariant Grade,
źródło Player/Rival i kontekst klasy/course, nie „32 slots”. Główny obszar 3D
pozostaje widoczny podczas edycji. Zakładki:

1. **Appearance** — wybór części kliknięciem auta/listą; kolor, preset materiału,
   podmiana tekstury, import modelu. Wheels/glass/logos widoczne z nazw.
2. **Driving** — AT/MT, liczba biegów; rozwijane Engine, Gears, Handling.
3. **Shop & progression** — name/maker/class label, price, upgrade price,
   unlock race class, z rozróżnieniem upgrade Grade vs difficulty Class.
4. **Variants** — wybór i porównanie istniejących Grade/LOD/rywali; zakres zmiany
   „this variant” / jawnie wybrane kompatybilne warianty.

**Tracks:** lista czterech nazw, kierunek Forward/Reverse i klasa 1–6. Wariant
niedostępny w grze pozostaje niedostępny lub w Advanced jako placeholder.
Duży viewport 3D/Top + mini-map. Tryby pracy:

1. **Scenery** — wybierz w świecie; move/rotate/duplicate/remove/add z katalogu
   nazwanych obiektów; materiały i tekstury; teren osobno od instancji.
2. **Road** — widoczna nawierzchnia + overlay fizyki; punkty/szerokość/wysokość,
   przesuwanie uchwytów, kontrola rozjazdu geometrii i drogi.
3. **Atmosphere** — fog, sky, kolory i timeline, strefy/tunele, światła.
4. **Race & cameras** — grid, AI, finish/laps, replay i intro camera paths,
   scenery animation/event timeline. Zaawansowane AI/reverb w details.

**Interface:** Title/Main menu, Course select, Car select, HUD, Pause, Race/class
screens. Wybór konkretnego elementu z rzeczywistym layoutem, nie całego atlasu
bez podpisu. Grafikę wymienia się PNG; crop/UV i anchoring pod Advanced.

**Audio:** Main menu, Course/car select, Race music, Menu sounds, Engine/tyres,
Announcer, Track ambience. Nazwane cue + preview, waveform/trim/loop/volume;
engine/sequence/bank mapping w Advanced. FMV w Interface → Videos; brak udawanej
możliwości importu, zanim compiled decoder/transcoder będzie sprawdzony.

**Saves:** zachować już działające flow i C bibliotekę; Team / Progress / Garage /
Records / Advanced. Team logo ma preview na aucie po wyborze zgodnego źródła.

**Projects & mods:** projekty robocze, gotowe paczki, wybór aktywnego zestawu i
czytelne konflikty. Tu jest metadata autora/version, import/share, nie podczas
każdej zmiany koloru. UI rozróżnia „Saved in project” i „Applied to game”.

### Wspólne zasady

- Na wierzchu nazwa, podgląd, najczęstsze 3–6 czynności; reszta pod opisanymi
  zakładkami i accordionami. Puste accordiony nie są atrakcją produktu.
- Każde pole: readable label, jednostka, krótki opis skutku, walidowany zakres,
  reset pojedynczej zmiany. Źródło/default w tooltipie lub details.
- Kolor to picker; materiał to preset Paint / Glass / Rubber / Metal / Emissive
  + Advanced z rzeczywistymi parametrami. Preset nie usuwa ręcznych zmian.
- Liczby nie zmieniają się od przypadkowego scrolla. Input buffer pozwala wpisać
  wartość częściowo; invalid nie znika/clampuje się potajemnie. Enter/blur commit,
  Escape rollback. Jeden drag/stroke = jeden krok undo.
- Wybór elementu w świecie i w drzewie jest dwukierunkowy. Escape deselect;
  Delete pokazuje skutki usunięcia współdzielonego obiektu.
- Pasek projektu: nazwa, unsaved, Undo/Redo, **Save & apply**, **Test in game**.
  Domyślny zapis przygotowuje wynik dla gry jednym kliknięciem. Save project only
  oraz Share są dodatkowe, nie „Save as disabled mod” na każdej formie.
- Desktop 1440/1280: lista + viewport + inspector. 960: lista/inspector wysuwane,
  viewport ma priorytet. Keyboard focus, HiDPI, kontrast/status tekstem.
- Nie pokazujemy generic „Export asset”. Kontekst: **Export model…**, **Replace
  texture…**, **Export track scene…**, **Share project…**. Advanced raw może pozostać.

## 3. Katalog znaczeniowy zamiast anonimowego archiwum

Jedna biblioteka C dekoduje źródło do **Catalog**, z której korzystają game i
editor. GUI dostaje names/roles/references/capabilities, nie rekonstruuje layoutu.
Przebudowa katalogu tylko po zmianie źródła/importera/projektu, nie per frame.

Każdy rekord ma stabilny ID, display name, kind, context (region/revision,
family/grade, course/class/direction), source bindings, preview bounds, role,
referencje do siatek/materiałów/obrazów i capability. Numeric bank ID jest wyłącznie
bindingiem do płyty. Identyfikator źródła: fingerprint + region **i** revision;
NTSC-J 1.0 i 1.1 nie są automatycznie wymienne.

- Nazwy aut ze wspólnej regionalnej tabeli + wybranego cars catalog. Original
  disc, built-in replacement i effective value są jawnie odróżnione.
- Siatki rywali deduplikujemy dopiero po layout/reference/material fingerprint,
  zachowując wszystkie miejsca użycia. „Compact rival — Class 2, body B” jest
  uczciwą nazwą, jeśli brak oryginalnego imienia; nie nazywamy wszystkiego GNADE.
- Tekstura ma thumbnail i **Used by**. Semantyczną nazwę typu Asphalt/Tunnel
  potwierdza mapping ręcznie lub fixture; nie zgadujemy na podstawie małej PNG.
  Nienazwana, ale rozpoznana: „Surface 07 — Tunnel section”, z kliknięciem miejsca.
- Course models, terrain faces i procedural scenery to osobne klasy selekcji.
  Użytkownik może nadać alias w projekcie; alias nie zmienia runtime tożsamości.
- Raport coverage musi sklasyfikować 135/135 wpisów: semantic/editable,
  structured/read-only, known container, reserved/unknown. Osobno CD i FMV.
  „135 exported files” nie spełnia tej bramki.
- Konflikty zgłaszamy „Esperanza Grade 2 → Front wheels → Glass texture”, nie
  `asset_016.bin` albo `car.3.material.4`. Technical details zachowują te klucze.

## 4. Jeden dokument i jeden zapis

Proponowany format **folder projektu**:

```text
My coast project/
  project.rrproject       # versioned JSON: metadata, source fingerprint, references
  cars/esperanza-g2.json  # tylko zmienione parametry/role/bindingi
  tracks/mythical-coast.json
  interface/main-menu.json
  audio/course-select.json
  meshes/*.glb            # źródła user edits, nie obowiązkowe dla każdego projektu
  textures/*.png
  audio/*.wav
  sources/               # opcjonalne .blend itp., nie trafiają do paczki runtime
```

To **proponowany** schema 1; nazwy/key syntax nie są obecnym runtime API.
JSON opisuje dokument i semantic deltas; nie ma dwóch kopii tego samego pola w
raw patchu, TOML i editor JSON. Compiled `ProjectBuild` wyprowadza runtime snapshot
z aktualnego C parsers/writers. `cars.toml` importujemy jako warstwę danych,
zachowujemy komentarze/unknown entries przy edycji zewnętrznej; format pozostaje
wspierany i eksportowalny. Nowe ceny/handling editor nie zapisuje jako raw spec patch.

Priorytet docelowy: disc defaults → opcjonalne built-in appearance → wybrany
cars catalog → aktywne paczki z jawnym providerem konfliktów → roboczy projekt
(tylko preview/test) lub zapisany projekt (po Apply). Cena/handling/availability
używają jednego resolved car record po tej kolejności. `retail_manual_only`
nadal respektowane i opisane, jeśli blokuje AT.

**Save & apply (domyślnie):** waliduje dokument, buduje runtime snapshot, zapisuje
staging + manifest/checksum i commit
transakcji projektu, a następnie atomowo aktywuje snapshot; dopiero po wyniku
aktualizuje dirty/active state. Save project only jest dodatkową opcją dla projektu,
którego nie chcemy jeszcze aktywować. W zwykłym flow: edytuj → Save & apply → Play. Autosave recovery jest osobne; nie
udaje użytkownikowego Save. Undo zachowuje stare references do immutable plików,
nie kopiuje 1 GB projektu na każdy gest. Usuwanie orphanów dopiero po Save/checkpoint.

**Aktywacja (część Save & apply):** cały zweryfikowany snapshot aktywnego zestawu
trafia do user
mod root wspólnego z grą. Gry uruchomionej w trakcie nie modyfikujemy; „Saved;
applied on next launch”. Game standalone i editor Play używają dokładnie tego
samego snapshot i precedence. Nie wymagamy edycji INI.

Nie podmieniamy folderu przez nieodzyskiwalną parę rename. Docelowo `mods/active.json`
atomowo wskazuje niezmienny `mods/builds/<id>/`; loader sprawdza marker/version/
source fingerprint **przed** aktywacją. Poprzedni build zostaje do rollbacku,
usuwanie przez GC, nie w trakcie commit. Version-1 `mods/mod.toml` i explicit
`mods.directory` pozostają kompatybilne, explicit empty nadal wyłącza mods.
Unmanaged folder/importowany mod nigdy nie jest potajemnie nadpisany.

Jeśli zapis projektu się uda, lecz aktywacja zawiedzie, status mówi „Saved in
project; not applied” z konkretnym błędem i Retry. Poprzednia aktywna wersja działa.
Nie pokazujemy sukcesu całego Save & apply po połowie transakcji. Autosave i
Save project only nie zmieniają aktywnego wskazania.

**Share:** jedna paczka projektu ze źródłami zaznaczonymi opcjonalnie; runtime
package zawiera tylko changed files, compiled data, metadata i required source.
Wyjściowy prosty format folder/ZIP, nie nowy instalator. Żadnych game executable
hooks ani dowolnych ścieżek. Import pozostaje disabled do świadomego Apply.

## 5. Formaty edycji i roundtrip

### Modele

Docelowy domyślny **GLB/glTF 2.0** dla całego auta lub sceny toru; zachować OBJ/MTL
jako prosty import/export części. GLB przenosi node hierarchy, materiały i obrazy
w jednym pliku; `.gltf` pozostaje opcją dla łatwego edytowania PNG osobno.
Mapping i metadata ustala importer C, nie `usemtl rage_123` wpisywane przez gracza.

Role body/front-wheel-left/right/rear-wheels, pivots, LOD i stable source IDs
trafiają w `extras` oraz do projektu. Blender wspiera import/export custom
properties przez glTF extras, ale użytkownik może je zgubić — importer ma wtedy
wizard przypisania części i materiałów, zamiast przypisać wszystko do body.
[Spec glTF](https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html),
[Blender glTF manual](https://docs.blender.org/manual/en/5.1/addons/import_export/scene_gltf2.html).

Kontrakt do przetestowania przed wdrożeniem: right-handed Y-up; scale konwersji
wynika ze współrzędnych silnika (nie deklarujemy arbitralnie 1 unit = 1 m);
transforms/pivots i negative determinant; poprawne normals/winding; UV wrap/window,
linear vs sRGB, vertex alpha; pełny material word i per-face depth bias;
transparency/double-sided; hierarchy/instancing; unknown extras zachowane.
Materiał preset/PBR jest mapowaniem do obecnych możliwości gry, nie obietnicą
pełnej zgodności dowolnego shader graph Blendera.

cgltf jest kandydatem C (nie wybrana/zintegrowana już zależność). Spike ma
przetestować write+parse+texture mapping i ograniczone embedded/external buffers.
Nie rozpoczynamy ogromnej warstwy wszystkich formatów 3D. FBX i bezpośredni `.blend`
nie są w pierwszym kontrakcie; external Blender eksportuje GLB/OBJ bez pluginu.
Unsupported animations/skins/extensions mają nazwany błąd, nie silent stripping.

### Obrazy, audio, video

- PNG RGBA domyślnie; popularne JPG do importu jeśli obecny compiled decoder
  wspiera bez dodatkowego deploy dependency. Runtime zawsze nasz zweryfikowany format.
- Paint mask, palette/variant i transparency są osobnymi właściwościami,
  nie paleta PS1 narzucona wszystkim nowym obrazom. Original classic texture
  patch ma nadal prawdziwe limity; natywna nowa ścieżka nie powinna ich dziedziczyć.
- WAV/PCM start audio, normalize format automatycznie; preview volume osobno
  od zapisanej gain. Loop points/duration/cue mapping są częścią dokumentu.
  OGG/FLAC dopiero z compiled codec + testami, nie shell ffmpeg wymagany od gracza.
- Muzyka menu pozostaje region-aware extraction → PCM → player. Replacement
  wskazuje PCM asset, nie reintrodukuje drugiego SEQ playera.
- Film: na początek native stream preview/read-only. Import MP4 jest osobną bramką
  wyboru/bundlowania compiled codec; nie dodajemy Python ani systemowego ffmpeg
  jako player prerequisite. Można dostarczyć editor bez video import i uczciwie
  pokazać tę granicę.

## 6. Auta — szczegółowy zakres

| Obszar | Podstawowe | Advanced / podgląd |
| --- | --- | --- |
| Appearance | Part list, Paint/glass/rubber preset, PNG replacement, Import model | Material source, roughness/metallic/opacity/emission, UV, depth bias, paint mask, LOD |
| Assembly | Wheel placement gizmo, front/rear/side views | Suspension offsets, mirrored instances, steer/spin test, variant families, collision/contact bounds |
| Driving | AT+MT / MT only; gears | Torque curve/band/loss, gear ratio/load/scale, rev limit/redline, AT scale, shifts as speed, steering/grip/drag/neGcon fields |
| Shop | Name, maker, class label, price, unlock Class | Upgrade price, Grade progression, per-region aliases, field origin |
| Lights | Select front/tail/brake surface, nighttime/braking preview | Beam/pivot settings only where runtime actually supports per-car overrides; add runtime schema before exposing them |
| Markings | Paint, logo, driver-name preview | Runtime composition/atlas mask, show source save vs project |
| Rival | Actual model and assembly; correct class/course filters | Current handling donor, eligible contexts, final Class 6 4-model restriction, per-context bindings |

Nie dodajemy nowych 14+ modeli w pierwszej wersji, zanim menu/save/session arrays
nie staną się data-driven. Podmiana istniejącego auta i dekoracje toru nie są tym
samym przedsięwzięciem co arbitralnie większy roster. Nie duplikować prawie
identycznych wariantów; batch edit dopiero jawny wybor affected contexts.

## 7. Tory — obowiązkowy zakres, bez udawania

### 7.1 Selection i wygląd

Z C importera zebrać terrain cells, course geometry i oba model banks wraz z
runtime placements. Scena obejmuje actual visible instances plus named groups:
Road, Terrain, Buildings, Barriers, Tunnel, Start/finish, Animated scenery, Rivals.
Sama bank geometry bez world placement nie daje podglądu toru.
Pick triangle → pokaż owner/face/material/reference. Wtopiona geometria ma nazwę
„part of terrain”, nie fałszywy ruchomy obiekt. Wypięcie fragmentu do native
placement jest osobną świadomą operacją.

Nowe decorated meshes/instances mają **generic native scene override** niezależny
od `s_authoredCars` i PS1 atlasu/OT. Transformy i bound checks są takie same w
editorze i grze. Nowe road collisions wymagają danych Road, nie mesh collision
obiecanej bez backendu. Shadows/fog/reflections rejestrują nowe instance w render
world/ray scene i usuwają je po zmianie course/session.

### 7.2 Road editing

Droga logiczna ma points + arc centers + surface widths/pitch/cross-slope.
Najprostsza uczciwa pierwsza implementacja: edycja istniejących punktów i
parametrów, overlay, walidacja, **guided matching** do wybranej surface.
Zmiana trasy pokazuje dwa jawne tryby:

- **Appearance only**: replace scenery/road mesh; fizyka bez zmian, overlay pokazuje rozjazd.
- **Driveable road**: edit route handles; aktualizuj route/surface/racing line,
  starty/finish/AI/camera intervals i visibility; zablokuj build przy niezgodności.

Nie próbujemy automatycznie dopasować nowej drogi do starego asfaltu w dowolnej
siatce. Road generator dodajemy jako prostą sweep mesh z szerokości i przekroju,
bez niszczenia istniejących detali: nowy corridor maskuje zastępowane fragmenty
po zaakceptowaniu użytkownika. Zachowaj originale, seam inspection i rollback.
Łuki/segmenty są bake'owane przez wspólny C backend; endpoint, wrap, reverse i
surface tangent tests obowiązkowe. Jeśli solver nie mieści wyniku w aktualnym
runtime formacie, pokazuje limit; nie truncuje halfwordów.

**Trasy od zera i dodatkowe sloty** są kolejną warstwą: dynamic track identity,
selector/records/save route context i native geometry/visibility. Zaplanowane
poniżej, ale nie deklarowane jako wynik samego importu GLB. Początkowy editor
musi edytować istniejące trasy z fizyką, a nie tylko wyświetlać nimi listę.

### 7.3 Atmosphere / Race / Cameras

Fog/sky/environment timeline jest autorstwem toru. Nowy snapshot po resolvowaniu
wykorzystują game camera i oba docelowe render adapters. Suwak czasu w viewport
pozwala zobaczyć zmianę, bez losowego globalnego timera.

Strefy światła/tunelu, race ambience, camera intervals i AI keys są pokazane
na wspólnej osi distance/lap, osobno per direction. Zmiana length remapuje
jawnie zaakceptowane procentowe pozycje i ostrzega o absolutnych pozycjach;
nie przestawia automatycznie wszystkich scen/eventów.

Grid editor widzi tylko aktywne samochody, kamera intro scrubbable, replay camera
hard cuts przetestowane. Flyby/scenery osobna oś czasu z presetami/keyframes;
proceduralne hard-coded obiekty najpierw przenieść do nazwanych danych, zachowując
retail output. Raw flags pozostają advanced do czasu semantycznej weryfikacji.

## 8. Runtime i compatibility

**Kluczowa decyzja:** nie udajemy, że classic już wczytuje RRMESH/PBR. Dzisiejszy
classic rysuje własne recovered streams. Plan rozdziela możliwości:

| Zmiana | Modern, cel pierwszych pionowych wdrożeń | Classic |
| --- | --- | --- |
| Car metadata/handling i route/events/cameras | Wspólne resolved gameplay records | Ten sam gameplay record |
| Zachowany indexed image patch | Istniejąca wspólna ścieżka, testy | Zgodny patch, paleta/rozmiar jawne |
| Native car/track geometry, nowe PNG/PBR | Generic override + semantic material registry | Wymaga native geometry adapter; nie twierdzimy, że już jest |
| Nowe scenery instances | RenderWorld + ray/visibility integration | Adapter albo jawny modern-only snapshot do czasu dostarczenia |
| Audio replacement | Wspólny PCM player/cue resolver | Ten sam player, bez różnic renderera |

Projekt/build deklaruje wymagania. UI i game loader muszą jasno zgłosić, że
classic nie obsługuje zawartości modern-only; nie przełączać samoczynnie na classic,
nie wyświetlać „Applied” gdy classic pomija model. Test-in-game wybiera wspierany
renderer i wyświetla tę informację. Native classic adapter ma osobną bramkę
quality/performance, by unikać przepisywania całego renderer stack w ramach UI.

Źródło i resolved content tworzą immutable **AssetSnapshot** per session. UI,
preview worker i game/test dostają snapshot identity, nie bieżący globalny
wybór auta/course. Reload tylko między sessions, unieważnienie caches przez revision.
W szczególności GP intro/presentation mogą zmieniać selected course — resolved
asset identity ma pozostać tą rzeczywiście załadowaną (`TrackAssetIdentity`).

## 9. Architektura o minimalnej liczbie nowych warstw

```text
Electron: Game / Cars / Tracks / Interface / Audio / Saves / Projects
              │ bounded commands, progress, cancellation
       Project + existing LauncherService job ownership
              │
    compiled Catalog / Decode / Validate / Build
              │
    immutable resolved AssetSnapshot
         ┌────┴─────────┐
     editor preview   actual game → modern / classic adapters
```

To konkretne moduły o ownership, nie Model/Presenter/Controller i trzy pliki
na każde pole. `Project` owns draft/history/references; `Catalog` owns decoded
source index; `Build` owns transaction result. UI selection/viewport camera
local do aktualnego workspace. Istniejący service nie dostaje kolejnego globalnego
stanu gameplay. Format/writer tests nie potrzebują bootu/menu ani globalnej gry.

- Wydzielić reuse C archive/image/model parsers z tools/runtime, **bez** trzeciej
  kopii. New track codecs mały zestaw source files; writers i validators wspólne.
- Private CLI commands w aktualnym mod helper lub jednym worker executable.
  Nie mnożyć poleceń użytkownikowych. Nie robić monstrualnego all-purpose protocol;
  bounded command/result dla Open source, Read asset, Preview, Validate, Build.
- Reuse istniejącego WebGL viewport do interakcji dla cars/tracks: jedna scena,
  picking/gizmos/texture/material renderer, nie separate viewer każdego zasobu.
  Native game verification jest świadomym drugim trybem podglądu, nie fałszywym
  zapewnieniem identycznego raytracingu w uproszczonym WebGL.
- `rage-render-stage` wykorzystać do native fidelity captures. Jeśli izolowany
  import sceny wymaga inicjalizacji gameplay, najpierw dodać deterministyczny
  source scene service. Nie emulator ani GUI-driving do każdego kliknięcia.
- Plain HTML/CSS controls, reuse existing styles. Nie nowy design-system package.
- C i UI tests pilnują także **usunięcia** poprzedniej ścieżki po zastąpieniu:
  spec raw writer z domyślnego flow, duplicate metadata mapping, Python exports.
  scc/size/duplicated parsing before/after jako audit, nie cel każdej linijki.

## 10. Kolejność wdrożenia i bramki

Każda faza to małe lokalne commity. Nie zostawiamy drugiej działającej aplikacji
na końcu fazy. Feature flag do niedokończonego workspace, bez pustych guzików
sugerujących gotową funkcję.

| Faza | Konkretny wynik | Bramka przed kolejną |
| --- | --- | --- |
| 0. Konsolidacja decyzji | Ten plan/mockupy; lista zachować/usunąć; ocena lokalnych zmian | Użytkownik może przejść auto/tor/save/apply w makiecie; brak nowych product changes |
| 1. Source catalog i Project | 135-entry audit, regional names/variants, dokument/deltas/history, źródła | Fixture + no-op roundtrip; Origin/effective rozróżnione; zero anonymous assets w zwykłym UI |
| 2. Shell + pierwszy car loop | Jeden desktop, embedded viewport; price/AT/paint → Save & apply → actual race | Parametry nie nadpisane przez cars.toml; standalone rezultat identyczny; malformed rollback |
| 3. Complete car authoring | All original/authored/rivals, GLB/OBJ/PNG, assembly/LOD/materials/markings | Blender roundtrip + actual classic/modern compatibility tests; Class 6 i replay |
| 4. Track appearance + scene override | Pełna scene selection, native generic geometry/placements/textures/env | Mythical Coast i Over Pass City: Save/reopen/test forward+reverse; pop-in i cache identity |
| 5. Driveable road + race data | Road editing/generator, starts/AI, cameras/events/animacje, visibility build | Full lap + AI + replay + GP intro; no visual/physics mismatch; wszystkie real variants |
| 6. Interface + Audio | Named element/cue editing; PNG/PCM imports; new runtime loaders | Boot/title/fmv transitions/PAL+NTSC music/confirm/cancel/no mis-timed sounds |
| 7. Cleanup i delivery | Wycofanie starych raw-centric UI i Python replaced paths, one package | Clean double-click mac/Linux/Windows; import/build/apply rollback; docs oraz full matrix |
| 8. Expanded content | Data-driven additional cars/tracks + native classic geometry adapter; video import jeśli codec spike przejdzie | Dynamic IDs, records/save migration, true compatibility; nie tylko więcej pozycji listy |

Faza 4 nie jest opcjonalną „kiedyś” po cars: bez niej nie nazywamy tego gotowym
asset editorem. Faza 5 jest wymagana dla driveable route editing. Faza 8 usuwa
prawdziwe ograniczenia roster/track slots; można wydać wcześniej editor zmian
istniejących zasobów, jasno opisując zakres.

Szacunek orientacyjny **jednego doświadczonego developera**, po tym audycie:
source/project + car loop 2–4 tyg.; complete cars/import 2–4; track appearance
2–4; driveable tracks/AI/cameras 4–8; interface/audio/cleanup/platforms 3–6.
Łącznie 13–26 tygodni pracy, z nakładającą się weryfikacją. Nie SLA ani obietnica;
spike source catalog i track writer powinny zawęzić przedział. Dodatkowe track/car
slots, native classic adapter i MP4 import **poza** tym oszacowaniem. Największe
ryzyko to powiązanie drogi fizyki, visual terrain i hard-coded scenery, nie UI.

## 11. Migracja: co zachować, co usunąć

**Zachować:** istniejące IPC isolation, source bootstrap, settings keys,
save C library/checksum tests, material parser, mod selection/provider/file
policy/snapshot/cancel behavior, native mesh decoder/writer, PCM menu import,
WebGL viewport/camera sync, native render stage i hidden test support.

**Zmienić:** launcher produkt/nawigacja; car catalog z authored-only na source
catalog; raw spec edits na resolved parameter deltas; import na modyfikację
Project; modal viewport na workspace; published selection na immutable builds
+ atomic active pointer + source identity check; one generic mesh resolver.

**Usunąć po coverage:** zwykły Asset library z anonimową listą, export/replace
raw z primary flow, tworzenie nowego moda na każdy save pola, duplicate role/
material mapping, Python extract/browser/glTF/audio behavior po compiled parity,
stare UX directions z aktywnych docs, oddzielne user-facing helper launches.
CLI developerskie nadal mogą zostać jako private test entrypoints.

Migracja starych paczek: read-only import → nowy projekt ze zachowanymi
metadata/unknown bytes → preview semantic changes → świadomy Save/Apply.
Unconvertible raw piece zostaje jako Advanced legacy component z compatibility
warning, nie „lost”. Existing profile.json i zależności nie wyrzucamy w rename.

## 12. Testowanie, wynik i następny konkretny krok

Szczegółowa [macierz](acceptance.md) definiuje dane, czynności i asercje.
No-op roundtrip jest osobną asercją od visual fidelity. Testy samego przeklejenia
bajtów nie dowodzą efektu w gameplay. Test Play pomija zbędne menu, ale przechodzi
**normalny session init**, bez sfabrykowanych globali/VRAM.

UI E2E i game graphics tests uruchamiają się **bez widocznego okna i focus theft**;
manual Play oczywiście widoczny. Isolated profiles, źródła read-only, brak zmian
save'ów/gracza. Real GPU native capture obowiązkowy dla modern, nie tylko
compatibility framebuffer smoke. Każdy feature ma zaznaczone renderer/region.

Po zatwierdzeniu kierunku pierwsza implementacja: **compiled Catalog dump dla
jednego prawdziwego auta i całej Mythical Coast**, z rolami/reference graph,
parsowanie wszystkich 11 bloków + tests; jeden Project z AT/price delta i PNG,
który daje identyczny resolved result w editorze i game. Żadnych kolejnych
zakładek na obecnym raw-mod flow. Potem od razu pierwszy track placement/texture
loop, zanim rozwiniemy wszystkie formularze.
