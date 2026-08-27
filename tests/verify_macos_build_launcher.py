#!/usr/bin/env python3
"""Do not let a stale pre-bundle executable shadow the current macOS game."""

from __future__ import annotations

import os
import sys
from pathlib import Path


def main() -> int:
    launcher = Path(sys.argv[1])
    executable = Path(sys.argv[2])
    if not launcher.is_symlink():
        raise AssertionError(
            f"{launcher} must be a symlink, not a stale standalone executable"
        )
    if launcher.resolve() != executable.resolve():
        raise AssertionError(
            f"{launcher} resolves to {launcher.resolve()}, expected {executable.resolve()}"
        )
    if not os.access(executable, os.X_OK):
        raise AssertionError(f"current bundle executable is not runnable: {executable}")
    print(f"macOS build launcher resolves to current bundle: {executable}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
