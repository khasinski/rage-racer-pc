#!/usr/bin/env python3
"""Keep simulation and modern scene code outside the PSY-Z boundary."""

from pathlib import Path
import sys


def main() -> int:
    source = Path(sys.argv[1])
    cmake = (source / "CMakeLists.txt").read_text(encoding="utf-8")
    sim_block = cmake.split("add_library(rage-sim STATIC", 1)[1].split(")", 1)[0]
    if "psyz" in sim_block.lower():
        raise AssertionError("rage-sim directly names PSY-Z")

    forbidden = ("<psyz/", '"psyz/', "<psyq/", '"psyq/')
    roots = [source / "src/render"]
    files = list((source / "src/render").glob("*.[ch]"))
    files += [source / path.strip().replace("${CMAKE_CURRENT_SOURCE_DIR}/", "")
              for path in cmake.split("set(RAGE_SIM_SOURCES", 1)[1]
                               .split(")", 1)[0].splitlines()
              if path.strip()]
    for path in files:
        text = path.read_text(encoding="utf-8")
        if any(token in text for token in forbidden):
            raise AssertionError(f"renderer-neutral code imports PSY-Z: {path}")
    if not roots[0].is_dir():
        raise AssertionError("Render World source directory is missing")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
