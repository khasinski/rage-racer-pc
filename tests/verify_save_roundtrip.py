#!/usr/bin/env python3
"""Write and reload a retail-format memory-card save through the host HAL."""

from __future__ import annotations

import os
import subprocess
import sys
import tempfile
from pathlib import Path


def main() -> int:
    executable = Path(sys.argv[1]).resolve()
    source_dir = Path(sys.argv[2]).resolve()
    with tempfile.TemporaryDirectory(prefix="rage-save-test-") as directory:
        work = Path(directory)
        (work / "assets").symlink_to(source_dir / "assets", target_is_directory=True)
        environment = os.environ.copy()
        environment.update(
            SDL_AUDIODRIVER="dummy",
            RAGE_PORT_SMOKE_FRAMES="700",
            RAGE_PORT_SMOKE_SAVE_ROUNDTRIP="1",
            RAGE_PORT_INPUT_SCRIPT="400:START,500:START,650:CROSS",
        )
        result = subprocess.run(
            [executable], cwd=work, env=environment,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
            timeout=25,
        )
        if result.returncode != 0:
            print(result.stdout, file=sys.stderr)
            return result.returncode or 1
        if "save roundtrip ok: money=123456789" not in result.stdout:
            raise AssertionError("saved progress was not restored")
        files = list((work / "bu00").iterdir())
        if len(files) != 1 or files[0].stat().st_size != 0x1300:
            raise AssertionError("host memory card did not contain one retail-size save")
        if files[0].name != "BESCES-00650 RAGE000":
            raise AssertionError(f"memory-card filename was truncated: {files[0].name!r}")

        menu_environment = os.environ.copy()
        menu_environment.update(
            SDL_AUDIODRIVER="dummy",
            RAGE_PORT_SMOKE_FRAMES="900",
            RAGE_PORT_INPUT_SCRIPT=(
                "400:START,500:START,600:DOWN,620:DOWN,650:CROSS"
            ),
        )
        menu = subprocess.run(
            [executable], cwd=work, env=menu_environment,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
            timeout=25,
        )
        if menu.returncode != 0:
            print(menu.stdout, file=sys.stderr)
            return menu.returncode or 1
        if "stopped at frame 900, scene 26" not in menu.stdout:
            raise AssertionError("title-screen memory-card menu did not open")
        if "files=1 free=15 mask=1 page=0" not in menu.stdout:
            raise AssertionError("graphical memory-card menu did not detect slot 0")
    print("Save round trip and graphical memory-card menu both succeeded")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
