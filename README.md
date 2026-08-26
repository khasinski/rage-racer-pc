# Rage Racer PC

Native Windows, Linux and macOS port of *Rage Racer* (PAL / Europe,
`SCES-006.50`). It is based
on the complete clean-room decompilation in
[rage-racer-decomp](https://github.com/khasinski/rage-racer-decomp).

`rage-racer-pc` contains the host port, platform packaging and the runtime
compatibility layer.  The decompilation remains the reference source for the
original PlayStation release.

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
as ZIP archives for macOS arm64, Linux x86-64 and Windows x86-64. Every archive
contains the documented `rage-port.ini` modern-renderer preset.

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
Movies are decoded in process, so no external tools are needed.

On first launch the port looks for a `.cue` sheet next to the executable and
uses it when one is there. Otherwise it asks, and remembers the answer. To pick
a different disc later, start the game with `--set disc.choose=1`, or pin one
with `[disc] cue` in `rage-port.ini`.

Normal runtime settings are read from `rage-port.ini`. Use `--config FILE` for
an alternative file and `--set section.key=value` for an individual override.

There are exactly two renderer modes. `classic` presents the faithful
PS1-compatible output. `modern` presents native `RenderWorld` geometry with a
normal depth buffer, configurable internal scale, optional 16:9 presentation,
an interpolated frame rate and FXAA. It never falls back to captured PS1 3D.
The supplied file defaults to `modern`:

```ini
[video]
renderer = modern
internal_scale = 4
aspect = 16:9
fps = vsync
post = fxaa
toggle_renderer_key = F10
```

Modern mode requires an imported native-asset cache. Generate it from Track 01
of your legally obtained disc and put it beside the executable:

```sh
python3 tools/assetbrowser/extract.py \
  "/path/to/Rage Racer (Europe) (Track 01).bin" \
  --out build/release/native-assets \
  --no-raw --no-audio --no-vram --no-fmv
```

For a cache stored elsewhere, set `[modern] assets = /path/to/native-assets`.
An absent or incompatible cache is an error in modern mode; the game will not
silently launch a different 3D renderer.

Press `F10` while playing to switch between classic and modern rendering
without restarting the race; the key is configurable as
`[video] toggle_renderer_key`. `F4` goes full screen and back.

In the modern renderer the rear-view mirror is a native second camera. It
renders the complete semantic track and traffic scene into its own small color
and depth target, reflects it left-to-right, and composites it under the
original HUD frame. It does not reuse the PS1 mirror ordering table, GTE
matrix, or classic visibility list.

The port keeps
game saves and the selected disc location in the user's local application data.
For compatibility with older portable releases, an existing `bu00` directory
beside the executable takes priority (beside the `.app` bundle on macOS). The
game does not create a portable directory automatically. Otherwise memory-card
saves are regular files in the `bu00` subdirectory: under
`~/Library/Application Support/Rage Racer/` on macOS,
`${XDG_STATE_HOME:-$HOME/.local/state}/rage-racer/` on Linux, and
`%APPDATA%\Rage Racer\` on Windows. They no longer depend on the directory
from which the executable was launched.
Each normal launch appends diagnostics to `rage-racer.log` in the platform's
application-state directory. Configure `[diagnostics] log` to choose another
file. The disc can likewise be pinned with `[disc] cue`; the legacy
`RAGE_PORT_LOG_PATH` and `RAGE_PORT_DISC_CUE` overrides remain available for
automation.

Rage Racer uses mixed-mode CD audio. The selected CUE must describe Track 01
and audio Tracks 02–17; a CUE containing only the data track can run the game
and FMVs but cannot provide the Grand Prix intro or race soundtrack. Keep all
BIN files referenced by the CUE together and select that full CUE on first run.

## 日本版について

日本版（NTSC-J）のディスクでも動作しますが、一部のメッセージは表示されません。
メモリーカードの確認、車の説明、ネジコンの調整画面の説明などが空欄になります。

このポートは欧州版の実行ファイルを移植したものです。欧州版はアルファベットの
フォントから一文字ずつ文章を組み立てますが、日本版は同じビデオメモリの位置に、
文章そのものを描いた画像を置いています。日本版のディスクには欧州版が必要とする
フォントが存在しないため、これらのメッセージは描画されません。

画面の配置の問題ではなく、二つの版がテキストを別の方法で持っているためです。
それ以外のメニュー、レース、動画は日本版でも問題なく動作します。

### About the Japanese release

An NTSC-J disc runs, but the messages the game builds from its shared text
sheet stay blank: the memory card prompts, the car descriptions, and the NeGcon
calibration instructions.

The port carries the European executable. That one keeps an alphabet at a fixed
place in video memory and sets a message one letter at a time. The Japanese
release puts whole pre-drawn Japanese sentences in that same place instead, so
the alphabet the European code looks for is not on the disc at all and those
messages draw nothing. It is not a matter of where the text sits on screen; the
two releases hold text in different ways. Everything else runs.

## Controls

### Gamepad

A connected gamepad is driven as a NeGcon, the analog controller the game was
built around. The left stick steers in proportion to how far it is pushed and
the triggers meter the throttle and the brake, while the d-pad still steers at
full lock so either input can be used. Dead zone and steering sensitivity are
the game's own NeGcon settings, adjustable in the OPTIONS menu as "play" and
"max twist". Set `[input] analog = false` to keep a pad on the digital
mapping instead.

The response of each axis can be shaped, using the same four settings and the
same ranges DuckStation gives a NeGcon, so values can be copied between them.
The axes are `steering`, `throttle` and `brake`, which are the twist and the I
and II buttons of a real NeGcon:

```ini
[input]
steering_deadzone = 0.1
steering_linearity = 0.5
```

`deadzone` runs 0 to 0.99 and `saturation` 0.01 to 1, both as fractions of the
travel. `scaling` runs 0.01 to 10. `linearity` runs -2 to 2 and is an exponent
rather than a fraction: 0 is a straight line, above it softens the middle of
the throw, below it sharpens it. A value out of range is reported and ignored.

These sit in front of the game's own NeGcon dead zone and twist range, which
stay where they always were, in the OPTIONS menu.

### Keyboard

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

Rebind any of these under `[input]` in `rage-port.ini`, one entry per
PlayStation button. Key names are SDL key names:

```ini
[input]
cross = X
square = Z
triangle = S
circle = D
up = Up
right = Right
down = Down
left = Left
l1 = Q
r1 = R
l2 = W
r2 = E
select = Backspace
start = Return
l3 = 1
r3 = 2
```

A copy of `rage-port.ini` in the user configuration directory takes precedence
over the one beside the executable. That directory is
`~/Library/Application Support/Rage Racer/` on macOS,
`${XDG_CONFIG_HOME:-$HOME/.config}/rage-racer/` on Linux, and
`%APPDATA%\Rage Racer\` on Windows.

## Modding

`rage-extract` unpacks the game archive into a directory you can edit. It needs
the archive as a plain file, which the port writes out of a mounted disc:

```sh
"Rage Racer" --set tools.dump_archive=RAGE.BIN
rage-extract RAGE.BIN mymod/
```

The result holds every archive entry under `raw/`, a `manifest.json` describing
them, and every image the entries carry decoded to PNG under `textures/`. Each
PNG sits next to a JSON sidecar recording the VRAM rectangle, the colour depth
and the palette it came from, which is what putting an edited image back needs.

Edit the PNGs and point the game at the directory:

```sh
"Rage Racer" --set mods.directory=mymod
```

Edited images are applied to the assets as they load, so a mod can be nothing
but PNGs and the directory is never written to. `rage-pack mymod/` does the
same to the files, for looking at the result or for shipping a mod as packed
assets rather than as images.

The modern renderer also supports source-independent texture mods. These do
not need an extracted `raw/` tree, PS1 palettes, VRAM coordinates or texture
sidecars. Put a `mod.toml` at the root selected by `mods.directory`:

```toml
[mod]
id = "example-hd-textures"

[textures]
"track.big1.terrain.material.3" = "textures/tunnel-wall.png"
"track.big1.course.material.7.variant.1" = "textures/sign-night.png"
```

The first mapping replaces every runtime variant of that material. The second
is more specific and replaces only variant 1. Exact variant mappings win over
the base material mapping. PNGs may use any dimensions up to 16384×16384 and
are decoded directly to RGBA; they are not quantized back to a PS1 palette.
Paths are relative to the mod root and use `/`, including on Windows.

Material IDs describe game content rather than archive entries. Track IDs use
`track.<big|mid|hi|oval><1-6>.<course|terrain|model-bank-1|model-bank-2>.material.<n>`;
player-car materials use `car.<0-31>.material.<n>`. Add
`.variant.<n>` to target one variant. A mapping not supplied by the mod falls
through to the immutable RGBA material imported from the original game. The
older extracted-asset workflow remains supported by the same setting.

Packing only touches texels an image actually changed, so running it over an
extract nobody edited rewrites nothing. A PNG has to keep the size it was
extracted at, since that size is fixed by where the texture lives in video
memory, and colours are matched against the palette already in the asset. A
palette usually sits in a strip of video memory that other textures sample
from, so it is left alone: recolouring one to suit a single image would repaint
parts of the game that have nothing to do with it. Colours with no match are
reported and the closest one is used.

Every archive entry present under `raw/` is used in place of the one on the
disc, and anything missing falls back to the disc, so a mod only has to carry
what it changes. Replacements may be larger than the original: cursors in the
loader advance by the size a load reports, and the asset region has far more
room than the console gave it. A replacement that would not fit the buffer it
loads into is refused with a message naming both sizes, and the original is
used instead.

One case the check cannot see: car models load into a fixed slot of 0x20000
bytes and the model for the other slot sits directly behind it, so a car pack
past that size will reach into its neighbour. Retail's largest is 0xD4A0.

Assets are read once at startup, so changes take effect on the next launch.

Extracted files are game data. Share the changes you made, not the extract.

## Running the runtime tests

`ctest -L portable` needs nothing but a build. The rest of the suite drives the
game against real PAL data, which is not in the repository, so those tests fail
until it is staged:

```sh
"Rage Racer" --set tools.dump_archive=assets/PAL/RAGE.BIN
ctest --test-dir build
```

`assets/` is ignored by git, and the archive must not be committed. With it in
place the runtime tests reach a race and check what they were written to check;
without it they stop at the title screen, because every asset read returns
nothing.

Ten of the eleven movies sit hours into a playthrough. `--set
diagnostics.fmv_stream=N` puts movie `N` where the opening one is asked for, so
one can be watched without earning it; `--set diagnostics.fmv_trace=1` reports
each frame as it is decoded.

The movies and the CD-DA music are not in that archive, they are on the disc,
so the tests covering them also want a cue:

```sh
RAGE_PORT_DISC_CUE="/path/to/Rage Racer.cue" ctest --test-dir build
```

## Known limitations

- Controller configuration currently retains the original preset-oriented
  UI. Full per-action controller remapping is still to come.
- Release builds are unsigned. Operating systems may require the user to
  explicitly allow the downloaded application.
- The legally obtained PAL/Europe disc (`SCES-006.50`) is the reference
  release. A Japanese (NTSC-J) disc runs, without the messages described above.
  No game data is included in release archives.

## Related repositories

- [rage-racer-decomp](https://github.com/khasinski/rage-racer-decomp) — clean-room
  decompilation and reference implementation
- [rage-racer-psyz](https://github.com/khasinski/rage-racer-psyz) — PSY-Z fork
  used by this port

## License

See [LICENSE.md](LICENSE.md).  Game code and data remain the property of their
respective owners.  This project does not distribute any original game assets.
