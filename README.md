# Rage Racer PC

Native macOS port of *Rage Racer* (PAL / Europe, `SCES-006.50`).  It is based
on the complete clean-room decompilation in
[rage-racer-decomp](https://github.com/khasinski/rage-racer-decomp).

`rage-racer-pc` contains the host port, macOS packaging and the runtime
compatibility layer.  The decompilation remains the reference source for the
original PlayStation release.

## 0.1-alpha

The first alpha is an unsigned arm64 macOS application.  It contains no game
data.  On first launch, the app asks the player to locate the `.cue` sheet for
their legally obtained PAL copy of Rage Racer, then saves that local choice for
future launches.  The game image and its data files are never uploaded to this
repository or attached to GitHub Releases.

## Build from source

The project uses CMake and its pinned PSY-Z compatibility layer:

```sh
git clone --recurse-submodules https://github.com/khasinski/rage-racer-pc.git
cd rage-racer-pc
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release --parallel
```

The resulting executable is `build/release/rage-racer`.  A macOS app bundle is
provided with the GitHub release.

## Runtime requirements

- macOS on Apple Silicon (arm64)
- A legally obtained PAL Rage Racer disc image, with its `.cue` sheet and
  referenced track files kept together

Input defaults are loaded from `rage-input.cfg` when present.  The port keeps
game saves and the selected disc location in the user's local application data.

## Related repositories

- [rage-racer-decomp](https://github.com/khasinski/rage-racer-decomp) — clean-room
  decompilation and reference implementation
- [rage-racer-psyz](https://github.com/khasinski/rage-racer-psyz) — PSY-Z fork
  used by this port

## License

See [LICENSE.md](LICENSE.md).  Game code and data remain the property of their
respective owners.  This project does not distribute any original game assets.
