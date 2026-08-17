#!/usr/bin/env python3
"""Release workflows ship the runtime files promised by the documentation."""

import re
import sys
from pathlib import Path


def main() -> int:
    root = Path(sys.argv[1])
    required = ("README.md", "LICENSE.md", "rage-port.ini",
                "rage-input.cfg", "race-scenario.ini")
    cmake = (root / "CMakeLists.txt").read_text(encoding="utf-8")
    if 'RAGE_RACER_RELEASE_VERSION "0.4-alpha"' not in cmake:
        raise AssertionError("CMake release version is not 0.4-alpha")
    for workflow_name in ("linux-release.yml", "windows-release.yml",
                          "macos-release.yml"):
        workflow = (root / ".github/workflows" / workflow_name).read_text(
            encoding="utf-8")
        for filename in required:
            if filename not in workflow:
                raise AssertionError(f"{workflow_name} does not package {filename}")
        if not re.search(r"default:\s*0\.4-alpha", workflow):
            raise AssertionError(f"{workflow_name} still defaults to an old version")
        if "tools/rage-launcher.py" not in workflow:
            if workflow_name != "macos-release.yml":
                raise AssertionError(
                    f"{workflow_name} does not package the scenario launcher"
                )
        if "*.zip" not in workflow:
            raise AssertionError(f"{workflow_name} does not upload a ZIP archive")
    for filename in required:
        if not (root / filename).is_file():
            raise AssertionError(f"missing packaged source file {filename}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
