# Rage Racer Editor — klikalna makieta

[Otwórz index.html](index.html) w przeglądarce, również bez serwera i internetu.
Źródła HTML/CSS/JS są samodzielne; nie wymagają npm, Electron, płyty ani instalacji.
Wszystkie auta, tor, waveform i wartości są **ilustracyjne**. To nie nowe narzędzie
produkcyjne i nie podgląd prawdziwego modelu z gry.

## Co przejrzeć

1. **Cars → Look**: wybierz Body/Glass/Wheels w widoku lub na pasku. Kolor ma
   natychmiastowy efekt; Surface properties i Assembly są rozwijane.
2. **Driving / Shop**: gearbox/gears, price/unlock Class. Zmień cenę na 0,
   użyj Undo/Redo i Save & apply. Projects & mods pokazuje cały zapisany rezultat.
3. **Import model…**: docelowy wizard brakujących ról GLB, bez ręcznych
   identyfikatorów materiałów. „Accept demo import” zapisuje tylko symulowaną zmianę.
4. **Tracks → Scenery**: kliknij budynek, zmień X/Z. Road pokazuje route handles;
   przeciągnięcie zapisuje zmianę, bez udawania, że wygenerowano nową fizykę.
5. **Tracks → Sky & fog / Race**: rozdzielone podstawowe i zaawansowane dane.
   Forward/Reverse i Class są context selectors; Oval niedostępny w Class 1–2.
6. **Interface / Audio / Saves**: rozpoznawalne ekrany/cue/team zamiast listy
   uploadów tekstur. Audio preview nie emituje dźwięku w tej makiecie.
7. **Game / Projects & mods**: Save & apply łączy zapis i aktywację; Save project only jest opcjonalne; Test in game pokazuje
   docelowy selected-session flow, ale nie uruchamia gry ani zapisów na dysku.

Makieta używa jednego schematycznego auta dla sample list. Modelowanie, real
import, source loading, renderer switching i native validation są opisane w
[pełnym planie](../../editor/plan.md), nie udawane przez te ilustracje.
Przy 960 px inspector przechodzi pod viewport; final UX może używać drawer.

## Sprawdzone

Hidden Chromium: 7 sekcji, bez JS errors / horizontal overflow przy 1440×960 i
960×680; price edit/undo/redo/save/apply; model mapping demo/undo; route drag;
Oval restriction; reverse atmosphere. Obejrzane screenshoty, drobne overlap
captions poprawione. To weryfikacja makiety, nie testy edytora/gameplay.

[Podgląd samochodów](previews/cars.png) · [Podgląd torów](previews/tracks.png).
