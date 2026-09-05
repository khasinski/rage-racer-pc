# Windows validation of the local performance changes (2026-09-05)

Environment: local Windows 11 Enterprise Evaluation 25H2 x64 VM, KVM/QEMU,
8 vCPUs / 12 GiB RAM; Visual Studio 2022 Build Tools with ClangCL 19.1.5.
Tested the current uncommitted source tree, including modified PSY-Z and new
cache/pacing tests, not the older snapshot already present in the VM.
Fresh guest paths: `C:\rage-perf-src`, `C:\rage-perf-results`.

## Initial results using the system graphics driver

- Release executable and selected test targets build successfully.
- 10/10 selected tests pass: `release_package`, `main_loop_frame`,
  `vram_read_cache`, `triangle_coverage`, `texture_span`, `texture_sample`,
  `rmesh`, `rmesh_cache`, `track_material_page`, `modern_frame_pacer`.
- Interactive-desktop startup with the existing local PAL CUE recognized the
  disc, generated native assets through the C importer and selected modern.
- Actual rendering and the route drive are **not validated**. SDL reports
  `SDL_CreateGPUDevice: No supported SDL_GPU backend found!`.
  The VM exposes Microsoft Basic Display Adapter, driver 10.0.26100.1.
- Audio initialization succeeded, but audible playback was not tested.

This is targeted coverage for the performance changes plus packaging, not a
full Windows test-suite or release sign-off. It provides no Windows FPS result.
The startup harness allows 30 seconds, then closes/stops only its newly
launched game process if it has not exited. No classic fallback or replacement
graphics driver was used. A Vulkan-capable Windows system (or suitable GPU
passthrough) is still required for gameplay and visual validation.

## Portability fixes found during this check

1. The phase profiler initially included SDL in the legacy-compiled
   `platform_stubs.c`. Compatibility macros there broke ClangCL intrinsic
   declarations. The implementation now lives in `modern_renderer.c`, which
   already uses SDL without those legacy compile settings.
2. Existing mesh tests passed Unix `-Wall -Wextra -Werror` flags to ClangCL,
   inadvertently enabling a different warning set. They now use `/W4 /WX`
   on the MSVC-compatible frontend and retain their original flags elsewhere.

Local evidence: `build/autopilot-check/windows-perf-first-results/` preserves
the failed attempts; `windows-perf-final-results/` contains the successful
retry, package/regression test log, binary hash, GPU inventory and startup log.
The source archive was transferred before the two fixes; the changed sources
and CMake file were then copied explicitly into the fresh guest tree.
`darwine` was also checked but SSH returned “No route to host”.

## Follow-up: existing software Vulkan driver

Edge already includes SwiftShader and its Vulkan ICD manifest. Selecting that
manifest for the test process via `VK_DRIVER_FILES` / `VK_ICD_FILENAMES`
worked when the interactive task ran as a normal, non-elevated user. The
elevated attempt continued looking for registry-installed graphics drivers.
No driver was installed, no registry setting changed and no SDL source edited.

The non-elevated probe reported `SDL_GPU Driver: Vulkan`,
`SwiftShader Device (Subzero)`, conformance 1.3.3.1 and advanced beyond the
initial boot scene. The subsequent PAL route test **completed one full tour**
in modern 4× (1706×960), with `autopilot result=complete laps=1` and no forced
termination. The harness did not record a usable numeric game exit code.
The native pipeline initialized and the requested frame-600 image was captured
and inspected: road, sky, scenery and HUD render, without obvious corrupted
textures in that image. One still frame does not prove absence of flicker.
Grading setup still failed, as on the Linux host; sound audibility and ordinary
player input were not verified by this route driver.

Guest evidence: `C:\rage-perf-results\swiftshader-drive`; copied locally to
`build/autopilot-check/windows-swiftshader-drive/`. The inspected PNG is
`build/autopilot-check/windows-software-race.png`.
This is a software-rendering correctness result, **not Windows hardware
performance**: the CPU driver rendered roughly 9–11 frames/s. It does not
change the initial finding that this VM's system graphics driver cannot run
the renderer. No software driver is silently enabled in the shipped game.

## Follow-up: Vulkan colour grading

The grading failure was caused by a Metal-only composite shader. Added compiled
SPIR-V alongside MSL and rebuilt the Windows Release game successfully.
The new `composite_gpu_tests` executed on Windows using the same process-local
SwiftShader setup: exit 0, all eight reference colours and alpha passed actual
GPU render/readback comparisons (one-byte RGB tolerance). It also passed on
Linux. Evidence: `build/autopilot-check/windows-composite-results/`.

The full Windows tour above predates this shader fix; the post-fix Windows
check covers the Release build and GPU colour fixture, not another full tour.

## Follow-up: prepared textured-triangle scanlines

After moving stable vertex sorting out of the per-scanline loop, the Windows
Release game rebuilt successfully. The new `texture_triangle` oracle test
passed 5,220,000 scanlines against the frozen original implementation, including
equal-Y vertices, degenerate triangles, clipping-range samples and draw offsets.
Both build and test exit codes are 0. Evidence:
`build/autopilot-check/windows-triangle-results/`. This follow-up did not repeat
the full Windows route drive.

## Follow-up: prepared camera transforms

The Windows Release game rebuilt with caller-owned camera transforms prepared
once per native draw build. The new `render_view_transform` test passed
256,000 comparisons of transformed coordinates and fog values against the
existing path, including Euler rotations, normalized/non-normalized and
invalid quaternions, non-finite inputs and independent camera snapshots.
Build and test exit codes are 0. Evidence:
`build/autopilot-check/windows-view-results/`. This validates the updated build
and math, not a new full Windows gameplay run or hardware FPS measurement.
