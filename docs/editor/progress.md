# Stan implementacji

Prace na `feature/editor`. Plan i makieta opisują docelowy zakres; nie są dowodem
ukończenia edytora. Istniejący launcher pozostaje jedyną aplikacją.

## 2026-10-11

- Zachowany wcześniejszy fundament warsztatu aut: sekcje wyglądu, parametrów i
  plików, podstawowe/zaawansowane pola, zapis włączonych modów dla gry.
  Obecny zapis katalogu modów jest rozwiązaniem przejściowym, nie docelowym
  wskaźnikiem na niezmienny build.
- `TrackPackOpen` w C odczytuje bez globali jedenaście bloków oryginalnego pakietu
  toru. Odczyt współdzielony przez runtime i CLI; walidatory zawartości bloków
  pozostają w grze. Nazwany blok nie oznacza jeszcze edytowalnej zawartości.
- Backend `tracks()` opisuje 24 oryginalne pakiety klas/tras, ich kierunki i role
  bloków. OVAL klas 1–2 jest oznaczony jako niedostępny do wyścigu. Nie uruchamia
  gry ani nie eksportuje wbudowanych zamienników aut w celu odczytu toru.
- Sprawdzony odczyt prawdziwego PAL Mythical Coast (asset 088), tylko do odczytu.
- Testy C: niealigned wejście, obcięty nagłówek, niewłaściwe offsety, brak
  częściowego wyniku; istniejący `race_asset_loading` przechodzi.
- Test backendu: kompletność katalogu, kierunki, niedostępne warianty, odrzucenie
  uszkodzonego ostatniego pakietu bez częściowego sukcesu.
- Suite launchera: 88 pass, 3 skip, 0 fail (91 testów). Testy nie pokazują okien.

## Następna praca

Dekodowanie struktur drogi, modeli i instancji do katalogu/viewportu, źródłowa
identyfikacja regionu i rewizji oraz wspólny resolver parametrów auta. Następnie
jeden projekt z historią i atomowym Save & apply, zamiast nowego moda dla każdej
operacji. UI torów i pełny import GLB nadal nie są zaimplementowane.

## Odczyt drogi i pierwszy widok torów

- C `RoadOpen`/`RoadRead` dekodują wszystkie pola 24-bajtowego punktu i zachowują
  referencje do centrów łuków. Runtime używa tego samego walidatora co edytor;
  usunięto wcześniejszą, osobną implementację walidacji punktów w grze.
- Sekcja Tracks w istniejącej aplikacji pokazuje dane wybranej trasy/klasy,
  mapę jej rzeczywistych punktów i parametry wskazanego punktu. Obsługa myszy
  i Enter/Space. Wciąż tylko odczyt; linie łączą punkty, nie zastępują geometrii
  łuków ani podglądu nawierzchni.
- Odczyt wszystkich 24 oryginalnych pakietów PAL przeszedł. Mythical Coast:
  333 punkty, 22 centra łuków, długość 154154 w jednostkach danych gry.
- Headless Chromium wyrenderował rzeczywiste dane Mythical Coast z 333 punktami;
  screenshot `build/editor-evidence/original-road.png` sprawdzony wizualnie.
- Testy `track_pack`, `install_track_points`, `race_asset_loading` przechodzą;
  build gry i CLI przechodzi. Suite launchera: 89 pass, 3 skip (92 testy).
- Nie ma jeszcze zapisu drogi, sceny 3D terenu, importu GLB ani pełnego Project.

## Wspólny dokument projektu

- `Project` w istniejącym serwisie JS przechowuje delty ze wszystkich ekranów,
  jedną historię undo/redo, reset do źródła i monotoniczną rewizję. Wysyłanie
  nieaktualnej rewizji lub błędnego batcha nie zmienia żadnego pola.
- Zapis `.rrproject` przez plik tymczasowy i rename; nieudany zapis pozostawia
  poprzedni dokument i stan dirty. Odczyt ogranicza rozmiar i odrzuca duplikaty.
- Serwis oblicza SHA-256 oryginalnego RAGE.BIN bez uruchamiania gry i przypisuje
  dokument do dokładnych bajtów źródła oraz regionu, także dla różnych NTSC-J.
  Kolejna płyta nie dziedziczy projektu. Na etapie Build trzeba ponownie sprawdzić
  źródło i zgodność ekstrakcji; cache otwartego projektu nie stanowi tej walidacji.
- Sekcja Projects udostępnia stan dokumentu, undo/redo i Save project only.
  Nie jest to jeszcze Save & apply; istniejące edycje aut nie są jeszcze
  przeniesione z osobnych modów do dokumentu. Nie ma jeszcze odzyskiwania draftu.
- Suite launchera przed dodaniem testu widoku: 93 pass, 3 skip; pięć testów
  projektu/serwisu/widoku po ostatnich zmianach przechodzi. Żadne widoczne okno
  gry ani edytora nie zostało otwarte przez testy.
- TypeSafe zainstalowany żądaną metodą `npx skills add` dla Codexa. Zgodnie ze
  skillem odczyt i walidacja danych pozostają deterministyczne w kodzie, bez
  zależności edytora od API AI. Przeczytano lokalny skill i live docs index.

## Wspólny profil samochodów i kompilacja nadpisań

- Parser/validator profilu wydzielony do `rage-car-catalog` bez globalnego stanu
  gry. Runtime, native CLI oraz testy linkują tę samą implementację C.
- Jedna tabela 28 pól steruje parsowaniem, łączeniem i serializacją; usunięto
  osobne schematy parsera dla skalarów/torque scale/shift points. Nie ma parsera
  TOML samochodów napisanego drugi raz w JS.
- `--car-profile` opisuje tożsamość 32 wariantów i obecne pola metadanych.
  Nie emituje jeszcze pełnych wartości specyfikacji ani domyślnych danych płyty.
- `--compose-car-profile` łączy bazę z deltasem przez model/grade, waliduje całość
  i tworzy nowy plik bez nadpisywania istniejącego wyniku. Nieaktywne pola
  zachowują bazę. Błąd nie publikuje częściowego profilu.
- Test C dowodzi roundtrip wszystkich pól w katalogu, zachowania specyfikacji
  przy zmianie ceny/AT, atomowego odrzucenia złej ceny i bounds wariantów.
  Naprawiono też błąd: grade poza rodziną nie wskazuje już wariantu następnego auta.
- Test CLI sprawdza oba regionalne katalogi, zmianę ceny/MT-only, zachowanie
  pozostałych aut, odmowę nadpisania wyniku i odrzucenie przepełnionej liczby.
- Build:native bundluje teraz oba cars*.toml obok binarki; packager wymaga ich
  obecności. Wcześniejsza ścieżka stage nie kopiowała tych plików.
- Build gry, smoke i narzędzi oraz testy car_catalog/race_asset_loading przeszły.
  Aktywacja profilu projektu i migracja formularza samochodu nadal pozostają
  niepodłączone: samo złożenie pliku nie jest jeszcze dowodem efektu w grze.

## Parametry auta → projekt → build → runtime

- Formularz parametrów nie instaluje już osobnego surowego moda. Rozstrzyga
  wartości z płyty, bundlowanego profilu i draftu; dodano ceny oraz unlock level.
  Origin i baseValue są zwracane przez serwis. Powrót do bazy usuwa deltę.
- Rewizja formularza chroni przed nadpisaniem równoległego editu. Shift points
  są jedną tablicą w historii projektu; zachowujemy wszystkie niewybrane biegi.
- Save & apply w formularzu/Projects kompiluje profil C do niezmiennego builda,
  ponownie go odczytuje i publikuje active.json po zapisaniu dokumentu. Save-only
  nie aktywuje draftu. Błędne/niewspierane edity blokują całą aktywację.
- Konfiguracja następnego uruchomienia z edytora wskazuje zatwierdzony profil
  przez `cars.profile`; main i smoke respektują tę samą ścieżkę wyboru.
- Test runtime C sprawdza zastosowanie złożonego profilu do ceny, dostępności
  skrzyni i specyfikacji, zachowując pozostałe parametry. Test serwisu kompiluje
  faktyczne pliki przez native CLI i sprawdza konfigurację uruchomienia.
- Sprawdzono: nieudany commit wskaźnika pozostawia wcześniejszy aktywny build,
  brak osieroconego wyniku po błędzie, zły profil, uszkodzenie zatwierdzonych
  bajtów, zmienione źródło i brak modyfikacji oryginalnych pakietów.
- Suite launchera: 97 pass, 3 skip (100 testów), testy profilu i runtime C
  przechodzą; gra/smoke/native tools budują się. Bez widocznych okien.

### Ważne granice aktualnego etapu

Nie wykonano jeszcze wizualnego pełnego e2e zakupu/wyścigu z tym formularzem. Resolver edytora
nie uwzględnia jeszcze parametrów istniejących raw modów. Pierwszy builder
obsługuje profil auta; edity torów/media blokuje jawnie, zamiast je pomijać.
Sceny toru, pełny GLB, media, draft recovery, rozszerzanie rosteru/tras i komplet
platformowych acceptance gates nadal wymagają implementacji z pełnego planu.

## Wspólny aktywny build wykrywany przez grę

- Builder zapisuje niezmienne wyniki w katalogu gry `mods/builds/<uuid>`
  i atomowo zmienia `mods/active.json`. Zapis dokumentu bez zastosowania nadal
  pozostawia poprzedni wynik aktywny.
- Gra i edytor używają tego samego walidatora C. Wskaźnik wiąże build z regionem
  i SHA-256 oryginalnego RAGE.BIN; sprawdzane są również bajty profilu i manifestu
  oraz poprawność profilu przez parser runtime. Błąd odrzuca cały build, bez
  domieszki starych plików z korzenia mods.
- Import raportuje odcisk faktycznie odczytanego archiwum; późniejsza zmiana
  źródła jest wykrywana. Zwykły start bez aktywnego projektu nie oblicza hasha.
- Test C obejmuje automatyczną detekcję, reload, inną płytę, uszkodzony profil,
  brak mieszania z legacy root oraz jawne wyłączenie modów. Testy CLI porównują
  SHA z Node na granicach bloków i sprawdzają powtórzone klucze JSON, traversal,
  zły region/odcisk, uszkodzone pliki i niepoprawny profil z poprawnym hashem.
- Sprawdzono 12 testów C oraz suite launchera: 99 pass, 3 skip (102 testy).
  Trzy pominięte testy wymagają regionalnych obrazów płyty. Bez okien gry.
- Na tym etapie build obejmował tylko profil auta; ograniczenie kompozycji
  istniejących paczek zostało usunięte w etapie poniżej.

## Profil auta i paczki zasobów w jednym buildzie

- Save & apply korzysta ze sprawdzonej kompozycji wybranych paczek: walidacja
  zależności/konfliktów, prywatne kopie plików, izolowane ścieżki providerów
  i wspólny manifest. Profil parametrów jest częścią tego samego wyniku.
- Wskaźnik formatu 2 wiąże `files.json` z odciskami payloadu. Lista jest
  posortowana, ograniczona i sprawdzana przez wspólną politykę plików runtime.
  Uszkodzony/usunięty model, tekstura lub raw pack odrzuca cały wynik razem
  z profilem. Format 1 poprzedniego etapu pozostaje czytelny.
- Hashe payloadu powstają w jednym wywołaniu kompilowanego CLI, zamiast osobnego
  procesu na każdy plik. Limity paczek i plików obowiązują również w tej fazie.
- Gotowy wynik przed commit aktywnego wskaźnika przechodzi ten sam walidator C
  co gra. Wywołanie ze złym odciskiem blokuje aktywację i usuwa nowy wynik;
  wcześniej zastosowany build pozostaje dostępny.
- Starszy przycisk zapisania modów deleguje do wspólnego buildera. Usunięto
  osobną operację zastępującą cały katalog gry. Wcześniejsze buildy pozostają.
- Launch z edytora dla zastosowanego projektu nie buduje następnej tymczasowej
  kompozycji. Wskazuje korzeń ze wskaźnikiem, a runtime wybiera profile i modele
  razem po walidacji zamontowanej płyty. Zakończenie procesu nie usuwa buildu.
- Testy serwisu używają rzeczywistego importera OBJ/RMESH i parsera manifestu:
  wspólny profil/model, niezależność od późniejszych zmian paczki, failed build,
  damaged payload, wyłączenie paczki, zachowana historia buildów oraz cykl
  procesu gry bez otwierania okna. Test C sprawdza odrzucenie payloadu podczas
  inicjalizacji asset session i zachowanie odrzucenia do jawnego reloadu.
- Nadal brakuje: pełnej wizualnej próby w grze, przeniesienia wyboru paczek
  i edycji geometrii do historii samego dokumentu, resolvera raw parametrów,
  pełnego GLB/scen tras, mediów i pozostałych gates pełnego planu.
- Walidacja etapu: suite launchera 100 pass / 3 skip (103 testy), 12 testów
  C dla hash/provider/session oraz modern_assets_retry przechodzą. Przebudowano
  grę, smoke, frame replay i narzędzia; testy nie otwierały okna gry.
