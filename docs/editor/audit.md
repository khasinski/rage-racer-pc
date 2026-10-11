# Rage Racer Editor — audyt i notatki

Data: 2026-10-11. Baza: `main`, commit `5b3749725`, z lokalnymi zmianami
warsztatu i automatycznego wykrywania modów. Dokument opisuje istniejący kod;
propozycje wdrożenia są w [plan.md](plan.md). W tej rundzie nie przebudowujemy produktu.

## 1. Co faktycznie mamy

| Element | Działa / przeznaczenie | Problem / decyzja |
| --- | --- | --- |
| `launcher/` Electron | Gra, import obrazu, INI, save'y, paczki, OBJ, podgląd samochodów | Właściwa baza **jednego** produktu. Zmiana modelu pracy, nie drugi launcher. |
| `launcher/main/service.cjs` | Operacje, procesy, zapis profilu | Zapis każdego importu jako kolejnego wyłączonego moda; brak jednego dokumentu auta/projektu. |
| `launcher/renderer/app.js` | Widoki i formularze; aktualnie duże sklejane fragmenty HTML | Formularze według operacji technicznych. Podział na zakładki sam tego nie rozwiązuje. |
| `launcher/renderer/model-preview.js` | WebGL: całe auto, kamera, materiały, porównanie | Modal zamiast centralnego obszaru roboczego. Uproszczone światło; nie reprezentuje pełnego wyniku gry. |
| `rage-save-cli`, `launcher/native/save/` | Wspólna biblioteka save'ów, checksumy, logo, karty | Zachować w jednej aplikacji; nie odtwarzać layoutu w JS. |
| `rage-mod-cli` i `src/render/mod_*` | Walidacja, materiały, konflikty, zależności, kopiowanie | Przydatny backend, ale jego pojęcia dominują dziś UI. CLI może pozostać prywatnym workerem. |
| `rage-extract` | Wszystkie 135 wpisów, PNG/sidecary, PCM muzyki menu | Manifest daje index/offset/size/raw, bez nazwy znaczeniowej. Heurystyczna inspekcja offsetów obrazków nie jest katalogiem edytowalnych obiektów. |
| `rage-mesh-obj` | Walidowany natywny import/export pojedynczej siatki OBJ/MTL | Pełne słowo `rage_N` koduje materiały/metadata. Użytkownik nie powinien znać go ani naprawiać. |
| `rage-pack` | Repacking archiwum | Niepotrzebny w zwykłym flow; nie musi być osobną aplikacją użytkownika. |
| `rage-menu-texture` | PNG custom button ↔ plik `.inc` do kompilacji | To narzędzie developerskie, **nie** gotowy runtime editor grafik menu. |
| `rage-render-stage`, frame/GTE/GP0 replay | Native renderer w izolacji, diagnostyka | Zachować prywatnie w testach/walidacji. Nie wystawiać jako kolejne narzędzia dla gracza. |
| `tools/assetbrowser/` | Python: rozbudowany dekoder / read-only browser / referencyjny glTF | Brak zapisu struktur toru; glTF traci metadata. Przenieść brakujące zachowania do C z coverage, potem wycofać; żadnego Python w produkcie. |
| `docs/ux/rage-*` | Starsze klikalne makiety | Zarchiwizować kierunek; nie rozwijać trzech niezależnych aplikacji. |
| `prototypes/car-editor/` | W tej kopii tylko `node_modules`, brak źródeł aplikacji | Nie ma czego odpalić ani bezpiecznie merge'ować na podstawie tego katalogu. |
| Nieśledzone `web/`, `server/` | Eksperyment sieciowy, inne prace | Nie zmieniać i nie wciągać do edytora. |

Git worktree list i lokalne drzewa przejrzane tylko do odczytu. Nie znaleziono
samodzielnego źródła edytora w `prototypes/car-editor/`; nie oznacza to, że nie
istnieje w innej historii lub na innej maszynie. Nie opieramy architektury na
nieodnalezionym prototypie.

## 2. RAGE.BIN: dokładna mapa rodzin

Źródła: `include/game/asset.h`, `tools/assetbrowser/extract.py` (tabela nazw
odtworzona z PAL), funkcje instalujące w `src/main/PAL/main/asset/`.
Nazwy archiwalne są dowodem pochodzenia, nie etykietą domyślnego UI.

| Wpisy dziesiętnie | Rodzina | Co zawiera / gdzie pokazać |
| --- | --- | --- |
| 0–9 | Boot/menu | Logo, title, VH/VB SFX, RES.DAT, car/save backgrounds, SEQ/VAB menu music, SELECT/OPTION obrazy/model. **Interface / Audio**, nie 10 anonimowych plików. |
| 10–73 | 32 pary wariantów aut | `.1ST`: metadata/model/tekstury; `.2ND`: parametry i audio. **Cars → rodzina → Grade**. |
| 74–85 | 12 plansz Grand Prix | **Interface → Race/class screens** z serią i klasą. |
| 86 | VOICE.BIN | **Audio → Announcer**, po zweryfikowanych cue'ach. |
| 87–134 | 24 pary torów | 6 klas × 4 sloty × `.1ST`/`.2ND`; **Tracks → nazwa → klasa/kierunek**. Slot nie dowodzi rzeczywistej dostępności trasy. |

Muzyka wyścigu CD-Audio i FMV `RAGE.STR` nie należą do tych 135 wpisów.
CUE opisuje ścieżki; legalny Track 01 BIN nie zawiera automatycznie pozostałych
ścieżek muzyki. UI ma pokazać brakujące utwory, nie obiecać odtworzenia brakujących danych.

### Pomiar na lokalnym, przygotowanym archiwum

Odczyt **bez zapisów**, Node Buffer LE, na podstawie layoutu z C. Lokalny manifest:
135 wpisów; 1207 wyeksportowanych PNG. Liczba PNG oznacza uploady/palety/
podbloki, **nie 1207 niezależnych, nazwanych assetów produktu**.
Nie wykonano Python ani nowej ekstrakcji. Pomiar dotyczy jednej lokalnej kopii;
nie ustanawia zgodności wszystkich regionów.

| Rodzina slotu | Punkty drogi | Komórki terenu | Course models | Umieszczone `CourseObject` |
| --- | ---: | ---: | ---: | ---: |
| BIG / Mythical Coast | 333 | 198 | 64 | 5 |
| MID / Over Pass City | 513 | 243 | 64 | 5 |
| HI / Lakeside Gate | 482 | 231 | 69 | 10 |
| OVAL, klasy 2–6 w archiwum | 199 | 130 | 60 | 1 |
| OVAL, klasa 1 w archiwum | 333 | 198 | 64 | 5 |

**Uwaga o OVAL:** kod `custom_race.c` i `course_navigation.c` dopuszcza prawdziwy
Oval dopiero od klasy 3 (indeks 2). Pack OVAL1 przypomina BIG; same wartości
count nie wystarczają, żeby udowodnić identyczną geometrię. OVAL2 ma strukturę
ovalo-podobną, ale pozostaje niedostępny w aktualnym wyborze. Edytor ma używać
reguł wyboru gry i nie przedstawiać tych placeholderów jako normalnych wariantów.

W klasach 1–5 primary model bank: 38 modeli; w klasie 6: 23. Secondary bank:
1 model w przebadanych paczkach. To liczby **siatek**, nie liczby samochodów.
Custom ma 4 prawdziwe modele rywali w ostatniej klasie; deduplikacja musi znać
role i layout, nie dzielić arbitralnie wszystkich siatek na „auta”.

## 3. Co oznacza tor

`InstallTrackRuntimeAssetPack` waliduje i instaluje **11 bloków**, w kolejności:

| Blok | Znaczenie | Edycja / zależności |
| ---: | --- | --- |
| 0 | Render table, sekcje tekstur, parametry modeli | Powiązania palet/środowiska, nie tylko rendering. |
| 1 | Environment palette | Kolory/varianty wspólne dla powierzchni. |
| 2 | Environment script | Timeline 9 slotów: fog, sky top/middle/horizon/bottom, dwie pary ground. |
| 3 | Primary model bank | Auta rywali **i** scenery; klasyfikować według rzeczywistych użyć. |
| 4 | Track points + arc centers | Droga fizyki: x/z/y, kąt, pitch, cross-slope, half-widths, arc ref, segment length. |
| 5 | Course models | Modele obiektów ze swoim vertex pool. |
| 6 | Secondary model bank | Osobna tożsamość; nie scalać z bankiem 1. |
| 7 | Terrain grid + visibility + cell geometry | 32×32 grid; 10-bit cell ID i region; per-region masks. |
| 8 | Course object placements | Model, x/y/z, yaw, flags; nie obejmuje każdej dekoracji toru. |
| 9 | Events, AI, animacje, intro | Starty/AI 2 serii, strefy światła/reverb/audio, flyby/scenery, intro camera, path position/rotation. |
| 10 | Camera tables | Default i dwie serie, tryby chase/track/path/orbit etc. |

Reverse to wybór serii/danych i kierunku, nie drugi niezależny dump geometrii.
`CourseSlot`/`CourseSeries`, `TrackPositionForSeries` i wybór kamer to konkretne
punkty integracji. Extra finale może ponownie używać standardowych assets.

### Główna pułapka

Widoczny asfalt to mesh, ale fizyka używa punktów/łuków/szerokości i nachylenia.
Przesunięcie asfaltu nie przenosi automatycznie samochodu. Przesunięcie punktu
nie deformuje automatycznie widocznej drogi. Dekoracje wtopione w terrain cell
nie są `CourseObject`; część jest emitowana przez `course_scenery.c`, z logiką
zależną od klasy, trybu i animacji. Editor selection ma pokazać ten podział.

Visibility jest authored i już powodowała pop-in/odległą geometrię. Nowe
geometrie muszą mieć bounds i poprawne maski albo natywny nowy tryb widoczności.
Nie można zmienić punktu drogi i zostawić starych kamer, AI i visibility bez walidacji.

### Granice, które nadal naprawdę istnieją w runtime

32×32 grid, limit cell ID 1023/sentinel, model limits 256, signed halfword track
sections/length, stałe 4 sloty trasy i 13 modeli gracza. To ograniczenia danych i
kodu, nie brak VRAM współczesnego GPU. Nie kopiować ich do nowego natywnego
formatu dekoracji; nie udawać, że znikają po schowaniu numerów w UI.

## 4. Auta: różne źródła i rozbieżności

- 13 rodzin gracza, 32 warianty; grade ma wpływ na geometrię i parametry.
- Rywale są w track bankach, zależą od klasy/course; pierwsze trzy bywają
  odpowiednikami gracza, reszta ma osobne modele i przypisanie handlingu
  w `custom_race.c`. Slot rywala nie jest trwałą tożsamością rodziny auta.
- `ModernAssetsCarCatalog` iteruje **`s_authoredCars`**, nie pełny dekodowany
  katalog oryginalnej płyty. `ModernAssetsExportCar` również eksportuje embedded
  authored mesh. Podgląd „Original” jest więc obecnie mylący.
- `ModernAuthoredCar` dopuszcza semantic override poprzez rekordy authored;
  generic car/track mesh loader niezależny od tych rekordów jeszcze nie istnieje.
  Samo dopisanie `[meshes] track...` do TOML nie uruchomi nowych modeli toru.
- Wbudowana wymiana obejmuje konkretne role/submesh i mapuje materiały page/CLUT
  do runtime slot; identyczny numeric slot w innym banku nie jest tym samym materiałem.
- `CarCatalogApplySpecification` nakłada `cars.toml` po danych disc/raw. Edytor
  `saveCarSpec` zapisuje raw i model flag, bez warstwy katalogu: jego parametry
  mogą zostać później przykryte przez katalog. Unit test surowych bajtów nie
  dowodzi rzeczywistego efektu w wyścigu.
- Names/manufacturer/class/price/unlock i pełne torque/ratio/grip dostępne są
  w C `CarCatalog`, podczas gdy formularz UI daje tylko 17 pól (MT + 4 + 12 shifts).
- „Dostępność AT” zależy również od `cars.retail_manual_only`. Edytor powinien
  pokazać, gdy aktywne ustawienie blokuje rezultat, nie zapisywać kolejny raw patch.
- `CarCatalogPrepareProfile`: katalog przy aplikacji przed user fallback.
  Warstwa projektu musi nakładać się **po** wybranym katalogu i wyświetlać źródło.
- Assembly: pozycje kół, lustrzana strona, wariant animacyjny/LOD, signed offsets,
  szkło/paint/depth bias. Zwykły body OBJ nie reprezentuje całego auta.
- Przelicznik istniejącego UI shift points: raw × 160/1168, 2 miejsca dziesiętne;
  musi być opisany jako skala prędkości, nigdy RPM. Finalne etykiety kph/mph
  mają użyć regionalnych funkcji z gry; nie wymyślać drugiego przelicznika.

## 5. Co jest faktycznie edytowalne dzisiaj

| Rodzina | Inspekcja/preview | Zapis i wynik w grze |
| --- | --- | --- |
| Wybrane authored auta | Assembled WebGL + OBJ/PNG | Modern semantic mesh/material/PNG; raw parametry z ryzykiem przykrycia przez catalog. |
| Oryginalne wszystkie auta | Loader potrafi odczytać retail | Brak pełnego source-independent katalogu/GUI. |
| Tor: terrain/course/model banks | C importer i renderer; Python read-only export | Brak structured track editor i generic semantic replacement. Raw pack override to nie rozwiązanie UX. |
| Punkty/AI/kamery/środowisko | Layout + runtime validators w C | Brak compiled writer i dokumentu do edycji. |
| Menu | Wyekstrahowane PNG, osobny custom bitmap helper | Raw indexed patch ma limity palet/rozmiaru; brak znaczeniowego runtime UI image API. |
| Muzyka menu | SEQ/VAB → PCM w extraction | Brak znaczeniowego audio override/writer; nie wracać do live SEQ fallback. |
| SFX/voices | C audio loader / banki; Python dekoder | Brak pełnego cue katalogu i GUI importu WAV. |
| Race music | CUE/CD decoder | Brak zunifikowanego named audio override menu. |
| FMV | Runtime decoder | Brak transcode/import pipeline dla popularnego MP4. |
| Save'y | C editor i Electron formularze | Istniejące roundtrip/checksum/copy coverage; zachować. |
| Unknown/reserved | Surowe bajty | Zachować bez zmian, Advanced: prawdziwy status; nie wymyślać nazw. |

## 6. Dziennik decyzji

1. Jeden produkt: **Rage Racer Editor**, w `launcher/`; nie nowy renderer UI,
   serwer albo równoległy prototyp aplikacji.
2. Jeden edytowalny **Project** zawiera wiele aut/torów/audio; import nie tworzy
   osobnego wyłączonego moda na każdą operację. Package jest wyjściem projektu.
3. Nie ogłaszamy ukończenia po samym car editor: testowany tor jest obowiązkową bramką.
4. Readable katalog od decoderów C, z odróżnieniem Disc / Built-in / Catalog /
   Project / Installed pack / Final. Identyfikatory tylko w details.
5. Zachować istniejące C parsers/validators/save, file policy i cancellation;
   nowe compiled writers współdzielą kontrakty gry. UI nie parsuje binarnego layoutu.
6. W trakcie planowania nie kasować narzędzi, nie modyfikować płyty/profilu,
   nie merge'ować eksperymentów i nie wydawać builda.

Dalsze rozstrzygnięcia i kolejność pracy: [pełny plan](plan.md).

## 7. Weryfikacja planu i makiety

- Pomiary zapisane w [inventory.json](inventory.json), `region = PAL` odczytany
  z przygotowanego źródła. Count i block hashes, bez retail bytes ani user paths.
- Hash porównanie BIG1/OVAL1: identyczne bloki points (4), bank2 (6), placements
  (8); **nie** cała geometria/paczka. Potwierdza potrzebę reguł availability,
  nie automatycznego traktowania slotu jako nowego toru.
- Makieta: 7 sekcji, input/selection, cena→undo/redo→save→apply, import role wizard
  demo→undo, route drag, Oval restriction, reverse atmosphere; hidden Chromium.
- 1440×960 i 960×680: brak poziomego overflow i błędów JS. Obejrzane screenshots
  aut/toru/układu mniejszego okna; makieta jest schematyczna, nie renderer gry.
- To testy **makiety**, nie nowe potwierdzenie edytora ani testy gry. Acceptance
  matrix oznacza plan przyszłych sprawdzeń, nie zakończone regressions.

7. Po przejściu makiety: domyślne **Save & apply** zamiast dwóch obowiązkowych
   czynności Save i Apply. Zachować rozróżnienie stanów przy failure/recovery i
   dodatkowe Save project only, ale zwykły user nie zarządza ręcznie paczką.
