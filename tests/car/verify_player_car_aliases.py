#!/usr/bin/env python3
"""Verify player-car interior aliases without a Ruby runtime dependency."""

import re
import sys
from pathlib import Path


EXPECTED = {
    "g_PlayerSegmentWeight": 0x38,
    "g_PlayerField3C": 0x3C,
    "g_PlayerSteerAngle": 0x44,
    "g_PlayerCarWheelAngle": 0x48,
    "g_PlayerRenderRotation": 0x50,
    "g_PlayerRenderY": 0x60,
    "g_PlayerFacingBackwards": 0xB8,
    "g_PlayerVelocity": 0xC4,
    "g_PlayerTransmission": 0x130,
    "g_PlayerTargetRpm": 0x134,
    "g_PlayerThrottle": 0x15C,
    "g_RacePosition": 0x160,
    "g_HudLapHighlightRow": 0x162,
}


def main() -> int:
    root = Path(sys.argv[1])
    aliases = (root / "include/game/player_car_aliases.h").read_text()
    car_header = (root / "include/game/car.h").read_text()
    # Retail state is one segment split across a file per owning
    # subsystem, so read it back as the one thing it is.
    host_state = "\n".join(
        path.read_text()
        for path in sorted((root / "src/port").glob("host_state*.c")))
    native_state = (root / "src/port/native_game_state.c").read_text()
    headers = "\n".join(path.read_text() for path in (root / "include").rglob("*.h"))

    for name, offset in EXPECTED.items():
        if re.search(rf"^unsigned char {re.escape(name)}\[", host_state, re.M):
            raise AssertionError(f"host state still allocates independent {name} storage")
        if re.search(rf"extern\s+[^;\n]*\b{re.escape(name)}(?:\[|\s*;)", headers):
            raise AssertionError(f"a header still declares independent {name} storage")
        if name == "g_PlayerThrottle":
            continue
        pattern = rf"#define\s+{re.escape(name)}\b[\s\\\n]*.*?\+\s*0x{offset:X}\b"
        if re.search(pattern, aliases, re.S) is None:
            raise AssertionError(f"{name} is not aliased at +0x{offset:X}")

    if re.search(r"void \*GetPlayerCarStorage\(void\).*?return &g_PlayerCar;",
                 native_state, re.S) is None:
        raise AssertionError("player alias storage accessor is missing")
    if car_header.count("must retain its retail alias offset") != 12:
        raise AssertionError("player layout assertions are missing")
    player_update = (root / "src/main/PAL/main/car/update_player_car.c").read_text()
    if "p->acceleratorInput.value" not in player_update:
        raise AssertionError("throttle must be read from the drivetrain field")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
