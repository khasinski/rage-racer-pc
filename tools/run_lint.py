#!/usr/bin/env python3
"""Run host-only static analysis from CMake's compilation database."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile


HOST_ROOTS = ("src/port/", "src/render/")


def require_tool(name: str) -> str:
    path = shutil.which(name)
    if path is None:
        raise RuntimeError(f"{name} is required; install it before running the lint target")
    return path


def host_sources(database: Path, root: Path) -> tuple[list[Path], list[dict[str, object]]]:
    commands = json.loads(database.read_text(encoding="utf-8"))
    selected: dict[Path, dict[str, object]] = {}
    for command in commands:
        path = Path(command["file"]).resolve()
        try:
            relative = path.relative_to(root).as_posix()
        except ValueError:
            continue
        if relative.startswith(HOST_ROOTS) and path.suffix == ".c":
            selected.setdefault(path, command)
    if not selected:
        raise RuntimeError("compilation database contains no host lint sources")
    return sorted(selected), list(selected.values())


def run(command: list[str]) -> None:
    subprocess.run(command, check=True)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, required=True)
    args = parser.parse_args()

    root = Path(__file__).resolve().parents[1]
    build_dir = args.build_dir.resolve()
    database = build_dir / "compile_commands.json"
    if not database.is_file():
        raise RuntimeError(f"missing compilation database: {database}")

    clang_tidy = require_tool("clang-tidy")
    cppcheck = require_tool("cppcheck")
    sources, commands = host_sources(database, root)

    with tempfile.TemporaryDirectory(prefix="rage-lint-") as temporary_dir:
        lint_database = Path(temporary_dir) / "compile_commands.json"
        lint_database.write_text(
            json.dumps(commands, indent=2) + "\n", encoding="utf-8")
        run([clang_tidy, "-p", temporary_dir, "--quiet", *map(str, sources)])
    run([
        cppcheck,
        "--enable=warning,style,performance,portability",
        "--error-exitcode=1",
        "--inline-suppr",
        "--suppress=missingIncludeSystem",
        "--std=c11",
        "-DRAGE_HOST_PORT=1",
        "-D__psyz=1",
        f"-I{root / 'src'}",
        f"-I{root / 'src/port'}",
        f"-I{root / 'src/port/include'}",
        f"-I{root / 'include'}",
        *map(str, sources),
    ])
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (RuntimeError, subprocess.CalledProcessError) as error:
        print(f"lint failed: {error}", file=sys.stderr)
        raise SystemExit(1)
