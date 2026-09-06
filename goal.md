# Goal: ulepszone modele samochodów działające w grze

Pracuj autonomicznie na branchu `re-asset` w `/Users/hasik/Projects/rage-port`. **Celem jest pozostawienie na rano działającej lokalnej wersji gry z ulepszonymi modelami samochodów, domyślnie używanymi przez modern renderer.** Same pliki Blendera, eksporty lub plan integracji nie realizują celu.

Przeanalizuj modele i pipeline assetów, następnie przemodeluj samochody przez Blender MCP, zachowując rozpoznawalne sylwetki, proporcje i stylistykę Rage Racera. Popraw geometrię i detale bez zmieniania fizyki oraz kolizji. Dobre tekstury zachowaj; wymagające poprawy możesz przygotować przez imagegen.

**Najpierw przeprowadź jeden samochód przez cały proces: modelowanie → eksport → integracja → uruchomienie w grze → testy → potwierdzenie wizualne. Następnie kontynuuj z pozostałymi samochodami.** Nie odkładaj integracji na koniec. Priorytetem jest komplet samochodów; otoczeniem zajmij się dopiero po nich. Jeśli czas okaże się ograniczony, zmniejsz zakres detali, a nie zakres weryfikacji: pozostaw ukończone modele działające w grze, a dla nieukończonych zachowaj oryginały.

Samodzielnie ustal i obsłuż formaty, skalę, orientację, UV, materiały, koła oraz wszystkie wymagania renderera. Nowe modele mają ładować się automatycznie — bez ręcznego podmieniania plików lub dodatkowych poleceń użytkownika. Zachowaj działanie classic renderera i przestrzegaj kontraktu dystrybucji oraz ograniczeń z `AGENTS.md`. Blender służy do tworzenia assetów; nie może stać się zależnością uruchomieniową gry. Nie dodawaj Pythonowych narzędzi do projektu.

Dodaj testy walidujące assety, eksport/import i ładowanie. Uruchom testy regresji oraz sprawdź wydajność. **Każdy ukończony model obejrzyj zarówno w Blenderze, jak i w uruchomionej grze.** Wykonuj i analizuj porównawcze zrzuty przed/po z kilku kamer. Sprawdź samochód gracza oraz rywali, tekstury, koła, culling i artefakty. Zweryfikuj także przejścia między wyścigami i attract mode. Nie deklaruj sukcesu wyłącznie na podstawie kompilacji lub testów automatycznych.

Użytkownik idzie spać. Nie czekaj na akceptację kolejnych etapów; podejmuj rozsądne, odwracalne decyzje w tym zakresie i kontynuuj do osiągnięcia działającego rezultatu. Zachowuj istniejące zmiany i oryginalne dane gry. Commituj lokalnie spójne, zweryfikowane etapy. **Nie pushuj, nie publikuj, nie twórz PR.**

Zakończ z gotowym do uruchomienia lokalnym buildem używającym nowych modeli, zapisanymi źródłami Blendera i powtarzalnym eksportem. Zostaw krótki raport z listą samochodów rzeczywiście podmienionych i sprawdzonych w grze, wynikami testów, zrzutami oraz znanymi ograniczeniami.
