# Native shader contracts

Build the production shader probes without the game, importer or disc data.
Requires a compiled SDL3 package with Vulkan or Metal GPU support. No shader
compiler is needed: the probes use the checked-in production shader headers.

```sh
cmake -S tests/native_shader_contract -B build/native-shader-contract \
  -DCMAKE_BUILD_TYPE=Release -DSDL3_DIR=/absolute/path/to/sdl-build
cmake --build build/native-shader-contract --config Release
ctest --test-dir build/native-shader-contract -C Release --output-on-failure
```

On Windows, configure with `-G "Visual Studio 17 2022" -A x64 -T ClangCL`.
On headless Linux, an SDL build supporting offscreen video can run with
`SDL_VIDEODRIVER=offscreen`. The probes request SPIR-V or Metal, not DXIL;
Windows therefore requires a working Vulkan implementation.

The native probe checks fog, lighting and deferred UVs. The shadow UV probe
checks the shadow vertex shader's instance binding. The mask test compares
actual depth pixels from the production alpha-discard shader against
CPU-expanded UVs and an unscrolled control. These are isolated GPU contracts,
not a full-game rendering or packaging gate. Exit 77 means the SDL video/GPU
device could not initialize: a skipped test is not a rendering pass.

For a release validation run, configure with `-DRAGE_REQUIRE_GPU=ON`. In that
mode an unavailable backend fails CTest instead of producing a skipped test.
