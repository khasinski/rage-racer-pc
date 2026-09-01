# Rage Racer save editor

A window onto a Rage Racer memory card save: every field the game stores, in
the format this port reads and writes, on Windows, macOS and Linux.

## What it edits

Everything in the save, not a chosen subset:

- the team name and the save counter from the file header
- grand prix, extra grand prix and time attack progress: course, car, class,
  highest class reached and money
- the three garages of thirteen cars each: grade, tyres, gearbox, both paint
  colours and whether the car is owned. Cars are listed by maker and name, and
  the five the Japanese release renames follow the release the file belongs
  to: the Erriso, Acceron, Vainqure, Bulshade and Squaldon are the Alouette,
  Instinct, Victoire, Tempest and Dragone on a Japanese save
- class records, best lap, total and sector times, and both sets of ranking
  and time attack records with their driver names
- the 64 by 64 team logo and its sixteen colours, painted with the mouse
- controller and neGcon calibration, music track and volumes
- the bytes the game does not read, so a file edited here keeps whatever it
  arrived with

Times are typed the way the game writes them, so `1'40"765` is what you enter.

A new save is not a block of zeroes but what the game itself holds after a
fresh boot: full volumes, the Gnade Esperanza owned and selected, no class
entered, empty class records rather than a field of first places, five
retries, and the one neGcon value the game does not start at zero, the play
in its steering.

## The three releases

| release | disc | file on a card |
| --- | --- | --- |
| PAL | SCES-006.50 | `BESCES-00650 RAGE00N` |
| NTSC-U | SLUS-004.03 | `BASLUS-00403 RAGE00N` |
| NTSC-J | SLPS-00744 | `BISLPS-00744 RAGE00N` |

The three store the same fields in the same order; only the name the file
needs on a card differs, and the editor sets that from the release you pick.
A file whose name it does not recognise still opens, and a card name it has
never seen still tells it the territory, because the second letter of a PS1
card file name is the region.

The PAL and American serials come from this repository. **The Japanese serial
is not stated anywhere here**, so it is marked as unconfirmed in the editor:
if it turns out to be wrong, the fix is one line in `kRegions`.

## Building

Needs CMake, a C and C++ compiler, and this repository's submodules, which
supply SDL3 and Dear ImGui. No other dependencies.

```sh
cmake -S editor -B build/editor -DCMAKE_BUILD_TYPE=Release
cmake --build build/editor -j
ctest --test-dir build/editor
```

The editor takes a file to open on the command line, and files can be dropped
on its window. `--screenshot FILE` draws one frame and writes it out, which is
how its own screens are checked.

## What it does to a file

It reads the whole file, edits it in memory, and writes it back whole. The
three checksums are recomputed on the way out, so a save that arrived with a
broken one leaves repaired. That also means a save the game would have
refused becomes one it accepts, which is usually why you are here.

It refuses a file with no memory card signature, and says what it found
instead; it refuses one of the wrong length, and says whether it looks like a
whole card block rather than one save.

## Where the saves are

The port keeps them beside its other state, one directory per card:

- macOS `~/Library/Application Support/Rage Racer/bu00`
- Windows `%APPDATA%\Rage Racer\bu00`
- Linux `$XDG_STATE_HOME/rage-racer/bu00`, or `~/.local/state/rage-racer/bu00`

A portable install keeps `bu00` beside the game instead. The editor looks in
the first three by itself and lists what it finds, so the usual answer to
"which file" is a name to click rather than a path to remember.

## How it knows the layout

`rage_save.h` includes the game's own `include/game/save_format.h` rather than
a copy of it. A field added to the save appears here, and a field that moves
breaks the build rather than silently writing to the wrong offset. The
checksums are checked against the game's own routine in the tests, because an
editor that agrees with itself and not with the game writes files the game
will not load.
