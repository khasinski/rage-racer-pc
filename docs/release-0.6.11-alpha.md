# Rage Racer PC 0.6.11-alpha

This alpha adds a live 30 Hz / 60 Hz race physics switch under Esc, Controls,
finishes 60 Hz support across manual shifting, jumps and collisions, adds an
on-screen touch control overlay, and speeds up the modern renderer, CHD disc
reads and the Rage Racer Editor.

## Changes

- Esc, Controls, Race physics rate switches active races between `30 Hz` (the
  original PlayStation two-field cadence) and `60 Hz` (every video field).
  Manual gear shifts no longer fire twice on odd substeps, countdown timers
  (`clutch`, `jumpTimer`, `boostTimer`, grip loss and auto-shift cooldowns)
  keep their full initial duration, and collision knockback, body kick, crest
  hops and cockpit tilt step at the retail rate. `game.tick_rate` in
  `rage-port.ini` (`retail` or `double`) sets the startup default.
- Touch input draws an on-screen steering wheel, throttle and brake pedals,
  gear shift buttons, camera toggle and menu controls, with High-DPI scaling
  and clean transitions back to gamepad or keyboard input.
- CHD disc images open separate data and CD-audio handles so background music
  streaming and asset reads never contend on the same decompression buffer.
- The modern renderer indexes prepared materials and resident GPU mesh buffers
  by hash, pre-filters shadow cascades, reuses static world-frame transforms
  and culls degenerate triangles earlier.
- The Rage Racer Editor caches disc fingerprints, track packs, whole-car
  previews and course texture sheets, preserves custom track (`bank >= 136`)
  mesh IDs in Car Studio, and saves road drafts automatically when switching
  workspaces.

## Still open

The Windows second-race render report is not confirmed fixed. Per-action
controller remapping is still the `rage-port.ini` bindings.
