# 120 Hz presentation and CPU geometry checkpoint

The requested target is at least 120 FPS on the user's machine, synchronized
to its display, with unchanged game speed and visual settings. It is not met
yet. Performance work now takes priority over general asset refactoring.

## Display detection and synchronization

KDE reports HDMI-A-4 at 3840x2160/120 Hz. The GPU is an AMD Navi 48, with an
Intel UHD 770 also present. The user's existing modern settings render at
1706x960 (scale 4, 16:9), with filtering, FXAA and vibrant grading retained.
These results do not claim native 4K rendering.

The host-built SDL lacked Wayland and XRandR support and reported 60 Hz.
Reconfigured the existing build-linux-gcc16 in the rage-racer-dev container,
which has the required development dependencies. Its SDL summary now enables
Wayland and XRandR, and the game logs `driver=wayland refresh_hz=120.000`.
Build future changes in that environment to preserve these features:

```sh
podman exec -w /var/home/hasik/rage-racer-pc rage-racer-dev \
  cmake --build build-linux-gcc16 --target rage-racer --parallel
```

Previously `video.fps=vsync` paced the modern renderer to the detected display
rate, while the platform could select immediate presentation because PAL
VBlank runs at 50 Hz. A host presentation request now selects the VSync
swapchain mode independently of the emulated VBlank clock. Classic restores
the platform policy. Intermediate acquisition remains nonblocking. Logs expose
both the display request and the independent logic driver-VSync flag.

## Measurements

PAL Mythical Coast, class 1, one automated lap, original local INI, prewarm on.
All four runs completed. FPS below is computed over complete 120-render
reporting windows after excluding the first startup window; the mean is
weighted by their duration. Queue counts measure submitted swapchain images,
not direct scanout instrumentation.

| Local session under build/ | Mean FPS | Slowest 120-frame window | Notes |
| --- | ---: | ---: | --- |
| perf-120hz-reference/20260907-142811-8632f1 | 59.90 | 58.50 | Missing display backends; per-frame tracing enabled |
| perf-120hz-wayland/20260907-143223-72ce97 | 107.17 | 96.13 | Correct refresh; experimental static CPU cache |
| perf-120hz-wayland-reference/20260907-143335-a56420 | 107.22 | 95.40 | Correct refresh; CPU cache disabled |
| perf-120hz-vsync/20260907-143626-704125 | 112.47 | 95.38 | Correct refresh and explicit display VSync; CPU cache removed |

These sequential runs establish the former 60 Hz detection problem and current
performance range, not an isolated speedup percentage from VSync. The first
run used heavier tracing. Per-run result.txt records executable/config hashes.
The final VSync run's worst window p95 was 17.385 ms, and its highest mean CPU
geometry preparation was 8.199 ms. The 120 FPS budget is 8.333 ms for the whole
frame, so geometry alone can exhaust it. Many other windows sustain 120 FPS.

Added mean CPU preparation to the low-overhead aggregate profiler, along with
the selected SDL display rate. Fixed the initial queue-count baseline so it
does not include earlier menu presentations. The final executable includes
that reporting fix; the table excludes the affected first window.

## Experiments and regression evidence

A bounded static-triangle CPU cache preserved exact draw dumps and images in
three paired 300-repeat frozen-scene checks. It improved median preparation
only about 4%, and the moving-lap pair above showed no meaningful improvement.
It was removed from the working implementation. The experiment is retained in
/tmp/rage-static-triangle-cache-wip.patch; it is not persistent GPU geometry.
The earlier geometry-pack prototype and user INI remain untouched. Unfinished
importer metadata/image splitting is parked in /tmp/rage-lazy-material-wip.patch.

After rebuilding the game and consumers, offscreen mirror, native-world,
renderer-toggle and timing-restore tests pass. The compiled modern-frame-pacer
test passes after building its previously absent executable. A separate real
Wayland renderer-toggle test passes, checks display VSync with
logic_driver_vsync=0, and verifies restoration after returning to classic.

Next: reduce repeated transformation/expansion and upload of vehicle geometry,
including interpolated frames. Keep the CPU reference path and compare the
same main/mirror views. Validate moving scenes, frame tails and original game
timing; do not infer completion from frozen-scene benchmarks or average FPS.
Windows/macOS synchronization and sustained 120 FPS remain unverified.
