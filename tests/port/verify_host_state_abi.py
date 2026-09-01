#!/usr/bin/env python3
"""Verify typed host-state storage against the checked ABI manifest."""

import json
import re
import sys
from pathlib import Path


BLOB = re.compile(
    r"^(?:const )?unsigned char\s+(g_\w+)\[(\d+)\]"
    r"(?:\s+__attribute__\(\(aligned\(16\)\)\))?",
    re.MULTILINE,
)

# State whose header declares a plain scalar is defined as that scalar, so a
# debugger shows a value rather than eight bytes. Those carry no pinned size;
# the manifest pins the ones that are still raw storage.
SCALAR = re.compile(
    r"^(?:volatile )?(?:s8|u8|s16|u16|s32|u32|f32|int|char|short)\s+(g_\w+)\s*(?:=|;)",
    re.MULTILINE,
)


# An array of a typed element is state like any other. It carries no pinned
# size here: what BLOB pins is the raw unsigned-char storage that has not been
# given a type yet, and an array that has one is no longer that.
# State whose header names a struct is defined as that struct, so a reader sees
# the positions and the vectors rather than the little-endian bytes they were
# assembled from. Like a typed array it carries no pinned size: the type and
# the element count say it.
STRUCT_ARRAY = re.compile(
    r"^(?:const )?(?:volatile )?(?:Vec4|SVec|DVec|Rect|CarEntry|CarHullPoint"
    r"|FontGlyph|ShuttlePath|SkyTileUV|StartGridSceneryStep"
    r"|GameEnvironmentColors)\s+(g_\w+)\[",
    re.MULTILINE,
)

TYPED_ARRAY = re.compile(
    r"^(?:const )?(?:volatile )?(?:s8|u8|s16|u16|s32|u32|f32|int|char|short)"
    r"\s+(g_\w+)\[",
    re.MULTILINE,
)

TYPED_POINTER = re.compile(
    r"^(?:const )?(?:TimedDrawCommand)\s+\*(g_\w+)\s*(?:=|;)",
    re.MULTILINE,
)


def declarations(text: str) -> dict[str, int]:
    found = {name: int(size) for name, size in BLOB.findall(text)}
    for name in (SCALAR.findall(text) + TYPED_ARRAY.findall(text)
                 + TYPED_POINTER.findall(text)
                 + STRUCT_ARRAY.findall(text)):
        found.setdefault(name, 0)
    return found


def main() -> int:
    root = Path(sys.argv[1])
    manifest = json.loads((root / "config/host-state-abi.json").read_text())
    source_text = {
        relative: (root / relative).read_text()
        for relative in manifest["minimum_symbols"]
    }
    actual = {
        name: size for text in source_text.values()
        for name, size in declarations(text).items()
    }
    errors = []
    for name, expected in manifest["symbols"].items():
        if actual.get(name) != expected:
            errors.append(f"{name}: expected {expected}, found {actual.get(name)}")
    for name in manifest["forbidden_detached_aliases"]:
        if name in actual:
            errors.append(f"detached PS1 address alias is allocated: {name}")
    for relative, minimum in manifest["minimum_symbols"].items():
        count = len(declarations(source_text[relative]))
        if count < minimum:
            errors.append(f"{relative}: expected at least {minimum} symbols, found {count}")
    if errors:
        raise AssertionError("\n".join(errors))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
