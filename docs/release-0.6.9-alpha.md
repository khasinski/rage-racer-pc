# Rage Racer PC 0.6.9-alpha

This alpha retires the experimental ray tracer from 0.6.8-alpha-raytracer.
The modern renderer is rasterized sun lighting, cascaded sun shadows, ambient
occlusion and car spot lights. It runs on the same GPU backends as the rest of
the modern renderer.

## Changes

- The sun follows the scene sky. Four cascaded shadow maps cover the car, and
  scenery past the camera still casts when it reaches a cascade. Full shade is
  a step brighter than in the first shadow build. Ambient occlusion darkens
  creases, and headlights, tail lamps and brake lights are spot lights.
- Esc pauses the game and opens settings. Graphics covers the renderer,
  resolution, aspect, frame rate, filtering, anti-aliasing, colour, shadows,
  ambient occlusion and draw distance. Controls covers analog steering, the
  wheel, steering feel and force feedback. Audio covers music and effects.
  View covers the HUD and the chase camera. Page Up, Page Down or the shoulder
  buttons change page.
- Graphics, analog steering, the wheel, steering feel, the HUD and the chase
  camera apply immediately and last until quit. Music and effects are the
  save-game volumes. Force feedback is remembered with the player file.
  Keyboard bindings stay in `rage-port.ini`.
- The title menu can start a Duel. A CPU rival can chase in that race.
- Arcade force feedback has a pause-menu switch.
- Custom Race can open the paint shop. Automatic transmission is offered on
  every car unless `cars.retail_manual_only` keeps the original rule.
- `video.fmv_zoom` can crop movies to fill the window.
- The editor imports Ridge Racer Type 4 courses, including Out of Blue, into
  extra track slots. Custom track slots go to 32.
- The standalone Rage Racer Editor ships beside the game.
- An Android build reads a disc image from app storage.
- macOS archives are Developer ID signed and notarized.

## Still open

The Windows second-race render report is not confirmed fixed. Per-action
controller remapping is still the `rage-port.ini` bindings.
