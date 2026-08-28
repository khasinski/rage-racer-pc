#!/usr/bin/env python3
"""Make the PS-EXE header relocatable.

splat emits the header with the entry point and the loaded size baked in as
literals. Both are correct for the retail layout and wrong the moment anything
changes size: the BIOS loads exactly `t_size` bytes, so a larger image would be
truncated on load and a smaller one would read past its end. That is invisible
to the linker, which is why the relocation work did not catch it.

Rewriting them as expressions is byte-neutral today, because
`main_ROM_END - main_ROM_START` is the value already there.
"""
import argparse
import re
import sys
from pathlib import Path

ENTRY_SYM = "D_800630B4"  # the .text array holding the entry stub, see boot/_start.s

REPLACEMENTS = [
    # (literal as splat writes it, expression, what it is)
    (r"\.word\s+0x800630B4(\s*/\*.*?\*/)?",
     f".word {ENTRY_SYM}       /* Initial PC */",
     "entry point"),
    (r"\.word\s+0x0008B000(\s*/\*.*?\*/)?",
     ".word main_LOAD_SIZE       /* .text size */",
     "loaded size"),
]


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--version", default="PAL")
    ap.add_argument("--basename", default="main")
    args = ap.parse_args()

    header = Path("asm") / args.version / args.basename / "header.s"
    if not header.exists():
        return 0

    text = header.read_text()
    changed = []
    for pattern, replacement, what in REPLACEMENTS:
        text, n = re.subn(pattern, replacement, text, count=1)
        if n:
            changed.append(what)

    header.write_text(text)
    if changed:
        print(f"symbolised PS-EXE header: {', '.join(changed)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
