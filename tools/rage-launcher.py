#!/usr/bin/env python3
"""Launch a configured Rage Racer race scenario through the retail flow."""

from __future__ import annotations

import argparse
import configparser
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_CONFIG = ROOT / "race-scenario.ini"


def integer(value: str, name: str, low: int, high: int) -> int:
    try:
        parsed = int(value, 0)
    except ValueError as error:
        raise ValueError(f"{name} must be an integer") from error
    if not low <= parsed <= high:
        raise ValueError(f"{name} must be in {low}..{high}")
    return parsed


def load_config(path: Path) -> dict[str, str]:
    parser = configparser.ConfigParser()
    if not path.is_file():
        raise ValueError(f"scenario file does not exist: {path}")
    parser.read(path)
    if "race" not in parser:
        raise ValueError(f"{path}: missing [race] section")
    return dict(parser["race"])


def main() -> int:
    cli = argparse.ArgumentParser(description="Launch a Rage Racer scenario")
    cli.add_argument("config", nargs="?", type=Path, default=DEFAULT_CONFIG)
    cli.add_argument("--mode", choices=("grand-prix", "time-attack"))
    cli.add_argument("--series", choices=("grand-prix", "extra-gp"))
    cli.add_argument("--class", dest="class_index", type=int)
    cli.add_argument("--course", type=int)
    cli.add_argument("--car", type=int)
    cli.add_argument("--after-finish", choices=("menu", "repeat", "exit"),
                     help="what to do after the result screens")
    cli.add_argument("--grid", help="11 comma-separated rival car IDs; -1 disables a slot")
    cli.add_argument("--start-point", type=int,
                     help="player track-point index used after grid initialization")
    cli.add_argument("--rival-points",
                     help="up to 11 comma-separated track-point indices; '-' keeps retail pose")
    cli.add_argument("--binary", type=Path, default=ROOT / "build/release/rage-racer")
    cli.add_argument("--dry-run", action="store_true")
    args = cli.parse_args()

    try:
        values = load_config(args.config.resolve())
        mode = args.mode or values.get("mode", "grand-prix")
        series = args.series or values.get("series", "grand-prix")
        if mode not in ("grand-prix", "time-attack"):
            raise ValueError("mode must be grand-prix or time-attack")
        if series not in ("grand-prix", "extra-gp"):
            raise ValueError("series must be grand-prix or extra-gp")
        class_index = integer(
            str(args.class_index if args.class_index is not None else values.get("class", "0")),
            "class", 0, 5,
        )
        course = integer(
            str(args.course if args.course is not None else values.get("course", "0")),
            "course", 0, 3,
        )
        car = integer(
            str(args.car if args.car is not None else values.get("car", "3")),
            "car", 0, 12,
        )
        after_finish = args.after_finish or values.get("after_finish", "menu")
        if after_finish not in ("menu", "repeat", "exit"):
            raise ValueError("after_finish must be menu, repeat, or exit")
        grid_text = args.grid if args.grid is not None else values.get("grid", "default")
        grid = None
        if grid_text.strip().lower() != "default":
            entries = [integer(item.strip(), "grid entry", -1, 12) for item in grid_text.split(",")]
            if len(entries) != 11:
                raise ValueError("grid must contain exactly 11 entries")
            grid = ",".join(map(str, entries))
        if mode == "time-attack" and series == "extra-gp":
            raise ValueError("time-attack does not use the Extra GP series")
    except ValueError as error:
        cli.error(str(error))

    binary = args.binary.resolve()
    if not binary.is_file():
        cli.error(f"binary does not exist: {binary}")
    command = [binary, "--scenario", args.config.resolve()]
    overrides = {
        "race.mode": mode,
        "race.series": series,
        "race.class": class_index,
        "race.course": course,
        "race.car": car,
        "race.after_finish": after_finish,
    }
    if grid is not None:
        overrides["race.grid"] = grid
    if args.start_point is not None:
        if args.start_point < 0:
            cli.error("--start-point must be non-negative")
        overrides["start.player_track_point"] = args.start_point
    if args.rival_points is not None:
        entries = [item.strip() for item in args.rival_points.split(",")]
        if not 1 <= len(entries) <= 11:
            cli.error("--rival-points must contain 1..11 entries")
        for entry in entries:
            if entry != "-":
                try:
                    integer(entry, "rival track point", 0, 1000000)
                except ValueError as error:
                    cli.error(str(error))
        overrides["start.rival_track_points"] = ",".join(entries)
    for key, value in overrides.items():
        command.extend(("--set", f"{key}={value}"))
    summary = (
        f"mode={mode} series={series} class={class_index} "
        f"course={course} car={car} grid={grid or 'default'} "
        f"after_finish={after_finish} "
        f"start={args.start_point if args.start_point is not None else 'scenario-file'} "
        f"rivals={args.rival_points or 'scenario-file'}"
    )
    print(f"Rage Racer scenario: {summary}", flush=True)
    if args.dry_run:
        return 0
    return subprocess.run(command, cwd=ROOT).returncode


if __name__ == "__main__":
    raise SystemExit(main())
