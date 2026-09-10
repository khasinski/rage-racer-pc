# Repeatable renderer investigation (Linux)

This is an opt-in diagnostic route driver, not a gameplay/physics test. Ordinary
launches have no autopilot hook. No Python runtime is needed:
the driver and tests are C; the launcher uses the existing CMake toolchain.

Build:

```sh
cmake --build build --target rage-racer debug_route_tests debug_autopilot_tests --parallel
ctest --test-dir build --output-on-failure -R '^debug_(route|autopilot_)'
```

Run from the repository, with a legally obtained CUE or Track 01 BIN:

```sh
cmake -DGAME="$PWD/build/rage-racer" \
  -DDISC="/absolute/path/to/Rage Racer (Europe).cue" \
  -DRUNS=2 -DLAPS=3 -P tools/debug_drive.cmake
```

Rendering settings are copied from the repository's `rage-port.ini` by default,
or from `-DCONFIG=/absolute/path/to/config.ini`. The copy is retained as
`renderer.ini` in the evidence folder. Scale, widescreen, sampling, post-processing
and interpolated presentation are **preserved**, not replaced with a low-resolution
logic-rate preset. The renderer itself is explicitly forced to modern. To compare
settings, supply separate config files and keep the route parameters unchanged.

Each process gets a fresh XDG config/state directory below a uniquely named
`build/debug-drive/` session. It imports native assets automatically, boots the
configured Grand Prix, drives, takes bounded marker bursts, then exits. Personal
settings, memory cards and `rage-port.ini` are untouched. The launcher uses dummy
audio: this does **not** test audible playback.

For race -> prize collection -> another race **in the same process**, use
`-DRUNS=1 -DRACES=2 -DCLASS=1 -DCOURSE=0`. A positive `RACES` replaces the
tour-count exit condition: the driver lets races finish, the scenario confirms
replay/prize screens and selects the same race through the normal menus. It exits
when the last race hands off to replay (the last prize screen is not included).
Keep marker scheduling/budgets large enough to reach the second race; `RUNS=2`
alone launches two fresh processes and does not exercise this transition.

The car follows interpolated track centerline points with the correct series
direction. For physical inspection independently of series selection,
`autopilot.direction=-1` forces forward and `autopilot.direction=1` forces
reverse; the default `0` follows the race. This does not change race assets,
opponent direction or record tables. It is useful for the shared finale course,
where both Grand Prix selections use forward assets. The start log records the
actual direction, and each completed tour reports cumulative track units.
Movement uses track units/second and the detected PAL/NTSC logic rate.
Active race updates use 25 Hz for PAL and 30 Hz for NTSC, independent of presentation
FPS. Track contact, lap progress, scenery, camera and texture updates still run. Player
drivetrain/input/collision physics are bypassed; the speedometer is not a speed
measurement for this mode. `laps` counts complete point-ring tours from the debug
start, not necessarily the HUD's finish-line lap count. This is not a substitute
for natural Grand Prix completion or FMV tests.

## Vulkan synchronization validation

To run the drive with the Khronos validation layer installed:

```sh
cmake -DGAME="$PWD/build/rage-racer" \
  -DDISC="/absolute/path/to/Rage Racer (Europe).cue" \
  -DVALIDATION=ON -DRUNS=2 -DLAPS=3 -P tools/debug_drive.cmake
```

Validation is explicitly enabled, including synchronization validation. The
launcher retains loader diagnostics, requires evidence that the Khronos layer
was inserted, and fails on validation errors/hazards. A clean report is not proof
that the image is correct, nor that all possible synchronization errors are absent.

## Evidence and limits

For lightweight repeated modern screenshots, use
`diagnostics.modern_dump=/absolute/path/frame` with
`diagnostics.modern_dump_every=12`. Adding `diagnostics.modern_dump_info=true`
writes a matching `.info.txt` with camera position/matrix, texture state and
draw information for each successful capture. The interval is in logic frames,
not metres; calibrate it against route speed before claiming physical spacing.
For reproducible geometry comparisons, select `video.fps=logic` to disable
wall-clock camera interpolation. The metadata also records `nativeCamera`,
the actual prepared GPU camera, separately from the captured game camera.
With `diagnostics.modern_dump_offscreen=true`, an explicit modern dump renders
and saves frames without presenting them to the window. This avoids swapchain
waits in unattended captures; it is not a normal-play FPS benchmark.

For visibility diagnosis, `diagnostics.native_far_plane` overrides the native
camera's far plane in world units (1024–262144; default 16384 for races,
262144 elsewhere). It applies to both main and mirror views. The default race
scene also follows authored visibility regions as the camera moves. For A/B
diagnosis only, `diagnostics.native_region_visibility=false` disables that
region filter; it can expose unrelated road sections above the scenery.

For paired renderer timing, `diagnostics.performance=true` plus
`diagnostics.frame_timing=true` records each submitted modern frame's monotonic
timestamp, logic frame and track point. This has much less logging than the
full `performance_trace`. Compare warmed-up runs with identical explicit timing
standard and an unobscured window; covered Metal windows may stall presentation.

Per run:

- `game.log`: scene transitions, route tours, marker frame IDs and Vulkan messages.
- `settings.txt`, `launcher*.log`, `result.txt`: settings and outcome.
- `state/rage-racer/markers/`: the existing M-key bundle (screenshots, VRAM,
  scene/world snapshots, draw and palette information).

Defaults: first marker at logic frame 900, then every 900, at most four automatic
requests. Each marker request saves four marker frames and requests **two future
GPU presents**. An RDC is not a retroactive capture of the triggering marker;
`after-logic` deliberately records that distinction. Manual M still works, and
can request captures until the GPU request limit is reached. Captures can consume
hundreds of MB per run, or more at high resolution. Previous-frame screenshot
history follows the chosen config: `diagnostics.marker_history=true` retains
16 prior presentations and costs substantial RAM/VRAM and per-frame copying.
Use a separate config with it disabled when comparing instrumentation overhead.

Useful launcher overrides:

| Parameter | Default | Meaning |
| --- | --- | --- |
| `RUNS`, `LAPS` | 2, 3 | Fresh processes and full route tours per process |
| `COURSE`, `SERIES` | 0, 0 | Course 0..3, forward/reverse series 0..1 |
| `CLASS` | 0 | Class 0..5; use 2 to investigate that environment variant |
| `SPEED` | 6000 | Track units/second, not km/h |
| `MARKER_FRAME`, `MARKER_EVERY` | 900, 900 | Initial capture window and cadence |
| `MARKER_LIMIT` | 4 | Automatic marker / GPU request budget |
| `MAX_FRAMES`, `TIMEOUT` | 30000, 900 | Logic-frame watchdog and wall-clock seconds |
| `OUTPUT` | build/debug-drive | Parent of unique evidence sessions |
| `CONFIG` | repository rage-port.ini | Read-only source of rendering settings |

Use the same image/course/series/speed/capture schedule to compare good and bad
passes. The launcher fails on timeout, an incomplete drive, unavailable GPU,
missing requested evidence or detected validation errors. **A successful drive
does not automatically classify flicker or corrupted textures.** The original M
markers remain essential for relating renderer output to game state.

For a manual launch, equivalent opt-in keys are `autopilot.enabled`, `.speed`,
`.laps`, `.max_frames`, `diagnostics.marker_frame`, `.marker_every`,
`.marker_limit`. Enable
`diagnostics.marker_capture` as well. Keep capture/validation runs separate from
baseline runs when checking whether instrumentation changes a timing-sensitive bug.

### Sampled VRAM evidence

Modern 3D markers are captured after the indicated snapshot's render submission.
Alongside the existing `marker-N-vram.raw` current 16-bit compatibility readback,
`marker-N-vram-sampled.rgba` stores the GPU texture actually sampled by the modern
overlay renderer. It is headerless 1024×512 RGBA8 (2,097,152 bytes), not another
16-bit raw dump. The info file records `sampledVram frame=... matchesScene=...`.
A missing or mismatched sampled texture is reported instead of saving it under
the new scene's frame. Passthrough/menu frames need not have a modern texture.

This distinguishes the sampled resource from subsequent VRAM changes; it does
not make the world snapshot a standalone replay bundle. Native meshes/materials
and packet-renderer replay remain separate dependencies. Four-frame M bursts add
8 MiB of sampled-VRAM data, so keep markers disabled for baseline profiling.

On Linux, build `rage-racer-smoke` and run
`SDL_VIDEODRIVER=offscreen ctest --test-dir build -R '^sampled_vram_marker' --output-on-failure`
to exercise marker association with logic, 60 FPS and vsync settings. Supply
`RAGE_PORT_DISC_CUE` if the PAL image is not at the repository's default path.
These tests use dummy audio and isolated XDG state directories.
# Retained importer texture generations

With marker history enabled, each history slot retains the already-captured
importer texture generation matching the native GPU cache revision. Overwriting
a slot or destroying history releases its reference. M writes
`ring-XX-fFRAME-gGEN-bankN.raw`: bank -1 is the unmodified CPU snapshot, banks
0/1 are reconstructed track pages in complete VRAM images. Each file contains
1024x512 native-endian uint16 words (1 MiB); a full 16-slot dump adds 48 MiB.
References share storage within a generation rather than copying it per slot.

These are importer inputs, not the GPU overlay's sampled RGBA texture, decoded
material images, or a self-contained replay bundle. Missing/mismatched importer
generations are not replaced by reading current VRAM. File-provider sessions
do not provide these importer generations. Cross-generation retention is unit
tested; the history integration test currently captures one PAL race generation.
