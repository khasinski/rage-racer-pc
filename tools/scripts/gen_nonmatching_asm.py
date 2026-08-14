#!/usr/bin/env python3
"""Regenerate local nonmatching asm wrappers from a user-supplied target EXE."""

from __future__ import annotations

import argparse
import re
import sys
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import dis_sym


BASE_VRAM = 0x80010000
BASE_OFF = 0x800


SUBSEGMENT_RE = re.compile(r"\[0x([0-9A-Fa-f]+),\s*([^,\]]+),\s*([^,\]\s]+)")
# Only an include that points into the generated asm/ tree names a symbol this
# script must disassemble. One that points at src/ names a checked-in file, and
# reading its stem as a symbol invents a function the game does not have.
ASM_WRAP_RE = re.compile(r'INCLUDE_ASM(?:_TU)?\(\s*"asm/[^"]*"\s*,\s*([A-Za-z0-9_]+)\)')
RODATA_WRAP_RE = re.compile(r"INCLUDE_RODATA\([^,]+,\s*([A-Za-z0-9_]+)\)")
C_FUNC_RE = re.compile(
    r"^\s*(?:[A-Za-z_][A-Za-z0-9_]*\s+)+(?P<name>func_[0-9A-Fa-f]{8})\s*\([^;]*\)\s*\{",
    re.MULTILINE,
)
# A decompiled function that carries a real name is defined as `Name(...) {` and
# gets its address from splat's symbol_addrs. Both halves are needed so that a
# unit which mixes named C functions with INCLUDE_ASM stubs still bounds each
# stub at the next C function instead of swallowing it.
C_NAMED_DEF_RE = re.compile(
    r"^[A-Za-z_][A-Za-z0-9_ \t\*]*?(?P<name>[A-Za-z_][A-Za-z0-9_]*)\s*\([^;{]*\)\s*\{",
    re.MULTILINE,
)
# A unit can also define a symbol without a C function definition: handwritten
# assembly declares it with .globl inside a top-level asm() block, and a data
# object can be placed in .text with __attribute__((section(".text"))). Both end
# the preceding stub, so both must count as boundaries.
# The .globl may carry the routine's real name, in which case symbol_addrs says
# where it is, exactly as it does for a named C function.
ASM_GLOBL_RE = re.compile(r"\.globl\s+(?P<name>[A-Za-z_][A-Za-z0-9_]*)")
# The same, for a HANDWRITTEN_ASM unit's .s file: glabel and dlabel name a
# function and a data object, and a bare .globl names either.
HANDWRITTEN_LABEL_RE = re.compile(
    r"^(?:glabel|dlabel|\.globl)\s+(?P<name>[A-Za-z_][A-Za-z0-9_]*)", re.MULTILINE)
FUNC_NAME_RE = re.compile(r"func_[0-9A-Fa-f]{8}")
# The object may carry its real name rather than func_XXXXXXXX, in which case
# the address comes from symbol_addrs like a named function's does.
TEXT_OBJECT_RE = re.compile(
    r"\b(?P<name>[A-Za-z_][A-Za-z0-9_]*)\s*(?:\[[^\]]*\])?\s*__attribute__"
)


def strip_comments(text: str) -> str:
    """Blank out comments, preserving length and line structure.

    The declaration regexes below allow a parameter list to span lines, so a
    stray "(" inside a comment above a declaration would otherwise start a match
    that runs on into the real declaration and captures the wrong name. That is
    how PushMatrix was read as "x280" and lost its alias, which made the
    generator swallow the following function into the preceding stub.
    """
    def blank(match: re.Match) -> str:
        return re.sub(r"[^\n]", " ", match.group(0))

    return re.sub(r"/\*.*?\*/|//[^\n]*", blank, text, flags=re.S)


SYMBOL_ADDR_RE = re.compile(
    r"^\s*(?P<name>[A-Za-z_][A-Za-z0-9_]*)\s*=\s*0x(?P<address>[0-9A-Fa-f]{8})\s*;",
    re.MULTILINE,
)


def collect_symbol_addrs(path: Path) -> dict[str, str]:
    """Map C function name -> asm symbol, read from splat's symbol_addrs.

    This is the whole of what this generator knows about names. A name that
    lives in symbol_addrs needs no asm() label in the source: splat emits it
    into the disassembly and the linker script, so `Name` is the address, and
    this generator only has to agree about where the function starts.
    """
    if not path.exists():
        return {}
    return {
        match.group("name"): f"func_{match.group('address').upper()}"
        for match in SYMBOL_ADDR_RE.finditer(path.read_text())
    }


def parse_subsegments(config: Path) -> list[tuple[int, str, str, int]]:
    rows: list[tuple[int, str, str]] = []
    for line in config.read_text().splitlines():
        match = SUBSEGMENT_RE.search(line)
        if match:
            rows.append((int(match.group(1), 16), match.group(2).strip(), match.group(3).strip()))

    result: list[tuple[int, str, str, int]] = []
    for index, (start, kind, name) in enumerate(rows):
        end = rows[index + 1][0] if index + 1 < len(rows) else start
        result.append((start, kind, name, end))
    return result


def parse_wrappers(
    src_root: Path, version: str, aliases: dict[str, str] | None = None
) -> tuple[dict[str, list[str]], dict[str, list[str]], dict[str, str], dict[str, str]]:
    aliases = aliases or {}
    asm_by_unit: dict[str, list[str]] = {}
    c_funcs_by_unit: dict[str, list[str]] = {}
    rodata_by_name: dict[str, str] = {}
    #: symbol -> the real name a source `.globl` gives it, so the disassembler
    #: spells references to it the way the definition does.
    globl_names: dict[str, str] = {}
    for path in src_root.rglob("*.c"):
        text = strip_comments(path.read_text())
        rel = path.relative_to(src_root).with_suffix("").as_posix()
        asm_by_unit[f"{version}/{rel}"] = [match.group(1) for match in ASM_WRAP_RE.finditer(text)]
        names = [match.group("name") for match in C_FUNC_RE.finditer(text)]
        for match in C_NAMED_DEF_RE.finditer(text):
            symbol = aliases.get(match.group("name"))
            if symbol is not None:
                names.append(symbol)
        for match in ASM_GLOBL_RE.finditer(text):
            label = match.group("name")
            if FUNC_NAME_RE.fullmatch(label):
                names.append(label)
                continue
            symbol = aliases.get(label)
            if symbol is not None:
                names.append(symbol)
                globl_names[symbol] = label
        for match in TEXT_OBJECT_RE.finditer(text):
            symbol = aliases.get(match.group("name"))
            if symbol is None and FUNC_NAME_RE.fullmatch(match.group("name")):
                symbol = match.group("name")
            if symbol is not None:
                names.append(symbol)
        # A HANDWRITTEN_ASM unit keeps its assembly in a .s beside the source,
        # so the labels that end the preceding stub are in that file rather
        # than in the C. Without them a stub runs on past the hand-written
        # block and redefines every label inside it.
        sibling = path.with_suffix(".s")
        if sibling.exists():
            for match in HANDWRITTEN_LABEL_RE.finditer(sibling.read_text()):
                label = match.group("name")
                if FUNC_NAME_RE.fullmatch(label):
                    names.append(label)
                    continue
                symbol = aliases.get(label)
                if symbol is not None:
                    names.append(symbol)
                    globl_names[symbol] = label
        c_funcs_by_unit[f"{version}/{rel}"] = names
        for match in RODATA_WRAP_RE.finditer(text):
            rodata_by_name[match.group(1)] = rel
    return asm_by_unit, c_funcs_by_unit, rodata_by_name, globl_names


SUBSEGMENT_START_RE = re.compile(r"^\s*-\s*\[\s*0x([0-9A-Fa-f]+)")


def parse_region_bounds(config: Path) -> list[tuple[int, int]]:
    """The vram span of each subsegment, the trailing end marker included.

    `parse_subsegments` drops the bare `[0x8B800]` that closes the segment
    list, which would leave the whole data segment looking like it belongs to
    no region at all.  Here it is the bound that makes the region before it
    finite.
    """
    starts = [
        int(match.group(1), 16)
        for match in (SUBSEGMENT_START_RE.match(line) for line in config.read_text().splitlines())
        if match is not None
    ]
    return [
        (BASE_VRAM + (start - BASE_OFF), BASE_VRAM + (end - BASE_OFF))
        for start, end in zip(starts, starts[1:])
        if end > start
    ]


def function_extents(starts: list[tuple[int, int]]) -> dict[int, int]:
    """Where each function ends: at the next one, or at the end of its unit."""
    extents: dict[int, int] = {}
    ordered = sorted(set(starts))
    for index, (address, segment_end) in enumerate(ordered):
        end = segment_end
        if index + 1 < len(ordered):
            following = ordered[index + 1][0]
            if address < following < end:
                end = following
        if end > address:
            extents[address] = min(extents.get(address, end), end)
    return extents


def unit_output_path(name: str, version: str) -> str:
    prefix = f"{version}/"
    if name.startswith(prefix):
        return name[len(prefix) :]
    return name


def labels_for_asm(labels: dict[str, dict[int, list[str]]], unit: str, asm_name: str) -> dict[int, list[str]]:
    return labels.get(f"{asm_name}.s") or labels.get(f"{Path(unit).name}.s", {})


def parse_labels(path: Path) -> dict[str, dict[int, list[str]]]:
    labels: dict[str, dict[int, list[str]]] = defaultdict(lambda: defaultdict(list))
    if not path.exists():
        return labels

    for raw in path.read_text().splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        output_file, address, label = line.split(maxsplit=2)
        labels[output_file][int(address, 16)].append(label)
    return labels


def parse_label_addresses(labels: dict[str, dict[int, list[str]]]) -> dict[str, int]:
    addresses: dict[str, int] = {}
    for output_labels in labels.values():
        for address, names in output_labels.items():
            for name in names:
                addresses.setdefault(name, address)
    return addresses


def fallback_function_address(name: str) -> int | None:
    match = re.fullmatch(r"func_([0-9A-Fa-f]{8})", name)
    if not match:
        return None
    return int(match.group(1), 16)


def function_address(
    name: str,
    label_addresses: dict[str, int],
    segment_vram: int,
    output_labels: dict[int, list[str]],
) -> int | None:
    return label_addresses.get(name) or fallback_function_address(name) or (
        min(output_labels) if output_labels else segment_vram
    )


def write_words(path: Path, section: str, data: bytes, vram: int, labels: dict[int, list[str]]) -> None:
    lines = [section, ""]

    for offset in range(0, len(data), 4):
        address = vram + offset
        for label in labels.get(address, []):
            lines.append(f".globl {label}")
            lines.append(f"{label}:")
        word = int.from_bytes(data[offset : offset + 4], "little")
        lines.append(f"/* {address:08X} {word:08X} */  .word 0x{word:08X}")

    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines) + "\n")


def write_code(
    path: Path,
    data: bytes,
    target: bytes,
    vram: int,
    labels: dict[int, list[str]],
    table: dis_sym.SymbolTable,
    stats: dis_sym.Stats,
) -> None:
    """Write a function as disassembly instead of an opaque run of `.word`.

    Every call and every address the symbol table accounts for comes out as a
    symbol reference, so the linker resolves it and the function is free to move
    once the last of the pinned addresses is gone.
    """
    lines = [".set noreorder", ".set noat", '.section .text, "ax"', ""]
    lines += dis_sym.disassemble(target, vram, len(data), table, labels, stats)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines) + "\n")


def with_entry_label(labels: dict[int, list[str]], address: int, label: str) -> dict[int, list[str]]:
    merged = {key: list(value) for key, value in labels.items()}
    if any(label in value for value in merged.values()):
        return merged
    merged.setdefault(address, [])
    merged[address].insert(0, label)
    return merged


@dataclass
class Plan:
    """What the split owes: which stub covers what, and the symbols in scope."""

    target_bytes: bytes
    stubs: list[tuple[Path, int, int, dict[int, list[str]]]]
    rodata: list[tuple[Path, bytes, int, dict[int, list[str]]]]
    table: dis_sym.SymbolTable


def build_plan(root: Path, version: str, basename: str) -> Plan | None:
    config = root / "configs" / version / f"{basename}.yaml"
    target = root / "assets" / version / f"{basename}.exe"
    src_root = root / "src" / basename / version
    out_dir = root / "asm" / version / basename / "nonmatchings"
    labels = parse_labels(root / "configs" / version / f"nonmatching_labels.{basename}.txt")

    if not config.exists() or not target.exists() or not src_root.exists():
        return None

    aliases = collect_symbol_addrs(root / "configs" / version / f"sym.{basename}.txt")
    asm_wrappers_by_unit, c_funcs_by_unit, rodata_wrappers, globl_names = parse_wrappers(
        src_root, version, aliases
    )
    label_addresses = parse_label_addresses(labels)
    # An INCLUDE_ASM stub may be spelled with the routine's real name rather
    # than func_XXXXXXXX, in which case nothing in the name says where it
    # starts and symbol_addrs is the only thing that does.  Without this the
    # lookup fell through to the start of the whole subsegment, which then
    # looked like a boundary in front of every sibling stub and silently
    # deleted them.
    for symbol_name, symbol in aliases.items():
        address = fallback_function_address(symbol)
        if address is not None:
            label_addresses.setdefault(symbol_name, address)
    target_bytes = target.read_bytes()

    # Two passes: the first works out which stub covers which address range, the
    # second disassembles them.  The symbol table in between needs the first
    # pass, because a label is only safe to reference once something defines it,
    # and for these labels that something is the stub the second pass writes.
    planned: list[tuple[Path, int, int, dict[int, list[str]]]] = []
    rodata_jobs: list[tuple[Path, bytes, int, dict[int, list[str]]]] = []
    #: every function start, so a switch table pointing into the middle of one
    #: can say which function it points into.
    function_starts: list[tuple[int, int]] = []

    for start, kind, name, end in parse_subsegments(config):
        if end <= start:
            continue
        vram = BASE_VRAM + (start - BASE_OFF)
        data = target_bytes[start:end]
        stem = Path(name).name

        if kind == "c":
            asm_wrappers = asm_wrappers_by_unit.get(name, [])
            c_func_boundaries = [
                function_address(c_func, label_addresses, vram, {})
                for c_func in c_funcs_by_unit.get(name, [])
            ]
            c_func_boundaries = [address for address in c_func_boundaries if address is not None]
            segment_end = BASE_VRAM + (end - BASE_OFF)
            function_starts += [
                (address, segment_end) for address in c_func_boundaries if address >= vram
            ]
            for index, asm_name in enumerate(asm_wrappers):
                output_labels = labels_for_asm(labels, name, asm_name)
                func_vram = function_address(asm_name, label_addresses, vram, output_labels)
                if func_vram is None:
                    continue

                next_boundaries = [address for address in c_func_boundaries if address > func_vram]
                if index + 1 < len(asm_wrappers):
                    next_name = asm_wrappers[index + 1]
                    next_asm_vram = function_address(
                        next_name,
                        label_addresses,
                        vram,
                        labels_for_asm(labels, name, next_name),
                    )
                    if next_asm_vram is not None:
                        next_boundaries.append(next_asm_vram)
                else:
                    next_boundaries.append(BASE_VRAM + (end - BASE_OFF))
                next_vram = min(next_boundaries) if next_boundaries else None
                if next_vram is None or next_vram <= func_vram:
                    continue

                output = out_dir / unit_output_path(name, version) / f"{asm_name}.s"
                planned.append(
                    (
                        output,
                        func_vram,
                        next_vram,
                        with_entry_label(output_labels, func_vram, asm_name),
                    )
                )
        elif kind == ".rodata" and f"{stem}_rodata" in rodata_wrappers:
            output = out_dir / rodata_wrappers[f"{stem}_rodata"] / f"{stem}_rodata.s"
            rodata_jobs.append((output, data, vram, labels.get(output.name, {})))

    stub_names: dict[int, str] = {}
    stub_extents: dict[int, int] = function_extents(
        function_starts + [(start, stop) for _, start, stop, _ in planned]
    )
    for _, func_vram, next_vram, stub_labels in planned:
        stub_extents[func_vram] = next_vram
        for address, names in stub_labels.items():
            for label in names:
                stub_names.setdefault(address, label)
    # A handwritten block that declares its own `.globl Name` defines only that
    # name, so references to it have to be spelled the same way.
    for symbol, label in globl_names.items():
        address = fallback_function_address(symbol)
        if address is not None:
            stub_names.setdefault(address, label)
    # An address in symbol_addrs is defined under that name by whichever object
    # holds it, so that is what a reference to it has to say.  Without this the
    # disassembly falls back to func_XXXXXXXX for a function whose C source
    # stopped spelling it that way, and nothing defines the fallback.
    for symbol_name, symbol in aliases.items():
        address = fallback_function_address(symbol)
        if address is not None:
            stub_names.setdefault(address, symbol_name)
    # A unit that still spells a function `func_8004F3EC(...)` in C defines
    # exactly that symbol, so a call to it can use the name.
    for unit_funcs in c_funcs_by_unit.values():
        for c_func in unit_funcs:
            address = fallback_function_address(c_func)
            if address is not None:
                stub_names.setdefault(address, c_func)
    regions = parse_region_bounds(config)
    table = dis_sym.build_symbol_table(
        root, version, basename, stub_names, stub_extents, regions
    )
    return Plan(target_bytes, planned, rodata_jobs, table)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--version", default="PAL")
    parser.add_argument("--basename", default="main")
    args = parser.parse_args()

    plan = build_plan(Path.cwd(), args.version, args.basename)
    if plan is None:
        return 0

    target_bytes = plan.target_bytes
    table = plan.table
    stats = dis_sym.Stats()
    generated = 0

    for output, func_vram, next_vram, stub_labels in plan.stubs:
        write_code(
            output,
            target_bytes[
                func_vram - dis_sym.FILE_TO_VRAM : next_vram - dis_sym.FILE_TO_VRAM
            ],
            target_bytes,
            func_vram,
            stub_labels,
            table,
            stats,
        )
        generated += 1
    for output, data, vram, data_labels in plan.rodata:
        write_words(output, '.section .rodata, "a"', data, vram, data_labels)
        generated += 1

    # The hand-written engine builds a few addresses with `lui`/`ori`, which no
    # %hi/%lo pair can spell.  Cutting the halves here keeps the encoding while
    # leaving the value a function of the symbol, so those pairs move too.
    halves = Path("linkers") / args.version / f"addr_halves.{args.basename}.txt"
    lines = ["/* Address halves for lui/ori pairs; see tools/scripts/dis_sym.py. */"]
    for alias, reference in sorted(stats.halves.items()):
        lines.append(f"{alias}_hi = ({reference}) >> 16;")
        lines.append(f"{alias}_lo = ({reference}) & 0xFFFF;")
    halves.parent.mkdir(parents=True, exist_ok=True)
    halves.write_text("\n".join(lines) + "\n")

    print(f"generated {generated} local nonmatching asm files for {args.version}/{args.basename}")
    print(
        f"  symbolised {stats.calls_named}/{stats.calls} calls and "
        f"{stats.pairs_named}/{stats.pairs} RAM address pairs over "
        f"{stats.words} instructions"
    )
    print(
        f"  left literal: {stats.pairs_absolute} hardware or constant pairs; "
        f"{stats.pairs_halved} lui/ori pairs spelled through {len(stats.halves)} "
        f"linker-computed address halves"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
