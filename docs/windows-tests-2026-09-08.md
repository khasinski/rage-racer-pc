# Windows test suite repair — 2026-09-08

The full Visual Studio 2022 / x64 / ClangCL Release build now succeeds,
including every test executable and `rage-hud-preview`. The previous report
contained 83 failing build targets: 82 tests and the HUD tool, not 83 failing
test assertions. The release workflow had built only a selected subset and
therefore did not catch this drift.

## Causes and changes

- Test CMake files passed GNU driver options directly to clang-cl. Shared
  warning, quote-include and forced-include options now preserve the intended
  Clang warnings and header precedence on Windows. Assertions remain enabled
  in Release; no tests were excluded to make the build pass.
- Several fixtures supplied their own incompatible PSY-Q types or omitted
  `__psyz`. They now use canonical headers and declarations, including the
  pointer-sized `u_long` on Win64. The audio runtime mock now matches the
  library's `short` parameters; reading those arguments as `long` produced an
  actual assertion failure under the Windows calling convention.
- The sound-cue fixture intercepted the CRT's `printf` symbol, which Windows
  defines inline. A test translation unit redirects only production diagnostic
  calls to the fixture observer, after loading the original headers.
- `game_logic` now creates exclusive temporary files/directories with the
  Windows CRT and checks the actual APPDATA config path. Unix behavior remains
  covered by its existing branch.
- The environment ABI test now checks the documented Windows packed size
  (110 bytes), while other ABIs retain 112 bytes. Production assertions still
  enforce the color-slot offset and element sizes. A UI address comparison
  explicitly uses its unsigned 32-bit address type on both ABIs.
- The terrain-material fixture used to include the entire importer and depend
  on linker dead stripping to hide unrelated game dependencies. Its production
  stream parsing and CLUT selection now live in `native_import_stream.h`, used
  unchanged by both the importer and fixture.
- The HUD tool now shares the game's ClangCL common-symbol policy and removes
  redundant definitions of render/car state.
- The load-buffer test no longer assumes that separate arrays cannot be
  adjacent in the executable. It still checks first/last bytes and addresses
  outside both arenas; a shared boundary legitimately starts the next arena.
- Four source checks (`native_car_names`, `architecture_boundaries`,
  `hud_anchoring`, `native_sky_projection`) now run in a compiled C executable.
  They retain the original checks, including decoding the generated Metal
  shader. CMake tracks the scanned source inventories. Each check was also
  verified against a deliberately broken source fixture before removing its
  Python predecessor.

## Validation

Windows was tested in `aya-win11` through `ssh darwine`, in the isolated
`C:\Users\hasik\rage-classic-check` checkout, with authored car embedding off.
The final full unit/functional run passed 406 of 407 tests, with zero failures
and one skip, using the existing PAL and NTSC-U images. An earlier run without
disc inputs passed 405 and skipped two. `render_stage_angles` requires a separately
exported native-asset fixture, absent in this VM. No test was newly disabled.

macOS: full build and all 414 unit/functional tests passed, including the stage
angle test using the existing exported assets. Linux on darwine: full build,
411 passes and two existing data-dependent skips out of 413 tests. Counts
differ because platform/tool-dependent registrations differ.

The Windows release workflow now builds all default targets and runs
`ctest -C Release -L '^(unit|functional)$' --output-on-failure`, instead of the
short release-test allowlist. This edit has been exercised locally in the VM;
the GitHub workflow has not been published or dispatched.

Evidence is under `build/windows-test-cleanup/`, including compiler logs,
CTest results and the four rejected source mutations. This validates the
test harness, not accelerated Windows graphics or default VM startup. The
previous GPU-driver limitation and requirement for manual release approval
remain; no release, tag or package was published.
