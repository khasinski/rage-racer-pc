# Ulepszone samochody — lokalny build

Aktualizacja po ocenie wizualnej: mocniejsza poprawka obejmuje już całą flotę,
włącznie z tuningiem i rywalami. Wszystkie koła są zaokrąglone, nadkola mają
wewnętrzne osłony, a szyby, lakier, guma i felgi mają osobne materiały.
[Aktualny zakres i weryfikacja](car_rounded_fleet.md),
[galeria przed/po](../build/car-upgrade-evidence/rounded-fleet-gallery.html),
[nowa aplikacja](../build/fleet/Rage%20Racer.app).
Poniżej zachowano raport wcześniejszego, subtelniejszego etapu.

Gotowy lokalny build: [Rage Racer.app](../build/package-audit/Rage%20Racer.app).
Uruchom go dwuklikiem. Samochody są zintegrowane i obejrzane w modern rendererze.
Pakiet uruchomiony przez macOS bez sąsiedniego cache automatycznie wygenerował
zasoby w C z zapamiętanego CUE i przeszedł do attract mode.

## Zakres

- Wszystkie 13 aut bazowych: Erriso, Abeille, Pegase, Esperanza, Acceron,
  Bayonet, Hijack, Fatalita, Istante, Ghepardo, Vainqure, Bulshade i Squaldon.
- Wszystkie 19 dalszych wersji gracza: Erriso ×3, Abeille ×2, Pegase ×1,
  Esperanza ×4, Acceron ×3, Bayonet ×2, Hijack ×1, Fatalita ×2, Istante ×1.
- Wszystkie nadwozia rywali w 24 bankach wyścigów, włącznie z dodatkowymi
  Esperanzami i trzema grupami kompaktów w ich wersjach klasowych i kolorach.

Łącznie 60 eksportowanych siatek: 32 wersje gracza i 28 nadwozi rywali.
Źródła są w [assets/cars](../assets/cars): 59 plików Blendera, ponieważ
Erriso gracza i rywala współdzielą jeden plik. Koła, dalekie LOD, fizyka
i kolizje zachowują oryginalne dane. CMake automatycznie osadza nowe siatki;
tekstury pochodzą z wybranego obrazu gry. Blender nie jest potrzebny do grania.

## Weryfikacja

- 375 testów regresji przeszło po zakończeniu kompletu rywali.
- Po integracji wariantów gracza przeszło siedem testów assetów/pipeline'u,
  ładowanie wszystkich 19 banków, 19 prób ponownego wyścigu i odczyt wszystkich
  19 banków z wcześniej wygenerowanego cache.
- Każde nadwozie obejrzano w Blenderze i w uruchomionej grze. Ostatni etap
  obejmuje 38 porównań przód/tył, czyli 76 zrzutów dla wariantów gracza.
- Ponowny eksport zapisanych źródeł daje identyczne siatki po konwersji w C.
- Końcowe próbki CPU submission: przód 0,827–0,878 ms przed i
  0,946–1,015 ms po; pościg 0,247–0,282 ms przed i 0,356–0,436 ms po.
  Porównanie obejmuje także rywali; nie jest to pełny benchmark GPU/FPS.

Przykłady: [Esperanza — przód](../build/car-upgrade-evidence/esperanza-grade4-front-verified-comparison.png),
[Pegase — tył](../build/car-upgrade-evidence/pegase-grade1-chase-verified-comparison.png),
[Hijack — przód](../build/car-upgrade-evidence/hijack-grade1-front-verified-comparison.png).
Oryginały są po lewej. Szczegółowe wyniki i wcześniejsze zrzuty opisuje
[raport etapów](car_upgrade_status.md), a [instrukcja eksportu](car_asset_authoring.md)
opisuje powtarzalny proces tworzenia assetów.

## Uruchomienie i ograniczenia

- Sprawdzono start pakietu przez macOS, attract mode oraz wyścig z samym
  Track 01 BIN. Modern załadował ulepszone Erriso grade 3 i rywali; classic
  przeszedł ten sam scenariusz na oryginalnych danych. Zrzut attract mode:
  [czysty pakiet](../build/car-upgrade-evidence/fleet-attract-clean.png).
- Aplikacja ma wyłącznie systemowe zależności macOS. Obraz użytkownika
  skopiowano lokalnie do `build/launch-disc`, a zapamiętaną ścieżkę CUE
  ustawiono na tę kopię. Oryginału nie zmieniono. Poprzednia ścieżka jest
  w `build/car-upgrade-evidence/disc-cue-path-before-app-audit.txt`.
  Start z Downloads blokowało żądanie dostępu macOS; uprawnień systemowych
  nie zmieniano. Interaktywny wybór pliku nie został zautomatyzowany.
- Scenariusze startu w środku trasy ujawniają istniejący problem atlasu tekstur
  otoczenia. Występuje również przy wyłączonych nowych modelach samochodów.
  Obejrzany naturalny attract mode z czystego importu wyświetlał tunel poprawnie;
  nie oznacza to sprawdzenia całego otoczenia na wszystkich trasach.
- Testy uruchomieniowe dotyczą lokalnego macOS; Windows i Linux nie były tu
  uruchamiane. Nie jest to deklaracja gotowego wydania na wszystkie platformy.
