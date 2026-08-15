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
    generator = Path(sys.argv[3]).resolve()
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

        for label, with_save in (("existing", True), ("empty", False)):
            load_work = work / f"load-{label}"
            load_work.mkdir()
            (load_work / "assets").symlink_to(
                source_dir / "assets", target_is_directory=True
            )
            (load_work / "bu00").mkdir()
            if with_save:
                source_save = files[0]
                (load_work / "bu00" / source_save.name).write_bytes(
                    source_save.read_bytes()
                )
            load_environment = os.environ.copy()
            load_environment.update(
                SDL_AUDIODRIVER="dummy",
                RAGE_PORT_SMOKE_FRAMES="1150",
                RAGE_PORT_RAW_INPUT_SCRIPT=(
                    "400:START,500:START,600:DOWN,620:DOWN,650:CROSS,"
                    "880:UP,900:CROSS,1050:CROSS"
                ),
            )
            load = subprocess.run(
                [executable], cwd=load_work, env=load_environment,
                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
                timeout=25,
            )
            if load.returncode != 0:
                print(load.stdout, file=sys.stderr)
                raise AssertionError(
                    f"LOAD GAME with {label} slot crashed ({load.returncode})"
                )
            if "stopped at frame 1150, scene 26" not in load.stdout:
                raise AssertionError(f"LOAD GAME with {label} slot left the card menu")

        complete_work = work / "complete"
        complete_work.mkdir()
        (complete_work / "assets").symlink_to(
            source_dir / "assets", target_is_directory=True
        )
        generated = subprocess.run(
            [generator, "--name", "UNLOCK"], cwd=complete_work,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
            timeout=5,
        )
        if generated.returncode != 0:
            print(generated.stdout, file=sys.stderr)
            raise AssertionError("complete save generator failed")
        complete_environment = os.environ.copy()
        complete_environment.update(
            SDL_AUDIODRIVER="dummy",
            RAGE_PORT_SMOKE_FRAMES="700",
            RAGE_PORT_SMOKE_COMPLETE_SAVE_LOAD="1",
            RAGE_PORT_INPUT_SCRIPT="400:START,500:START,650:CROSS",
        )
        complete = subprocess.run(
            [executable], cwd=complete_work, env=complete_environment,
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
            timeout=25,
        )
        if complete.returncode != 0:
            print(complete.stdout, file=sys.stderr)
            raise AssertionError("game rejected the generated complete save")
        if "complete generated save loaded: classes=4/5 cars=13/13/13" not in complete.stdout:
            raise AssertionError("generated save did not unlock complete progress")
    print("Save round trip and graphical memory-card menu both succeeded")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
