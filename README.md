# Rage Racer PC

Native Windows, Linux and macOS port of *Rage Racer* (PAL / Europe,
`SCES-006.50`). It is based
on the complete clean-room decompilation in
[rage-racer-decomp](https://github.com/khasinski/rage-racer-decomp).

`rage-racer-pc` contains the host port, platform packaging and the runtime
compatibility layer.  The decompilation remains the reference source for the
original PlayStation release.

## 0.4-alpha

This alpha adds an opt-in modern renderer next to the faithful PS1 one:
z-buffered rendering at a configurable internal scale, optional 16:9
presentation, an uncapped interpolated frame rate, an extended draw distance
and FXAA. Select it in `rage-port.ini` next to the executable:

```ini
[video]
renderer = modern
internal_scale = 4
aspect = 16:9
fps = vsync
post = fxaa
toggle_renderer_key = F10
```

The supplied file defaults to the classic PS1-accurate renderer, and `F10`
switches between both renderers without restarting the race. The
release also fixes the rear-view mirror rendering rivals with a mirrored view
matrix, mirror slide-in flashes, paused-mirror rendering, and depth ordering
of car models at hill crests.

The release additionally restores race sound effects and engine audio, fixes
sequenced menu music timing, enables original intro/ending FMVs through
FFmpeg, repairs mirrored-track control and geometry, keeps Lakeside Gate's
waterfalls visible at close range, and corrects Trophy View, controller setup,
retire-camera and title-logo sprite rendering. Runtime settings now use
`rage-port.ini`, normal sessions write `rage-racer.log`, and the scenario
launcher can start any GP, Extra GP or Time Attack race at deterministic track
positions.

## 0.2-alpha

This alpha provides unsigned builds for arm64 macOS, x86-64 Linux and x86-64
Windows. It includes current work on game-state recovery, menu flow, save
state, input replay, and more accurate PsyZ CD and sound behaviour. It
contains no game data. On first launch, the app asks the player to locate the
`.cue` sheet for their legally obtained PAL copy of Rage Racer, then saves that
local choice for future launches. The game image and its data files are never
uploaded to this repository or attached to GitHub Releases.

## Build from source

The project uses CMake and its pinned PSY-Z compatibility layer:

```sh
git clone --recurse-submodules https://github.com/khasinski/rage-racer-pc.git
cd rage-racer-pc
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release --parallel
```

For local decompilation assets, stage either legally obtained disc image by
passing its cue sheet explicitly (paths may be relative or absolute):

```sh
make stage STAGE_ARGS='--pal-cue "/path/to/Rage Racer (Europe).cue"'
```

The resulting executable is `build/release/rage-racer` on Linux,
`build/release/Rage Racer.app` on macOS and `build/release/Release/rage-racer.exe`
with the Visual Studio generator on Windows. Release downloads are supplied
for macOS arm64, Linux x86-64 and Windows x86-64.

## Scenario launcher

`race-scenario.ini` can launch a race through the normal asset-loading flow
without manually navigating the menus:

```sh
./tools/rage-launcher.py
./tools/rage-launcher.py --class 3 --course 2 --car 8
./tools/rage-launcher.py custom.ini --grid 0,1,2,3,4,5,6,7,8,9,10
```

CLI options override values from the INI file. The optional `grid` contains
the eleven rival car IDs in retail start-position order; `-1` leaves a slot
empty. The player retains the normal starting position.
The launcher passes this file directly to the game as `--scenario`; it no
longer expands it into a collection of environment variables.

After the race, `after_finish = menu` (the default) leaves the replay and
result screens to the retail flow but stops scenario automation, returning
control to the player. Use `repeat` to launch the configured race again or
`exit` to close the port after leaving the result screens. The same setting is
available as `--after-finish menu|repeat|exit`.

For renderer and rear-view debugging, scenarios may start cars at exact points
of the loaded track:

```ini
[start]
player_track_point = 120
rival_track_points = 118,116,114,112,110,108,106,104,102,100,98
# Reapply these positions every frame for deterministic rendering diagnostics.
freeze = true
```

The engine recalculates world position, height, heading, track section and lap
progress from these indices. A `-` entry keeps that rival's retail grid pose.

## Runtime requirements

- macOS on Apple Silicon (arm64), a glibc-based x86-64 Linux distribution, or
  64-bit Windows 10/11
- A legally obtained PAL Rage Racer disc image, with its `.cue` sheet and
  referenced track files kept together
- The `ffmpeg` command available on `PATH`. It decodes original `RAGE.STR`
  sectors; the game never links against FFmpeg's unstable internal ABI and
  does not require pre-converted FMV files.

Verify FFmpeg before starting the game with `ffmpeg -version`. Windows users
can install it with `winget install Gyan.FFmpeg`; common Linux distributions
provide an `ffmpeg` package. If FFmpeg is unavailable, gameplay remains
available and the port falls back past movies, recording the failure in
`rage-racer.log`.

Normal runtime settings are read from `rage-port.ini`. Use `--config FILE` for
an alternative file and `--set section.key=value` for an individual override.
Press `F10` while playing to switch between classic and modern rendering; the
key is configurable as `[video] toggle_renderer_key`.

Input defaults are loaded from `rage-input.cfg` when present.  The port keeps
game saves and the selected disc location in the user's local application data.
Each normal launch appends diagnostics to `rage-racer.log` in the platform's
application-state directory. Configure `[diagnostics] log` to choose another
file. The disc can likewise be pinned with `[disc] cue`; the legacy
`RAGE_PORT_LOG_PATH` and `RAGE_PORT_DISC_CUE` overrides remain available for
automation.

## Keyboard controls

The default bindings emulate the original PlayStation pad:

| Game action | Default key |
|---|---|
| Accelerate (`CROSS`) | `X` |
| Brake / reverse (`SQUARE`) | `Z` |
| Change camera (`TRIANGLE`) | `S` |
| Handbrake (`CIRCLE`) | `D` |
| Steer | Arrow keys |
| Shift down (`L1`) | `Q` |
| Shift up (`R1`) | `R` |
| Look back (`L2`) | `W` |
| Unused PS1 shoulder input (`R2`) | `E` |
| Start / confirm | Return |
| Select / back | Backspace |
| L3 / R3 | `1` / `2` |

Put `rage-input.cfg` in the user configuration directory to override any of
these bindings. This is `~/Library/Application Support/Rage Racer/` on macOS,
`${XDG_CONFIG_HOME:-$HOME/.config}/rage-racer/` on Linux, and
`%APPDATA%\Rage Racer\` on Windows. A bundled file beside the executable is
used when no user override exists.
Button names are PlayStation pad names, key names are SDL key names, and lines
beginning with `#` or `;` are comments.  For example:

```ini
# rage-input.cfg
L2=W
R2=E
L1=Q
R1=R
TRIANGLE=S
CIRCLE=D
CROSS=X
SQUARE=Z
SELECT=Backspace
L3=1
R3=2
START=Return
UP=Up
RIGHT=Right
DOWN=Down
LEFT=Left
```

## Known limitations

- Controller configuration currently retains the original preset-oriented
  UI. Full per-action controller remapping is planned after 0.4-alpha.
- Left-stick-as-D-pad and analog steering are not exposed yet. The eventual
  analog path will share the existing NeGcon calibration and steering
  mechanics instead of adding a second physics input model.
- Release builds are unsigned. Operating systems may require the user to
  explicitly allow the downloaded application.
- Only the legally obtained PAL/Europe disc (`SCES-006.50`) is supported and
  no game data is included in release archives.

## Related repositories

- [rage-racer-decomp](https://github.com/khasinski/rage-racer-decomp) — clean-room
  decompilation and reference implementation
- [rage-racer-psyz](https://github.com/khasinski/rage-racer-psyz) — PSY-Z fork
  used by this port

## License

See [LICENSE.md](LICENSE.md).  Game code and data remain the property of their
respective owners.  This project does not distribute any original game assets.
