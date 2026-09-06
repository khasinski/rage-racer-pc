# Regional modern-renderer smoke tests

On Linux, build `rage-racer-smoke` and supply legally obtained CUE images:

```sh
SDL_VIDEODRIVER=offscreen \
RAGE_PORT_PAL_CUE='/path/to/Europe.cue' \
RAGE_PORT_NTSC_U_CUE='/path/to/USA.cue' \
RAGE_PORT_NTSC_J_CUE='/path/to/Japan.cue' \
ctest --test-dir build -R '^modern_region_' --output-on-failure
```

The tests use separate configuration/state directories and do not modify the
player's INI. PAL can also use the repository's conventional local disc path;
NTSC tests require their dedicated variables. Missing images are reported as
skipped. A supplied image from the wrong region fails rather than satisfying
another region's test.

Each scenario checks detected disc identity, automatic PAL 50 Hz or NTSC 60 Hz
logic timing, modern-renderer activation and four submitted-frame captures
around logic frame 1000 in class 1/course 0. Captured sampled VRAM must match
the recorded scene frame. These are smoke/consistency checks, not pixel
golden-image comparisons, measured display FPS, FMV-audio validation or long
race/reward sequences. Audio uses the dummy driver. Other suites cover those
separate requirements; this test alone does not certify a release.

For the longer resource-lifecycle regression, build `rage-racer` and use the
same environment variables with `ctest --test-dir build -R '^modern_repeat_'`.
These scenarios complete two class-1/course-0 races in one process with the
intervening replay/reward/menu flow, three presentation restarts, automatic
regional timing, a sampled-VRAM cache oracle and ordered teardown checks.
The final race exits at the handoff to replay, before its own reward screen.
The driver follows the route and bypasses player input/drivetrain physics;
this is not an input replay or FMV audio test. Rendering settings come from a
read-only configuration; instrumentation overrides disable marker/history
capture. Allow up to ten minutes per region. The tests carry the `endurance`
label so they can be selected separately from the short smoke scenarios.
