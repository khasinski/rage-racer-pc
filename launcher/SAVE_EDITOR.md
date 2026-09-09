# Integrated save editor

The save editor lives in Rage Mod Manager (`launcher/`). There is one desktop
application, one root CMake build and one C save-format library. The former
standalone SDL/ImGui editor has been retired.

Open **Save editor** from the sidebar. No game image or asset import is required
for save editing. You can open a single save, a raw 128 KiB memory card or a
DexDrive `.gme` card, or create a fresh save for PAL, NTSC-U or NTSC-J.

The editor covers team names and logos, progression, all three garages, record
names/cars/times, controller/neGcon calibration, audio settings, save counters
and reserved bytes. A new save uses the game's fresh-boot defaults, not an
all-unlocked profile. The default filename matches the selected region:

| Region | Default filename |
| --- | --- |
| PAL | `BESCES-00650 RAGE000` |
| NTSC-U | `BASLUS-00403 RAGE000` |
| NTSC-J | `BISLPS-00600 RAGE000` |

**Save a copy** preserves the source file and recalculates checksums. When editing
an entry on a memory card, the output is a copy of the entire card; unrelated
entries and the DexDrive header are retained. Close the game before creating or
writing saves. Times accept segmented entry or a full `01:40.765` paste. Unknown
values are preserved unless explicitly changed.

## Development and tests

Follow [the launcher instructions](README.md) for building and packaging the
application. The native library is `native/save/rage_save.c`, using the game's
`include/game/save_format.h`; there is no second copy of the save layout.

From the repository root:

```sh
cmake -S . -B build/release -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build/release --target rage-save-cli rage-save-format-tests
ctest --test-dir build/release -R '^save_format$' --output-on-failure
```

The C tests retain the former standalone editor's coverage, including checksums
against the game's routine, fresh-save defaults, region filenames, logo packing,
raw/DexDrive card preservation, malformed input and byte-preserving round trips.
`launcher/tests/save-cli.test.cjs` additionally exercises the compiled CLI through
the application service, including edits without a disc, all three regions,
source protection and operation locking. `npm test` runs these after native tools
have been staged with `npm run build:native`.
