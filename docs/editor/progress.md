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
