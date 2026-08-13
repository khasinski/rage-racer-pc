# Rage Racer PC

Native macOS port of *Rage Racer* (PAL / Europe, `SCES-006.50`).  It is based
on the complete clean-room decompilation in
[rage-racer-decomp](https://github.com/khasinski/rage-racer-decomp).

`rage-racer-pc` contains the host port, macOS packaging and the runtime
compatibility layer.  The decompilation remains the reference source for the
original PlayStation release.

## 0.1-alpha

The first alpha provides unsigned builds for arm64 macOS and x86-64 Linux. It
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

The resulting executable is `build/release/rage-racer` on Linux and
`build/release/Rage Racer.app` on macOS. Release downloads are supplied for
macOS arm64 and Linux x86-64.

## Runtime requirements

- macOS on Apple Silicon (arm64), or a glibc-based x86-64 Linux distribution
- A legally obtained PAL Rage Racer disc image, with its `.cue` sheet and
  referenced track files kept together

Input defaults are loaded from `rage-input.cfg` when present.  The port keeps
game saves and the selected disc location in the user's local application data.

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

Put `rage-input.cfg` in `~/Library/Application Support/Rage Racer/` to override
any of these bindings (a file in the current directory is also accepted). On
Linux the same directory is under `$HOME/Library/Application Support/` for this
alpha.
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

## Related repositories

- [rage-racer-decomp](https://github.com/khasinski/rage-racer-decomp) — clean-room
  decompilation and reference implementation
- [rage-racer-psyz](https://github.com/khasinski/rage-racer-psyz) — PSY-Z fork
  used by this port

## License

See [LICENSE.md](LICENSE.md).  Game code and data remain the property of their
respective owners.  This project does not distribute any original game assets.
