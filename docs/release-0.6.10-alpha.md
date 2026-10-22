# Rage Racer PC 0.6.10-alpha

This alpha keeps the nearest piece of road when it crosses the camera, adds a
switch for car lights, and makes the chase camera's corner look a small glance.

## Changes

- At high speed in the cockpit, the road quad passing through the camera was
  dropped for one logic frame and the empty background showed through. That
  quad is clipped to the near plane and kept. A corner the PlayStation
  geometry engine wraps to the opposite edge of the screen is no longer the
  interpolation target, and a child polygon on the lower display buffer
  follows its parent with the draw offset included.
- Esc, Graphics, Car lights turns headlights, tail lamps and brake lights on
  or off for every car. On is the previous behaviour: headlights in the dark
  and in tunnels, tail lamps dim in daylight, brake lamps while braking. Off
  removes all of them, including the daylight tail glow. The beams are drawn
  by the modern renderer. `video.car_lights` in `rage-port.ini` is `on` or
  `off` and sets the value used at startup.
- Esc, View, Corner look is much smaller. Full is about 3 degrees at full
  steering lock, and Half is about 0.7 degrees. Off is still the original
  camera. `[camera] chase_turn_lookahead` still runs from `0` to `1`, and `1`
  is still about 30 degrees.

## Still open

The Windows second-race render report is not confirmed fixed. Per-action
controller remapping is still the `rage-port.ini` bindings.
