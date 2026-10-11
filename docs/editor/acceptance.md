# Macierz odbioru Rage Racer Editor

Plan testów, nie raport testów wykonanych. [Plan](plan.md) · [Audyt](audit.md).

## Bramki wspólne

| ID | Test / zestaw | Wynik wymagany |
| --- | --- | --- |
| S01 | Clean package, brak dev tools, double-click: macOS / Linux / Windows | Jedna aplikacja startuje; nie prosi o npm/CLI/Python; save editor bez płyty |
| S02 | CUE, Track 01 BIN, istniejący obsługiwany CHD; Unicode i spaces | Automatyczny import; rozpoznany region/revision; missing CD tracks uczciwie pokazane |
| S03 | PAL, NTSC-U, NTSC-J 1.0 i 1.1, drugi source o innym fingerprint | Nie mieszają names/spec/texture layout/cache; fingerprint nie opiera się wyłącznie na ścieżce |
| S04 | Cancel/failure podczas read/decode/copy/build | Previous source/project/active snapshot nadal działa; staging sprzątnięty |
| S05 | Asset coverage source-only i built-in enabled | 135 wpisów sklasyfikowane, CD/FMV dodatkowo; Origin/effective mają poprawne labels |
| S06 | No-op import→save→reload/build | Gameplay identyczne; unchanged source bytes/reserved bytes zachowane; brak całego archive w paczce |
| S07 | Save & apply / Save project only, game running, direct binary launch | Save & apply aktywuje na next session; draft-only nie jest active; direct game czyta tę samą wersję |
| S08 | Missing/invalid manifest, no mods, explicit empty/configured path | Deterministyczny precedence i diagnostic; brak „Applied” dla nieskutecznego moda |
| S09 | Region/revision mismatch w autodiscovered snapshot | Odrzucony cały niezgodny snapshot przed state mutation, visible error w editorze; original gra startuje |
| S10 | Crash/power loss: przed/po active pointer commit | Przy kolejnym uruchomieniu całkowity stary albo nowy build; nigdy częściowy mix |
| S11 | Disk full/access denied/staging alias/symlink/traversal/oversize | Named actionable error; original and previous build intact; no arbitrary file write |
| S12 | Imported legacy profile/packages/provider conflicts/dependencies | Zachowana metadata i nonconflicting changes; wybór human-readable provider; żadna zgubiona raw zmiana |
| S13 | Reopen/recovery/undo after import/reset/drag/paint/history trim | Jeden gest = jeden undo; redo poprawne; history references bez orphan deletion przed checkpoint |
| S14 | Automated desktop/game/native GPU captures | Ukryte okna, brak focus theft; manual Play świadomie widoczny; isolated profiles |

## Auta

| ID | Test / zestaw | Wynik wymagany |
| --- | --- | --- |
| C01 | Wszystkie 13 rodzin × 32 oryginalne Grade variants, built-in on/off | Nazwy/grade/source/role poprawne; full original selectable; zero authored-only catalogue |
| C02 | Price=0, unlock Class=1, Lizard/Leizard Bulshade, AT, stock catalog obecny | Kupowane w Class 1, właściwe body/wheels/textures, AT naprawdę zmienia biegi; resolved spec asserted |
| C03 | Zmieniony shift threshold, AT scale, rev limit/redline | Final loaded record pasuje projektowi po overlay; nie tylko raw byte assertions; regional speed units correct |
| C04 | MT-only, retail_manual_only on/off, negative/overflow invalid fields | Oba reguły jasne, inline error; nie można wygenerować mieszanego availability |
| C05 | OBJ+MTL roundtrip, GLTF external, GLB embedded, PNG | Geometry/roles/UV/normals/material/depth bias/color/alpha zachowane; sidecars nie wymagają rage_* od usera |
| C06 | Mirrors/negative scale/nonuniform transforms, front/rear wheel pivots | Winding/normals prawidłowe; steer/spin smooth; body nie maskuje losowo kół; no animated variant loss |
| C07 | Blender manual GUI roundtrip dwóch kompletnych aut, extras retained i removed | Standard export działa; missing role metadata prowadzi do mapping wizard; nie silent body flatten |
| C08 | Lost texture/material ID, unsupported extensions/skin, NaN/index overflow | Import preview wymienia błędy części; previous document unchanged |
| C09 | Shared textures między częściami/rywalami/klasami | Used by i explicit scope; local edit nie zmienia niezamówionych aut; break-link tworzy nowy binding |
| C10 | Actual rival models in Custom, class/course changes, Class 6 four rivals | Correct body/glass/front/rear wheels; count/family labels; inactive slots not dereferenced |
| C11 | Rival car player: Classic/Modern race→replay | Native support matrix respected; no invisible player/frozen extra cars/wheel flicker; clean teardown |
| C12 | Near/far LOD, paint masks, logos, driver name, glass | Preview modes nazwane; native frame match; appearance-only test nie udaje physics validation |
| C13 | Day/night/tunnel/headlights/tail/stop, 1st/3rd person | Edytowane lamp data daje final effect; simulation advances once/tick, nie per view |

## Tory — obowiązkowe przed stwierdzeniem „gotowy editor zasobów”

| ID | Test / zestaw | Wynik wymagany |
| --- | --- | --- |
| T01 | 24 pair archive inventory; real choices per class/series | Placeholders OVAL1/2 poprawnie rozróżnione; reverse korzysta z właściwych danych |
| T02 | Full Mythical Coast + Over Pass City scene assembly | Terrain placement 2048 vs mesh scale prawidłowy; winding; UV texture windows; bank identities separate |
| T03 | Pick terrain face/course instance/procedural scenery | Nazwa/role/material/Used by; inspector nie obiecuje instance transform terrain face |
| T04 | Move/rotate/duplicate/remove/add scenery, save/reopen/apply | Żądane instance w actual modern scene i ray queries; bounds/visibility działa; performance budget mierzony |
| T05 | Large hi-res PNG and modern native geometry beyond PS1 upload atlas | Bez niejawnego PS1 VRAM cap; engine memory/file limits jawne; classic compatibility explicit |
| T06 | Change road width/height/cross-slope/arc and add segment | C bake validates, car height/contact prawidłowy; visual road pokrywa physics overlay |
| T07 | Appearance-only imported road, incompatible driveable route | UI jawnie rozróżnia; build blokuje driveable mismatch zamiast floating/sinking car |
| T08 | Route closed seam/start-finish/wrap/reverse; minimal/max lengths | No gaps/overflow; actual full lap and replay; finish/lap triggers poprawne |
| T09 | AI line, active rival starts, speed keys, changed length | AI przejeżdża pełne okrążenie, nie przecina pobocza; references poprawne dla obu serii |
| T10 | Intro/replay/default camera edits; cuts and course selection changes | Actual GP intro isolated posiada geometrię; TrackAssetIdentity nie przechodzi na inne assets; sky correct after cuts |
| T11 | Fog/time/sky/tunnel zones, environment variant transitions | Preview authoring i game zgodne co do resolved data; minimum visibility; no stale textures/palettes |
| T12 | Flyby/path objects/spinning scenery/class freeze | Timeline zmienia rzeczywistą animację, named data replaces code; Class 6 nie odtwarza nieaktywnych slotów |
| T13 | Visibility bounds/masks rebuild and start/end of Over Pass City | Brak dalekiej złej geometrii/pop-in regression, measured frametime vs baseline |
| T14 | Other tracks + forward/reverse/classes + exit/next race/replay | Zero leftover instances/lights/mesh cache; source generation invalidation deterministic |
| T15 | New dynamic track/car IDs (expanded-content phase) | Picker, records, saves, restart i custom session bez hard-coded array OOB; migration tested |

## Interface / Audio / Saves / UX

| ID | Test / zestaw | Wynik wymagany |
| --- | --- | --- |
| U01 | Every shown editor property/control | Ma label/units/help/range/reset; advanced zachowuje values przy hide/show; invalid Save blocked |
| U02 | Main menu/title/Custom button replacement PNG | Layout preview i final frame zgadzają się; tło/logo bez kolizji atlasu; runtime replacement bez recompile |
| U03 | HUD/Pause/course/car select classic+modern+widescreen+HiDPI | Pause widoczny i miga; anchors nie rozpychają marquee/chances; old defaults restored after disable |
| U04 | Audio cue preview/replace, confirm/cancel/FMV skip/attract transitions | Cue name rzeczywisty; właściwy event/time, one playback; no runaway accelerated snippets |
| U05 | PAL/NTSC course-select PCM original/override/missing generated WAV | Regional tempo/timbre/gain correct; missing original triggers extraction, bez alternate playback fallback |
| U06 | CD race tracks missing/present; engine loop and ambience | Named playable contents; loop/mix starts at correct trigger, audio stops/reloads with session |
| U07 | MP4/new video capability (codec phase) | Compiled bundled import/player, AV sync/aspect/skip/restart, no external installed decoder prerequisite |
| U08 | Save single/raw card/DexDrive, create all regions | Correct checksums, unrelated entries preserved, save copy source intact, no-record semantics verified |
| U09 | Team logo/name + car markings | Actual palette/mask and runtime composition preview; paint and window text not overwritten |
| U10 | Keyboard-only, 960×680, 1280×900, 1440×960, 200% UI | No inaccessible controls/overflow, visible focus, tab order, dialogs fit, viewport not dominated by forms |
| U11 | No disc/no project/no selected object/busy/error/conflict | Actionable empty state; no anonymous file list or dead button; cancellation visible |
| U12 | New user usability pass: edit color, price, track object and save→test | Bez pomocy technicznej/CLI; nazwane obiekty, user wie co jest active i co tylko saved |

## Rodzaje evidence

1. C unit fixtures: pure parse/validate/write/resolve, malformed tables, field
   preservation, mapping, snapshot limits. Synthetic data w repo; real disc read-only poza repo.
2. JS service integration: actual compiled worker, filesystem transactions,
   cancellation/history/profile migration; UI nie mockuje successful build.
3. Hidden Electron UI tests: field editing/keyboard/details/unsaved/undo/project
   switching/import errors/source labels. Hidden native rendering nadal real GPU.
4. Actual game verification: prepared snapshot normal race session init; assert
   final loaded records plus native capture + critical scene assertions. One test
   per task (GP intro, race/replay etc.), bez wielominutowego menu tour.
5. Manual review: Blender interchange, preview quality/audio timbre, clean
   packaged install na 3 OS, novice workflow. Dokumentowana observation, nie fake CI pass.

Każda faza w planie dostarcza swoją podmacierz. Rozszerzenia bez wykonanych testów
mają status planned/unsupported, nawet jeśli parser potrafi odczytać plik.
