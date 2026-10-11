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
- Nadal brakuje: pełnej wizualnej próby w grze, przeniesienia edycji geometrii
  do historii samego dokumentu (wybór paczek zrealizowany poniżej), resolvera raw parametrów,
  pełnego GLB/scen tras, mediów i pozostałych gates pełnego planu.
- Walidacja etapu: suite launchera 100 pass / 3 skip (103 testy), 12 testów
  C dla hash/provider/session oraz modern_assets_retry przechodzą. Przebudowano
  grę, smoke, frame replay i narzędzia; testy nie otwierały okna gry.

## Wybór paczek i priorytety w historii projektu

- Dokument formatu 2 przechowuje wybór paczek i rozstrzygnięcia konfliktów
  razem z deltami parametrów. Jedno undo/redo obejmuje operacje przeplatane
  między ekranami. Historia jest ograniczona do 200 wpisów i 32 MB, dokument
  do 8 MB; edycja przekraczająca limit nie zmienia istniejącego draftu.
- Format 1 przy otwarciu przejmuje dotychczasowy wybór z profilu launchera.
  Po zapisaniu format 2 jest samodzielnym źródłem wyboru. Przy ponownym otwarciu
  dokument wygrywa nad starszym/nowszym cache enablement/priorities w launcher.json.
- Lista modów jest projekcją wyboru w dokumencie. Toggle i wybór priorytetu
  walidują zależności przez istniejący kompilowany resolver. Błąd zapisu cache
  nie dodaje wpisu historii ani nie zmienia rewizji/draftu. Stare formularze
  otrzymują konflikt rewizji po zmianie wyboru w innym ekranie.
- Builder czyta wybór i priorytety bezpośrednio z dokumentu, a nie z flag
  `enabled` cache. Brak wskazanej paczki/wrong region blokuje build jawnie.
  Projekt pokazuje brakujące referencje i pozwala wyczyścić wybór z undo.
- Usunięcie paczki używanej przez bieżący, zapisany lub historyczny stan tego
  projektu jest blokowane, aby undo nie prowadziło do skasowanych plików.
- Launch z zaimportowanego źródła nie komponuje niezastosowanego draftu nawet
  wtedy, gdy nie ma jeszcze aktywnego buildu. Save-only pozostawia grę w stanie
  ostatniego zastosowanego projektu albo oryginału. Starszy profil bez archive
  zachowuje dotychczasową kompozycję do czasu migracji.
- Projects pokazuje wybrane paczki, przyciski usunięcia referencji/wyczyszczenia
  wyboru oraz rozwijane priorytety. Statusy głównego ekranu i modów rozróżniają
  wybór od zastosowania. Snapshot serwisu przekazuje aktualną rewizję do UI.
- Poprawiono kodowanie JSON native CLI: UTF-8 nazw/ścieżek jest zachowywany,
  zamiast traktowania każdego bajtu jako osobnego punktu Unicode. Test używa
  rzeczywistego katalogu `Łódź-東京` i sprawdza zwrócone ścieżki.
- Sprawdzono rzeczywisty renderer w headless Chromium z LauncherService i
  kompilowanymi narzędziami za testowym mostem HTTP: toggle dwóch paczek,
  priorytet, mouse/keyboard undo/redo, clear, save-only, apply i wynik manifestu.
  Brak wyjątków JS i overflow przy 1440 px / 1100 px. Screeny w
  `build/reports/editor/project-selection-{desktop,compact}.png`.
  To nie jest jeszcze test transportu Electron IPC ani wizualny wyścig w grze.
- Nadal do realizacji: immutable referencje importowanej/edytowanej geometrii
  i tekstur w samym dokumencie (paczek nadal nie wersjonujemy), draft recovery,
  pełna scena toru i GLB, media, rozszerzanie rosteru/tras oraz cały zestaw
  platformowych acceptance gates z planu.
- Suite launchera: 107 pass / 3 skip (110 testów). Dodatkowe nowe przypadki
  obejmują limity historii/dokumentu, malformed format 2, brakujący wybór,
  rollback zapisu, odtworzenie cache i start po save-only. Native tools
  przebudowano i wystage'owano; brak widocznych okien podczas testowania.

### Magazyn niezmiennych modeli projektu — fundament

- `ProjectFiles` zapisuje modele pod SHA-256 zawartości obok dokumentu.
  Publikacja przez hard link nie nadpisuje istniejącego obiektu; ponowny
  import tych samych bajtów nie duplikuje pliku. Każdy odczyt sprawdza hash,
  rozmiar i odrzuca symlinki oraz nieprawidłowe referencje.
- Dwa nowe testy sprawdzają przejście dwóch referencji przez wspólną historię,
  zapis i otwarcie dokumentu, reset, deduplikację, uszkodzone istniejące
  obiekty i symlinki. Razem z testami dokumentu: 10 pass.
- To jeszcze fundament, nie gotowa ścieżka użytkownika: import, efektywny
  podgląd i builder wymagają podłączenia magazynu. Walidacja RMESH pozostaje
  w istniejącym kompilowanym kodeku przed umieszczeniem pliku w magazynie.

### Podłączenie pojedynczego modelu do projektu

- Import OBJ/RMESH po natywnej walidacji slotu i materiałów zapisuje referencję
  modelu w historii projektu. Nie tworzy wpisu modów dla źródła z archiwum.
  Starsze profile bez archiwum zachowują dotychczasowy import paczki.
- Domyślny podgląd części i złożonego auta odczytuje zmianę modelu projektu;
  jawny podgląd paczki zachowuje swoje znaczenie.
- Apply zamraża zweryfikowane bajty w nowym buildzie i scala tabelę modeli
  z teksturami/materialami wybranych paczek. Model projektu ma pierwszeństwo
  dla swojego slotu. Pliki objęte są dotychczasowym manifestem hashy buildu.
- Test integracyjny przechodzi import, undo/redo i apply przez natywne kodeki;
  uszkodzenie obiektu blokuje kolejny apply bez zmiany aktywnego buildu.
  Pełny suite: 110 pass / 3 skip (113 testów), native save_format pass.
- Pozostają m.in. import całego zestawu do wspólnej historii, wersjonowanie
  tekstur, efektywny eksport oraz wspólne rozwiązywanie wyglądu paczek i zmian
  projektu w podglądzie. Nie zweryfikowano jeszcze wizualnie nowego modelu w grze.

### Zestaw auta i tekstury w jednej historii

- Import zestawu do projektu zapisuje trzy modele i zweryfikowane natywnie
  tekstury PNG jako jedną operację historii, bez dodawania paczki do biblioteki.
  Magazyn identyfikuje obiekty hashem i typem (`rmesh` / `png`).
- Builder nakłada tekstury projektu na wybrane paczki, kopiuje je do nowego
  buildu i obejmuje kontrolą integralności razem z modelami. Podgląd materiałów
  pobiera tekstury projektu przez te same referencje, mapując oryginalne
  page/CLUT na semantyczny slot materiału.
- Rozszerzony test zestawu sprawdza cztery zmiany w jednym kroku historii,
  undo/redo, zgodność pikseli podglądu, manifest zastosowanego buildu i brak
  zmiany dokumentu/rewizji przy odrzuconym PNG. Suite: 110 pass / 3 skip;
  native save_format pass. Wszystkie testy bez widocznej gry.
- Nadal niezamknięte: efektywny eksport, rozwiązywanie wyglądu wybranych paczek
  w domyślnym podglądzie, edycja właściwości materiałów w historii, GLB/glTF,
  edytor sceny toru i media oraz pełne platformowe/wizualne acceptance gates.

### Eksport edytowanego wyglądu auta

- Eksport części i pełnego zestawu zapisuje model oraz tekstury po nałożeniu
  wybranych paczek i zmian projektu. Geometria projektu wygrywa w swoim slocie;
  tekstury projektu wygrywają nad dostawcą paczki. Domyślnym formatem obrazu
  jest PNG. Materiały gry zachowują oryginalne identyfikatory i UV.
- Kompozycja paczek jest prywatnym snapshotem usuwanym po eksporcie. Operacja
  nie aktywuje buildu ani nie modyfikuje źródła. Pomija dekodowanie oryginalnej
  tekstury, gdy używa jej edytowanego odpowiednika.
- Test istniejącego zestawu teraz przechodzi również eksport wszystkich trzech
  części, zgodność bajtów PNG i ponowny import przez natywny parser OBJ:
  referencje zmian pozostają identyczne. Suite: 110 pass / 3 skip; natywny
  save_format pass.
- Oświetlenie i szkło w OBJ/MTL nadal są uproszczone; GLB oraz pełne parametry
  materiałów wymagają dalszej pracy. Domyślny podgląd wymaga jeszcze wspólnej
  kompozycji paczek zgodnej z eksportem i builderem.

### Wspólny podgląd i eksport wyglądu

- `car-view` zarządza czasem życia prywatnej kompozycji oraz pierwszeństwem
  modeli paczek/projektu. Podgląd części, całego auta i eksport korzystają
  z tej samej ścieżki. Jedna kompozycja wystarcza dla wszystkich części pełnego
  auta, również podczas eksportu zestawu.
- Domyślny podgląd pokazuje wybrane paczki i zmiany projektu; jawny podgląd
  paczki pomija zmiany projektu. Zastąpione tekstury nie są dekodowane ponownie
  z oryginału. Usunięto z serwisu powtórzoną obsługę geometrii/kompozycji.
- Testy sprawdzają jedną kompozycję dla wielu części, sprzątanie po sukcesie
  i błędzie, pierwszeństwo projektu oraz izolację porównania paczki. Test
  pełnego zestawu dodatkowo sprawdza geometrię i PNG domyślnego podglądu.
- Pełny suite przed ostatnim dodatkowym testem pierwszeństwa: 111 pass / 3 skip
  (114 testów), native save_format pass. Oba testy car-view: pass.
- Nadal brak wizualnego odbioru zmodyfikowanego auta w grze i pełnego zakresu
  torów/media/GLB z planu. Właściwości materiałów wymagają edycji w projekcie.

### Właściwości materiałów w projekcie

- Zapis materiału dla źródła z archiwum waliduje ID i komplet wartości przez
  natywny parser, a następnie zapisuje zmianę z kontrolą rewizji do wspólnej
  historii. Paczka biblioteki pozostaje niezmieniona. Starszy profil bez archive
  zachowuje dotychczasową ścieżkę zapisu paczki.
- Builder scala właściwości projektu z materiałami wybranych paczek; podgląd
  korzysta z tych samych wartości. Formularz czyta wartości projektu i opisuje
  zapis do projektu oraz oddzielne zastosowanie w grze.
- Test integracyjny obejmuje stale revision, niedozwoloną wartość, undo/redo,
  natywny manifest zastosowanego buildu i niezmienioną paczkę. Test publikacji
  przekazuje teraz wymaganą rewizję, zachowując kontrolę atomowego rollbacku.
  Suite: 113 pass / 3 skip (116 testów), native save_format pass.
- Formularz nadal otwierany jest z biblioteki materiałów; wybór nazwanej części
  w głównym warsztacie powinien prowadzić do tych samych ustawień. Eksport OBJ
  nie zachowuje pełnej semantyki materiałów, a pozostały zakres planu trwa.

### Materiały wybranej części w warsztacie

- Appearance udostępnia edycję materiałów nadwozia oraz każdej nazwanej części.
  Natywne metadane page/CLUT wyznaczają listę faktycznie używanych tekstur;
  użytkownik wybiera pozycję listy, bez wpisywania semantycznego ID.
- Lista odczytuje aktualne właściwości z kompozycji paczek i projektu.
  Ten sam formularz działa bez paczki biblioteki; techniczny ID jest zwinięty,
  a zapis przekazuje rewizję projektu i trafia do wspólnej historii.
- Test zestawu sprawdza katalog materiałów części, edycję bez paczki,
  aktualne wartości i cofnięcie. Test formularza sprawdza brak zależności od
  biblioteki, aktualną rewizję, wszystkie 11 kontrolek i zablokowany ID.
- Suite przed dodaniem ostatniego testu formularza: 113 pass / 3 skip
  (116 testów), native save_format pass. Testy formularzy po zmianie: 3 pass.
- Etykiety materiałów są uczciwie numerowanymi teksturami używanymi przez
  nazwaną część; źródło nie nadaje im nazw typu „lakier”/„reflektor”. Wybieranie
  powierzchni bezpośrednio w 3D i wizualna weryfikacja UI pozostają do zrobienia,
  podobnie jak pozostały zakres torów, mediów i popularnych formatów z planu.

### Kodek położeń obiektów toru

- Ustalono rzeczywisty format tabeli CourseObject: licznik i rekordy 20 bajtów
  (model, yaw, pozycja XYZ, cztery flagi). Kodek `PlacementsOpen/Read/Write`
  czyta niezależnie od wyrównania i endianness hosta. Zapis modyfikuje jeden
  zweryfikowany rekord, zachowując inne wpisy i nieznane końcowe bajty.
- `--track-scene` udostępnia obiekty obok drogi. Odczyt wszystkich 24 oryginalnych
  paczek PAL przeszedł; zawierają od 1 do 10 wpisów. To tabela oddzielnych
  obiektów, nie cała geometria budynków/terenu.
- Rozszerzony test C obejmuje signed coordinates/yaw, wyłączony model -1,
  zakres modeli/flag, truncation, overflow licznika, zachowanie wyjścia i
  brak częściowego zapisu przy błędzie. `track_pack` pass; dwa testy torów
  launchera pass. Native tools przebudowano i wystage'owano.
- Kodek zapisu nie jest jeszcze podłączony do operacji projektu i UI. Następny
  krok: atomowa kompilacja zmian tych obiektów do paczki toru, wspólna historia
  i scena 3D z osobnym dekodowaniem geometrii terenu/modeli.

### Kompilacja zmian obiektów toru

- Natywne `--edit-track-stdin` waliduje cały zestaw zmian, zakres liczb, model
  i flagi przed stworzeniem prywatnego wyjścia. Zachowuje offsets, pozostałe
  bloki i nieznane bajty. Brak poprawnego wyjścia przy błędzie w środku batcha.
- Zmiany `track.big1.object.0` (i pozostałych torów/klas) w projekcie trafiają
  przez builder do raw override nowego buildu. Wybrana paczka raw jest bazą;
  w przeciwnym razie bazą jest oryginał z importu. Cały plik obejmuje istniejący
  manifest hashy buildu. `trackScene` podgląda te same skompilowane zmiany.
- Test integracyjny obejmuje batch rollback, przepełnienie liczby, undo/redo,
  podgląd, apply, niezmienione pozostałe bajty/original i zachowanie aktywnego
  buildu po niepoprawnym modelu. Suite: 115 pass / 3 skip (118 testów),
  native save_format pass; nowy test po poprawce zakresu liczb również pass.
- Brakuje formularza/3D manipulacji obiektami w UI i dekodowania pełnej sceny
  geometrii. Nie oznacza to zakończenia edytora torów ani pozostałego planu.

### Formularz obiektu toru

- Tracks pozwala wybrać oddzielny obiekt sceny i zmieniać XYZ oraz obrót
  w stopniach. Model/flagę umieszczono pod rozwijanym Advanced wraz z opisem
  każdego bitu. Zapis do projektu i zastosowanie w grze są odrębnymi akcjami.
- `saveTrackObject` waliduje nową zmianę natywnie na kopii oryginału lub
  wybranego raw override przed modyfikacją historii. Niepoprawne wartości
  i stale revision nie zmieniają dokumentu. Podgląd odczytuje nowy stan.
- Test formularza sprawdza degrees, wybór wyłączonego obiektu, rewizję i
  rozdzielenie basic/advanced. Integracja toru przechodzi teraz przez publiczne
  API edycji, sprawdzając również odrzucenie błędu przed wpisem do historii.
  Suite: 116 pass / 3 skip (119 testów), native save_format pass.
- Obiekty mają tymczasowe nazwy „Object N”; pełna scena 3D, picking oraz
  rozpoznanie/nazwanie geometrii pozostają konieczne. Mapka nadal jest
  inspekcją drogi, nie podglądem terenu. Formularz nie kończy zakresu torów.

### Geometria toru z importera gry

- `tools.track_export` w istniejącej binarce działa przed inicjalizacją okna,
  storage i logowania sesji gry. Instaluje zweryfikowaną paczkę toru i używa
  tego samego NativeAssetImporter co modern renderer do zapisania course/terrain
  RMESH. Udostępnia oryginalne bindingi materiałów page/CLUT/texture window.
  Nie dodano osobnego narzędzia ani drugiego parsera geometrii.
- Wyjścia są tworzone wyłącznie, bez nadpisywania istniejących plików. Błąd
  usuwa tylko nowo zapisane wyjścia. Importer zwalnia pamięć po operacji.
- CLI ma osobne `--mesh-bank` / `--preview-bank` z zakresami submeshów;
  dotychczasowe jednoprzedmiotowe API nadal odrzuca banki.
- Headless eksport wszystkich 24 rzeczywistych paczek PAL i walidacja obu
  banków przeszły: 981 780 wierzchołków łącznie. Raport:
  `build/reports/editor/track-export-pal.json`. Mythical Coast class 1 ma
  64 modele course oraz 198 terrain cells. Brak okna/fokusu w testach narzędzia.
- Suite przed dodaniem testu bank-preview: 117 pass / 3 skip (120 testów),
  native save_format pass. Dwa testy narzędzia po dodaniu bank-preview pass:
  malformed source nie dotyka wyjść, puste submeshe zachowują zakresy,
  jednoprzedmiotowe API pozostaje restrykcyjne. Native tools wystage'owano.
- Eksporter jest jeszcze fundamentem podglądu: instancjonowanie modeli,
  tekstury, picking, pełny interfejs sceny 3D i odbiór wizualny pozostają do
  zrobienia wraz z pozostałym zakresem edytora.

### Podgląd geometrii toru w 3D

- Tracks udostępnia „View track in 3D”. Serwis kompiluje bieżące zmiany toru
  na prywatnej kompozycji, eksportuje banki natywnie i składa wyłącznie używane
  submeshe. Teren korzysta z siatki (odwrócony wiersz Z, pitch 2048), obiekty
  z pozycji i yaw; geometria GTE jest skalowana 1/4, zgodnie z rendererem gry.
- Obiekty kursu mają współrzędne modulo 65536. Pierwszy screen ujawnił obiekty
  wysoko nad torem; poprawiono eksport poprzez istniejący
  `CourseCoordinateNearReference`, wokół środka domeny toru i Y=0.
- Mythical Coast class 1 przez rzeczywisty serwis daje 36 416 wierzchołków,
  18 208 trójkątów i 203 instancje. Headless Chromium renderuje rzeczywistą
  geometrię bez błędów JS; screen obejrzano po poprawce współrzędnych:
  `build/reports/editor/track-preview-3d.png`. Browser QA nie testuje Electron IPC.
- Test składania sprawdza zakresy modeli, skalę, obrót, źródła materiałów,
  remap indeksów i odrzucenie nieznanego modelu. Suite: 119 pass / 3 skip
  (122 testy), native save_format pass. Native exporter przebudowano po
  poprawce modulo; rzeczywisty podgląd PAL uruchomiono ponownie.
- To podgląd geometrii bez tekstur: UI mówi o tym wprost. Tekstury, picking,
  manipulacja w 3D, większy viewport sceny i edycja drogi/AI/kamer pozostają
  do realizacji wraz z pozostałym zakresem planu.

### Tekstury w podglądzie toru

- Podgląd 3D dekoduje używane materiały course/terrain z oryginalnych uploadów
  tekstur w jednym natywnym wywołaniu. Zachowuje palety i texture windows,
  pełne identyfikatory materiałów oraz rozdział banków. Wybrany snapshot
  dostarcza raw overrides i istniejące wpisy materiałów/tekstur.
- PNG korzysta z kompresowanego writera SDL zamiast ręcznego stored-deflate.
  Test batch sprawdza piksele i okna, odrzucenie błędnego wejścia przed zapisem
  oraz ochronę istniejących plików. Test składania chroni ID ponad 64 i banki.
- Rzeczywisty PAL Mythical Coast: 340 materiałów, 36 416 wierzchołków,
  11,23 MB JSON, przygotowanie około 2,1 s. Headless Chromium wyrenderował
  teksturowany tor bez błędów JS; screen obejrzano:
  `build/reports/editor/track-preview-textured.png`.
- Pełna suite przed dodatkowym testem remapu: 120 pass / 3 skip (123 testy).
  Picking, manipulacja obiektami w 3D i edycja drogi/AI nadal nie są gotowe;
  niniejszy etap nie domyka całego planu edytora.

### Wybór obiektu w podglądzie 3D

- Kliknięcie trójkąta obiektu kursu wybiera jego rzeczywisty indeks i otwiera
  istniejący formularz pozycji/obrotu w Tracks. Zapis nadal przechodzi przez
  walidowany projekt i wspólne undo/redo; przeciąganie obraca podgląd.
- Picking korzysta z tej samej projekcji ortograficznej co shader i wybiera
  najbliższy trafiony trójkąt. Teren bierze udział w zasłanianiu. Test sprawdza
  głębokość, brak trafienia, zasłanianie oraz obrót widoku.
- Headless WebGL potwierdził rzeczywisty pointer click (object 7), brak wyboru
  przy przeciąganiu oraz brak błędów JS. Nie był to test Electron IPC.
- Ograniczenia: picking jest geometryczny (nie uwzględnia przezroczystych
  pikseli tekstur); nie ma jeszcze gizmo ani przesuwania obiektu w viewport.

### Natywny zapis punktów drogi

- `RoadWrite` aktualizuje jeden 24-bajtowy rekord, zachowując łuki i wszystkie
  pozostałe bajty. Waliduje długość całej drogi i odwołania do łuków przed
  zapisem; błędne wejście pozostawia blok bez zmian.
- Projekt rozpoznaje `track.<course><grade>.road.<index>` i kompiluje jego
  pola przez ten sam atomowy track writer co obiekty. CLI sprawdza zakresy
  int16/int32 i tworzy wyjście dopiero po walidacji całej paczki zmian.
- C test pokrywa niealigned zapis, wartości graniczne i brak modyfikacji po
  błędzie. Integracja natywnego CLI sprawdza odczyt zapisanych pól, zachowanie
  innych bloków oraz brak wyjścia po błędzie w drugiej zmianie paczki.
- Formularz edycji punktów nie jest jeszcze podłączony. To zapis danych drogi,
  nie automatyczna przebudowa geometrii wizualnej czy aktualizacja AI/kamer.

### Formularz punktów drogi i pełny przepływ projektu

- Tracks pozwala zapisać pozycję/height i half-widths wybranego punktu;
  kierunek, nachylenie, długość i packed arc reference są po rozwinięciu,
  z opisem jednostek i braku automatycznej przebudowy wizualnego asfaltu.
- IPC używa `saveRoadPoint`; walidacja i staging są wspólne z obiektami,
  zamiast drugiej implementacji. Pola projektu używają snake_case, zgodnie
  z kontraktem nazewnictwa projektu; adapter mapuje do istniejącego C codec.
- Integracja publicznego serwisu sprawdza zapis, odrzucenie błędnej długości
  bez nowej rewizji, undo/redo, effective preview oraz apply i odczyt
  wynikowego banku. Oryginalny asset pozostaje nienaruszony.
- Test UI sprawdza obecność wszystkich dziesięciu pól, basic/advanced,
  rewizję i wartości original (w tym szerokości i cross-slope).
- Nadal brakuje przebudowy geometrii, arc centers, AI i kamer; formularz
  jawnie opisuje te granice i nie przedstawia surowych zmian jako gotowego
  nowego toru.

### Środki łuków drogi

- `RoadArcRead/Write` czyta/zapisuje X/Z środka łuku, zachowując reserved
  word rekordu. Runtime `CarTrackMeasureArc` rzeczywiście używa tych X/Z,
  a `GameTrackArcCenter.reserved08` jest opisany jako nieczytany.
- Wybrany curved point pokazuje formularz jego wspólnego środka. Zapis używa
  `track.<course><grade>.arc.<index>`, tej samej walidacji/stagingu/historii,
  co punkty drogi i obiekty. Nie udostępnia reserved word.
- Walidacja zapisu track entry obejmuje teraz cały efektywny draft wybranego
  toru, łącząc wcześniejsze zmiany z nowymi przed kompilacją.
- C test: unaligned read/write, skrajne int32, zachowanie reserved word,
  nieistniejący indeks i brak mutacji po błędzie. CLI test: byte preservation,
  nieudana druga zmiana bez wyjścia. Service: effective preview, błędny indeks
  bez rewizji, undo/redo. UI: poprawny indeks środka, original values, opis
  wspólnego użycia i brak edycji reserved word.
- Mapa nadal przedstawia połączenia control points, nie próbkowany łuk.
  Geometria asfaltu, przebudowa drogi oraz AI/kamery pozostają otwarte.

### Zakręty na mapie przebiegu drogi

- Mapa próbuje łuki według zapisanych X/Z środka, wybierając najkrótszy span
  kąta, zgodnie z `GetAngleDelta/GetAngleDistance`. Mirrored curve zmienia
  lateral sign w fizyce, nie wybór kierunku centerline.
- Promień interpolowany między punktami daje geometryczny overview;
  nie udaje bit-exact fixed-point fizyki. Opis UI zaznacza tę różnicę.
- Bounds obejmują próbkowane zakręty, punkty wyboru nadal odpowiadają
  rekordom źródłowym, a zmiany środka wpływają na następną effective mapę.
- Testy: ćwiartka koła, reversed/mirrored, przejście przez +/-pi, brak
  środka i pusta droga. Rzeczywisty PAL Mythical Coast: 428 próbek,
  headless Chromium bez błędów JS. Obejrzany screenshot:
  `build/reports/editor/track-road-curves.png`.

### Runtime dla zastępowania geometrii toru

- Dotychczas `[meshes]` było stosowane do authored car parts; geometria
  course/terrain nie korzystała z wpisów manifestu. Runtime modern obsługuje
  teraz pełne banki `track.<course><grade>.course` i `.terrain`.
- Wczytanie jest ograniczone do 128 MiB i zgodnej liczby submeshów. Jedna
  immutable kopia przypada na bank w sesji; resident lookup nie robi I/O,
  teardown zwalnia wszystkie kopie. Uszkodzony albo niezgodny opcjonalny
  mod zgłasza diagnostykę i pozostawia oryginalny bank.
- Compiled test przechodzi przez prawdziwy manifest/cache: zmieniony vertex,
  resident lookup, niemutowalność po uszkodzeniu źródła, teardown/reload,
  odrzucenie uszkodzonego pliku i poprawnego banku z niewłaściwą liczbą modeli.
  Asset ID test pokrywa klasy/kursy, niepoprawny bank/set i mały bufor.
- Gra została przebudowana. Testy `asset_id` i `modern_assets_retry` pass.
- To prerequisite runtime: import/export banków w projekcie i UI jeszcze
  wymagają podłączenia. Classic adapter, GLB i przebudowa drogi pozostają
  otwarte; ten etap nie domyka edycji geometrii toru.

### Modele toru w effective preview i buildzie projektu

- Projekt przyjmuje geometryczne sloty `track.<course><grade>.course/terrain`
  jako immutable RMESH references. Track pack writer omija te zmiany;
  project build zapisuje je w `[meshes]` zamiast próbować edytować raw rekordy.
- Podgląd stosuje wybrane pakiety, potem immutable delta projektu, przed
  dekodowaniem banku i przypisaniem jego oryginalnych materiałów.
- Build waliduje liczbę submeshów względem rzeczywistego efektywnego raw
  packa, również dla wybranych pakietów. Walidacja jest wykonywana w stagingu
  przed aktywacją. Nie wykrywa samodzielnie przestawienia modeli tej samej
  liczby — zachowanie kolejności pozostaje kontraktem importu.
- Native mesh-bank summary udostępnia teraz mesh count. Test sprawdza
  package/project precedence, undo, pełne immutable bytes i odrzucenie
  banku z inną liczbą modeli.
- Rzeczywisty PAL Mythical Coast class 1: zmieniony bank 64 modeli przeszedł
  publiczny effective preview i apply; plik wynikowego builda byte-exact
  względem immutable referencji. Test wykonał się w temp root, z oryginalnymi
  danymi wyłącznie do odczytu. Nie był to test widocznego przejazdu.
- Suite: 128 pass / 3 skip (131 tests). Import/export modeli toru w UI,
  GLB i classic adapter nadal wymagają wdrożenia.

### Import/export natywnych banków toru w UI

- Advanced → Native model banks udostępnia kurs/teren aktualnie wybranego
  course/class. Eksport uwzględnia selected packages oraz immutable draft;
  import tworzy jedną zmianę projektu, bez tworzenia osobnego library mod.
- Import kopiuje wejście do prywatnego stagingu, waliduje RMESH, liczbę modeli
  oraz zgodność material IDs z oryginalnym bankiem (untextured 65535 dozwolone).
  Rewizja projektu chroni przed nadpisaniem nowszego draftu. Export używa
  atomowego zapisu w wybranym przez użytkownika miejscu.
- Prawdziwy PAL przez publiczne API: export → zmiana vertex → import →
  effective export byte-exact, stale revision rejection, undo original,
  redo replacement, corrupt import bez zmiany rewizji ani effective bytes.
  Wszystko wykonano w temp root, bez modyfikacji oryginalnych danych.
- Testy kontrolują native bank contract i slot/revision w UI. Ograniczenia
  są jawne: to natywny advanced workflow dla modern, bez OBJ/GLB i classic
  adaptera. Pełny, prosty model exchange nadal wymaga dalszego wdrożenia.

### OBJ dla wybranego obiektu kursu

- Wybrany placed object udostępnia Export/Import model OBJ. Operacja dotyczy
  źródłowego modelu, wspólnego dla wszystkich instancji z tym model ID;
  UI opisuje ten zakres. Eksport tworzy osobny folder z OBJ/MTL i instrukcją.
- Import korzysta z istniejącego compiled OBJ codec i RuntimeMeshReplace,
  zachowując kolejność/ranges pozostałych modeli. Wynik zapisuje się jako
  immutable bank geometry w tej samej historii projektu.
- Nowa natywna komenda `--replace-mesh` waliduje oba wejścia, zakres modelu
  oraz ograniczenia rozmiaru i zapisuje wynik wyłącznie do nowego pliku.
- Compiled integration: zachowanie innego modelu, replacement range,
  brak nadpisania istniejącego output, zły index bez output. UI test:
  source slot, model number, revision i opis wspólnej geometrii.
- Prawdziwy PAL Mythical Coast: export model 55 → zmiana vertex → import
  → bank nadal 64 modeli → effective textured preview pass. Temp root,
  oryginalny obraz i dane wyłącznie do odczytu; bez widocznej gry.
- PNG tekstury nie są jeszcze dołączane do tego OBJ exportu (opisane w UI
  i README). Pełny track interchange, GLB, wygodny wybór modeli terenu i
  classic adapter nadal wymagają pracy.

### OBJ/PNG exchange dla modeli kursu

- Export OBJ dołącza tylko materiały używane przez wskazany model, dekodując
  PNG przez istniejący batch/window decoder. MTL odwołuje się do tych PNG.
  Rozwiązanie uwzględnia selected raw/texture overrides i draft texture edits;
  preview korzysta z tej samej ścieżki dekodowania.
- `track-model.json` zapisuje source identity, model slot/index i mapę PNG.
  Import z tym manifestem tworzy jeden batch history dla banku i tekstur.
  Bez manifestu nadal można importować samą geometrię OBJ.
- Manifest musi pasować do źródła i modelu; tylko oryginalne material IDs
  oraz stałe lokalne PNG filenames są akceptowane. Symbolic links, uszkodzony
  PNG i niezgodne źródło są odrzucane. Produkcyjny import zamraża PNG
  w prywatnym stagingu przed native validation i immutable store.
- Project build obsługuje track course/terrain texture edits i materials;
  track pack writer omija geometry/image/material references.
- Testy: source/index binding, traversal, PNG decode failure i immutable
  references. Prawdziwy PAL model 55: dwa PNG w MTL, trzy project changes
  (bank + PNG), effective preview i applied manifest pass. Dane źródłowe
  tylko do odczytu, temp root, bez widocznej gry.
- Texture slot może być współdzielony przez inne modele — opis w UI.
  GLB, pełny track exchange i classic adapter nadal pozostają otwarte.

### Wybór modelu terenu w viewport

- Picker zwraca teraz source entity (bank/mesh/object), zamiast odrzucać
  trafienia terenu. Assembler zachowuje mesh index obok instancji i ranges.
  Course objects prowadzą do formy transformacji; teren otwiera panel OBJ/PNG
  dla wybranego source modelu. Współdzielenie geometrii jest opisane w panelu.
- Import modelu zamyka dialog, żeby następna operacja nie używała starej
  rewizji projektu. Nie dodano osobnego backendu ani stanu wyboru modelu.
- Unit picking/assembly sprawdza type i source mesh oraz terrain occlusion.
  Headless WebGL: klik wybiera terrain model 12, przeciąganie nie wybiera;
  bez błędów JS (nie test Electron IPC).
- Rzeczywisty PAL: klikany bank terrain zawiera model 197; jego OBJ + dwa PNG
  wyeksportowano i zaimportowano przez publiczne API. Powstały trzy zmiany
  projektu i effective textured preview pass. Temp root, source read-only.
- Brak gizmo/przesuwania komórek terenu, edycji topologii całego toru i GLB;
  wybór wspólnego modelu terenu nie zastępuje tych wymagań.

### Podgląd wybranego modelu toru

- Course object i panel klikniętego terrain model udostępniają Preview model.
  Podgląd składa tylko wybrany submesh, więc kadr nie obejmuje innych modeli
  ani nieużywanych wierzchołków banku. Zachowuje source material IDs/PNG.
- Preview i OBJ export dzielą `texturedPart`, zamiast drugiej ścieżki
  dekodowania tekstur. Pusty/nieistniejący model zgłasza czytelny błąd.
- Prawdziwy PAL: course model 55 ma 68 vertices / 2 textures, terrain model
  197 ma 12 vertices / 2 textures. Publiczne preview API zwraca pojedynczą
  instancję; niepoprawny index odrzucony. Headless WebGL bez błędów JS,
  screenshoty obu modeli obejrzano:
  `build/reports/editor/track-course-model.png`,
  `build/reports/editor/track-terrain-model.png`.
- UI test sprawdza przycisk Preview dla source modelu. Nadal nie ma
  scene gizmo/manipulacji bezpośredniej, GLB i classic mesh adaptera.

### Natywna edycja przypisania komórki terenu

- `TerrainOpen` przenosi dotychczasową runtime walidację grid/offsets/streams
  do shared C codec z unaligned reads. `IsValidTerrainCellAsset` deleguje do
  niego zamiast utrzymywać osobny validator. Zasady akceptacji zachowane.
- `TerrainWriteModel` zmienia tylko low 10 model bits wskazanej komórki,
  zachowując region bits, visibility rows, model streams i inne banki.
  1023 oznacza brak komórki. Index/model bounds walidowane przed zapisem.
- Project slot `track.<course><grade>.cell.<index>` kompiluje się przez ten
  sam track writer. Publiczne `saveTerrainCell` używa pełnego draft validation
  i shared history. Formularz wyboru modelu komórki nie jest jeszcze podłączony.
- C tests: unaligned data, region 63, missing model, invalid index/model,
  bad offsets/truncated stream i brak mutacji po błędzie. Runtime model_banks
  tests również pass. CLI test kontroluje byte preservation całego packa.
- Wszystkie 24 rzeczywiste PAL terrain blocks przeszły native no-op edit
  i byte-exact comparison. Mythical Coast cell 259: hide → effective preview
  203→202 instances, undo →203. Temp root, oryginały read-only.

### Przypisywanie modeli terenu w działającym edytorze

- Track workspace udostępnia wszystkie 1024 komórki, również puste. Wybór
  modelu lub Empty cell zapisuje zmianę przez `saveTerrainCell`, native codec
  i wspólną historię projektu. Nie zmienia źródłowej płyty.
- Kliknięcie terenu w podglądzie 3D wybiera source cell, nie tylko współdzielony
  model. Panel udostępnia istniejące Preview oraz import/export OBJ + PNG.
  Niezapisane przypisanie blokuje zmianę komórki również przez kliknięcie 3D.
- Pełny zestaw launcher: 138 testów, 135 pass, 3 optional regional skips.
  Headless rzeczywisty app.js + LauncherService + native importer na PAL:
  wybór cell 259, Empty cell, zapis, odczyt zmienionego projektu i odświeżenie
  panelu pass, bez błędów JS. Źródło read-only, prywatny temp project.
- To rzeczywista funkcja edytora, nie makieta. Ten test nie dowodzi jeszcze
  działania Electron IPC, publikacji moda ani wizualnego efektu w grze.

### Weryfikacja publikacji edytowanej komórki terenu

- Stały test obejmuje `saveTerrainCell` → `applyProject` → aktywny build:
  opublikowany pack jest byte-exact względem native edit, oryginał nietknięty.
  Undo + apply usuwa override; redo odtwarza zmianę. Nieprawidłowy model
  odrzuca publikację i zachowuje dotychczasowy active.json.
- Rzeczywisty PAL, Mythical Coast cell 259: Save & apply opublikował raw pack,
  native reader potwierdził Empty cell. Cały plik poza dwoma bajtami tej
  komórki pozostał identyczny. Prywatny temp modsRoot, źródło read-only.
- Focused track-edits: 4 pass. To dowodzi buildu i odczytu natywnego packa;
  screenshot działającego wyścigu oraz Electron IPC pozostają do sprawdzenia.

### Pakiet rzeczywistego edytora

- Nazwa desktop window, menu aplikacji, pakietu i package probe to Rage Racer
  Editor, zgodnie z istniejącym UI. Zachowano userData i bundle ID, aby nie
  zgubić istniejących profili. Historyczne IMPLEMENTATION wpisy nietknięte.
- Zbudowano aktualny lokalny macOS arm64 pakiet wraz z compiled tools:
  `launcher/out/Rage Racer Editor-darwin-arm64/Rage Racer Editor.app`.
  Nie opublikowano ani nie uruchamiano okna zabierającego fokus.
- Skorygowano opis banków: OBJ + PNG działa dla poszczególnych modeli;
  brakującym formatem pozostaje GLB. Track workspace tests: 8 pass.
- Sam sukces pakowania nie dowodzi poprawności działania packaged IPC.

### Publikacja projektu bez zewnętrznych pakietów

- Naprawiono realny przycisk Save & apply: legacy publishMods wymagał co
  najmniej jednego enabled package, choć własne edycje projektu nie wymagają
  żadnego zewnętrznego moda. Publikacja korzysta teraz zawsze z project builder.
- Test komórki terenu wywołuje tę samą publiczną ścieżkę co przycisk i sprawdza
  opublikowany raw pack przy pustej bibliotece pakietów. Pięć focused tests pass.
- UI odrzuca Apply przy zmienionych, niezapisanych polach data-original,
  zamiast po cichu publikować poprzednie wartości. Headless actual app.js + PAL
  service potwierdza komunikat i brak zapisu projektu przed submit formularza.

### Zmiana położenia obiektu z podglądu 3D

- Shift-drag trafionego course object zapisuje poziome przesunięcie przez
  istniejące `saveTrackObject`. Y, model i flags pozostają nietknięte; zmiana
  korzysta z rewizji oraz historii projektu. Podgląd przeładowuje effective
  geometry, zachowując kadr i obrót. Bez drugiego formatu lub writer backendu.
- Współrzędne ruchu odwracają projekcję ortograficzną na płaszczyźnie XZ;
  source Z ma przeciwny znak. Niemal poziomy widok odrzuca ruch z instrukcją
  użycia Top/tilt, zamiast zgadywać głębokość.
- Unit sprawdza odwracanie yaw/pitch/aspect/scale; headless WebGL sprawdza
  rzeczywisty Shift-drag, target object, kierunki delta i odświeżenie bez
  błędów JS. Zapis/native publication mają osobną istniejącą regresję.
- Pełny launcher suite: 139 tests, 136 pass, 3 optional regional skips.
- To przesunięcie stosowane po puszczeniu przycisku, jeszcze bez live gizmo,
  uchwytów wysokości/obrotu i snapping. Test gestu używa synthetic geometry,
  nie dowodzi jeszcze kompletnego PAL scene → ruch → wyścig.

### Rzeczywisty PAL: przeciągnięcie obiektu

- Headless actual app.js + LauncherService + compiled decoder: otwarto
  Mythical Coast class 1, znaleziono widoczny course object przez ten sam
  picking, wybrano Top i wykonano Shift-drag myszką. Odczyt effective source
  potwierdził zmianę X/Z przy identycznym Y/yaw/model/flags; undo przywróciło
  pełny original object. Temp project, PAL dane read-only, zero pageerrors.
- Sprawdzenie ujawniło zbyt wczesny komunikat sukcesu: pojawiał się podczas
  kolejnego odczytu native preview. Teraz callback czeka na wynik tego odczytu
  przed komunikatem, zamiast zachęcać do operacji przy zajętym backendzie.
- Powtórzono test po poprawce. Nadal nie jest to weryfikacja Electron IPC ani
  wizualnego efektu w rendererze gry; browser bridge wywołuje prawdziwy service.

### Parametry prowadzenia samochodów

- Native car-spec reader/writer i shared profile service obsługują sześć
  dotąd niewidocznych pól: steering grip response, AI steer response,
  reference turn radius, neGcon assist scale, speed drag divisor, base grip.
  Odczyt zachowuje typ unsigned steerResponse. Edycja AI response jest
  ograniczona do dodatniego signed zakresu używanego przez obecną fizykę.
- GUI umieszcza je w osobnym Advanced handling & drag z opisami. Engine/AT
  pozostają w dotychczasowej sekcji; nie dodano osobnego backendu zapisu.
- Tests: native write porównuje cały pack byte-exact, read unsigned 40000,
  bounds, project read/apply wszystkich sześciu pól, UI brak duplikatów.
  Full launcher: 140 tests, 137 pass, 3 optional regional skips. Po dodaniu
  unsigned fixture focused car-spec: 5 pass.
- Rzeczywisty PAL: odczyt builtin origin, edycja drag/AI response, apply
  i odczyt aktywnego cars.toml pass w temp profile. Nie weryfikuje to jeszcze
  odczuwalnego prowadzenia w wyścigu. Krzywe torque/gear arrays nadal brakuje
  w GUI i pozostają częścią pełnego zakresu.

### Krzywe silnika i tablice przekładni

- Native spec reader/writer obsługuje torque_curve/band/loss_value/loss_rpm,
  gear_load/ratio oraz torque_scale. Indexed fields zachowują 32/16-bit LE
  szerokości; test porównuje cały wynikowy pack i graniczne wartości.
- Shared service wiąże indeksy z pełną tablicą profilu. Zmiana jednej próbki
  zachowuje pozostałe; shift points używają teraz tego samego mechanizmu.
  Przywrócenie wszystkich bazowych próbek usuwa override tej tablicy.
- GUI ma rozwijane siedem grup z oryginalnymi indeksami i opisem jednostek.
  Dane liczbowe są edytowalne; jeszcze bez wykresów i przeciągania punktów.
  Dodano margines przewijania dla pól nad sticky footer.
- Full suite: 141 tests, 138 pass, 3 optional regional skips. Po rozszerzeniu
  undo/reset fixture focused car-spec: 5 pass.
- Actual app.js + compiled PAL catalog/spec: Cars → Driving → engine curves,
  edycja torque sample 0, Save project only, odczyt zmienionej pełnej tablicy
  pass. Temp root, read-only source, headless browser, brak pageerrors.
  Screenshot formularza obejrzano: build/reports/editor/car-engine-arrays.png.
- GUI weryfikacja używa bridge do service, nie Electron IPC; test nie dowodzi
  jeszcze zmiany osiągów podczas wyścigu ani kompletnej walidacji fizyki.
