#!/usr/bin/env python3
"""Where the tests look for the disc, and what to say when there is none.

The movies and the CD-DA music are on the disc rather than in the extracted
archive, so the handful of tests covering them need one. In test mode the port
takes the disc from disc.image or disc.cue, falls back to disc/PAL under the
source tree, and carries on quietly when it finds neither. That is right for
the hundred tests that do not care, and it is what leaves these ones failing on
a missing sound or an unreadable movie rather than on the missing disc. They
skip with the reason instead.
"""

from __future__ import annotations

import os
from pathlib import Path

# ctest reads this exit code as "skipped" rather than "failed".
SKIP_EXIT_CODE = 77

DEFAULT_CUE = Path("disc") / "PAL" / "Rage Racer (Europe).cue"


def find_disc(source: Path) -> Path | None:
    """The places HostInitDisc looks in test mode, in the same order."""
    for name in ("RAGE_PORT_DISC_IMAGE", "RAGE_PORT_DISC_CUE"):
        value = os.environ.get(name)
        if value:
            return Path(value) if os.access(value, os.R_OK) else None
    candidate = source / DEFAULT_CUE
    return candidate if os.access(candidate, os.R_OK) else None


def require_disc(source: Path) -> None:
    """Ends the test as skipped when no disc is reachable."""
    if find_disc(source) is not None:
        return
    print(
        "no disc image. This test covers the movies or the CD-DA music, which "
        "are on the disc and not in the extracted archive.\n"
        f"Put one at {DEFAULT_CUE} under the source tree - a symlink to the "
        "directory holding the cue sheet and its track files is enough - or "
        "set RAGE_PORT_DISC_CUE to a cue sheet."
    )
    raise SystemExit(SKIP_EXIT_CODE)
