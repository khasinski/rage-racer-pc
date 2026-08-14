# Rage Racer Decompilation

![functions](https://img.shields.io/endpoint?url=https%3A%2F%2Fraw.githubusercontent.com%2Fkhasinski%2Frage-racer-decomp%2Fmain%2Fdocs%2Fbadges%2Ffunctions.json) ![code](https://img.shields.io/endpoint?url=https%3A%2F%2Fraw.githubusercontent.com%2Fkhasinski%2Frage-racer-decomp%2Fmain%2Fdocs%2Fbadges%2Fcode.json)

A complete, byte-exact decompilation of the PAL PlayStation release of
Rage Racer, target `SCES-006.50`.

This repository is source-only. It does not contain disc images, extracted game
assets, CDDA/XA data, Sony PSYQ SDK files, compiler binaries, generated split
output, local post-build rewrite passes, or decompilation scratch work.
Contributors must provide their own legally obtained copy of the game and local
toolchains.

## Target

| Field | Value |
|---|---|
| Game | Rage Racer, PAL / Europe |
| Executable | `SCES_006.50` |
| Target path | `assets/PAL/main.exe` |
| SHA-1 | `2913e15648eddef40821c5f666460abc04155ee6` |
| Entry PC | `0x800630B4` |
| Text VRAM | `0x80010000..0x8009B000` |

The USA executable `SLUS_004.03` is kept as a comparison target with SHA-1
`2661e8bf18d209c98fd70d33e18ab88b10abd52b`.

## Progress

The linked executable is byte-identical to retail, and every object is compared
against the game function by function.

| Scope | Functions | Code bytes | Data bytes |
|---|---:|---:|---:|
| Game code | 705 / 705 (100.00%) | 100.00% | 99.88% |
| PsyQ libraries | 497 / 497 (100.00%) | 100.00% | 0.00% |
| **Whole executable** | **1202 / 1202 (100.00%)** | **100.00%** | **99.82%** |

### What "matched" means here

Two separate checks, and neither is a judgement about the source text:

- `make check` links the executable and compares its SHA-1 against retail. This
  is the whole-image claim, and it either holds or it does not.
- `make report` disassembles the retail executable into one `.s` per
  translation unit, assembles those, and has
  [objdiff](https://github.com/encounter/objdiff) compare them with the objects
  this tree compiles, function by function. That report is what feeds the table
  above and [decomp.dev](https://decomp.dev/khasinski/rage-racer-decomp). See
  `tools/scripts/gen_expected.py`.

Earlier revisions of this file called a function decompiled when its source
carried no `INCLUDE_ASM` and no inline assembly. That describes how the source
is written, not whether it reproduces the game, and using it as the headline
number overstated the result. It is still counted, separately and under its own
name: **237 of 337 translation units are plain C**, the rest holding
hand-written assembly the original shipped that way.

### Where the remaining gap is

Both halves of the binary are reported. decomp.dev requires a report to account
for every function in the executable so that projects on the site are measured
the same way, so the game and the PsyQ libraries are separated with a progress
category rather than by leaving the libraries out.

Every function matches. What is left is **2788 bytes of data**, all of it jump
tables: gcc emits a `switch` table into `.rodata` and addresses it relative to
the section without giving it a name, while the disassembler has to invent one
for the branch target. objdiff pairs data by symbol name, so there is nothing
to pair it against. The bytes themselves are identical, and the linked image
proves it.

Regenerate everything with `make report progress`.

## Layout

- `configs/PAL/` - splat config, symbols, relocs, checksum for `SCES_006.50`.
- `configs/USA/` - comparison target metadata for `SLUS_004.03`.
- `src/main/` - decompiled C translation units for the main executable.
- `include/` - project headers and local PSYQ-compatible declarations.
- `tools/scripts/` - project-specific build and analysis helpers.

Source files are named after their subject, not after whichever function happens
to sit first in them. Generated directories such as `asm/`, `linkers/`, `build/`,
`assets/`, and `disc/` are intentionally ignored, along with local
scratch/proposal work.

## Toolchain

Linking and assembling use standard GNU binutils on `PATH`:

```text
mipsel-none-elf-as
mipsel-none-elf-ld
mipsel-none-elf-objcopy
```

Compiling matching C uses old PlayStation GCC variants plus
[maspsx](https://github.com/mkst/maspsx) to translate Psy-Q style assembly to
GNU assembler input. The wrapper fetches public helper tooling into
`build/toolchain/` when needed, but does not publish compiler binaries or SDK
material in this repository.

Expected local inputs:

```text
assets/PAL/main.exe
assets/USA/main.exe        # optional comparison target
```

## Quick Start

```sh
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt

mkdir -p assets/PAL
cp /path/to/your/SCES_006.50 assets/PAL/main.exe

make split VERSION=PAL
make build VERSION=PAL
make check VERSION=PAL
make report progress
```

`make split` regenerates `asm/` and `linkers/` from the user-supplied
executable, `make build` compiles and links `build/PAL/main.exe`, and
`make check` verifies its SHA-1 against retail. `make report` disassembles the
retail executable into a second set of objects and has objdiff compare them
unit by unit; `make progress` turns that report into the badge JSON and the
table shown above.

## License

See [LICENSE.md](LICENSE.md). Game code and data remain the property of their
respective owners; this repository contains only clean-room reimplementation
source.
