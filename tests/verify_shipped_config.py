#!/usr/bin/env python3
"""The shipped configuration does not turn on experimental camera or distance.

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
    camera = re.search(
        r"^\s*chase_turn_lookahead\s*=\s*([0-9.]+)", text, re.MULTILINE)
    if camera is None:
        raise AssertionError("rage-port.ini no longer sets chase_turn_lookahead")
    lookahead = float(camera.group(1))
    if lookahead != 0.0:
        raise AssertionError(
            "rage-port.ini changes the retail third-person camera by default: "
            f"chase_turn_lookahead={lookahead}")
    linearity = re.search(
        r"^\s*steering_linearity\s*=\s*([-0-9.]+)", text, re.MULTILINE)
    if linearity is None or float(linearity.group(1)) != 0.5:
        raise AssertionError(
            "rage-port.ini must ship steering_linearity=0.5")
    marker = re.search(
        r"^\s*marker_capture\s*=\s*(\S+)", text, re.MULTILINE)
    if marker is None or marker.group(1).lower() not in ("false", "off", "no", "0"):
        raise AssertionError(
            "rage-port.ini must disable M-key marker capture by default")
    print(
        f"shipped draw_distance is {value}; chase lookahead is disabled; "
        "steering linearity is 0.5; M-key diagnostics are disabled")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
