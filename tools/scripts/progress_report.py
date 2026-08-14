#!/usr/bin/env python3
"""Print the progress table and refresh the badge JSON from the objdiff report.

Progress is whether the code this tree produces is the code the game shipped,
so the numbers come from the same objdiff report decomp.dev ingests: every
object is compared, function by function, against an object disassembled from
the retail executable. `make report` writes it; this only reads it.

An earlier version of this script counted a function as done when its source
carried no included assembly and no inline assembly. That measures how much of the
tree is written in C, which is worth knowing but is not the same claim, and
stating it as progress overstated the result: a function can be plain C and
still compile to something the game never contained. The C-versus-assembly
count is still printed, under its own name.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from datetime import date
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DOCS = ROOT / "docs"
BADGES = DOCS / "badges"

# Sony shipped libgpu, libgte, libspu and the rest with the SDK. Both halves are
# reported - decomp.dev requires every function in the binary to be accounted
# for - but they are counted separately, because matching someone else's
# library is not the same work as decompiling the game.
CATEGORY_NAMES = {"game": "Game code", "psyq": "PsyQ libraries"}

ASM_INCLUDE = re.compile(r"\bHANDWRITTEN_ASM\s*\(")
# Assembler directives count only where they can actually occur in C: inside a
# string literal handed to asm(). Matched against bare source they also hit
# ordinary struct field accesses -- `foo.word` on a union with a `word` member
# reads as a `.word` directive and silently reclassifies plain C as assembly.
ASM_DIRECTIVE = re.compile(r"\.(?:word|globl|ent|include)\b")
STRING_LITERAL = re.compile(r'"(?:[^"\\\n]|\\.)*"')
ASM_STATEMENT = re.compile(r"(^|[^_a-zA-Z0-9])(__asm__|asm)\s*(volatile\s*)?\(")


def color(percent: float) -> str:
    if percent >= 100:
        return "brightgreen"
    if percent >= 90:
        return "green"
    if percent >= 60:
        return "yellow"
    if percent >= 30:
        return "orange"
    return "red"


def badge(path: Path, label: str, message: str, percent: float) -> None:
    path.write_text(
        json.dumps(
            {
                "schemaVersion": 1,
                "label": label,
                "message": message,
                "color": color(percent),
            },
            indent=2,
        )
        + "\n"
    )


def percent(measures: dict, key: str) -> float:
    return float(measures.get(key, 0.0) or 0.0)


def count(measures: dict, key: str) -> int:
    return int(measures.get(key, 0) or 0)


def carries_assembly(text: str) -> bool:
    """True when a translation unit still holds assembly of any kind."""
    if ASM_INCLUDE.search(text):
        return True
    if any(ASM_DIRECTIVE.search(lit.group(0)) for lit in STRING_LITERAL.finditer(text)):
        return True
    return bool(ASM_STATEMENT.search(text))


def source_mix(src_root: Path) -> tuple[int, int]:
    """How many translation units are plain C, out of how many altogether.

    Deliberately per file rather than per function: a unit that mixes C with a
    hand-written block is not plain C, and pretending otherwise is how the old
    count drifted away from what it claimed to measure.
    """
    total = plain = 0
    for path in sorted(src_root.rglob("*.c")):
        total += 1
        if not carries_assembly(path.read_text(errors="ignore")):
            plain += 1
    return plain, total


def table(report: dict) -> list[str]:
    overall = report["measures"]
    rows = [
        "| Scope | Functions | Code bytes | Data bytes |",
        "|---|---:|---:|---:|",
    ]
    for category in report.get("categories", []):
        measures = category["measures"]
        rows.append(
            "| %s | %d / %d (%.2f%%) | %.2f%% | %.2f%% |"
            % (
                CATEGORY_NAMES.get(category["id"], category["name"]),
                count(measures, "matched_functions"),
                count(measures, "total_functions"),
                percent(measures, "matched_functions_percent"),
                percent(measures, "matched_code_percent"),
                percent(measures, "matched_data_percent"),
            )
        )
    rows.append(
        "| **Whole executable** | **%d / %d (%.2f%%)** | **%.2f%%** | **%.2f%%** |"
        % (
            count(overall, "matched_functions"),
            count(overall, "total_functions"),
            percent(overall, "matched_functions_percent"),
            percent(overall, "matched_code_percent"),
            percent(overall, "matched_data_percent"),
        )
    )
    return rows


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--version", default="PAL")
    parser.add_argument("--report", default=None)
    args = parser.parse_args(argv)

    report_path = Path(args.report) if args.report else (
        ROOT / "build" / args.version / "report.json")
    if not report_path.exists():
        raise SystemExit("%s missing - run `make report` first" % report_path)
    report = json.loads(report_path.read_text())

    overall = report["measures"]
    functions_pct = percent(overall, "matched_functions_percent")
    code_pct = percent(overall, "matched_code_percent")

    BADGES.mkdir(parents=True, exist_ok=True)
    badge(
        BADGES / "functions.json",
        "functions matched",
        "%d/%d %.2f%%" % (count(overall, "matched_functions"),
                          count(overall, "total_functions"), functions_pct),
        functions_pct,
    )
    badge(
        BADGES / "code.json",
        "code matched",
        "%.2f%%" % code_pct,
        code_pct,
    )

    plain, units = source_mix(ROOT / "src" / "main")
    lines = [
        "# Decompilation Progress",
        "",
        "_Generated by `tools/scripts/progress_report.py` from "
        "`%s` on %s. Regenerate with `make report progress`._"
        % (report_path.relative_to(ROOT), date.today().isoformat()),
        "",
        "A function counts as matched when the object this tree compiles and an "
        "object disassembled from the retail executable contain the same code, "
        "as judged by objdiff. The linked executable is byte-identical to "
        "retail (`make check`).",
        "",
    ]
    lines.extend(table(report))
    lines += [
        "",
        "%d of %d translation units are plain C, the rest holding hand-written "
        "assembly the original shipped that way. That is a separate measure "
        "from the table above and does not feed it." % (plain, units),
    ]
    print("\n".join(lines))
    return 0


if __name__ == "__main__":
    sys.exit(main())
