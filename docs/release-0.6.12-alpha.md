# Rage Racer PC 0.6.12-alpha

This alpha fixes the 60 Hz jump/landing drivetrain stall on Mythical Coast,
eliminates frame-rate drops when running 60 Hz race physics in the modern
renderer, finishes 60 Hz parity across standing starts, drifting, gear shifts,
rival AI and the chase camera, and adds support for higher-resolution mod
textures across cars, tracks, skies and menus.

## Changes

- 60 Hz race physics (`game.tick_rate = double` or `Esc`, `Controls`,
  `Race physics rate`) no longer stalls the car after the jump on Mythical
  Coast: countdown timers (`jumpTimer`, `clutch`, `boostTimer`, grip loss and
  auto-shift cooldowns) hold on the first substep of each pair and decrement on
  the second, keeping `CAR_MOTION_AIRBORNE` active until the landing frame so
  `ReconnectPlayerDrivetrain` always runs.
- 60 Hz race physics now matches the retail 30 Hz simulation across gear-shift
  and airborne `engineRpm` / `engineLoad` interpolation, standing starts and
  body bounce, counter-steering zero crossings, drift takeoff and recovery
  (`targetHeading`, `trackCurveBias`, depleted launch), knockback re-triggering,
  wheel rotation (`wheelSpinSpeed`, `frontWheelSpin`), rival AI steering and
  respawns, player-rival collision separation, chase-camera tracking and
  mid-frame finish/retire/pause transitions.
- The modern renderer now prepares 3D world materials and meshes during 60 Hz
  race substeps (`frameLimit == 0xC0`) instead of deferring preparation to the
  presented frame, and caches fully prepared submeshes in a 1024-slot hash
  table so `ModernAssetsPrepareInstance` skips redundant per-vertex scans.
- Higher-resolution PNG texture replacements now work across car materials
  (with scaled car-paint masks), track surfaces and scenery, sky panoramas, raw
  texture patches and menu background images.

## Still open

The Windows second-race render report is not confirmed fixed. Per-action
controller remapping is still the `rage-port.ini` bindings.
