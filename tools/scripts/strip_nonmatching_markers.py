#!/usr/bin/env python3
"""Remove splat's `nonmatching` markers from the freshly split assembly.

splat writes one above every symbol it disassembles. That is its default for
anything it produces and carries no claim either way, but the macro it names
means "this symbol does not match yet" - and in this tree nothing is in that
state: `make check` links an executable byte-identical to retail.

Leaving them in is not free either. The macro declares <symbol>.NON_MATCHING as
a second global object at the symbol's own address, and two symbols sharing an
address stop objdiff pairing that section at all, which reported the whole
129 KB game data table as unmatched.

Only the marker line goes; the instruction and data words are untouched, so the
build is unaffected. include/macro.inc still defines the macro, so a symbol
that genuinely does not match can still be marked as one by hand.
"""

import argparse
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]

MARKER = re.compile(r'^nonmatching\s+.*$\n?', re.MULTILINE)


def strip(text):
    return MARKER.sub('', text)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--version', default='PAL')
    parser.add_argument('--basename', default='main')
    args = parser.parse_args(argv)

    root = ROOT / 'asm' / args.version / args.basename
    if not root.is_dir():
        raise SystemExit('%s missing - run `make split` first' % root)

    stripped = 0
    for source in sorted(root.rglob('*.s')):
        text = source.read_text()
        cleaned = strip(text)
        if cleaned != text:
            source.write_text(cleaned)
            stripped += 1
    print('stripped nonmatching markers from %d file(s)' % stripped)
    return 0


if __name__ == '__main__':
    sys.exit(main())
