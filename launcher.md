# Rage Mod Manager — handoff

Stan na 2026-09-06 (aktualizacja po domknięciu zależności modów). Aktywnym celem jest ukończenie desktopowej aplikacji
Electron w `launcher/` dla Windows, macOS i Linux. Interfejs produktu jest po
angielsku. Ten dokument opisuje pozostałą pracę; szczegółowy dziennik dowodów
jest w `launcher/IMPLEMENTATION.md`, a założenia produktu w
`docs/rage_mod_manager.md`.

## Zasady, których nie wolno naruszyć

- Przestrzegaj głównego `AGENTS.md`: wydanie ma startować po dwukliku, użytkownik
  wskazuje wyłącznie legalny obraz Rage Racer CUE albo Track 01 BIN, wszystkie
  zasoby nowego renderera powstają automatycznie, a gra uruchamia renderer modern.
- Nie dodawaj Pythona ani helperów wymagających Pythona. GUI jest w Electronie,
  a dekodowanie, import assetów, zapis save'ów i walidacja pozostają w C lub
  istniejącym kompilowanym toolchainie.
- Nie publikuj, nie pushuj i nie twórz release'u bez wyraźnej autoryzacji.
- Nie dołączaj publicznie obecnego binarnego buildu gry przed audytem praw.
  Aktualna gałąź zawiera retail-derived zamienniki samochodów.
- Nie ruszaj niepowiązanych `.claude/` ani `imgui.ini`. Worktree jest celowo
  brudny i nie ma jeszcze commitów tych zmian.

## Co obecnie działa

- Electron 44.2.0 z sandboxem, `contextIsolation`, wąskim preload/IPC i bez
  dostępu Node z renderera.
- Empty state i automatyczny import CUE/BIN/CHD, wykrywanie regionu, ekstrakcja
  wszystkich 135 wpisów oraz uruchomienie gry w rendererze modern.
- Czytelne ustawienia, z nazwami aut i intro wynikającymi z wykrytej wersji gry.
- Edytor save'ów: osobna kopia, checksumy, raw/DexDrive, rekordy, garaż,
  transmisja, ownership, kontroler, audio i edytor logo 64×64. Czasy mają segmenty
  oraz wklejanie pełnego `01:40.765`.
- Instalowanie/eksportowanie folderów modów, region compatibility, konflikty z
  wyborem providera, raw assets, materiały, tekstury i modele RRMESH.
- Warsztat aut: gracz sloty 0–31, osobno rywale według klasy/trasy, kompletny
  zestaw body/front wheels/rear wheels + wspólne PNG, import/eksport OBJ/MTL,
  walidacja materiałów i tekstur, podgląd teksturowanego całego auta.
- Parametry aut gracza: MT-only, limity silnika, liczba biegów, acceleration
  scale i sześć par progów AT. Zapis powstaje jako wyłączony mod.
- Lista modów związanych z wybranym autem, edycja name/author/version/description
  oraz podgląd Original/Modified ze wspólną kamerą i skalą.
- Pakiety macOS arm64, Linux arm64 i Linux x64 były budowane i uruchamiane.
  Krótki kontrolowany wyścig w rendererze modern przeszedł na Linux arm64/x64.
- Zależności modów (`packageId`, `requires`) są egzekwowane i widoczne: lista
  My mods pokazuje przy wymaganiu, który zainstalowany mod je spełnia
  („satisfied by X”, „enable X first”, „not installed”, „multiple enabled
  copies”) oraz ostrzega o dwóch paczkach z tym samym `packageId` jeszcze
  przed aktywacją. Zweryfikowane w spakowanym macOS harnessem
  `build/launcher-evidence/verify-mod-dependencies.cjs` (oba mody przygotowane
  przed startem aplikacji, czytelny błąd, kolejność base → addon, odmowa
  wyłączenia bazy, czysty exit 0).
- Pakiety Linux arm64/x64 zostały przebudowane z obsługą zależności i przeszły
  headless probe; aktualny pakiet x64 jest w `launcher/out/`.
- Aktualnie `cd launcher && npm test` przechodzi: 44/44 na macOS i w obu
  kontenerach Linux.

## Zależności modów: stan po domknięciu

Zrobione 2026-09-06:

1. Harness `build/launcher-evidence/verify-mod-dependencies.cjs` przepisany:
   importuje addon i bazę do profilu przed startem spakowanej aplikacji, więc
   proces czyta własny stan. Sprawdza widoczność wymagania, błąd przy włączeniu
   dodatku bez bazy, kolejność base → addon, odmowę wyłączenia bazy, zgodność
   `launcher.json` z UI i exit code 0. Zrzuty:
   `electron-mod-dependencies-{list,error,enabled}.png`.
2. Lista modów pokazuje status wymagania i duplikaty `packageId`
   (`launcher/renderer/mod-dependencies.mjs`,
   `launcher/tests/mod-dependency-display.test.cjs`).
3. Decyzja v1: wystarczy exact version albo any version. Zakresy semver i
   graficzny edytor grafu zależności pozostają poza zakresem.
4. Linux: pliki kopiuje się do kontenerów (`docker cp`, brak mountu). Wszystkie
   kroki uruchamiaj jako `node` (`docker exec -u node`): jako root Chromium
   odmawia startu bez `--no-sandbox`, a pakiet zbudowany przez roota jest dla
   `node` nieczytelny (EACCES). Po pomyłce trzeba usunąć
   `/tmp/electron-packager` i oddać `/work/launcher/out` użytkownikowi `node`.

Otwarte: wybór, która z dwóch kopii tego samego `packageId` ma spełniać wymóg,
nadal odbywa się przez włączenie tylko jednej z nich; nie ma osobnego selektora.

## Blokery wydania v1

1. **Windows.** Nie było realnego buildu ani uruchomienia na Windows. Trzeba
   wykonać `.github/workflows/launcher-check.yml` albo użyć Windows 2022/11 z
   VS2022 ClangCL, a następnie sprawdzić build native, 43 testy, package, probe,
   pierwsze wskazanie CUE/BIN, pełną ekstrakcję, ścieżki z Unicode, save copy,
   import moda i Play. Manifest UTF-8 jest w `packaging/windows/utf8.manifest`,
   ale jego osadzenie i działanie nie zostały potwierdzone.
2. **Czyste uruchomienie na wszystkich platformach.** macOS i Linux mają dowody;
   Windows nie. Linux x64 był emulowany na hoście ARM i używał Xvfb/llvmpipe.
   Warto wykonać próbę na fizycznym x64 z GPU i dźwiękiem.
3. **Artefakty dystrybucyjne.** `@electron/packager` tworzy katalog/aplikację,
   ale nie ma jeszcze instalatora Windows, podpisu/notaryzacji macOS ani ustalonego
   formatu wydania Linux (np. AppImage/tar). Trzeba ustalić minimalne systemy,
   przygotować ikony/metadane, podpisać lub jasno obsłużyć niesygnowane buildy i
   sprawdzić instalację na czystych maszynach bez Node/CMake/toolchainu.
4. **Prawa do publikacji.** Przed wrzuceniem na GitHub trzeba oddzielić kod
   narzędzia od retail-derived assetów i audytować licencje zależności. Release
   nie może zawierać materiałów z obrazu gry. Użytkownik powinien generować je
   lokalnie ze swojego CUE/BIN.
5. **Workflow CI.** Jest tylko `workflow_dispatch`, nie został wypchnięty ani
   uruchomiony zdalnie. Matrix powinien przejść na Windows/macOS/Linux i zachować
   brak publikacji artefaktów, dopóki audyt praw nie będzie gotowy.

## Ważne brakujące elementy produktu

- Instalacja moda działa z folderu. Nie ma jeszcze drag-and-drop ani pojedynczego
  przenośnego archiwum paczki z ekranem podsumowania przed instalacją.
- Brak pełnego systemu profili z osobnymi zestawami aktywnych modów. Jest jeden
  bieżący stan i zapisane decyzje konfliktów.
- Car set można zaimportować wyłącznie z powrotem do tego samego slotu i regionu.
  Cross-slot import wymaga remapowania materiałów/tekstur.
- Model/tekstury i parametry jazdy powstają obecnie jako osobne mody. Brakuje
  projektu auta, w którym autor składa geometrię, tekstury, materiały, parametry
  oraz metadane w jedną paczkę i może ją dalej edytować.
- Brak staging workspace z undo/redo, Restore original, listą wszystkich zmian,
  walidacyjnym podsumowaniem przed zastosowaniem i Build package pokazującym
  dokładną zawartość.
- Podgląd ma przełącznik Original/Modified, ale nie ma Side by side. Raw/legacy
  texture overrides, warianty palet inne niż zero, kolejność przezroczystości i
  dokładne oświetlenie gry nie są w pełni odwzorowane.
- Dynamiczne oznaczenia z save'a — logo na masce i napis na górze szyby — nie są
  wiarygodnie odtwarzane w podglądzie. Do akceptacji warsztatu trzeba je sprawdzić
  w działającym rendererze modern, nie tylko na statycznym modelu.
- Nie ma przycisku Test in game przy wybranym samochodzie/assetcie ani
  automatycznego scenariusza testowego z powrotem do managera.
- Asset library udostępnia wszystkie 135 wpisów jako raw, ale nie ma jeszcze
  przyjaznych edytorów Tracks/Textures ani dowolnego dekodowanego assetu RAGE.BIN.
- Advanced car parameters nie obejmują jeszcze m.in. surowych gear ratios,
  torque/load curves ani parametrów rywali. Nie dodawaj nazw bez potwierdzenia
  znaczenia pól w kodzie gry.
- Edytor save'ów jest szeroki funkcjonalnie, lecz trzeba jeszcze przejrzeć
  pozostałe nieopisane kontrolki i ranking/time-attack semantics. Nie wymyślaj
  sentinela „No record”: kod gry naprawia nieprawidłowe czasy do defaults.
- Konflikty legacy textures mają testy kompozycji, ale dodatkowe targety warto
  zweryfikować w działającej grze.

## Znane ograniczenia i kwestie techniczne

- macOS 26.3 na tym hoście sporadycznie pozostawiał proces Electron po zamknięciu.
  Minimalny Electron z BrowserWindow reprodukował ten sam stos `usleep`; wariant
  bez okna kończył się poprawnie. Podobne upstream issue:
  https://github.com/electron/electron/issues/52582. Późniejsze lokalne próby
  przechodziły, więc nie uznawaj problemu za definitywnie naprawiony. Probe teraz
  wymaga exit code 0; sprawdź również na nowszym macOS.
- Test podglądu jest w `launcher/scripts/probe-preview.cjs`. Polecenie
  `node scripts/probe-package.cjs --preview-regression` sprawdza 20 przełączeń
  Original/Modified oraz close/decode race. Na headless Linux dodaj
  `--software-rendering`; flagi SwiftShader są wyłącznie testowe.
- Aktualne pakiety i `resources/bin` są ignorowane przez git. Nie kopiuj binariów
  między platformami. Każda platforma musi wykonać własne `npm run build:native`.
- `launcher/IMPLEMENTATION.md` jest chronologicznym dziennikiem. Starsze akapity
  zawierają nieaktualne liczby testów i stany typu „unfinished”; późniejsze wpisy
  oraz aktualny kod są nadrzędne.
- `build/launcher-evidence/` jest ignorowane i zawiera lokalne profile, logi,
  screenshoty oraz owned-disc evidence. Nie publikuj tego katalogu.

## Zalecana kolejność dalszej pracy

1. (zrobione) Packaged dependency UI harness i dowód w
   `launcher/IMPLEMENTATION.md`.
2. (zrobione) `npm test`, `npm run package` i
   `node scripts/probe-package.cjs --preview-regression` na macOS.
3. (zrobione) Linux arm64/x64: 44 testy, package i headless probe z
   `--software-rendering`; pakiet x64 skopiowany do `launcher/out/`.
4. Zdobyć rzeczywiste środowisko Windows i przejść pełny matrix oraz owned-disc
   first-run/Play. To najważniejsza brakująca weryfikacja cross-platform.
5. Domknąć format paczki/projektu moda i UX warsztatu, szczególnie jeden projekt
   auta, dynamiczne oznaczenia i Test in game.
6. Wykonać audyt praw/licencji, usunąć retail-derived elementy z publicznego
   release'u i dopiero wtedy przygotować podpisane artefakty oraz publikację.

## Polecenia startowe

```sh
cd /Users/hasik/Projects/rage-port/launcher
npm ci
npm run build:native
npm test
npm run package
node scripts/probe-package.cjs --preview-regression
```

Na Linux CI ostatnie polecenie uruchamiaj pod Xvfb:

```sh
xvfb-run -a node scripts/probe-package.cjs --preview-regression --software-rendering
```

Nie uruchamiaj ponownie kosztownego native build w istniejących kontenerach bez
potrzeby: `rage-launcher-linux-sandbox` (ARM64) i `rage-launcher-linux-x64` mają
działające platformowe buildy. Po skopiowaniu zmian JS wystarczy zwykle test i
package; po zmianach C/CMake trzeba wykonać pełny build native.
