# 0.6.4-alpha validation on darwine

Status: bounded validation completed; no publication authorized. Original manual-test
archives are unchanged. The checks use rebuilt executables from the current
working tree, including the attract geometry, classic seam and regional speed
display fixes.

## Environments and scope

- macOS arm64/Metal: repeat of the four latest regressions after the additional
  offscreen queue fix; all pass.
- darwine Linux x86-64: Ubuntu 26 host, GCC 14, NVIDIA Vulkan under Xvfb.
  The unrelated GPU workload is left running. These are correctness/stability
  checks, not measurements of foreground FPS.
- Linux release baseline: separate Ubuntu 24.04 container build, authored cars
  disabled, highest required glibc symbol version 2.38. Its eleven selected
  release/unit checks and NTSC-U attract GPU comparison pass.
- Windows 11 VM on darwine: Visual Studio 2022/ClangCL. Rendering uses Edge's
  SwiftShader Vulkan through process-local environment variables. No software
  renderer configuration is added to the shipped INI. This cannot validate
  hardware GPU performance or driver-specific crash reports.

Both available legal regions, PAL and NTSC-U, are exercised. No NTSC-J image
was available for this run; its unit-selection behavior is covered by compiled
tests, not a real-disc playthrough.

The route matrix uses Overpass City forward and reverse, modern and enhanced
classic, with automatic regional timing. Linux renders at 1280x720; Windows
software rendering uses 426x240. Each route covers one full lap and clean
teardown. Classic requests 120 presentations per second, which does not imply
that software rendering achieves that rate. A separate release-baseline Linux
probe covers three modern reverse laps. Route automation follows track points
and bypasses player driving; it does not constitute a manual GP championship.

## Findings

All 16 one-lap matrix runs passed: eight on Linux and eight on Windows,
covering both regions, both renderers and both directions. Each recorded
route completion and clean resource teardown. The four attract -> GP tests
(two regions on each OS) passed all four reference-image comparisons each.
The Ubuntu 24.04 binary also passed its attract test and three-lap probe.
Selected modern/classic images from both systems were inspected; this is not
exhaustive visual validation of every frame or every track.

A fresh Windows launch with the shipped defaults and no SwiftShader overrides
still fails in this VM: `SDL_CreateGPUDevice: No supported SDL_GPU backend
found!`. Its session also has no default audio device. The process did not
exit itself; the startup probe stopped only its own process (PID 7332) after
10 seconds. This is not a successful default-install/startup result, and the
software-rendered matrix must not be used to claim otherwise. Accelerated
Windows manual testing remains required. The VM is left running; the automated
game processes have exited or, for that bounded failed-start probe, been stopped.

The NTSC-U attract comparison initially crashed on Linux in SDL Vulkan uniform
buffer allocation. Reproduction under GDB isolated the explicit offscreen
capture path: skipping swaps removed compatibility-renderer queue backpressure
on the title screen, before native 3D rendering and its offscreen fences began.
Waiting for preceding GPU work on all offscreen presentations bounds the queue
through 2D and interlaced scenes too. The exact failing test now passes with
four GPU images identical to the reference; the macOS rerun also passes.

The Linux unit/functional selection has 411 passes and two skips (missing disc
stream-table data and prebuilt stage assets). An initial release-package failure
was caused by stale workflow files in the isolated copy; synchronizing those
files and rerunning the test resolves it.

The initial Windows run built the game and passed twelve selected checks,
but its full build failed in 83 targets (82 tests and the HUD preview tool).
That limitation has since been repaired: see
[Windows test suite repair](windows-tests-2026-09-08.md) for the complete
ClangCL build, full unit/functional results and expanded release CI gate.
The graphics/startup limitations above remain unchanged.

Detailed local evidence: `build/validation-darwine-064/`. Remote Linux evidence:
`/home/hasik/Projects/rage-port-classic-check/build/validation-064/`. Windows:
`C:\Users\hasik\rage-classic-check\build\validation-064\`.
