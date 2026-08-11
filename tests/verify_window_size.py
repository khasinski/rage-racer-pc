#!/usr/bin/env python3
import os
import subprocess
import sys


binary, root = sys.argv[1:3]
env = os.environ.copy()
env.update(
    SDL_AUDIODRIVER="dummy",
    RAGE_PORT_SMOKE_FRAMES="1",
    RAGE_PORT_SMOKE_WINDOW_SIZE="1",
)
result = subprocess.run(
    [binary], cwd=root, env=env, stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT, text=True, timeout=20,
)
if result.returncode != 0:
    print(result.stdout, file=sys.stderr)
    raise SystemExit(result.returncode or 1)
if "window size: 640x480" not in result.stdout:
    print(result.stdout, file=sys.stderr)
    raise AssertionError("initial window is not logically 640x480")
print("initial window is logically 640x480")
