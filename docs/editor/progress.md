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

### Nazwa, producent i oznaczenie klasy

- Basic formularz samochodu edytuje name/manufacturer/class label, niezależnie
  od unlock level. Shared profile read/edit stosuje builtin/project origins,
  limit 31 UTF-8 bytes oraz zakaz znaków nieobsługiwanych przez obecny codec:
  #, quote, backslash, control. Nie powstał nowy serializer.
- Car catalog scala projektowe nazwy body slots. Po zapisie formularza UI
  odświeża katalog, więc lista pokazuje nową nazwę bez restartu programu.
- Service test obejmuje published profile wszystkich trzech tekstów,
  widoczną nazwę katalogu i błędne/za długie UTF-8 dane. UI test zapewnia
  trzy tekstowe pola w podstawowej sekcji i brak duplikatów.
- Full suite przed ostatnią dodatkową # fixture/refresh: 142 tests, 139 pass,
  3 optional skips. Końcowe focused car-spec/hierarchy: 11 pass.
- Actual app.js + PAL native catalog/spec: edytowano name i torque sample,
  Save project only, potwierdzono pełny array oraz nową nazwę na liście.
  Test czeka na koniec odświeżenia, nie jedynie zamknięcie dialogu. Source
  read-only, temp project, headless, zero pageerrors; nie Electron IPC.

### PNG przycisków menu: runtime i project build

- Runtime akceptuje `ui.title.<grand-prix|extra-grand-prix|time-attack|custom|
  load-save|options>` oraz `.selected` w semantic texture manifest. Menu czyta
  112×16 RGBA PNG bez quantization do PS1 palety i bez uploadu do VRAM atlasu.
  Obecny rasterizer używa logicznych pixel runs, alpha <128 jest przezroczysta;
  nie jest to jeszcze hi-res/partial-alpha sprite renderer.
- Loaded pixels należą do cache sesji moda. Shutdown zwiększa generation,
  więc ponowna sesja nie zachowuje starego PNG lub negative cache. Brak
  override pozostawia istniejący przycisk. Normal/selected są osobnymi slotami.
- Wykorzystano existing bounded PNG decoder, aby odrzucić oversize header
  przed alokacją obrazu. Nie dodano nowego image dependency ani launchera tool.
- Project builder obsługuje interface texture slots, immutable PNG blobs,
  native decode i exact layout validation. Wadliwy import nie podmienia
  active pointer. Formularz import/export tych slotów nadal do podłączenia.
- Compiled menu_images test ma włączone assertions także w Release i sprawdza
  RGBA, transparent pixel, tile colors, reveal height, normal/selected slot,
  cache, session invalidation i invalid dimensions. CTest pass. Project-build
  focused tests: 6 pass, w tym interface build i previous-build preservation.
- Pełny game build jest osobną weryfikacją; screenshot title screen i GUI
  flow import/apply pozostają wymagane przed stwierdzeniem gotowości.

### Panel Interface podłączony do prawdziwego projektu

- Nazwane normal/selected slots wszystkich sześciu title buttons udostępniają
  effective preview, export PNG, import PNG i reset. Nowa zakładka Interface
  używa bounded preload/IPC; file dialogs pozostają w procesie głównym.
- Shared title_image C codec zastępuje duplicate bitmap/palette właściciela
  w menu i eksportuje te same pixels przez existing rage-mod-cli. Nie dodano
  kolejnego standalone tool. PNG export odpowiada kolorom runtime (5-bit <<3).
- Service uwzględnia selected package composition oraz immutable project PNG,
  sprawdza 112×16/native decode/revision, nie mutuje oryginałów. Reset usuwa
  override i wraca do package/built-in; undo odtwarza project PNG.
- Full launcher: 145 tests, 142 pass, 3 optional regional skips. Native
  menu_images CTest pass po przeniesieniu shared bitmap codec.
- Actual app.js + native service: Interface → export selected CUSTOM PNG →
  import jako normal → publish → reset, bez pageerrors. Temp modsRoot,
  headless, read-only PAL source; screenshot panelu obejrzano:
  build/reports/editor/title-menu-images.png. File dialogs/Electron transport
  i final game screenshot nie są objęte tym browser bridge testem.
- Latest full game build/staging uruchomiono osobno. Nie stwierdzamy jeszcze
  kompletnego U02/U03: logo/background/fonts/HUD/layout nadal brakuje edytorów.
- Latest native staging zakończył się sukcesem: przebudowano title_menu_labels,
  zlinkowano pełną grę i odświeżono launcher/resources/bin. Nie uruchomiono
  widocznego okna ani publikacji release.

### PNG sprawdzone w rzeczywistej grze

- Edytor wyeksportował selected CUSTOM PNG, zaimportował go do normal slot i
  opublikował snapshot z fingerprintem prawdziwego PAL RAGE.BIN. Game runtime
  rozwiązał active.json i załadował PNG na obu rendererach.
- Ukryty native smoke zatrzymano dokładnie w głównym menu (scene 4, frontend
  2, timer 180). Własny portable bu00, config/mods/profile i istniejący PCM
  izolują test od zapisów i konfiguracji użytkownika. Nie otwarto widocznego okna.
- Cztery kadry oryginał/override w classic/modern obejrzano. CUSTOM zmienia
  szary przycisk na czerwony; logo i tło zachowane. Lokalne evidence:
  build/reports/editor/title-{classic,modern}-{original,edited}.ppm.
- Powtarzalny opt-in launcher/tests/title-game.test.cjs uruchamia rzeczywistą
  grę z prywatnym projektem. Wymaga RR_EDITOR_GAME_SOURCE (imported game root
  z RAGE.BIN i data/), RR_EDITOR_GAME_DISC (PAL CUE) oraz
  RR_EDITOR_GAME_SMOKE (native smoke executable). Porównuje wszystkie pixels:
  zmiana musi wystąpić w CUSTOM i nie może dotknąć innego obszaru kadru.
- Service test dodatkowo potwierdza precedence project > selected package >
  built-in, reset oraz wyłączenie pakietu. Nie oznacza to gotowości całego
  Interface workspace: HUD/fonts/layout pozostają poza aktualnym zakresem.

### Audio: prawdziwy WAV → projekt → PCM runtime

- Nowy panel Audio ma nazwany course/car select cue, rzeczywisty odsłuch
  effective WAV, długość/origin, import/export/reset i Save & apply. Format
  i preview volume są pod details. Nie dodano kolejnego playera gry ani
  osobnego narzędzia: konwersja używa istniejącego rage-mod-cli i SDL.
- Native import sprawdza RIFF/chunk bounds, PCM/float format, channels/rate
  oraz input/output 128 MiB przed dekodowaniem. Normalizuje do canonical
  44.1 kHz stereo S16LE. Exclusive output nie nadpisuje istniejącego pliku;
  błędny WAV nie zmienia projektu. Immutable WAV blobs korzystają ze wspólnej
  historii i build snapshotu.
- [audio] w shared C manifest, audio file policy i provider composition mają
  semantic conflict key audio:music.course-select. Precedence project > chosen
  package > original PCM, reset/undo/redo i package conflict choice sprawdzono
  przez native service. Build corruption pozostawia poprzedni active.json.
- Runtime MenuMusicPrepare odczytuje tę samą binding przed original PCM;
  istniejące event timing, volume i whole-track loop pozostają w tym playerze.
- Actual app.js + native service, headless PAL: audio decoded przez browser,
  duration 50.94 s, export/import/publish/reset PASS, bez pageerrors. Dodano
  potrzebny media-src data: do CSP, bez rozszerzania uprawnień skryptów.
- Test zwykłego executable ujawnił wcześniejszą kolejność inicjalizacji:
  ModAssetsSetSource wywoływano przed HostInitDisc, więc fingerprint nie istniał
  i poprawny aktywny projekt był odrzucany. Przeniesiono source registration
  po udanym HostInitDisc. Smoke miał już poprawną kolejność i nie ujawniał błędu.
- Nie stwierdzamy pełnego U04/U05/U06: inne cues, waveform/trim, saved gain,
  custom loop points, CD replacement i odsłuch manualny wszystkich regionów
  pozostają do wykonania. Nowy panel jawnie opisuje bieżący zakres.
- Końcowy native build/staging PASS. Normalny release executable, nie smoke,
  załadował opublikowany WAV przez Psyz_PcmMusicLoad w classic i modern:
  launcher/tests/audio-game.test.cjs PASS. Test zatrzymuje prawidłowo
  zainicjalizowaną grę na początku FMV (scene 5), bez wielominutowego przejścia
  ekranów lub mockowania stanu. Sprawdza runtime diagnostic z real game.log;
  nie jest to odsłuch całego menu ani test miksu podczas wyboru auta.
- Reprodukcja wymaga RR_EDITOR_GAME_SOURCE (PAL imported root),
  RR_EDITOR_GAME_DISC (PAL CUE) i RR_EDITOR_GAME_RELEASE (normal executable).
  Wszystkie zapisy/config/mods są izolowane; original source jest read-only.
- Full npm test: save_format PASS i 151 JS tests: 146 pass, 5 explicit skips
  (optional regional fixtures i dwa opt-in game tests). Audio focused 4 PASS;
  opt-in normal-game test wykonany osobno PASS na obu rendererach. Standalone
  Release mod contracts 11/11 PASS, z aktywnymi assertions w file policy.

### Audio: trim, saved gain i rzeczywisty waveform

- Zapisane start/end frames oraz gain 0–200% są nondestructive properties
  wspólnego projektu. Source WAV pozostaje kompletny. Native --wav-edit
  generuje preview/export/build PCM, więc runtime player nie dostaje kolejnego
  zestawu audio-state/processing branches. Default 100% jest sample-identical.
- Panel pokazuje native stereo min/max envelope, odsłuch po przetworzeniu,
  source-relative seconds i saved volume pod details. Clipping jest raportowany;
  gain 0 tworzy ciszę. Discard przywraca formularz, publish blokuje niezapisane
  wartości. Import nowego WAV resetuje parametry w tej samej undo operation.
- Native + service tests sprawdzają dokładny wybór samples i gain, saturację,
  zero gain, malformed ranges, waveform, undo/redo/reset, source preservation
  oraz equality preview/build PCM. Audio focused: 6 PASS. Full npm test:
  save_format PASS; 153 JS tests: 148 PASS, 5 optional skips, zero failures.
- Actual app.js + native PAL service headless: trim 1–3 s / 50%, waveform i
  browser preview 2 s, export/import/publish/reset PASS, zero pageerrors.
  Normal release executable załadował przetworzony WAV w classic i modern:
  opt-in audio-game test PASS. Test sprawdza player load przy boot, nie
  deklaruje odsłuchu pełnych menu transitions. Native CLI rebuilt/staged.
- Custom loop points, pozostałe cues i CD music nadal pozostają do wykonania.

### Tory: dodawanie, duplikowanie i usuwanie dekoracji

- Real Tracks workspace ma wybór course model + native model preview, Add at
  selected road point, Duplicate i Remove. Wszystkie działania używają wspólnego
  projektu, revision checks, compiled validation i Undo/Redo. Niezapisany
  object form nie jest tracony przez duplicate/remove/add.
- Native editor dopisuje 20-byte placement record, zwiększa count oraz offsety
  późniejszych bloków. Nie reserializuje road/camera/event/model payloads ani
  nie gubi nieznanych końcowych bajtów. Edits sortowane po numeric object ID
  przed append zapobiegają błędom po przekroczeniu index 9.
- Remove ustawia istniejący disabled model -1, wspierany już przez oba
  renderery. Nie przesuwa IDs pozostałych instancji. Duplicate zachowuje pełny
  transform/flags; Add bierze pozycję wybranego road point. Nowe instancje
  używają istniejących course models; dodawanie nowych modeli to osobny etap.
- Ujawniono i poprawiono runtime gate: poprawny manifest/active snapshot może
  dostarczać raw bank bez raw/asset_000.bin. Legacy packs bez manifestu zachowują
  marker detection; legacy texture patch path nadal wymaga starego layoutu.
- Native runtime regressions: 9/9 PASS, w tym verified active raw payload bez
  asset_000. Track service tests 5/5 PASS: twelve new rows, source/non-placement
  blocks/trailing bytes preserved, invalid/stale actions rejected, build/history.
- Actual PAL app.js + native service, headless: 5 → 7 slots po duplicate,
  removal i add; native game InstallTrackRuntimeAssetPack + 3D importer
  uwzględnia nową instancję, pomija wyłączoną; publish PASS, zero pageerrors.
- Opt-in scenery-game.test.cjs: normal race loaded larger bank 88 and reached
  scene 12 timer 20 w classic i modern bez marker file; PASS. Wymaga tych samych
  RR_EDITOR_GAME_SOURCE/DISC/RELEASE co audio game gate, używa isolated portable
  state i normal direct race initialization. Nie jest to ocena wyglądu nowych
  obiektów w całym okrążeniu ani visibility bounds T13.
- Full npm test: save_format PASS; 156 JS tests, 150 PASS, 6 optional skips.
  Native CLI i normal game zostały przebudowane i staged. T04 nadal wymaga
  szerokiej weryfikacji ray queries/visibility/performance i manual authoring.
- Chosen-model dropdown + actual native preview modal open/close sprawdzono
  w tym samym headless PAL UI przebiegu. Dodatkowy workspace test obejmuje
  empty object list oraz add/duplicate/remove IDs i model choices.

### Tory: rzeczywisty timeline atmosfery

- Native CLI dekoduje environment script z block 2: sky row, loop length,
  ordered cues, duration, mode i dziewięć RGB slots. Nowy panel Atmosphere
  pokazuje nazwane fog/sky colors; timing/palette/ground są pod details.
- Edycja wszystkich changed cues jest jedną zmianą wspólnego projektu. C
  kompilator waliduje wynikowy timeline po całym batchu, a nie przejściowe
  stany. Cue 0 pozostaje time 0; starts rosną i mieszczą się w loop length;
  mode 0–4, duration 0–32767 i RGB 0–0xffffff. Reserved bytes, alpha/unused
  bytes, sentinel oraz reszta toru pozostają nietknięte.
- SaveTrackChanges współdzieli istniejący compiled track patch path z object,
  road, arc i terrain edits. Nie dodano alternatywnego runtime środowiska.
  Niezapisane formy nie są tracone podczas wyboru object/course/road context;
  jeden aktywny track form draft naraz, z możliwością discard atmosphere.
- Native service test sprawdza przesunięcie kilku cues przez ich stare granice,
  exact RGB readback, invalid ranges/order/modes, preserved unrelated bytes,
  source preservation, build i Undo/Redo. Form test nazywa wszystkie 9 slots.
- Actual PAL UI headless: zmiana Fog #123456, unsaved context guard, Save,
  native bank readback i publish PASS, zero pageerrors. Normal race opt-in
  scenery-game test rozszerzono o zmienione kolory wszystkich cues: game load
  i race timer 20 PASS na classic/modern. Modern capture:
  build/reports/editor/atmosphere-modern.ppm; magenta sky gradient jest
  widoczny w faktycznie renderowanym kadrze, nie tylko w UI swatch.
- Native CLI rebuilt/staged; full npm test save_format PASS oraz 158 JS tests:
  152 PASS, 6 optional skips. T11 pozostaje częściowy: custom fog density,
  sky image/palettes, timeline add/delete, loop length/sky row editing,
  tunnel/event lighting i zgodność preview authoring jeszcze nie są gotowe.
- Kontrolny normal-game capture bez moda, z tym samym race/timer/renderer:
  build/reports/editor/atmosphere-original.ppm. Obejrzano oba kadry: zmienił
  się gradient nieba, a ciemny startup frame występuje także w oryginale.
  Ta obserwacja nie jest oceną pełnego wyścigu ani wszystkich palette variants.

### Atmosfera: kolory pięciu palet nieba

- Native scene metadata odczytuje pięć 16-color RGB palettes z block 1. Edycje
  palette.<index>.color_<slot> zmieniają dokładnie trzy bajty, bez zmian innych
  kolorów, scriptu lub trailing bytes. RGB/index bounds waliduje C patcher.
- Palety są pod Sky image palettes we wspólnym Atmosphere form. Dyskretny
  pasek pokazuje ich kolory; wszystkie 80 pól ma label i native color control.
  Palette i cue changes mają oddzielne typed IDs, ale zapisują się jako jedna
  operacja Undo. Nie dodano kolejnego runtime ani innej ścieżki publikacji.
- Native service tests: 3 PASS, w tym pierwszy/ostatni palette slot, concurrent
  cue/palette edits, exact preserved bytes, bad indices/colors/types, build,
  original source preservation i Undo/Redo. Full npm test: save_format PASS;
  159 JS tests, 153 PASS, 6 optional skips, zero failures.
- Actual headless PAL UI: cue color + Palette 1/Color 2, native readback obu,
  unsaved form guard i publish PASS, zero pageerrors. Normal race load PASS
  na classic/modern; capture atmosphere-palettes-modern.ppm obejrzano:
  oryginalne cloud pixels przyjmują green palette przy odrębnym magenta
  gradient z cue colors. Jest to rzeczywisty frame z gry.
- Opt-in scenery-game gate sprawdza też green pixels w górnej części modern
  native capture. To test zastosowania palette, nie ocena jakości pełnego skyboxu.
- Native CLI rebuilt/staged. Original indexed sky nadal ma 16-color palettes;
  PNG/hi-res sky image replacement, header/timeline add/delete, fog density
  i tunnel/event editing nadal pozostają osobnymi nieukończonymi wymaganiami.

### Atmosfera: loop length i sky texture-row pair

- Header changes mają typed environment.0 IDs w tym samym track patch path:
  positive int32 loop length i sky row 0–3. Walidacja całego końcowego scriptu
  pozwala razem wydłużyć loop i przesunąć cue poza poprzedni zakres; błędne
  final ordering/ranges nie zapisują output ani nie zmieniają projektu.
- Loop & sky layout pod details we wspólnym Atmosphere form. Cue time limits
  aktualizują się przy zmianie długości i wracają do original przy discard.
  Global click handler błędnie preventDefault dla type=reset: poprawiono,
  przywracając natywny reset w Atmosphere oraz Audio.
- Native service/header tests sprawdzają extend+cue move, exact header bytes,
  reserved payload preservation, invalid length/row/index, build i Undo.
  Environment focused 4 PASS. Full npm test save_format PASS; 160 JS tests:
  154 PASS, 6 optional skips, zero failures.
- Actual PAL headless UI: length 40000, row 1, mixed cue/palette changes,
  native readback i publish PASS; discard restores length 28800 + max 28799.
  Osobny actual Audio UI test: discard gain 25 → 100 oraz późniejszy
  trim/export/import/publish/reset PASS. Zero pageerrors w obu przebiegach.
- Normal race: updated length/row + scenery/colors/palettes load and stop at
  race timer 20 PASS na classic/modern. Green-sky-pixels gate PASS. Capture
  atmosphere-layout-modern.ppm obejrzano: inne rozmieszczenie oryginalnych
  clouds niż w atmosphere-palettes-modern.ppm, zgodne ze zmienionym row pair.
  Nie testowano pełnego zawinięcia długiej pętli ani wszystkich row/region pairs.
- Native CLI rebuilt/staged; gameplay loader nie wymagał nowej implementacji.
  Timeline add/delete, new sky images, fog density oraz tunnel/events są nadal
  nieukończonymi wymaganiami, więc pełny T11 pozostaje otwarty.

### Compiled GLB geometry i rzeczywisty car-part flow

- Zintegrowano MIT cgltf pinned 85cd62382dfea638278962690cf515023f33ed00
  (external/cgltf), bez Python i bez nowego standalone executable. Existing
  rage-mesh-obj ma import-glb/export-glb; license w resources/licenses jest
  uwzględniany przez istniejący packaging resources path.
- Export generuje bounded self-contained GLB dla jednego native mesh part.
  Consecutive material runs zachowują triangle order. POSITION/UV/linear RGBA
  i pełny material word (extras + name fallback) są zachowane. NORMAL jest
  normalized, _RR_NORMAL zachowuje native fixed-point magnitude dla no-op.
- Import używa bounded cgltf metadata allocator i validate, jednego aktywnego
  mesh node, world transform/inverse transpose/mirrored winding. Odrzuca
  external buffers, multiple mesh nodes, sparse/unsupported attributes,
  extensions/skins/animation i nieobsługiwane material/texture edits. Nie
  flattenuje całego auta w body ani nie odczytuje URI plików poza kontenerem.
- Existing Car editor/preload/IPC: Export GLB geometry, Import replacement
  (GLB/OBJ/RRMESH), validation, immutable project mesh i Save & apply. UI
  jawnie mówi geometry-only/modern oraz zachowanie istniejących textures.
  Complete car-set worker przyjmuje też geometry GLB dla osobnego part file;
  domyślny cały export nadal OBJ/PNG.
- Native converter regression: triangle order, full word, RGBA/UV, fixed
  normals, renamed materials with extras, negative nonuniform scale +
  translation, bad buffers/nodes/mapping/extensions/singular matrix/counts,
  exclusive output + source intact PASS. Existing compiled mesh_obj PASS.
- Real PAL body 10/0: export/import + native preview wykazały identyczne dane
  per-corner positions/normals/material/UV dla 2430 triangle corners; publish
  PASS. Actual app.js + native service headless Export GLB → Import part →
  Save & apply PASS, zero pageerrors. Native dialogs/Electron IPC transport
  oraz Blender są nadal osobnymi bramkami, których ten bridge nie udaje.
- Normal modern race opt-in glb-game test PASS: loader log potwierdza override
  car.player.10.part.0 i scene 12 timer 20; authored cars wyłączone, isolated
  portable storage, read-only PAL source. Nie deklarujemy zmienionej geometrii
  w classic ani physics/collision z samego GLB.
- Full npm test przed nowym opt-in gate: 161 JS tests, 155 PASS, 6 optional skips;
  game gate wykonany osobno PASS. Native worker rebuilt/staged. Nie stwierdzamy
  C03/C08 completion: full car GLB roles/pivots/images/mapping wizard, standard
  unit conversion, unknown extras preservation, external .gltf, native image
  import i manual Blender/all-platform coverage pozostają wymagane.
- Final npm test po dodaniu opt-in gate i unsupported material/UV/normal
  cases: 162 JS tests, 155 PASS, 7 optional skips, zero failures; save_format
  PASS. glb-game uruchomiony osobno PASS. Car-part GLB nie zastępuje istniejącej
  Python bank-to-glTF ścieżki bez pełnej compiled behavior/coverage parity.

### GLB części auta: embedded PNG i atomic geometry+image import

- Existing compiled converter opcjonalnie dokłada PNG do GLB BIN, referenced
  przez ordinary glTF images/textures/baseColorTexture. Deduplication jest po
  native source ID; geometry material words i native shader semantics pozostają.
  Export single part ma jeden GLB + README, bez osobnych wymaganych PNG.
- Import wyciąga bounded PNG buffer views, nie otwiera image URI. Waliduje
  signature/layout/dimensions/size, źródła i conflicting images przed output.
  Native PNG decoder w service sprawdza całe obrazy przed project mutation.
  Glass/untextured surfaces i unsupported texture maps/UV transforms nie są
  milcząco traktowane jako zwykłe body texture. Filtering pozostaje game-owned.
- Native source → page/clut → original material slot mapping łączy PNG z
  odpowiednimi semantic texture IDs. Shared-surface conflicts są jawne.
  Geometry + immutable PNG refs zapisują się jedną Project.edit operation;
  undo/redo odtwarza oba. Invalid PNG nie zmienia revision ani previous changes.
- CLI tests: exact embedded PNG bytes/source mapping, geometry unchanged,
  existing output preservation, URI rejection, conflicting PNG source, no
  partial files PASS. Service test: compiled car material table, one geometry
  + texture history transaction, bad decoded PNG leaves project intact PASS.
- Actual PAL body 10/0: edited embedded PNG read back przez actual preview,
  per-corner geometry/normal/material/UV unchanged dla 2430 corners; publish
  PASS. Actual headless app.js Export GLB → Import → Save & apply PASS, zero
  pageerrors. Normal modern race test PASS: car mesh override oraz native
  texture override z edited 1×1 PNG, scene 12 timer 20, isolated source/state.
- Full npm test: save_format PASS; 164 JS tests, 157 PASS, 7 optional skips.
  Final focused GLB tests 3 PASS; native worker rebuilt/staged. Blender
  import/export, complete-car single GLB, roles/pivots, material wizard,
  shader-property mapping, all-platform coverage i standard units nadal
  pozostają do wykonania; nie deklarujemy pełnego C03/C08 completion.

### Complete-car GLB export in the real editor

- Existing Export car set now also produces `car.glb`: embedded base-color PNGs,
  three role-tagged meshes, body/rear-wheel nodes and two shared front-wheel
  instances. Positions/scales come from the same compiled disc layout decoder
  used by the whole-car preview; wheels are not flattened into the body.
- Compiled bounded GLB assembly remaps buffer views, accessors, material/image/
  texture/sampler references, carries source region/slot and part IDs, validates
  the final GLB and writes exclusively. Temporary part exports are removed.
- Complete OBJ/PNG import remains available. README explicitly states that
  complete GLB re-import is not implemented; this is export progress, not full
  interchange completion. No mock or additional UI was introduced.
- Existing native-backed car-set regression now checks the assembled hierarchy,
  shared wheel mesh, native part IDs, embedded PNG bytes and remapped references.
  Focused car-set/GLB tests: 4 PASS. Full npm suite: 157 PASS, 7 optional skips,
  0 failures (164 tests), plus save-format native checks.
- Actual read-only PAL source exports: player bank10/body0, 685088-byte GLB,
  10 embedded images, front pivots ±88/0/296; rival bank88/body0, 609012-byte
  GLB, 5 embedded images, front pivots ±120/0/368. Both validate through cgltf
  and export via LauncherService in private temporary projects, without a
  visible game window. Blender verification and complete import remain open.

### Complete-car embedded GLB import

- The real car screen now offers Import edited GLB beside Export for editing.
  OBJ set-folder import stays under advanced options; no separate tool or mock.
- Compiled `separate-glb` validates source region/slot/archive fingerprint,
  part and instance custom properties, active scene and supported static layout.
  It splits roles without flattening the car. Node world transforms are mapped
  back through the actual disc pivots; common local wheel scale/transform edits
  are baked into the shared wheel mesh, including normal transforms.
- Missing role IDs, duplicate/missing instances, incompatible shared-wheel edits,
  different wheel meshes, unsupported extensions/animation and wrong source are
  errors before project changes. Lost-property remapping wizard remains open.
- Complete and per-part GLB set import accepts embedded PNGs, resolves source
  IDs through original page/CLUT bindings and validates native PNG decoding.
  Conflicting images for one shared material are rejected. Three mesh refs and
  every image ref commit as one history operation; source and old project stay
  unchanged on failed validation.
- Regression: complete round trip; edited body vertex; both shared wheel scales
  with inverse-transpose normals; one-step undo; wrong slot/fingerprint, missing
  role metadata, incompatible wheel transforms, singular matrix and NaN geometry.
- Actual PAL player 10/0 and rival 88/0 whole-car previews preserve every corner's
  nine geometry/material/UV fields (14718 and 14385 corners), publish and undo.
  Headless actual app.js basic Export → Import GLB → Save & apply PASS, no JS
  errors (native dialogs supplied by the test bridge).
- Upgraded opt-in normal-race GLB regression imports all three parts and edited
  embedded PNGs. Real headless modern GP reaches scene12/timer20 and logs all
  three runtime mesh overrides plus 1×1 texture overrides: PASS (~29 sec).
  This is loaded-runtime evidence, not Blender or all-platform visual parity.
- Full editor suite before final fingerprint validation: 157 PASS, 7 optional
  skips, 0 failures; final build/test result recorded below. Blender executable
  was not found locally; Blender round trip, external .gltf, material remapping,
  full shader property mapping and standard physical units remain unverified.
- Final native rebuild/stage and fingerprint checks: full suite 157 PASS / 7
  optional skips / 0 failures; normal headless race PASS (~30 sec); actual UI
  export/import/publish PASS. Source provenance is also stored on the Car node
  for editors that discard document extras; final focused tests 5 PASS including
  node-property-only provenance. No published release or source asset mutation.

### Recover missing car part/material mappings in the actual editor

- Import edited GLB automatically opens a real assignment form when source,
  part/instance or material mappings are missing/incompatible. Target car/region
  is explicit. Every active mesh node receives one unique role and every used
  material receives an original material word. This covers static complete cars
  with the existing four-instance layout; arbitrary assembly/LOD remains open.
- Compiled inspect/map commands reuse the existing GLB worker. They validate a
  bounded self-contained static scene, report active mesh/material names, retain
  full 32-bit words and unknown JSON extras while assigning metadata, and leave
  geometry/BIN/UV/color/image bytes unchanged. No new parser in renderer JS.
- Material options are read from compiled original GLB exports: the existing
  mesh summary only contains low 16 bits and must not supply full-word options.
  Actual PAL UI validation exposed this and the corrected path preserves flags.
- A private source copy and opaque pending token isolate the staged candidate;
  one pending import per service. Duplicate roles, unsupported target materials,
  invalid tokens and stale revisions are rejected. Cancellation cleans staging;
  successful mapping still passes complete-car geometry/PNG/binding validation
  and saves as one history operation. Failed validation leaves edits intact.
- Regression tests: lost all node/mesh/material/document extras, renamed
  materials, complete recovery, unchanged source file/project on failed maps,
  cancellation cleanup, expired token, stale project. Native mapping test covers
  full 0x12340003 words, byte-identical decoded geometry, unknown metadata,
  exclusive outputs, duplicate/out-of-range mapping rejection. Form test checks
  required named assignments and imported-name escaping.
- Actual headless app.js + PAL service Export → erase metadata → form assignments
  → Import → Save & apply PASS. The test bridge supplies native file picker
  results; actual form and service/codecs run. Normal hidden modern GP after
  recovered complete-car mappings loads all 3 mesh overrides and edited 1×1
  embedded PNGs, preserves every preview corner and stops scene12/timer20 PASS.
- Full npm suite: 166 tests, 159 PASS, 7 optional skips, 0 failures; native
  save-format checks PASS. Focused native/service/form tests: 7 PASS. No visible
  game windows, no source mutations, no release publication.
- This form does not yet provide candidate 3D selection or material thumbnails;
  lost-property recovery for separate single-part imports, external .gltf,
  Blender proof, all-platform parity, full shader/property/unknown-extras project
  roundtrip and standard physical unit conversion remain open. Full goal stays
  active; the broader car/track/interface/media editor is not declared complete.

### Actual part inspection and target-texture thumbnails in import mapping

- Mapping form now offers View part for every active imported node, with embedded
  3D orbit/zoom/front/side/rear/top/wireframe controls. The existing WebGL preview
  mounts inside the form; role/material selections remain unchanged. Closing the
  dialog disposes its embedded GPU resources. It does not open another window.
- Compiled GLB metadata worker can select a node into a private inspection GLB,
  retaining its world matrix and the original BIN. Synthetic per-material source
  slots let the existing strict GLB→RRMESH/PNG importer inspect a file whose game
  material IDs were lost. Finite geometry/normals/images remain validated; no JS
  binary parser or alternative renderer was added. This view precedes the chosen
  game-material assignments and says so explicitly.
- Target choices distinguish texture number, glass/inner surface and owning part
  while retaining the complete native material word. View target texture uses the
  existing disc palette/texture decoder plus current project/selected package
  overrides. Native PNG validation runs before display. Glass and untextured
  surfaces display colour swatches. Cache deduplicates variants with the same
  part/surface; changing selection clears stale images, and late results cannot
  overwrite a newer selection.
- Service regression: original and mirrored imported nodes, exact embedded PNG,
  cached preview, valid target image, invalid node/material rejection, unchanged
  project revision after every inspection. Form tests retain all required
  assignments and check preview/thumbnail controls. Existing import/undo/failure
  regression still passes after inspecting the candidate.
- Actual PAL headless app.js test: Export → remove metadata → set Body role →
  inspect body and front wheel → inspect actual target PNG → assign all roles/
  materials → Import → Save & apply PASS, zero page errors. Repeated with live
  backend state notifications: PASS; selected role and form survive updates.
  Raster captures inspected: /tmp/rr-import-body-preview.png and
  /tmp/rr-import-target-texture.png. They are test artifacts, not bundled assets.
- Full npm: 166 JS tests, 159 PASS, 7 optional skips, 0 failures; native save-format
  checks PASS. Focused native/service/form suite 7 PASS. No game runtime changed
  in this step; the existing recovered-GLB normal-race gate remains applicable.
- Remaining interchange gaps include Blender proof, external .gltf, source-file
  retention for unknown-extra roundtrip, complete shader mapping, physical units,
  arbitrary assemblies/LOD and single-part lost-property recovery. Track/media/
  interface acceptance scope remains part of the full goal.

### Closing an active car-import operation

- Fixed closing the mapping dialog while part/target-texture inspection was
  active: the old cancel path tried to acquire a second job lock and could leave
  the private candidate behind. Editor operations now carry an optional owner
  and a completion promise; the import cancels only its owned controller, waits
  for the operation (including child termination/final cleanup), then removes
  its private staging. Unrelated editor work is not aborted.
- The candidate is marked cancelled before waiting, blocking subsequent mapping
  or inspection requests. Mapping and complete-car import carry the same owner;
  cancellation is checked before the synchronous project history commit. Empty
  or expired cancellation handles are harmless. Source files are unchanged.
- Regression with a real running Node child checks its PID no longer exists
  before cancellation returns, job lock is released and staging removed. Separate
  queued-preview cancellation test covers close during initial state emission.
  An unrelated live job keeps its signal and completes normally.
- Native-backed car-set regression closes during mapping, complete import and
  target texture jobs, then checks revision/changes and cleanup. A final test
  cancels after the first immutable file reference is staged and confirms no
  project commit occurs; retry/history data remain intact.
- Actual PAL headless app.js/live backend-state flow: close during part preview,
  wait for cancellation, reopen the same edited GLB, inspect parts/target texture,
  map/import/publish PASS. No page errors or visible game window; cancelled
  candidate and busy lock cleared, project revision unchanged after close.
- Full npm suite: 168 tests, 161 PASS, 7 optional skips, 0 failures; native
  save-format checks PASS. Focused cancellation + car-set tests 11 PASS; final
  late-commit cancellation case PASS in the native-backed car-set test. No game
  runtime or release changed. Broader editor acceptance requirements remain open.

### External glTF import through the existing compiled worker

- Complete-car picker and individual-part picker accept `.gltf` beside GLB.
  Same car/project/undo pipeline; folder car sets can also reference glTF parts.
  The editor normalises into a private GLB before geometry/image validation or
  missing-property mapping. Source JSON/BIN/PNG stay read-only.
- Compiled pack-gltf uses cgltf + existing yyjson; merges up to 64 external BINs
  with four-byte alignment, remaps buffer-view offsets and embeds external PNGs.
  It preserves scene hierarchy, accessor/material references and JSON extras.
  Validates original declared bounds before merging (otherwise invalid old views
  could cross into the next merged buffer), then validates the resulting GLB.
- Bounded source JSON (8 MiB), total GLB (128 MiB), PNG inputs (32 MiB); exclusive
  output. Asset paths stay under selected glTF directory, decode URI percent
  bytes/UTF-8, allow ./ and nested directories; reject parent/absolute/remote/data
  paths, missing files, links/reparse points and non-file sidecars. Missing assets
  are named in errors. JPEG/embedded data-URI support remains open.
- Native test with two buffers (unaligned first buffer), encoded Unicode/space
  BIN and PNG paths: exact native geometry/full words/RGBA/UV/normals and exact
  PNG bytes, preserved unknown extras, output exclusivity and source integrity.
  Tests reject escapes/encoded escapes, remote/absolute/backslash/NUL/bad-percent
  paths, symlink sidecar, original-bound mismatch and truncated BIN.
- Service tests: whole-car external BIN/PNG round trip, independent part glTF
  import, same one-operation geometry/image history. Pending glTF candidate stays
  unchanged after external BIN is overwritten following file selection.
- Actual PAL headless app.js with live state: external glTF + URI-encoded BIN +
  separate PNGs → missing-ID recovery → body/wheel preview → target texture →
  Import → Save & apply PASS, no page errors. Native pickers supplied by bridge.
- Normal headless modern GP gate now covers external glTF recovered mappings:
  all three mesh overrides plus edited 1×1 PNGs loaded; every whole-preview corner
  unchanged; scene12/timer20 PASS (~30 sec). Full npm 169 tests: 162 PASS,
  7 optional skips, 0 failures; native save-format PASS. Final model-file stat
  guard rerun in the focused suite; no game/runtime source or release changed.
- Still open: Blender proof, data URI/JPEG variations, broader assemblies/LOD,
  complete shader/unknown-extra project roundtrip, physical units, single-part
  missing-ID recovery and broader track/interface/media acceptance scope.

### Self-contained glTF data URI buffers and PNGs

- Existing compiled glTF normaliser now accepts canonical base64 buffer URIs
  (`application/octet-stream`, `application/gltf-buffer`) and PNG image URIs
  (`image/png`). They join the same bounded, validated private GLB path as local
  sidecars; no separate UI, decoder service, Python or extra dependency.
- Decoder validates alphabet, quartet length, padding location and unused pad
  bits before returning bytes; checks decoded size before allocation. MIME types
  are explicit, whitespace/non-base64 data are errors. Literal NUL in URI JSON
  strings is rejected instead of accepting a truncated C-string prefix.
- JSON limit remains 8 MiB including encoded payloads; normalised GLB 128 MiB,
  image bytes 32 MiB. Larger models can use external BIN/PNG. Remote/parent/link
  restrictions remain; no external request is made for data URIs.
- Compiled regression: all three padding-length cases, exact decoded native
  geometry/full words/RGBA/UV/normals and PNG bytes; malformed quartet/padding/
  alphabet/pad bits/whitespace/MIME/NUL and malformed image payloads rejected
  without output files. Whole-car and single-part service round trips PASS;
  invalid inline PNG leaves revision and changes untouched, original JSON intact.
- Actual PAL headless app.js/live state: data-URI glTF → missing-ID assignment →
  body/wheel inspection → target texture → Import → Save & apply PASS, zero page
  errors. Native file selection is supplied by the test bridge.
- Normal hidden modern GP with recovered data-URI glTF: all three mesh overrides
  and edited 1×1 PNGs loaded; whole-preview geometry/material/UV fields unchanged;
  scene12/timer20 PASS (~31 sec). Full npm 170 tests: 163 PASS, 7 optional skips,
  0 failures; native save-format checks PASS. Focused compiled/service suite
  7 PASS. JPEG, Blender proof and the remaining full editor scope stay open.

### JPEG model textures using the already-bundled SDL codec

- Normalisation now handles GLB as well as external/inline glTF; local JPEG,
  image/jpeg data URI and JPEG buffer-view images become embedded PNGs before
  inspection/import. All whole/part/set import entry points use this same path.
  PNG/RRMESH semantics, scene hierarchy, full material words and source metadata
  remain unchanged; original JPEG is read-only.
- Uses the existing SDL MJPG→RGBA decoder and PNG writer, without SDL_Init,
  a new dependency, an interpreter, an external converter or Psyz changes.
  The existing mesh worker links the same SDL target as other bundled tools.
- Bounded JPEG marker/SOF inspection checks supported 8-bit dimensions/components
  before pixel allocation (≤4096×4096, encoded/converted PNG ≤32 MiB). Requires
  EOI and rejects malformed/truncated segments; decoded pixels become opaque
  RGBA. Data-URI MIME/signature mismatches are rejected. EXIF orientation/ICC
  metadata are not retained/reinterpreted by this texture conversion.
- Cross-platform regression fixture red.jpg is a self-created 1×1 red image;
  its JPEG bytes are checked in, so tests/runtime do not need its authoring tool.
  An initially invalid fixture was caught and replaced with a valid JPEG before
  any passing claim. Native test covers local/data URI/GLB conversion, exact
  geometry, dimensions, red RGB and alpha255, original source integrity and
  truncated/header-only/oversized-SOF rejection without outputs.
- Whole-car and single-part service tests: converted PNG and all meshes saved
  with the existing single history action; undo restores previous references.
  Existing source/binding/invalid-PNG/cancellation cases remain passing.
- Actual PAL headless app.js/live state JPEG glTF → missing-ID mapping →
  body/wheel preview → target texture → Import → Save & apply PASS, no errors.
  Normal hidden modern GP loads all three mesh overrides and converted 1×1 PNGs;
  whole preview geometry/material/UV unchanged, scene12/timer20 PASS (~31 sec).
- Full npm: 171 JS tests, 164 PASS, 7 optional skips, 0 failures; native
  save-format PASS. Focused native/service tests 8 PASS before final oversized
  case; final worker rebuild/stage and native regression recorded below.
- Blender proof, broad assemblies/LOD, physical units, full shader/unknown-extra
  project roundtrip and remaining track/interface/media acceptance stay open.
  No release or game-runtime implementation changed in this step.
- Final rebuilt/staged worker: focused native/service suite 8 PASS, including
  oversized JPEG dimension rejection. Linux POSIX declarations are explicitly
  enabled before headers; SDL linkage uses the existing bundled target. No new
  external/runtime image-conversion requirement was introduced.

### Rival racing-line hint authoring in Tracks

- Added a named Forward/Reverse → Stretch editor for runtime sideways nudges:
  lateral min/max and signed per-tick adjustment are basic fields; progress
  boundaries are advanced. No anonymous archive/record browser or mock.
- C decoder/edit path matches TrackEventData using compile-time offset/count/
  size assertions (132-byte offset, two 30-entry series, 12-byte records). Reads
  unaligned signed fields, retains sentinel/unused entries/reserved and all other
  bank bytes. Batch edits validate final ordered non-overlapping ranges after
  the complete change, allowing min/max to move together. Equal ranges are
  valid retail no-ops. UI does not call historic 'height' fields actual height.
- Original PAL 24-bank survey: normal tables in both directions except bank104
  reverse has unsupported records. Its valid forward table remains available;
  reverse is explicit null/uneditable, not guessed or used to block other edits.
- Typed track.ai changes use existing composition/build/source identity and one
  project edit/undo. Native/service/form regression: reserved-byte preservation,
  final batch ranges, signed -32768 correction, series IDs, sentinel/out-of-range/
  overlap/overflow rejection without files, effective scene, undo and revision.
- Track input protection now includes AI forms. Other dirty forms cannot be
  submitted over them; live state updates do not redraw dirty track drafts.
- Actual PAL headless app.js/live state invalid range → error → draft still300,
  unchanged revision → correction and Save → effective hint → publish PASS.
  Full npm: 172 JS tests, 165 PASS, 7 optional skips, 0 failures; native
  save-format checks PASS. Focused AI/track workspace suite 10 PASS.
- Updated existing normal-race gate edits hint0 adjustment to0 along with scenery/
  atmosphere, verifies exact built event bytes and loads bank88 in both classic
  and modern without invalid-pack diagnostics, reaching scene12/timer20 PASS
  (~54 sec). This proves bank activation, not full-lap AI trajectory behaviour.
- Still open: full-lap T09 behaviour, hint insertion/removal, start grids, speed
  keys, cameras/intro/replay/events, driveable mesh bake and broader editor scope.

### Rival pace-key authoring

- Added named Forward/Reverse → Pace key forms: four rival target values are
  basic fields; progress and slide pitch are advanced. UI explains these are
  internal acceleration-limit tuning values, not speedometer units, and later
  rivals derive their target from fourth contender's target.
- Same C track-event codec owns bounded 2×48×12-byte speed records. Offset1140,
  count and size asserted against actual TrackEventData. Reads signed targets,
  progress and wrapped pitch; edits preserve all other bytes/sentinels/tails.
  Does not sort or reject local reversals, and supports all48 active slots.
  Sentinel -1 cannot be introduced through editing an active key.
- Original PAL24-bank survey matches runtime tables; e.g bank96 reverse uses
  all48 slots. Synthetic native/service/form regression covers local reversal,
  full48, signed boundary values, overflow/sentinel/nonexistent-row rejection,
  exact changed-byte inventory, effective direction/table and one-step undo.
- Actual PAL headless app.js/live state: Forward key1 rival1 target143→150 →
  save to project → effective scene150 → publish PASS, no JS errors. Test picker
  flow/controls use real application and native service, no new mock layout.
- Existing normal hidden race gate verifies built event target150 alongside AI
  hint/scenery/environment edits, then loads bank88 on classic and modern,
  scene12/timer20 without invalid-pack diagnostics PASS (~55 sec). This proves
  activation, not full-lap AI trajectories or observed acceleration changes.
- Full npm173 JS tests:166 PASS,7 optional skips,0 failures; native save-format
  PASS. Dedicated hint+pace codec/service/form tests2 PASS. Cameras, start grids,
  key insertion/removal, full-lap T09 and broader editor requirements remain open.

### Player/rival starting-placement authoring

- Added named Race → Starting placements → Forward/Reverse → Player/Rival forms.
  X/Z are basic fields; road reference and read-only progress/activity seed are
  advanced. Inactive rows are explicit. Height uses runtime road contact; an
  off-route position triggers the original road-point fallback, stated in UI.
- C codec covers fixed 2×12×12-byte TrackRivalStart records at offset852, with
  runtime layout assertions. Signed 32-bit X/Z and bounded signed16 road indices;
  no seed/activity write API. Edits retain all other fields and bytes. Flattened
  start IDs use the same typed delta/compose/build/undo path as other track data.
- Runtime review: start.activeFlag is also passed to SeedCarLapProgress; it is
  not merely a boolean. Keeping it unchanged avoids activating Class6 slots that
  have no model. The player start entry is included, not mistaken for rival1.
- Native/service/form regression: exact changed bytes including INT32 extremes,
  forward/reverse IDs, out-of-road/negative point and row overflow rejection,
  explicit rejection of seed/active writes, inactive row move stays -1, undo and
  source integrity. UI exposes no mutable seed field.
- Actual PAL Class6 bank128: inactive rival slot5 position edit/publish/undo
  preserves every one of the 24 source flags PASS. Actual headless app.js/live
  state Player X46828→46829 → effective scene/same seed → publish PASS, no errors.
- Existing hidden normal-race gate checks exact starting X+1 and unchanged seed
  bytes in the built bank, loads bank88 on classic and modern without invalid
  pack diagnostics and reaches scene12/timer20 PASS (~54 sec). This is activation
  evidence, not a full-lap or every grid-placement geometry verification.
- Full npm174 JS tests:167 PASS,7 optional skips,0 failures; native save-format
  PASS. Hint/pace/start native/service/form suite3 PASS. Activity/seed authoring,
  full-lap AI, cameras and wider editor acceptance requirements remain open.

### Track-camera list authoring

- Added named Track cameras → GP forward/GP reverse/Default → View forms.
  Section/duration are basic; mode and mode-specific world/orientation/offset
  parameters are advanced. UI distinguishes rendering sections from road points,
  native angles/world units, runtime duration fallback and the separate GP intro.
- C parser matches the runtime table (12-byte header, 36-byte nodes, assertions),
  requires aligned in-bounds list offsets, valid0..5 modes and terminating -1.
  No sort assumption. Physical node IDs preserve aliases/shared tails between
  lists; Used by identifies all referencing tables. Editing cannot target header,
  interior, sentinel or unreferenced bytes, or introduce a sentinel/mode overflow.
- Typed camera project deltas share existing composition/build/undo. Node values
  retain full signed32 bits; mode/section retain signed16 layout. Reserved data,
  list offsets/sentinels/tails and other bank blocks survive field edits exactly.
  No game camera code or global runtime state was replaced.
- Native/service/form regression: shared forward/reverse list alias, independent
  default/path list, exact changed bytes, invalid IDs/modes/sections/overflow
  rejected without output, effective shared change and one-step undo.
- Actual PAL headless app.js/live state GP forward view1 world Y5922→5923 → save
  → effective scene → publish PASS, no errors. Normal hidden race gate verifies
  edited camera bytes alongside prior track edits and loads bank88 successfully
  on classic/modern, scene12/timer20 PASS (~54 sec).
- Full npm175 JS tests:168 PASS,7 optional skips,0 failures; native save-format
  PASS. Focused camera+AI suite4 PASS. The runtime gate proves camera-table
  activation, not observed replay-cut framing/sky or isolated GP intro geometry.
  Those T10 gates, camera insertion/removal/preview, GP intro event path and the
  remaining editor scope stay open.

### Race starting-camera authoring (real editor)

- Added Race → Starting camera forms to the existing track workspace, with
  forward/reverse keys, world X/Y/Z and expandable simulation-tick timing.
  This edits RunRaceIntroCamera before a single race. Correction to the preceding
  entry: this event script is not the Grand Prix announcement background camera;
  the announcement path remains a separate investigation.
- Compiled codec asserts the runtime event-offset/header/key layout, bounds the
  relative script offsets and walks each direction to its terminal key using
  runtime mode/duration validation. Physical IDs preserve shared script tails.
  First-key indices, terminal modes/triggers, unused keys, offsets and other
  blocks are retained. No insertion/removal or additional runtime globals.
  Terminal transition-frame edits are rejected: advancing a terminal key could
  enter unused records. Other transition frames retain signed16 semantics rather
  than imposing an invented timeline order; frames outside 2..89 do not trigger
  a transition in the current runtime. Interpolation retains signed16 deltas.
- Connected typed project edits, effective scene, undo, native IPC and existing
  immutable mod composition. Invalid edits produce neither output nor project
  revision changes. Native/service/form tests cover shared forward/reverse keys,
  byte-exact changes, invalid IDs/ranges/durations/terminal triggers, malformed
  script offsets/headers/modes, unused keys and one-step undo.
- Actual PAL survey: all 48 direction scripts across 24 track banks readable.
  Headless actual app.js with live state + service: bank88 key1 Y5975→5976,
  submit, effective scene and published raw bank all agree. No visible window,
  source mutation or frontend errors. Temporary harness /tmp/rr-intro-ui.cjs.
- Full npm176 JS tests:169 PASS,7 optional skips,0 failures; native save-format
  PASS. Focused camera/starting-camera suite2 PASS after the terminal-trigger
  guard. Isolated normal race loads the edited event bank on classic and modern,
  scene12/timer20 PASS (~54 seconds), with exact published camera bytes checked.
  This is activation evidence, not visual camera-path/cut fidelity verification.
  Camera visual preview, full timeline authoring, Blender and platform gates and
  the broader editor acceptance criteria remain open.

### Track model GLB/glTF interchange

- Existing placed-object and terrain-model controls now import OBJ/GLB/glTF.
  GLB export is the primary model action; placed objects retain an OBJ export
  alternative. GLB embeds effective source/package/project PNG textures, with
  no external PNG files needed beside the exported model. Native bank exchange
  stays advanced. UI explains shared geometry/texture scope and modern-only
  appearance changes, separate from the driveable physics route.
- Reused the compiled static-part GLB codec and bounded glTF normalizer (local
  external/data-URI PNG/JPEG). Track codec keeps the full low16 material source;
  car bank modulo64/untextured rules remain unchanged. Compact mapping has an
  explicit 64 texture-source limit per imported/exported part, not a source-ID
  limit. Full material words, native normals/colors/UVs/triangle order remain
  preserved. Embedded images are decoded before project mutation.
- Replacement retains bank model order/count and original material slots; one
  project edit contains geometry plus all imported PNG references, with normal
  undo/build composition. Malformed input does not advance project revision.
  This remains single static mesh-node exchange; material mapping recovery,
  arbitrary PBR/animation, classic native geometry and road bake remain open.
- Native regression fixture distinguishes source3/source67/source131 (same car
  modulo64 result): three independent embedded/extracted images, byte-exact
  native geometry and existing-output protection. Existing car GLB/glTF tests
  remain green. UI assertions cover both GLB and OBJ action wiring.
- Actual PAL headless app.js/live-state/native-service test: click export model
  GLB, change node Y by4 renderer units, click import, effective preview Y+1 game
  unit, publish and undo PASS. No visible game/editor window or source changes.
  Temporary harness /tmp/rr-track-glb-ui.cjs; latest log rr-track-glb-ui3.log.
- Normal isolated classic/modern race gate imports the changed course model and
  embedded textures, checks effective preview displacement, rejects damaged GLB
  without revision change, then publishes and loads the mod without invalid
  track-mesh diagnostics (~54seconds PASS). Activation evidence does not prove
  every imported surface visually matches Blender. Expanded gate also exercises
  the same real model through data-URI glTF before activation.
- Final full npm177 JS tests:170 PASS,7 optional skips,0 failures; native
  save-format PASS. Expanded actual PAL GLB+glTF activation gate PASS (~54sec)
  on both renderers. Blender manual roundtrip and all-platform gates remain open.

### Direct track texture editing

- Added Textures actions to placed-model and terrain-cell controls (including
  model changes in the cell picker). Surface dialog shows effective PNG, pixel
  dimensions, disc/package/project origin, shared Used by model list and
  technical binding in details. Replace/export PNG/reset are connected to the
  real project; no whole-model export or sidecar is required for a texture edit.
- Compiled texture extraction and existing bank composition provide the same
  effective images as model preview/export. Usage is derived from effective
  indexed geometry, retaining full low16 track source identity and excluding
  untextured faces. Replacements require a used, replaceable source, regular
  non-symlink PNG ≤32MiB and ≤4096×4096, decoded before immutable storage/edit.
  Undo/reset/build preserve source and package contents. Cancellation before
  committing import/reset is checked. UI states modern-only scope.
- Fixed a real preview/build precedence discrepancy: project base track-surface
  PNG now replaces selected-package exact-variant texture bindings in the built
  manifest too. Explicit project variant changes retain priority. Reset restores
  the package variants, without changing unrelated surfaces or material settings.
  Native manifest/build regression verifies replacement/reset and library bytes.
- Tests cover shared indexed usage, full material IDs3/67, untextured/empty models,
  named/escaped controls, thumbnails, identity, revisions and disabled reset.
  Actual PAL headless app.js + live state + compiled service: Textures → Replace
  PNG1×1 → Export exact PNG → Publish → invalid PNG rejection with unchanged
  revision → Reset original → Undo restores replacement PASS. Initial temporary
  harness missed awaiting export completion; corrected harness uses the UI toast
  and reset state as completion evidence (/tmp/rr-track-texture-ui2.log).
- Expanded normal isolated race gate performs direct PNG replacement/export,
  checks effective origin/dimensions/content, rejects unknown surface and corrupt
  PNG with no revision mutation, and loads the build on classic/modern PASS
  (~54seconds). This is activation evidence; it does not verify every surface
  visually or add classic texture replacement support. Region/platform/manual
  usability and broader editor acceptance gates remain open.
- Final full npm180 JS tests:173 PASS,7 optional skips,0 failures; native
  save-format PASS. Shared usage collection scans each bank model once rather
  than once per surface; focused surface/build regression9 PASS after that change.
