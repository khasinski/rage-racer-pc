#!/usr/bin/env python3
import os
import subprocess
import sys

binary, root = sys.argv[1:3]
env = os.environ.copy()
env.update(SDL_AUDIODRIVER="dummy", RAGE_PORT_SMOKE_FRAMES="1",
           RAGE_PORT_SMOKE_INITIAL_STATE="1")
result = subprocess.run([binary], cwd=root, env=env, stdout=subprocess.PIPE,
                        stderr=subprocess.STDOUT, text=True, timeout=60)
if result.returncode != 0:
    print(result.stdout, file=sys.stderr)
    raise SystemExit(result.returncode or 1)
expected = "initial state: time_attack_enabled=1 selected_car=3"
if expected not in result.stdout:
    print(result.stdout, file=sys.stderr)
    raise AssertionError(f"missing {expected!r}")
print(expected)
