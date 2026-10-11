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
