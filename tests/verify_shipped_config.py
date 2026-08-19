#!/usr/bin/env python3
"""The shipped configuration does not turn on the extended draw distance.

Above 1 the modern renderer draws faces the game rejects by depth, and at some
places on Lakeside Gate one of them covers a quarter of the view in black. The
option stays, since it is worth finishing, but it is not what a player gets by
opening the box.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path


def main() -> int:
    text = (Path(sys.argv[1]) / "rage-port.ini").read_text()
    match = re.search(r"^\s*draw_distance\s*=\s*([0-9.]+)", text, re.MULTILINE)
    if match is None:
        raise AssertionError("rage-port.ini no longer sets draw_distance")
    value = float(match.group(1))
    if value > 1.0:
        raise AssertionError(
            f"rage-port.ini ships draw_distance={value}, which draws geometry "
            "the game rejects and shows a black plane on some corners")
    print(f"shipped draw_distance is {value}, the original distance")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
