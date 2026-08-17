#!/usr/bin/env python3
"""Verify typed host-state storage against the checked ABI manifest."""

import json
import re
import sys
from pathlib import Path


def declarations(text: str) -> dict[str, int]:
    pattern = re.compile(
        r"^(?:const )?unsigned char\s+(g_\w+)\[(\d+)\]"
        r"(?:\s+__attribute__\(\(aligned\(16\)\)\))?",
        re.MULTILINE,
    )
    return {name: int(size) for name, size in pattern.findall(text)}


def main() -> int:
    root = Path(sys.argv[1])
    manifest = json.loads((root / "config/host-state-abi.json").read_text())
    sources = "\n".join(
        (root / relative).read_text()
        for relative in ("src/port/host_state.c", "src/port/native_initialized_state.c")
    )
    actual = declarations(sources)
    errors = []
    for name, expected in manifest["symbols"].items():
        if actual.get(name) != expected:
            errors.append(f"{name}: expected {expected}, found {actual.get(name)}")
    for name in manifest["forbidden_detached_aliases"]:
        if name in actual:
            errors.append(f"detached PS1 address alias is allocated: {name}")
    if len(actual) < 1200:
        errors.append(f"host-state scan unexpectedly found only {len(actual)} symbols")
    if errors:
        raise AssertionError("\n".join(errors))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
