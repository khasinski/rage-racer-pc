# Rage Racer PC project rules

## Shipping contract

- A clean release must start by double-clicking it on every supported platform.
- The only game data a player may be asked to provide is their legally obtained
  Rage Racer CUE or Track 01 BIN image.
- The game must generate every native asset required by the modern renderer
  automatically after that selection and then start in the modern renderer.
- Missing generated assets must not be worked around by silently falling back
  to the classic renderer or by asking the player to run a separate command.

## Implementation constraints

- Runtime, asset import, tools, builds, and tests must be implemented in C or
  the repository's existing compiled toolchain. Do not add or require Python,
  an embedded Python interpreter, PyInstaller, or Python-based helper programs.
- Remove existing Python components only after their behavior and regression
  coverage have compiled replacements, so the migration remains testable at
  every commit.
