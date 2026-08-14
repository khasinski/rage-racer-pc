#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SOURCE_ROOT = ROOT / "src/main/PAL/main"
HEADER_ROOT = ROOT / "include/game"
BASELINE = ROOT / "tools/code_debt_baseline.json"

PATTERNS = {
    "byte_pointer_arithmetic": re.compile(
        r"\(\s*(?:const\s+|volatile\s+)?(?:u8|s8|char)\s*\*\s*\)"
        r"(?!\s*\()[^;=\n]*\+"
    ),
    "raw_offset_dereferences": re.compile(
        r"\*\s*\([^)]*\*\s*\)\s*\("
        r"(?:[^();\n]|\([^();\n]*\))*?[+-]\s*(?:0x[0-9A-Fa-f]+|[1-9][0-9]*)"
    ),
    "pointer_integer_casts": re.compile(
        r"\(\s*(?:s32|u32|long|unsigned\s+long)\s*\)\s*"
        r"(?:[A-Za-z_&]|\(\s*[A-Za-z_&])"
    ),
    "address_reinterpret_casts": re.compile(
        r"\*\s*\(\s*(?:(?:const|volatile)\s+)*"
        r"(?:u8|s8|u16|s16|u32|s32)\s*\*\s*\)\s*\(?\s*&"
    ),
    "aggregate_address_casts": re.compile(
        r"\(\s*(?:(?:const|volatile)\s+)*[A-Z][A-Za-z0-9_]*\s*\*\s*\)"
        r"\s*\(?\s*&"
    ),
    "explicit_pointer_casts": re.compile(
        r"\(\s*(?:(?:const|volatile)\s+)*"
        r"(?:void|char|u8|s8|u16|s16|u32|s32|[A-Z][A-Za-z0-9_]*)"
        r"\s*\*+\s*\)"
    ),
    "manual_byte_offsets": re.compile(r"\.byteOffset\b"),
    "address_integer_arithmetic": re.compile(
        r"\b[A-Za-z_][A-Za-z0-9_]*Address\.(?:value|offset)\s*"
        r"(?:\+=|-=|=[^;\n]*(?:\+|<<))"
    ),
    "negative_pointer_indexing": re.compile(
        r"\b[A-Za-z_][A-Za-z0-9_]*(?:\s*->\s*[A-Za-z_][A-Za-z0-9_]*)?"
        r"\s*\[\s*-[1-9][0-9]*\s*\]"
    ),
    "field_macros": re.compile(r"\b(?:(?:FIELD|RAW_FIELD)\w*|RAW)\s*\("),
    "register_pins": re.compile(
        r"\bregister\b[^;\n]*\basm\s*\(\s*\"(?:\$\d+|zero|at|v[01]|a[0-3]|t[0-9]|s[0-8]|k[01]|gp|sp|fp|ra)\""
    ),
    "empty_barriers": re.compile(
        r"\b(?:__asm__|asm)\s*(?:volatile\s*)?\(\s*\"\""
    ),
    "statement_expressions": re.compile(r"\(\s*\{"),
    "asm_aliases": re.compile(r"\.globl\s+func_[0-9A-Fa-f]+"),
    "header_asm_aliases": re.compile(
        r'^\s*extern\b[^;\n]*\basm\s*\(\s*"(?!0x)', re.MULTILINE
    ),
    "unknown_fields": re.compile(r"\b(?:field_[0-9A-Fa-f]+|unk[0-9A-Fa-f]+)\b"),
    "externs_in_c": re.compile(r"^\s*extern\b", re.MULTILINE),
    "declaration_overrides": re.compile(
        r"^\s*#define\s+GAME_[A-Z0-9_]+_(?:TYPE|QUALIFIER|DECL)\b", re.MULTILINE
    ),
}

HEADER_PATTERNS = {
    "header_byte_offset_members": re.compile(r"\bs32\s+byteOffset\s*;"),
    "header_address_integer_arithmetic": re.compile(
        r"\.(?:value|offset)\s*(?:\+=|-=|=[^;\n]*(?:\+|<<))"
    ),
}


def strip_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.DOTALL)
    return re.sub(r"//[^\n]*", "", text)


def count_debt(source_root: Path = SOURCE_ROOT, header_root: Path | None = None) -> dict[str, int]:
    totals = {name: 0 for name in PATTERNS}
    totals.update({name: 0 for name in HEADER_PATTERNS})
    for path in sorted(source_root.rglob("*.c")):
        source = path.read_text(errors="ignore")
        text = strip_comments(source)
        for name, pattern in PATTERNS.items():
            if name == "register_pins" and "HANDWRITTEN_ASM" in source:
                continue
            totals[name] += len(pattern.findall(text))
    if header_root is None and source_root == SOURCE_ROOT:
        header_root = HEADER_ROOT
    if header_root is not None:
        for path in sorted(header_root.rglob("*.h")):
            text = strip_comments(path.read_text(errors="ignore"))
            totals["header_asm_aliases"] += len(
                PATTERNS["header_asm_aliases"].findall(text)
            )
            for name, pattern in HEADER_PATTERNS.items():
                totals[name] += len(pattern.findall(text))
    return totals


def main() -> int:
    parser = argparse.ArgumentParser(description="Measure decompilation scaffolding in game C")
    parser.add_argument("--check", action="store_true", help="fail if any count exceeds baseline")
    parser.add_argument("--json", action="store_true", help="print machine-readable output")
    args = parser.parse_args()

    current = count_debt()
    if args.json:
        print(json.dumps(current, indent=2, sort_keys=True))
    else:
        for name, count in current.items():
            print(f"{name:28} {count:5}")

    if not args.check:
        return 0

    baseline = json.loads(BASELINE.read_text())
    increases = {
        name: (baseline.get(name, 0), count)
        for name, count in current.items()
        if count > baseline.get(name, 0)
    }
    if increases:
        for name, (before, after) in increases.items():
            print(f"increased: {name}: {before} -> {after}")
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
