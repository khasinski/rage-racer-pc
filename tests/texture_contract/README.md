# Compiled texture patch contract

Run the complete isolated gate (including building the pinned SDL dependency):

```sh
cmake -P tests/texture_contract/run.cmake
```

This uses `build/texture-ci` by default; override with
`-DRAGE_TEXTURE_CONTRACT_BUILD_DIR=/absolute/build/path` before `-P`.
The `texture-contract-check` workflow runs the same script on Linux, Windows
and macOS, checking out only libchdr, psyz and its SDL submodule. It needs no
disc, display, game loop or Python. Windows uses VS2022/ClangCL; Unix needs Ninja.

The smaller standalone project below builds only the production texture patcher
and the same synthetic C fixture used by the main build. No game image, SDL,
GPU, Node or Python is needed in this mode. The libchdr
submodule must supply `deps/miniz-3.1.2/miniz.c` and `miniz.h`. The vendored
`external/yyjson` parser is built from the same sources as the game.

```sh
cmake -S tests/texture_contract -B build/texture-contract -DCMAKE_BUILD_TYPE=Release
cmake --build build/texture-contract
ctest --test-dir build/texture-contract --output-on-failure
```

For Windows with Visual Studio 2022 Build Tools and ClangCL:

```powershell
cmake -S tests/texture_contract -B build/texture-contract -G "Visual Studio 17 2022" -A x64 -T ClangCL
cmake --build build/texture-contract --config Release
ctest --test-dir build/texture-contract -C Release --output-on-failure
```

The fixture checks edits and unchanged repacks at 4/8/16 bits, palette alignment,
neighboring nibbles, row padding, asset ownership, unsafe index lines, missing
metadata, invalid dimensions/depth/stride, missing or out-of-bounds palettes,
and asset bounds. Rejected edits must leave every byte intact.

Enable `-DRAGE_TEXTURE_ARCHIVE_TESTS=ON -DSDL3_DIR=<SDL3 CMake package directory>`
to build the extract/pack tools and compiled archive roundtrip too. This covers
byte-exact repacks, PNG dimensions, isolated edits, malformed images and their
diagnostics, compressed-stream expansion bounds and unaligned palette edits.
These compiled tests replace `verify_mod_tools.py`; both passed on Linux and
Windows 11 ClangCL Release. They do not prove every PNG filter/color variant and
are not renderer or GUI tests.
