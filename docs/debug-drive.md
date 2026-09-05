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
direction. Movement uses track units/second and the detected PAL/NTSC logic rate.
Active race updates use 25 Hz for PAL and 30 Hz for NTSC, independent of presentation
FPS. Track contact, lap progress, scenery, camera and texture updates still run. Player
drivetrain/input/collision physics are bypassed; the speedometer is not a speed
measurement for this mode. `laps` counts complete point-ring tours from the debug
start, not necessarily the HUD's finish-line lap count. This is not a substitute
for natural Grand Prix completion or FMV tests.

## RenderDoc and synchronization validation

With RenderDoc and the Khronos validation layer installed:

```sh
cmake -DGAME="$PWD/build/rage-racer" \
  -DDISC="/absolute/path/to/Rage Racer (Europe).cue" \
  -DRENDERDOC="/absolute/path/to/renderdoccmd" -DVALIDATION=ON \
  -DRUNS=2 -DLAPS=3 -P tools/debug_drive.cmake
```

The local test machine also has extracted Fedora x86-64 RenderDoc and validation
packages under `build/debug-tools/root`, without system installation. On this
machine replace `-DRENDERDOC=...` with `-DTOOLS_ROOT="$PWD/build/debug-tools/root"`.
The harness creates a session-local RenderDoc layer manifest with the correct
library path and supplies layer/library search paths only to the child process.
These tools are not shipped with the game. `TOOLS_ROOT` expects the Fedora package
layout (`usr/bin/renderdoccmd`, `usr/lib64/renderdoc/librenderdoc.so`).

RenderDoc must be injected **before** Vulkan initialization. The game only looks
up the already-loaded API; it never silently loads it late. The vendored MIT
application API header is from RenderDoc tag `v1.35`:
https://github.com/baldurk/renderdoc/blob/v1.35/renderdoc/api/app/renderdoc_app.h

Validation is explicitly enabled, including synchronization validation. The
launcher retains loader diagnostics, requires evidence that the Khronos layer
was inserted, and fails on validation errors/hazards. A clean report is not proof
that the image is correct, nor that all possible synchronization errors are absent.

## Evidence and limits

Per run:

- `game.log`: scene transitions, route tours, marker frame IDs, queued/completed
  RenderDoc captures, and Vulkan messages.
- `settings.txt`, `launcher*.log`, `result.txt`: settings and outcome.
- `state/rage-racer/markers/`: the existing M-key bundle (screenshots, VRAM,
  scene/world snapshots, draw and palette information).
- `state/rage-racer/gpu-*-after-logic-*_frame*.rdc`: GPU captures.

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
does not automatically classify flicker or corrupted textures.** Open the RDCs
and compare the bound texture/CLUT, draw order and depth at the affected pixels.
The original M markers remain essential for relating GPU resources to game state.

For a manual launch, equivalent opt-in keys are `autopilot.enabled`, `.speed`,
`.laps`, `.max_frames`, `diagnostics.renderdoc`, `.renderdoc_burst`,
`.renderdoc_limit`, `.marker_frame`, `.marker_every`, `.marker_limit`. Enable
`diagnostics.marker_capture` as well. Keep capture/validation runs separate from
baseline runs when checking whether instrumentation changes a timing-sensitive bug.
