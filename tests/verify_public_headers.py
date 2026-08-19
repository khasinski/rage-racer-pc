#!/usr/bin/env python3
"""Compile modular gameplay headers one at a time to enforce self-containment."""

import pathlib
import subprocess
import sys
import tempfile


HEADERS = (
    "game/car_control_command.h",
    "game/car_select_controller.h",
    "game/course_select_controller.h",
    "game/design_controller.h",
    "game/memory_card_controller.h",
    "game/memory_card_types.h",
    "game/menu_controller.h",
    "game/menu_dialog_controller.h",
    "game/menu_runtime.h",
    "game/option_controller.h",
    "game/player_car_simulation.h",
    "game/shop_controller.h",
    "game/team_logo_editor_controller.h",
    "game/team_name_controller.h",
)


def main() -> int:
    compiler = sys.argv[1]
    source_root = pathlib.Path(sys.argv[2]).resolve()
    include_dir = source_root / "include"

    with tempfile.TemporaryDirectory(prefix="rage-header-check-") as temp_dir:
        source = pathlib.Path(temp_dir) / "header.c"
        for header in HEADERS:
            source.write_text(f'#include "{header}"\n', encoding="utf-8")
            completed = subprocess.run(
                [compiler, "-std=c99", "-D__psyz=1", "-Wall", "-Wextra",
                 "-Werror", "-fsyntax-only", "-I", str(include_dir),
                 str(source)],
                text=True,
                capture_output=True,
            )
            if completed.returncode != 0:
                sys.stderr.write(f"header is not self-contained: {header}\n")
                sys.stderr.write(completed.stdout)
                sys.stderr.write(completed.stderr)
                return completed.returncode
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
