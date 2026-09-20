# Rage Racer PC 0.6.8-alpha-raytracer

**Experimental graphics pre-release. Ray tracing is enabled by default.**
This build is intended for testing; it does not replace the stable Latest release.

The modern renderer now combines rasterized geometry with ray-traced sun shadows
and closest-hit reflections. This is a hybrid renderer, not a path tracer. Ray
traversal uses portable GPU shaders and instanced acceleration structures, with
static mesh data retained between frames. It does not require an RTX-specific
backend, but performance depends strongly on the GPU and rendering resolution.

## Changes

- Scene sky colors drive directional lighting and shadow color. Cars and track
  surfaces can receive partial shadows from scenery, bridges and tunnel shells.
- Modern fog and vehicle contact with the road have been adjusted.
- Player and rival cars have automatic headlights and rear running lights in
  darkness and tunnels, plus brake lights. Lamp layouts follow their individual
  models and textures; replays preserve the active cars' brake commands.
- Headlights illuminate nearby surfaces and have an optical halo. Shadow-map
  filtering and brake-light presentation latency have also been improved.
- Additional fixes preserve scene fades, class-clear fanfare, transmission
  selection and menu reset state, and prevent empty memory-card directories
  from overriding the selected save location.

## Known experimental issues

- Patterned/self-shadowing artifacts can remain on car bodies at intermediate
  lighting levels. Some vehicle shadows still appear blocky or unstable.
- The headlight beam is too broad vertically and can light upper building
  surfaces instead of remaining concentrated on the road.
- Intermittent brake-light flicker has been reported and is not yet confirmed
  fixed. The interpolation delay has been addressed separately.
- Reflections and shadow coverage are incomplete and visual artifacts are
  expected, including incomplete transparency/cutout handling in traced shadows.
  Ray tracing can substantially reduce frame rate.

To disable ray tracing, edit the `rage-port.ini` beside the game and restart:

```ini
[modern]
ray_tracing = off
```

The other modes are `shadows`, `reflections`, and `full` (the default). Existing
configurations that explicitly select `off` retain that choice. Disabling ray
tracing does not disable the modern renderer or the car lights.

## Running the game

Download the archive for macOS Apple Silicon, Linux x86-64 or Windows x86-64,
extract it, and launch the game. Provide your legally obtained Rage Racer CUE
or Track 01 BIN when requested. Required native assets are imported automatically;
no separate extraction tool or downloaded game assets are needed. The editable
`cars.toml` and `cars.ntscj.toml` templates are included.

The gear shift points and Custom menu icon are by Sevish. The adjustable chase
camera settings were suggested by Passion Wagon.
