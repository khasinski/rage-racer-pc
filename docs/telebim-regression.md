# Start-line animated screen regression (open)

Implemented candidate: retain the old clip-depth adjustment near the camera
but bound it by the projection of 0.25 world units per authored bias step.
GLSL and generated SPIR-V/MSL are updated together. Point 288 now shows the
complete screen with the roof retained. native_depth_bias_gpu reads actual
gl_FragCoord.z from the production vertex shader: 192 cases at depths
1000..4500 with bias +/-9 stay within +/-4 world units; at depths <=2500
the bias must also retain its direction. Both the old shader and a zero-bias
mutation fail; the candidate passes alongside fog and shadow UV tests.

A production PAL/class-1/course-0 route completed three laps and one race
(/tmp/rage-bias-drive/game.log). A second bounded route recorded frames
1130..1248 individually in /tmp/rage-bias-barrier-dense and stopped at scene
12/timer 1030. Visual inspection of consecutive frames 1176..1183 shows a
continuous yellow chevron barrier in main and mirror views. This is sampled
visual evidence, not exhaustive temporal/pixel verification of all barriers;
Windows/Metal GPU execution and additional regional visual checks remain open.

Cause isolated: zeroing only the terrain depth-bias contribution restores the
complete screen while retaining the roof (/tmp/rage-telebim-no-bias.ppm).
The current vertex shader adds bias/1048576 in normalized depth; at this
distance the terrain's positive bias can override real screen/roof separation.
This is not a missing imported backing. The global zero-bias experiment was
reverted and smoke rebuilt: removing every terrain bias could revive barrier
flicker. A bounded depth-separation correction with screen and coplanar-barrier
regressions remains required before calling the issue fixed.

Latest isolation: omitting only COURSE mesh 57 from native GPU submission at
point 288 reveals the entire black screen backing and frame, with animation
still present. Evidence: /tmp/rage-telebim-without57.ppm. The backing therefore
exists in the native scene; model 57 is the occluder. This supersedes the
earlier missing-backing hypothesis below. The omission was reverted and smoke
rebuilt. Next compare native model-57 geometry/depth with the classic draw,
including SubmitCourseModel2 ordering semantics; do not ship mesh omission.

Reported symptom: the roof behind the screen obscures it while approaching;
the screen gradually emerges. Do not treat successful route completion as a
visual regression gate for this issue.

## Local reproduction

Linux offscreen Vulkan, current development build after da5aff410, PAL,
Mythical Coast, class 1, car 3, scenario direct boot. Freeze the player at
track point 288 and capture scene 12 at timer 430. At this position classic
shows a black screen surface under the finish gantry; modern shows the blue
roof in that area with fragments of the animation still visible.

Common arguments to build/rage-racer-smoke:

```
--scenario race-scenario.ini --set race.class=1 --set race.course=0
--set start.player_track_point=288 --set start.freeze=true
--set diagnostics.marker_capture=false
--set stop.scene=12 --set stop.timer=435 --set run.frames=1500
```

For modern add video.renderer=modern and diagnostics.modern_dump pointing to
a PPM path, diagnostics.modern_dump_scene=true,
diagnostics.modern_dump_scene_id=12 and diagnostics.modern_dump_timer=430.
For classic use video.renderer=classic and capture.directory (must already
exist), capture.scene=12, capture.timer_min=430, capture.timer_max=430,
capture.timer_stride=1. Prefix these settings with --set as above.

Local evidence (temporary, not portable fixtures):

- /tmp/rage-telebim-288.ppm and its .draws.txt, .world.bin, .scene.bin siblings.
- /tmp/rage-telebim-classic288/timer-00430-s12.ppm and capture-manifest.csv.
- Additional modern positions: 280, 285, 290, 298, 302, 306, 330.

## Established and unresolved

The modern dump contains the animated layers: source entities 196640/196641,
course asset key 96, meshes 35/8, six vertices each, materials 6/2. Therefore
these layers are not entirely dropped before draw construction. This does not
prove their transform, material alpha, supporting geometry or depth is right.

DrawAnimatedScenery submits both layers through
GameRenderWorldSubmitDynamicCourseOverlay. Modern draws these after opaque
scenery, with ordinary LESS_OR_EQUAL depth testing and a small normal-based
offset. The textured shader also discards transparent texels. Next diagnosis
must distinguish missing/transparent backing from genuine roof occlusion;
neither disabling depth testing nor making all black pixels opaque is a
justified fix without that evidence. No cause or first-bad commit established.

The screenshots differ in resolution and aspect; they establish a visible
candidate, not a pixel-identical oracle. User confirmation of the exact object
is still useful. No renderer changes have been made for this issue yet.

A temporary material-load probe at point 288 confirms material 2's decoded
256x256 atlas contains no zero-alpha texels and tens of thousands of opaque
black texels (counts vary with palette variant). Material 6 contains 3387
zero-alpha texels. Thus black is not universally decoded as transparent.
These are whole-atlas counts, not UV-local samples, and do not establish the
screen backing's identity. Probe log: /tmp/rage-telebim-material-probe.log.
The temporary instrumentation was removed and smoke rebuilt afterward.

Further isolation: disabling depth only for the two animation entities did
not restore the black backing. Disabling both terrain-quad and course-face
backface rejection also did not restore it (the roof itself changed). Both
experiments were reverted and smoke rebuilt.

The marker-face analyzer now prints draw/model identity. In the point-288
snapshot, logical pixel (140,95) intersects a terrain quad in cell slot 14,
CLUT 7a4d/page 001e, view Z 17895..17871, and course model 57 (draw 5),
CLUT 7b80/page 001a, Z 17937..17956. Animated layers are draws 2/3,
models 35/8; their ordering bucket is 138 versus terrain 148 and model 57
156. This narrows the candidate backing/roof pair but is not yet a verified
mapping to native triangles or an explanation of the incorrect final pixel.
