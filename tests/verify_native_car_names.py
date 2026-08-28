#!/usr/bin/env python3
"""Keep native car names in the model-index order stored by retail."""

from __future__ import annotations

import re
import sys
from pathlib import Path


def main() -> int:
    source = (Path(sys.argv[1]) / "src/port/native_game_state.c").read_text(
        encoding="utf-8")
    values = dict(re.findall(
        r'static char (g_CarName\w+)\[\] = "([A-Z]+)";', source))
    match = re.search(
        r"char \*g_NativeCarNames\[13\] = \{([^}]*)\};", source,
        re.DOTALL)
    if match is None:
        raise AssertionError("g_NativeCarNames does not have thirteen entries")
    names = [values[token] for token in re.findall(r"g_CarName\w+", match.group(1))]
    expected = [
        "ERRISO", "ABEILLE", "PEGASE", "ESPERANZA", "ACCERON", "BAYONET",
        "HIJACK", "FATALITA", "ISTANTE", "GHEPARDO", "VAINQURE", "BULSHADE",
        "SQUALDON",
    ]
    if names != expected:
        raise AssertionError(f"native car-name order is {names}, expected {expected}")
    print("native car names match the retail model-index order")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
