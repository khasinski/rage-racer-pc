# Standalone compiled mod contracts

This gate compiles the production C modules and reuses the tests from
`tests/disc`. It needs a C11 compiler and CMake, but no disc, SDL, renderer,
generated meshes, Node or submodules. Release assertions stay enabled.

Linux/macOS:

```sh
cmake -S tests/mod_contract -B build/mod-contract -DCMAKE_BUILD_TYPE=Release
cmake --build build/mod-contract --parallel 4
ctest --test-dir build/mod-contract --output-on-failure
```

Windows with Visual Studio 2022, ClangCL and the Windows SDK installed:

```powershell
./tests/mod_contract/run-windows.ps1 -CMake cmake
```

`-CMake` also accepts an absolute path to CMake bundled with Build Tools;
`-BuildDirectory` selects an isolated build. The CRT policy matches the main
project. Project sources and test sources compile with warnings as errors.

Coverage:

- JSON metadata syntax, owned Unicode fields and malformed-input rejection;
- the shared dependency graph, both ID namespaces, version/region matching,
  ambiguity, mixed cycles and a full 128-node chain;
- provider choices, stale candidate sets and bounds;
- runtime TOML parsing, override selection and its stricter graph adapter;
- the real native provider stdin decoder, framing, byte limit and Windows
  binary handling (including Ctrl-Z as data);
- streaming file snapshots, byte equality, exclusive creation, limits and
  removal of failed output without overwriting existing files;
- package-relative file classification, all 1000 three-digit raw indices and
  rejection of unsupported extensions and unsafe relative paths, plus global,
  semantic, legacy and omitted copy roles.

On 2026-09-06, all seven tests passed on Linux/GCC Release and a local Windows
11 evaluation VM using ClangCL 19.1.5, VS Build Tools 2022 and SDK 10.0.26100.
The Windows sources were copied to an isolated test directory; this was not
a full checkout/game build. The GitHub workflow also specifies macOS, but
adding the workflow is not evidence of a macOS or hosted-CI pass.

These tests do not validate the full Electron package, native dialogs, graphics,
audio, race transitions or the shipping first-launch contract. Those remain
separate integration/release gates.
