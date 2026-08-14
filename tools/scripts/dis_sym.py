#!/usr/bin/env python3
"""Symbol-aware MIPS disassembly for the local nonmatching stubs.

A stub written as `.word 0x0C01E8B4` is unshiftable: the call target lives in
the instruction encoding, so nothing in the image can move.  `jal func_8007A2D0`
is the same four bytes today and a relocation tomorrow.  This module turns a
range of the target EXE into real assembly, replacing every encoded address it
can account for with a symbol reference:

  * `j` / `jal` targets            -> the function symbol
  * `lui` + consumer `%hi`/`%lo`   -> `%hi(sym)` / `%lo(sym)`, with `sym + N`
                                      when the address lands inside a known
                                      object rather than on its first byte
  * branch / jump targets inside the same function -> local `.L########`

Everything the symbol table cannot account for stays a literal, which is the
right answer for the I/O and scratchpad addresses at 0x1F80xxxx: those really
are absolute and must not move.

The output is byte-identical to the input by construction, because every symbol
still resolves to the address that was encoded there.  `make check` proves it
for the image as a whole, and `make report` proves it per object.
"""

from __future__ import annotations

import bisect
import re
import struct
from dataclasses import dataclass, field
from pathlib import Path

import rabbitizer


# gas knows the coprocessor 0 registers only by number, so `mtc0 $t1, Status`
# is a syntax error where `mtc0 $t1, $12` is the same instruction.
rabbitizer.config.regNames_vr4300Cop0NamedRegisters = False

BASE_VRAM = 0x80010000
BASE_OFF = 0x800
# vaddr = file_offset + FILE_TO_VRAM
FILE_TO_VRAM = BASE_VRAM - BASE_OFF

# Addresses outside RAM are hardware: the scratchpad and the I/O registers at
# 0x1F80xxxx are absolute by nature and must never become symbols.
RAM_START = 0x80000000
RAM_END = 0x80800000

# Instructions that can carry the low half of an address built by a lui.
# The coprocessor loads are deliberately absent: their first operand is a
# coprocessor register that this printer would have to spell by hand.
LO_CONSUMERS = {
    "lw", "sw", "lh", "lhu", "lb", "lbu", "sh", "sb",
    "lwl", "lwr", "swl", "swr",
    "addiu", "ori",
}
# ... of which these produce a register holding the whole address.
LO_ADDRESS_FORMERS = {"addiu", "ori"}
# Adding an index into the register that holds the upper half of an address.
INDEX_ADDS = {"addu", "add"}

#: How far back to look for a symbol whose object encloses an address.
ENCLOSING_SEARCH_SPAN = 0x4000

GENERIC_NAME_RE = re.compile(r"^(?:func|D|jtbl|jpt)_[0-9A-Fa-f]{8}$")
BRANCH_TARGET_RE = re.compile(r"\. \+ 4 \+ \(-?0x[0-9A-Fa-f]+ << 2\)")


def is_generic(name: str) -> bool:
    return GENERIC_NAME_RE.match(name) is not None


@dataclass
class SymbolTable:
    """Addresses the build is known to define a name for.

    Only names that some part of the build really defines belong here: an asm()
    alias on a C declaration, a label in the split data, a label this generator
    writes into a stub, or an assignment in a linker script.  Emitting a name
    that nothing defines would trade a frozen `.word` for a link error.
    """

    names: dict[int, str] = field(default_factory=dict)
    #: address -> end of the object starting there, for `sym + N` references.
    extents: dict[int, int] = field(default_factory=dict)
    #: the vram spans the split accounts for; anything else is bss or heap.
    regions: list[tuple[int, int]] = field(default_factory=list)
    _sorted: list[int] = field(default_factory=list)

    def add(self, address: int, name: str) -> None:
        current = self.names.get(address)
        if current is None or (is_generic(current) and not is_generic(name)):
            self.names[address] = name
        self._sorted = []

    def set_extent(self, address: int, end: int) -> None:
        if end > address:
            self.extents[address] = max(self.extents.get(address, 0), end)

    def _addresses(self) -> list[int]:
        if not self._sorted:
            self._sorted = sorted(self.names)
        return self._sorted

    def exact(self, address: int) -> str | None:
        return self.names.get(address)

    def reference(self, address: int, synthesize: bool = True) -> str | None:
        """`sym`, or `sym + 0xN` when the address is interior to a known object."""
        if not RAM_START <= address < RAM_END:
            return None
        name = self.names.get(address)
        if name is not None:
            return name
        # Symbols overlap: splat infers one from every reference it sees, so a
        # small symbol often sits inside a large labelled table.  The nearest
        # preceding symbol may therefore stop short of the address while the
        # object that really contains it starts further back.
        addresses = self._addresses()
        index = bisect.bisect_right(addresses, address) - 1
        while index >= 0 and address - addresses[index] <= ENCLOSING_SEARCH_SPAN:
            start = addresses[index]
            end = self.extents.get(start)
            if end is not None and address < end:
                return f"{self.names[start]} + 0x{address - start:X}"
            index -= 1
        if synthesize and not any(start <= address < stop for start, stop in self.regions):
            # Past the end of the image: bss or heap, where nothing has named
            # this address and nothing overlaps it either.  The build's address
            # aliases turn the name back into this address, exactly as they
            # already do for the same addresses referenced from C.
            return f"D_{address:08X}"
        return None


def _strip_comments(text: str) -> str:
    def blank(match: re.Match) -> str:
        return re.sub(r"[^\n]", " ", match.group(0))

    return re.sub(r"/\*.*?\*/|//[^\n]*", blank, text, flags=re.S)


ASM_ALIAS_SYMBOL_RE = re.compile(r'asm\("(?P<symbol>[A-Za-z_][A-Za-z0-9_]*)"\)')
LINKER_ASSIGN_RE = re.compile(
    r"^\s*(?P<name>[A-Za-z_][A-Za-z0-9_]*)\s*=\s*(?P<value>[^;]+);", re.MULTILINE
)
DATA_LABEL_RE = re.compile(r"^\s*(?:glabel|dlabel|jlabel|alabel)\s+(?P<name>[A-Za-z_][A-Za-z0-9_]*)")
# The word itself is present on lines symbolise_data_words.py has rewritten,
# absent elsewhere; missing that case silently loses the label above it.
DATA_ADDR_RE = re.compile(
    r"^\s*/\* [0-9A-Fa-f]+ (?P<vaddr>[0-9A-Fa-f]{8})(?: [0-9A-Fa-f]{8})? \*/"
)


def address_from_name(name: str) -> int | None:
    match = re.fullmatch(r"(?:func|D|jtbl|jpt)_([0-9A-Fa-f]{8})", name)
    if match is None:
        return None
    return int(match.group(1), 16)


def collect_asm_alias_symbols(*roots: Path) -> dict[int, str]:
    """Symbols a C declaration pins to an address with asm("func_XXXXXXXX")."""
    found: dict[int, str] = {}
    for root in roots:
        if not root.exists():
            continue
        for pattern in ("*.h", "*.c"):
            for path in root.rglob(pattern):
                text = _strip_comments(path.read_text(errors="ignore"))
                for match in ASM_ALIAS_SYMBOL_RE.finditer(text):
                    symbol = match.group("symbol")
                    address = address_from_name(symbol)
                    if address is not None:
                        found[address] = symbol
    return found


def collect_linker_symbols(*paths: Path) -> dict[int, str]:
    """Names a linker script assigns, keyed by the address they resolve to.

    Only assignments whose value is a literal or a name that carries its own
    address are usable; anything else would need the full link to evaluate.
    """
    found: dict[int, str] = {}
    for path in paths:
        if not path.exists():
            continue
        for match in LINKER_ASSIGN_RE.finditer(path.read_text(errors="ignore")):
            name = match.group("name")
            address = _evaluate_linker_value(match.group("value").strip())
            if address is None:
                address = address_from_name(name)
            if address is not None:
                found.setdefault(address, name)
    return found


def _evaluate_linker_value(value: str) -> int | None:
    total = 0
    for index, term in enumerate(re.split(r"\s*\+\s*", value)):
        term = term.strip()
        if re.fullmatch(r"0[xX][0-9A-Fa-f]+", term):
            total += int(term, 16)
        elif re.fullmatch(r"\d+", term):
            total += int(term)
        elif index == 0:
            base = address_from_name(term)
            if base is None:
                return None
            total += base
        else:
            return None
    return total


def collect_data_labels(data_root: Path) -> tuple[dict[int, str], dict[int, int]]:
    """Labels the split data carries, plus the extent of each labelled object.

    The extent is what makes `%hi(D_8007B664 + 0x120)` possible: an address that
    lands inside a table is still an address in that table, not a fresh symbol.
    """
    names: dict[int, str] = {}
    extents: dict[int, int] = {}
    if not data_root.exists():
        return names, extents

    for path in sorted(data_root.rglob("*.s")):
        pending: list[str] = []
        ordered: list[int] = []
        last_address = None
        for line in path.read_text(errors="ignore").splitlines():
            label = DATA_LABEL_RE.match(line)
            if label is not None:
                pending.append(label.group("name"))
                continue
            addr_match = DATA_ADDR_RE.match(line)
            if addr_match is None:
                continue
            address = int(addr_match.group("vaddr"), 16)
            last_address = address
            for name in pending:
                names[address] = name
                ordered.append(address)
            pending.clear()
        # Each label owns everything up to the next one; the last one owns the
        # rest of its file.  Both bounds come from the file, so a table that a
        # `%hi` points into the middle of resolves to that table and no other.
        for index, address in enumerate(ordered):
            end = ordered[index + 1] if index + 1 < len(ordered) else (
                (last_address + 4) if last_address is not None else address + 4
            )
            extents[address] = end
    return names, extents


#: An address this far past a symbol with no known size is assumed to be a
#: different object, not an interior reference into that one.
UNSIZED_EXTENT_LIMIT = 0x1000


def build_symbol_table(
    root: Path,
    version: str,
    basename: str,
    extra_names: dict[int, str] | None = None,
    function_extents: dict[int, int] | None = None,
    regions: list[tuple[int, int]] | None = None,
) -> SymbolTable:
    table = SymbolTable(regions=list(regions or []))

    linker_dir = root / "linkers" / version
    # undefined_addr_aliases is deliberately not a source: the build writes it
    # from the objects it just compiled, so reading it here would make the
    # generated asm depend on whether a build had run.
    sources: list[dict[int, str]] = [
        collect_linker_symbols(
            linker_dir / f"undefined_syms_auto.{basename}.txt",
            linker_dir / f"undefined_funcs_auto.{basename}.txt",
            linker_dir / "undefined_syms_manual.txt",
            # Section markers main.ld computes.  Not linked from here; see the
            # header of the file.  Without it the boot stub's reference to the
            # end of bss would come out as a frozen absolute address.
            linker_dir / f"section_syms.{basename}.txt",
        ),
        collect_asm_alias_symbols(root / "include", root / "src" / basename / version),
    ]
    data_names, data_extents = collect_data_labels(root / "asm" / version / basename / "data")
    sources.append(data_names)
    if extra_names:
        sources.append(extra_names)

    for source in sources:
        for address, name in source.items():
            table.add(address, name)
    for address, end in data_extents.items():
        table.set_extent(address, end)
    for address, end in (function_extents or {}).items():
        table.set_extent(address, end)
    _infer_extents(table, regions or [])
    return table


def _infer_extents(table: SymbolTable, regions: list[tuple[int, int]]) -> None:
    """Guess how far a symbol reaches when nothing recorded its size.

    Most of the symbols splat inferred from references carry no size, so an
    address a few bytes into the object they name looks like nothing at all.
    The next symbol bounds it, the containing segment bounds it, and a hard
    limit keeps one lonely symbol in an unlabelled stretch from claiming
    everything after it.
    """
    addresses = sorted(table.names)
    for index, address in enumerate(addresses):
        if address in table.extents:
            continue
        end = addresses[index + 1] if index + 1 < len(addresses) else address
        for start, stop in regions:
            if start <= address < stop:
                end = min(end, stop)
                break
        else:
            # Outside every known segment - the bss and heap, where a gap
            # between two symbols means nothing at all.
            continue
        table.set_extent(address, min(end, address + UNSIZED_EXTENT_LIMIT))


@dataclass
class Stats:
    calls: int = 0
    calls_named: int = 0
    #: lui/consumer pairs whose value is a RAM address, so a symbol is wanted.
    pairs: int = 0
    pairs_named: int = 0
    #: pairs building something that is not a RAM address: a hardware register,
    #: a scratchpad address or a plain 32-bit constant.  Those stay literal on
    #: purpose - they have nowhere to move to.
    pairs_absolute: int = 0
    #: pairs that are RAM addresses but cannot be spelled relocatably.
    pairs_frozen: int = 0
    #: pairs spelled through linker-computed halves; see `_half_symbols`.
    pairs_halved: int = 0
    words: int = 0
    #: alias base name -> the expression its `_hi`/`_lo` halves are cut from.
    halves: dict[str, str] = field(default_factory=dict)


def _gte_name(word: int, vram: int) -> str | None:
    """The GTE mnemonic for a COP2 operation, for use as a comment.

    gas has no `rtps`; `cop2 0x0180001` is the spelling it accepts and it
    encodes to the same word, so the mnemonic survives as documentation.
    """
    try:
        instruction = rabbitizer.Instruction(
            word, vram=vram, category=rabbitizer.InstrCategory.R3000GTE
        )
        text = instruction.disassemble().strip()
    except Exception:  # pragma: no cover - rabbitizer refusing is not fatal
        return None
    if not text or text.startswith("."):
        return None
    return re.split(r"\s+", text)[0]


def disassemble(
    data: bytes,
    vaddr: int,
    size: int,
    table: SymbolTable,
    labels: dict[int, list[str]] | None = None,
    stats: Stats | None = None,
) -> list[str]:
    """Disassemble `size` bytes at `vaddr` into assembler source lines."""
    labels = labels or {}
    stats = stats if stats is not None else Stats()
    count = size // 4
    offset = vaddr - FILE_TO_VRAM
    words = [struct.unpack_from("<I", data, offset + 4 * index)[0] for index in range(count)]
    instructions = [
        rabbitizer.Instruction(word, vram=vaddr + 4 * index) for index, word in enumerate(words)
    ]
    end = vaddr + 4 * count

    def internal(target: int | None) -> bool:
        return target is not None and vaddr <= target < end

    # Targets inside the function become local labels unless the address
    # already carries a real one that this file is about to define.
    local: dict[int, str] = {}
    for index, instruction in enumerate(instructions):
        target = _target_of(instruction)
        if internal(target) and target not in labels:
            local.setdefault(target, f".L{target:08X}")

    hi_at, lo_at, halves, notes = _pair_addresses(instructions, table, stats)

    lines: list[str] = []
    for index, instruction in enumerate(instructions):
        address = vaddr + 4 * index
        for label in labels.get(address, []):
            lines.append(f".globl {label}")
            lines.append(f"{label}:")
        if address in local:
            lines.append(f"{local[address]}:")

        word = words[index]
        text = _instruction_text(
            instruction, index, local, labels, table, hi_at, lo_at, halves, stats
        )
        if index in notes:
            text = f"{text}  /* {notes[index]} */"
        lines.append(f"/* {address:08X} {word:08X} */  {text}")
        stats.words += 1
    return lines


def _target_of(instruction: rabbitizer.Instruction) -> int | None:
    if instruction.isBranch():
        return instruction.getBranchVramGeneric()
    if instruction.getOpcodeName() in ("j", "jal"):
        return instruction.getInstrIndexAsVram()
    return None


def _written_registers(instruction: rabbitizer.Instruction) -> list[str]:
    written: list[str] = []
    try:
        if instruction.modifiesRt():
            written.append(instruction.rt.name)
        if instruction.modifiesRd():
            written.append(instruction.rd.name)
    except Exception:  # pragma: no cover - a word with no register fields
        return []
    return written


def _pair_addresses(
    instructions: list[rabbitizer.Instruction], table: SymbolTable, stats: Stats
) -> tuple[dict[int, str], dict[int, str], dict[int, str]]:
    """Match each `lui` with the instructions that consume its upper half.

    A single `lui` often feeds several loads; every one of them gets a `%lo`,
    so none of the offsets stay frozen.  The register stops holding an address
    as soon as anything writes to it, which is what keeps an unrelated later
    `lw` from being read as part of the pair.
    """
    hi_at: dict[int, str] = {}
    lo_at: dict[int, str] = {}
    halves: dict[int, str] = {}
    notes: dict[int, str] = {}
    pending: dict[str, tuple[int, int]] = {}
    counted: set[int] = set()

    for index, instruction in enumerate(instructions):
        name = instruction.getOpcodeName()
        if name == "lui":
            pending[instruction.rt.name] = (index, instruction.getProcessedImmediate() & 0xFFFF)
            continue
        if name in INDEX_ADDS:
            # `lui $at, %hi(sym)` / `addu $at, $at, $index` / `lw $v0, %lo(sym)($at)`
            # is how an indexed global is addressed: the register still holds
            # the upper half, so the pair survives the add.
            destination = instruction.rd.name
            if destination in pending and destination in (
                instruction.rs.name,
                instruction.rt.name,
            ):
                continue
        if name in LO_CONSUMERS:
            held = pending.get(instruction.rs.name)
            if held is not None:
                hi_index, upper = held
                address = ((upper << 16) + instruction.getProcessedImmediate()) & 0xFFFFFFFF
                reference = table.reference(address)
                # `ori` zero-extends where `%lo` relocation arithmetic assumes
                # the sign extension of `addiu` and the loads: when bit 15 of
                # the address is set the pair's `lui` holds a different upper
                # half than `%hi` would compute, so `%hi`/`%lo` cannot spell
                # this pair.  Cut the two halves in the linker script instead
                # and reference them by name -- same encoding, but the value
                # is still computed from the symbol, so the pair can move.
                blocked = reference is not None and name == "ori" and bool(address & 0x8000)
                if blocked:
                    alias = _half_symbols(reference, stats)
                    halves[hi_index] = f"{alias}_hi"
                    halves[index] = f"{alias}_lo"
                    reference = None
                if hi_index not in counted:
                    counted.add(hi_index)
                    if not RAM_START <= address < RAM_END:
                        stats.pairs_absolute += 1
                    else:
                        stats.pairs += 1
                        if blocked:
                            stats.pairs_halved += 1
                if reference is not None:
                    if hi_index not in hi_at:
                        stats.pairs_named += 1
                    hi_at[hi_index] = reference
                    lo_at[index] = reference
        for register in _written_registers(instruction):
            pending.pop(register, None)

    # A `%hi` with no `%lo` behind it is an unmatched relocation the linker
    # cannot resolve, so drop any pair whose low half was rejected.
    for hi_index in list(hi_at):
        if not any(lo_index > hi_index for lo_index in lo_at if lo_at[lo_index] == hi_at[hi_index]):
            hi_at.pop(hi_index)
    return hi_at, lo_at, halves, notes


def _half_symbols(reference: str, stats: Stats) -> str:
    """Name the linker symbols that hold the two halves of `reference`.

    `lui $t2, name_hi` assembles to an R_MIPS_LO16 against `name_hi`, i.e. the
    low 16 bits of whatever the linker script assigns it, so defining
    `name_hi = (expr) >> 16` reproduces the literal upper half without `%hi`'s
    sign compensation.
    """
    alias = re.sub(r"[^0-9A-Za-z_]+", "_", reference).strip("_")
    stats.halves[alias] = reference
    return alias


def _instruction_text(
    instruction: rabbitizer.Instruction,
    index: int,
    local: dict[int, str],
    labels: dict[int, list[str]],
    table: SymbolTable,
    hi_at: dict[int, str],
    lo_at: dict[int, str],
    halves: dict[int, str],
    stats: Stats,
) -> str:
    name = instruction.getOpcodeName()
    word = instruction.getRaw()

    if name in ("j", "jal"):
        target = instruction.getInstrIndexAsVram()
        stats.calls += 1
        symbol = _label_for(target, local, labels) or table.exact(target)
        if symbol is not None:
            stats.calls_named += 1
            return f"{name} {symbol}"
        # No name for it: an encoded target is still better than a bare word,
        # but say so, because this one cannot move.
        return f".word 0x{word:08X}  /* {name} 0x{target:08X}: no symbol */"

    if index in halves:
        if name == "lui":
            return f"lui ${instruction.rt.name}, {halves[index]}"
        return f"ori ${instruction.rt.name}, ${instruction.rs.name}, {halves[index]}"

    if index in hi_at:
        return f"lui ${instruction.rt.name}, %hi({hi_at[index]})"

    if index in lo_at:
        symbol = lo_at[index]
        if name in LO_ADDRESS_FORMERS:
            return f"{name} ${instruction.rt.name}, ${instruction.rs.name}, %lo({symbol})"
        return f"{name} ${instruction.rt.name}, %lo({symbol})(${instruction.rs.name})"

    if instruction.isBranch():
        target = instruction.getBranchVramGeneric()
        symbol = _label_for(target, local, labels)
        if symbol is None and target is not None:
            symbol = table.exact(target)
        text = instruction.disassemble()
        if symbol is not None and BRANCH_TARGET_RE.search(text):
            return _tidy(BRANCH_TARGET_RE.sub(symbol, text))
        return _tidy(text)

    text = instruction.disassemble()
    if text.lstrip().startswith(".word"):
        # rabbitizer only refuses words it has no CPU mnemonic for, which here
        # means the GTE coprocessor operations of the handwritten engine.
        if (word >> 26) == 0x12 and (word & (1 << 25)):
            mnemonic = _gte_name(word, instruction.vram)
            suffix = f"  /* {mnemonic} */" if mnemonic else ""
            return f"cop2 0x{word & 0x1FFFFFF:07X}{suffix}"
        return f".word 0x{word:08X}"
    return _tidy(text)


def _label_for(
    target: int | None, local: dict[int, str], labels: dict[int, list[str]]
) -> str | None:
    if target is None:
        return None
    if target in local:
        return local[target]
    if target in labels and labels[target]:
        return labels[target][0]
    return None


def _tidy(text: str) -> str:
    """Collapse rabbitizer's column padding; the comment prefix aligns already."""
    parts = text.split(None, 1)
    if len(parts) == 1:
        return parts[0]
    return f"{parts[0]} {parts[1].strip()}"
