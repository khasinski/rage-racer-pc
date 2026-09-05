# FMV regression checks

Build `rage-racer-smoke`, `rage-pcm-check` and `rage-fmv-pacing-check`, then run
`ctest --test-dir build -R '^modern_fmv_audio' --output-on-failure`.
Set `RAGE_PORT_DISC_IMAGE` to a legally obtained Track 01 BIN, or
`RAGE_PORT_DISC_CUE` to a CUE. On a headless Linux system, set
`SDL_VIDEODRIVER=offscreen` if that SDL/Vulkan backend is available. Each run
uses SDL dummy audio and writes evidence into a unique `build/modern-fmv-*`
directory. The tests do not change the user's INI configuration.

The eleven cases cover each retail movie, including all promotion movies,
opening and ending. They require explicit modern selection, every frame of
the first playback in order, XA start/end and nonzero captured stereo PCM.
The C PCM verifier independently checks actual sample count and absolute
amplitude sum against the mixer counters. The ending is allowed its authored
silent picture tail. These checks do not assert uninterrupted audio at every
instant, physical speaker output or pixel-exact image correctness.

Opening (stream 0) and promotion (stream 5) additionally run the C sector
pacing oracle. It opens the original BIN/CUE without SDL or the game loop,
finds RAGE.STR and its stream table, walks STR chunks, and compares every
reported frame-end sector with the actual data. The elapsed simulation ticks
must agree within 2% with both XA duration (retail stereo 37800 Hz / 4-bit) and
150 sectors/second. The reported tick rate must match the disc region.
Unknown XA encoding, malformed chunk order and invalid traces fail closed.
The timing oracle currently accepts BIN/CUE, not CHD.

Each paced case also creates a separate negative trace with an artificially
delayed last frame, preserving the frame count, sectors and PCM metrics. The
oracle must reject it. Original logs and disc data are never modified.

The tools can also check previously captured evidence:

```
rage-pcm-check audio.s16le EXPECTED_STEREO_FRAMES EXPECTED_ABSOLUTE_ENERGY
rage-fmv-pacing-check "game.cue" 5 game.log
```

Smoke is unthrottled. Correct simulation-tick pacing is not a wall-clock
audio/video synchronization or audio-device-latency measurement. Direct FMV
selection also does not test winning an entire class or the award transition.
Existing Python pacing tests remain until the compiled migration has complete
equivalent coverage; no new Python dependency is introduced here.
