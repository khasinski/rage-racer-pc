# Modern Renderer Plan

Status: implemented through phase R6 (2026-08-14). Implements the renderer
direction recorded in `PORT_BRIEF.md` §1 (decision 2026-08-12): two explicit
paths, `compat` and `enhanced`. This document names the enhanced path the
**modern renderer**; §0 records what was built and where it deviates from the
original plan below.

## 0. Implementation status

All phases R0–R5 and the post-processing part of R6 are implemented:

- **R0** — `rage-port.cfg` (`renderer=compat|modern` + `modern.*` keys,
  parsed by `src/port/port_config.c`), PsyZ present-source hook
  (`Psyz_PresentSource_SDL3GPU`) and the `src/port/modern/` module.
- **R1** — `src/port/modern/scene_capture.c` records per logic frame: 3D
  submissions with their GTE state at the `native_geometry.c` seam, every
  emitted 3D face semantically (local vertices, lit colours, UV/CLUT/tpage,
  texture window, OT depth and bias — terrain captured at parent-quad level,
  so the modern path needs no subdivision or seam lines), and the 2D layer
  packet-by-packet from an end-of-frame ordering-table walk that skips 3D
  byte ranges. Fields the PS1 GPU ignores are zeroed so captures compare
  equal whenever they render equal. The `scene_capture` test requires
  deterministic traces and populated layers.
- **R2** — `src/port/modern/modern_renderer.c` renders snapshots on the
  shared SDL3 GPU device: float transforms, depth buffer,
  perspective-correct texturing, PS1 blend semantics (premultiplied
  ONE/ONE_MINUS_SRC_ALPHA with a reverse-subtract pipeline for ABR 2).
  **Deviation:** instead of a decoded-texture cache with VRAM-write
  invalidation, the fragment shader samples PsyZ's live VRAM texture with
  CLUT indirection, like the compat rasterizer — no cache or invalidation
  needed (the cache design in §3.3 becomes relevant only for replacement
  textures or raytracing).
- **R3** — depth policy: each vertex's true z is clamped into its face's
  compat ordering-table bucket window (model/terrain buckets are 4<<otShift
  z units, the course dispatcher's are 128; negative buckets from the +128
  OT base are representable), so cross-bucket ordering matches the oracle
  exactly while same-bucket faces intersect with real depth; opaque faces
  draw in reverse submission order to keep compat's within-bucket LIFO rule.
  True view z in w gives hardware near clipping. The mirror renders as a
  second pass over a recleared depth buffer, scissored to the drawing area
  its stream installs, layered backdrop → 3D → frame; environment packets
  apply every E-command word (DR_AREA carries E3+E4 in one packet). Scenes
  without 3D (menus, FMV, 480-line screens) pass through to the compat
  image.
- **R4** — `modern.internal_scale` (free target size),
  `modern.aspect=16:9` (widened 3D FOV, 4:3-centered 2D and mirror),
  `modern.texture_filter=linear` (presentation blit). **Not done:** extended
  draw distance — the compat path's culling and the retail 64-entry
  visible-cell list decide what is captured, so geometry can pop in at the
  16:9 edges.
- **R5** — `modern.fps=vsync|<n>`: the main loop's frame-sync wait presents
  interpolated frames through `Psyz_VideoPresentIntermediate` (present
  without VBlank bookkeeping — presenting via VSync(0) deadlocks the wait).
  Draws are matched by kind/model/mirror/table occurrence, terrain cells by
  cell index; scene changes, timer jumps and camera teleports snap. Scene
  hashes are identical with fps mode on.
- **R6** — post-processing pass with one built-in effect
  (`modern.post=fxaa`, edge anti-aliasing); the pass is the extension point
  for further effects. **Not implemented:** replacement textures (needs the
  §3.3 decoded cache), modernised lighting, raytracing (appendix B
  unchanged).
- **R7 (effects round 1)** —
  - `modern.texture_filter=linear` now also selects CLUT-aware 4-tap
    bilinear texel filtering in the 3D fragment shader (per-tap palette
    lookup, transparency-key taps drop out of the blend, the nearest texel
    keeps the cutout/semi decision, so silhouettes stay compat-identical);
    2D/HUD stays nearest.
  - Per-vertex GTE depth cueing: fogged faces un-bake the captured flat
    IR0 and re-apply `IR0 = (H/z·DQA + DQB) >> 12` toward the far colour at
    each vertex (course walls now carry the FOGGED flag too);
    `RAGE_PORT_MODERN_FLAT_FOG=1` restores per-face fog. Note Rage Racer
    depth-cues only some geometry (spinning scenery, terrain dispatches
    0/2), so many scenes are unchanged.
  - `modern.bloom=on|off|<0..2>` — highlight glow: bright-pass into a
    quarter-res target, separable Gaussian blur, screen-blend composite.
  - `modern.grading=vibrant|off` — vibrance + gentle contrast in the same
    composite pass (constants baked into the generated MSL).

### Future effect ideas (not scheduled)

- **Bloom without the HUD** — the current bloom samples the finished frame,
  and the brightest content is usually the HUD (tachometer, timer text), so
  the glow lands mostly on the overlay. Running the bright pass on a
  3D-only intermediate target (before the foreground 2D replay) would
  restrict the glow to scene highlights; until then the effect defaults
  off.

- **SSAO** — depth buffer exists; low intensity to avoid noise on low-poly
  geometry.
- **Dynamic shadow maps** — the captured GTE light matrix gives the light
  direction and the semantic scene gives geometry; a depth-only pass with
  PCF could replace the retail blob shadows. Largest visual win, largest
  effort (bias/peter-panning tuning on low-poly meshes).
- **Specular / environment mapping on cars** — normals are not captured
  (lighting is baked by the GTE) but face normals can be recomputed from
  positions; subtle highlights only, easy to overdo.
- **xBRZ-style texture upscaling** — offline cache keyed by tpage+CLUT;
  significant engineering around VRAM invalidation.
- **Quality downsample** — verify the internal-scale → window resolve uses
  a proper filter (effectively free SSAA improvement).
- **Distance haze for the extended draw distance** — geometry past the
  retail cutoff pops in unfaded today; a synthetic haze ramp would soften
  it (retail has no fog there to match).
- Motion blur was considered and **rejected by the project owner** — do
  not propose it again.

Verification: game-state and scene-hash parity with the renderer toggled
(§3's capture-alongside design makes this structural); visual A/B against
the compat oracle at prologue frame 899 (road/wall/stand pixels within a few
units, subtractive night tile correct) and Grand Prix frames including the
live rear-view mirror. Diagnostics: `RAGE_PORT_MODERN=1` (smoke),
`RAGE_PORT_MODERN_DUMP[_FRAME]`, `RAGE_PORT_MODERN_SPAN_TRACE`,
`RAGE_PORT_MODERN_FACE_TRACE`, `RAGE_PORT_MODERN_SOLID`,
`RAGE_PORT_SCENE_TRACE[_VERBOSE]`.

Known issues: a localized ordering/UV mismatch in the prologue's top-left
building cluster (overlapping same-CLUT terrain walls from neighbouring
cells); 16:9 edge pop-in per R4 above; interpolated presentation needs
interactive verification for perceived smoothness (headless runs only prove
state parity and pacing).

## 1. Goals and non-goals

The modern renderer must provide, as an opt-in runtime choice:

- a real **depth buffer** instead of ordering-table painter's algorithm;
- **programmable shaders** (perspective-correct texturing, per-pixel fog,
  optional per-pixel lighting, room for post-processing);
- **arbitrary internal resolution and window size/aspect**, not the PS1
  320×240 pipeline scaled up;
- **arbitrary presentation frame rate**, decoupled from the PAL 50/25 Hz game
  logic, via interpolation;
- a structure that later admits **raytracing** experiments (appendix B).

Non-goals and invariants:

- The **compat renderer stays intact and remains the default**. It is the
  behavioural oracle; every existing characterization, smoke, and visual test
  keeps running against it unchanged.
- The modern renderer must **never change game behaviour**. Physics, AI, RNG,
  race timing, replays, and saves must be bit-identical whichever renderer is
  displayed. This is an architectural requirement, not a QA hope — see §3.
- PsyZ remains the compat backend. Per the port brief, PsyZ is *"a
  transitional compatibility backend, not the public API of the enhanced
  renderer"* — the modern renderer therefore lives in `src/port/`, not inside
  `external/psyz/`.

## 2. Where the current pipeline can be cut

The frame today (all references are current tree):

```
MainLoop (src/main/PAL/main/boot/main_loop.c:96)
  scene handler                       g_SceneHandlers[g_SceneId]()
    3D:  SubmitModel / SubmitCourseModel{,2} / SubmitTerrainCells
         -> src/port/native_geometry.c (portable GTE engine)
         -> GP0 packets into per-frame ordering tables
    sky: DrawSkyBackground (track/draw_terrain_cells.c:50) -> POLY_FT4/G4 bands
    2D:  draw_prims.c / text.c / text_drawing.c / scripted_draw.c /
         race_hud.c / sprite_string.c emitters -> GP0 packets
  DrawSync(0); VSync wait; PutDrawEnv/PutDispEnv
  DrawOTag(ot[0] tail); DrawOTag(ot[1] tail)   -> PsyZ GP0 interpreter
  PsyZ Draw_PushPrim -> SDL3 GPU (Metal) into emulated 1024×512 VRAM -> present
```

There are exactly two plausible interception levels:

**GP0 packet level (rejected as the primary seam).** Everything is already
projected to 320×240 integer screen space with affine UVs and OT buckets;
depth, world positions, and normals are gone. This level can only reproduce
the PS1 image (PsyZ already does that, including up-to-8× internal resolution
via `Psyz_VideoSetInternalResolution`). It cannot give a z-buffer, wide FOV,
draw distance, or interpolation. It stays useful for the 2D layer only.

**Semantic submission level (chosen).** At the entry of `SubmitModel`,
`SubmitCourseModel{,2}`, `SubmitTerrainCells`, and `DrawSkyBackground`, the
full semantic scene still exists:

- camera: `SCRATCH_VIEW_MATRIX_GTE`, `SCRATCH_CAMERA_POS`, view angles,
  `SetGeomOffset`/`SetGeomScreen` (FOV), mirror matrix;
- per object: model bank + model index + LOD, object rotation matrix and
  translation (set through `SetGteObjectMatrix`,
  `render/draw_packet_queue.c:170`), light/colour matrices, palette row
  (`g_ScratchRenderMode`);
- terrain: visible cell list + per-cell translation, dispatch mode, fog flags;
- environment: `SetFarColor`/`SetFogNear` state, `g_EnvironmentColors`.

This seam is narrow (single-digit function count, all already centralised in
`src/port/native_geometry.c` and `draw_terrain_cells.c`) and the port brief
explicitly designates it: *"the enhanced path consumes semantic scene
submissions before PS1 screen projection."*

The 2D emitters are equally centralised (~20 functions in
`render/draw_prims.c`, `render/text.c`, `render/text_drawing.c`,
`render/scripted_draw.c`, `render/race_hud.c`, `render/sprite_string.c`,
`render/draw_packet_queue.c`) and become the capture points for the overlay
layer.

## 3. Architecture: capture-alongside, render-instead

The single most important design decision: **the compat submission path always
runs, in both renderer modes.** The capture layer records the semantic scene
*while* the compat path executes; the modern renderer only changes what gets
presented.

Why this shape:

1. **Zero behavioural divergence by construction.** Game code and its side
   effects (scratchpad cursors, counters, smoke-test hashes) are identical in
   both modes. The invariant "modern never changes behaviour" becomes trivially
   testable: state hashes must match with the renderer toggled.
2. **Instant runtime toggle and A/B comparison.** Both images exist every
   frame; a hotkey can flip between them, and a debug view can show both.
3. **The oracle stays alive.** Visual comparison tooling
   (`rage_visual_compare.rb`, GP0 traces, VRAM dumps) keeps working unchanged.
4. **It is cheap.** The compat path is a PS1-era workload (tens of thousands
   of packets/frame); running it always costs little on a modern CPU.

```
                       ┌────────────────────────────────────────────┐
 game code ──────────► │ compat path (unchanged)                    │──► PsyZ VRAM image
   scene handlers      │  native_geometry.c → GP0 → OT → PsyZ       │        │
        │              └────────────────────────────────────────────┘        │
        │                       │ (capture hooks at the same seam)           ▼
        │              ┌────────────────────────────────────────────┐   ┌─────────┐
        └────────────► │ RageScene snapshot (per logic frame)       │   │ present │
                       │  camera, draw items, terrain, sky, 2D list │   │ selector│
                       └────────────────────────────────────────────┘   └─────────┘
                                        │                                    ▲
                                        ▼                                    │
                       ┌────────────────────────────────────────────┐        │
                       │ modern renderer (src/port/modern/)         │────────┘
                       │  SDL3 GPU (Metal): z-buffer, shaders,      │
                       │  arbitrary res/aspect/FPS, interpolation   │
                       └────────────────────────────────────────────┘
```

### 3.1 The `RageScene` snapshot

A plain-data structure filled once per **logic** frame, double-buffered (two
snapshots retained for interpolation):

```c
typedef struct RageSceneCamera {
    float view[3][4];          /* from SCRATCH_VIEW_MATRIX_GTE (4.12 → float) */
    float position[3];
    int   geomScreen;          /* SetGeomScreen h → FOV */
    int   geomOffsetX, geomOffsetY;
    /* fog: SetFogNear/SetFarColor state, DQA/DQB equivalents */
} RageSceneCamera;

typedef struct RageSceneModelDraw {
    uint16_t bankId, modelIndex;   /* resolved via NativeModelBank sidecar */
    uint8_t  lod, paletteRow;      /* g_ScratchRenderMode >> 16 */
    float    rot[3][3];  float trans[3];
    uint8_t  lightSetId;           /* index into captured light/colour matrices */
    int16_t  otBias;               /* layering intent, see §3.4 */
    uint32_t objectKey;            /* stable identity for interpolation, see §3.6 */
} RageSceneModelDraw;

/* analogous records for course models, terrain cells (cell index +
   translation + dispatch/fog mode), sky bands, and the 2D command list */
```

Capture points (one-line hooks at function entry, active always — recording
into the snapshot is cheap):

| What | Where the hook goes |
|---|---|
| camera | `SetCameraRotMatrix` (`render/rot_matrix.c:63`) + `SetGeomOffset/Screen` call sites |
| model draw | `SubmitModel` (`src/port/native_geometry.c:1018`) |
| course model | `SubmitCourseModel{,2}` (`native_geometry.c:1228/1232`) |
| terrain | `SubmitTerrainCells` (`native_geometry.c:1236`) |
| sky | `DrawSkyBackground` (`track/draw_terrain_cells.c:50`) — capture band parameters, not packets |
| mirror pass | `BeginMirrorPass`/`EndMirrorPass` (`render/rear_view_mirror.c:31/115`) — marks a sub-camera scope |
| 2D | the emitters in `draw_prims.c`, `text.c`, `text_drawing.c`, `scripted_draw.c`, `race_hud.c`, `sprite_string.c` — record the *arguments* (screen rect, UV, CLUT, blend, OT bucket), not the GP0 bytes |

Geometry itself is **not** copied per frame: model banks, terrain cell tables
and course banks are static assets already resolved into native sidecars
(`asset/model_banks.c` → `NativeModelBank`). The snapshot stores references;
the modern renderer uploads each bank's vertex/normal/face data to GPU buffers
once, at `RegisterModelBank`/`InstallTerrainCellData` time.

### 3.2 The modern 3D pass

Backend: **SDL3 GPU**, same device PsyZ already creates (Metal via MSL on
macOS). Sharing the device and window avoids a second swapchain and lets the
present selector be a trivial "which texture do we blit". PsyZ's overlay API
(`Psyz_OverlayInit_SDL3GPU` receives the `SDL_GPUDevice`;
`Psyz_OverlayRender_SDL3GPU` runs inside the swapchain pass) proves the device
handoff already works; §5 lists the one small PsyZ extension needed.

Pipeline per presented frame:

1. **Sky layer** — the captured panorama/gradient bands rendered first as a
   background (depth writes off, matches OT buckets 702/703 semantics).
2. **Opaque 3D** — models, course models, terrain, with:
   - float MVP built from the captured camera; perspective projection with
     FOV equal to `atan(120 / geomScreen)`-style mapping so geometry lands
     where the PS1 projection put it at 4:3, extended horizontally for wide
     aspect;
   - **z-buffer** (true view-space z; near ~16 units, far beyond the retail
     `0x14000` visible-cell range);
   - vertex shader transforming native model vertices (no GTE emulation —
     floats), normal lighting via the captured light/colour matrices
     reproducing `NCCS/NCS` semantics, or optionally a modernised lighting
     mode;
   - fragment shader sampling decoded textures **perspective-correct** with
     depth-cue fog reproducing `DPCS` (`DQA/DQB`) as a smooth per-pixel
     function.
3. **Translucent 3D** — semi-transparent faces sorted back-to-front (their
   count is small), ABR modes 0–3 mapped to blend states exactly as PsyZ's
   fragment shader already does.
4. **Mirror pass** — captured mirror scope rendered to its own render target
   with the mirror camera, composited into the mirror panel rect. This gives
   the brief's "independent mirror render targets" for free (the mirror can
   render at full resolution instead of 148×36).
5. **2D overlay** — the captured 2D command list rendered with PS1 blend and
   modulation semantics into an overlay composited on top. Layer ordering
   comes from the recorded OT bucket (the reversed OT draws bucket 703 first,
   bucket 0 last; 3D occupies roughly buckets 128–447+bias, HUD sits at 0–1),
   so the compositor slots the 3D image between background and foreground 2D
   at the same boundary.

**Terrain subdivision disappears.** Retail subdivides quads
(`EmitSubdividedTerrainQuad`) purely to fight affine texture warping and
per-vertex fog banding; a perspective-correct rasterizer renders the *parent*
quads directly. The seam-cover `LINE_F3` packets (retail's crack filler) also
become unnecessary. This is both a quality and a large batch-size win — the
Grand Prix frame drops from ~20k packets to the un-subdivided face count.

**Culling policy.** The compat path's screen-rect reject, `depth >= 448`
reject and `BuildVisibleCells(-12288, 0x14000)` range are PS1 constraints. The
modern pass ignores the screen-rect/depth rejects (frustum + z-buffer replace
them). Extending *visibility* (draw distance, wide-aspect cell selection)
requires enlarging the `BuildVisibleCells` range and the per-cell far-path
record skip — that is a game-code change gated behind the modern mode
(`if (RageModernActive()) …` or a captured-parameters override), phase R4.

### 3.3 Textures: decoding PS1 VRAM

Modern shaders need RGBA textures, not live CLUT indirection (raytracing
definitely will). Build a **texture cache** keyed by
`(tpage, clut, texture-window)`:

- source of truth remains the emulated VRAM (PsyZ already maintains the
  1024×512 image and `Psyz_VideoAllocCapturedVram` can read it);
- on first use of a key, decode the 4/8/16-bpp page region through the CLUT
  into an RGBA texture (the decode math is identical to PsyZ's fragment
  shader; do it once on CPU or in a compute pass);
- **invalidation**: any `LoadImage`/`MoveImage`/`ClearImage`/`StoreImage`
  whose rect intersects a cached page or CLUT row evicts those entries. Hook
  point: PsyZ's `Draw_LoadImage`/`Draw_MoveImage`/`Draw_ClearImage` (or a
  thin notification callback added in §5). This automatically covers the two
  live texture animations: `StepTrackTextureSwap` (row `{576,0,448,1}`
  shuffles) and `UploadCarImage` (`{704,0,64,256}`), plus car paint CLUT
  edits (`ApplyBodyColor1/2`) and the HUD blink CLUT.

Texel `0000` stays the transparency key; modulation (`tex*col/16`, clamp)
matches the PS1 formula by default so parity comparisons stay meaningful. A
later "replacement textures" feature slots in here naturally: the cache
consults an override directory before decoding VRAM.

### 3.4 Depth policy: what OT bias means in a z-buffered world

The game encodes deliberate layering through OT bucket manipulation, and a
z-buffer must honour the *intent*:

| Retail mechanism | Modern translation |
|---|---|
| per-face signed bias byte (`faces[stride-3]`, `native_geometry.c:902`) | polygon depth-bias units (bias × configurable epsilon), applied in the vertex shader |
| mirror pass `depth += 0x800` | separate render target — bias irrelevant |
| showroom stand `SCRATCH_OT_BASE += 30` buckets | captured as a per-scope bias, becomes depth-bias |
| sky at fixed buckets 702/703 | background layer, depth test off |
| `DrawSkyBackground` course strip F4/G4 ordering | preserved by layer order within the sky pass |
| coplanar decals relying on OT insertion order | stable submission order used as z-tiebreaker: equal-depth fragments resolve by draw order (draw opaque coplanar sets with `depth-equal-pass` and painter order) |

Expect a short tail of scene-specific tuning here; the A/B toggle plus the
pixel-trace tooling make each case diagnosable. This is the main "unknown
unknowns" budget of the project (see §7).

### 3.5 Resolution, aspect, window

- Internal 3D resolution: free (render target size), independent of window.
- The 2D overlay renders at an integer multiple of 320×240 (crisp scaled UI);
  optionally with proper filtering later.
- Aspect: the projection widens FOV horizontally for 16:9; 2D overlay stays
  4:3 centred (HUD anchoring to edges is a later cosmetic option).
- Window: freely resizable in modern mode; compat mode keeps the current
  fixed ×2 logical size until the 1:1 milestone allows otherwise.
- 480-line menu scenes (`SetupDisplay480`) and FMV are pure 2D/compat content:
  in modern mode they present the compat image (scaled) — the modern 3D path
  only engages for scenes that submit 3D geometry.

### 3.6 Arbitrary FPS: snapshots + interpolation

Game logic cadence is load-bearing and stays untouched: menus tick every PAL
VBlank, race logic every second VBlank (`g_FrameSyncThreshold` 0x80/0x180,
`main_loop.c:125`). The modern renderer decouples presentation:

- keep the last **two** `RageScene` snapshots;
- inside the frame-sync wait (today a busy loop on `VSync(1)`), a port hook
  renders interpolated frames at display rate: object rotations slerped,
  translations and camera position lerped by the fraction of the logic
  interval elapsed;
- **object identity**: interpolation needs to match draw items across
  snapshots. Cars are stable (car index), terrain cells are static, course
  objects match by object index. The `objectKey` field carries this.
- **cut detection**: never interpolate across camera cuts (prologue cuts,
  finish camera, scene changes). Heuristics: scene id/timer change, camera
  teleport beyond a threshold, or explicit capture of the game's own cut
  events (`g_PrologueCameraCuts`, `UpdateScriptedCamera` key advances).
  On a detected cut, snap.
- 2D overlay is not interpolated (it changes at logic rate; HUD at 25/50 Hz
  is authentic and unobjectionable).

Phase order: modern renderer first ships rendering 1 frame per logic frame
(50/25 Hz, same as compat); interpolation lands as a separate phase (R5) with
its own toggle, because it is the only part that touches the main loop's
timing structure.

### 3.7 Configuration and UX

Extend the existing plain-text config approach (`rage-input.cfg` pattern) with
a `rage-port.cfg` (or a `[video]` section):

```
renderer = compat | modern        # default: compat
modern.internal_scale = 2.0       # or explicit WxH
modern.aspect = 4:3 | 16:9 | auto
modern.fps = logic | vsync | <n>  # 'logic' = no interpolation
modern.draw_distance = 1.0        # multiplier, phase R4
modern.texture_filter = nearest | linear
modern.post = none | fxaa         # edge anti-aliasing pass
modern.bloom = off | on | <0..2>  # highlight glow ('on' = 0.6)
modern.grading = off | vibrant    # vibrance + gentle contrast
```

Runtime: a debug hotkey toggles presented renderer (both images exist), a
second hotkey shows a split/side-by-side diff view. The PsyZ ImGui overlay
hooks are the natural home for a small debug panel (renderer stats, cache
counters, interpolation state).

## 4. Game-code changes (allowed, kept minimal and mechanical)

1. Capture hooks at the §3.1 seam functions — one call each, compiled in
   always, recording only.
2. A port hook inside the `VSync(1)` wait loop of `MainLoop` for interpolated
   presentation (R5). `RagePortBeforeSceneHandler` already establishes this
   hook pattern.
3. Gated visibility extensions (R4): `BuildVisibleCells` range parameter,
   far-path record skip threshold, sky band coverage for wide aspect.
4. Nothing else. Emitters, scratchpad protocol, scene handlers, and all logic
   stay byte-for-byte on the compat contract.

## 5. PsyZ changes (small, on the existing `rage-racer-port` branch)

1. **Present hook**: a callback letting the host replace/augment the VRAM →
   swapchain blit (`PlatformBackend_Present`, `sdl3_gpu.c:628`) with its own
   texture. The overlay API already runs inside this pass; this is a ~30-line
   generalisation.
2. **Device/queue accessor** for the modern renderer (the overlay init path
   already exposes `SDL_GPUDevice`; promote it to a supported getter).
3. **VRAM-write notification callback** for texture-cache invalidation
   (`Draw_LoadImage`/`Draw_MoveImage`/`Draw_ClearImage`).
4. Optionally later: let `VSync` presentation be driven externally in modern
   mode (R5) so the frame limiter doesn't double-pace.

All are additive; none change compat behaviour.

## 6. Phased plan

Each phase is verified before the next starts. The compat oracle and all
existing tests run at every step; "state parity" below means smoke-harness
game-state hashes are identical with the renderer toggled.

**R0 — plumbing (small).**
Config file + `renderer=` switch, present-selector that today can only choose
compat, PsyZ present hook + device accessor, empty `src/port/modern/` module,
debug overlay panel. *Verify:* modern mode selected → identical output to
compat (it presents the compat image); all tests pass.

**R1 — scene capture (medium).**
`RageScene` snapshot, all §3.1 hooks, double-buffering, object keys.
*Verify:* state parity; new characterization tests — golden snapshots (hashed
`RageScene` per smoke frame) for prologue, race, showroom, mirror; capture
overhead < 1 ms/frame.

**R2 — modern 3D MVP (large).**
GPU upload of model/terrain/course banks at registration; opaque pass with
z-buffer and float transforms; texture cache with VRAM invalidation; Gouraud
lighting per captured matrices; per-pixel fog; sky layer; 2D overlay from the
captured command list; compositor. Renders at logic rate, internal res =
window res, 4:3. *Verify:* state parity; A/B toggle shows the same scene
(geometry within a pixel-scale tolerance at 320×240 internal res against the
compat frame — reuse `rage_visual_compare.rb` with a tolerance preset); no
subdivision, no seam lines.

**R3 — parity tail (medium, iterative).**
Semi-transparency ordering, texture windows, depth-bias policy tuning per §3.4
(showroom stand, decals, HUD-adjacent 3D), mirror render target, scripted
scenery edge cases, 480-line scene passthrough, FMV passthrough. *Verify:* a
curated scene checklist (prologue cuts, each course, mirror slide-in at timer
361–371, showroom rotation, results screens) signed off visually; regression
screenshots recorded per scene as modern-renderer goldens.

**R4 — beyond PS1 limits (medium).**
Arbitrary internal resolution and window resize; 16:9 with widened FOV;
extended draw distance (gated `BuildVisibleCells` range + far-path skip +
sky coverage); mirror at full resolution; optional texture filtering.
*Verify:* state parity (the gates must not leak into logic when disabled);
wide-aspect frame shows no cell popping at former screen edges.

**R5 — arbitrary FPS (medium/large).**
Snapshot interpolation, cut detection, main-loop wait hook, `modern.fps`
modes, PsyZ pacing handoff. *Verify:* state parity at any fps setting; replay
determinism unchanged; recorded 25 Hz race logic presents smoothly at display
rate; cut scenes never interpolate across cuts (prologue characterization).

**R6 — extras (open-ended, each optional).**
Post-processing hooks, replacement textures/meshes via the cache/bank seams,
modernised lighting mode, raytracing per appendix B.

Rough effort ranking: R2 is the bulk of the work; R1 and R5 are the two other
substantial chunks; R0 is days; R3/R4 are iterative tails.

## 7. Risks and mitigations

| Risk | Mitigation |
|---|---|
| OT-order-dependent visuals (decals, deliberate overlap) fight the z-buffer | §3.4 bias policy + draw-order tiebreaker; A/B toggle and pixel trace make each case diagnosable; budget the R3 tail for this |
| Interpolation artefacts across camera cuts / respawns | cut detection with snap fallback; per-scene characterization |
| Texture cache invalidation misses a VRAM write path | invalidate from the PsyZ `Draw_*` choke points (all writes pass through them); a debug mode that flushes the cache every frame isolates staleness bugs |
| Widescreen exposes culling/LOD assumptions beyond `BuildVisibleCells` (sky bands, scripted scenery placement) | R4 gates each extension separately; ship 4:3 modern first |
| PsyZ upstream drift (fork branch) | keep PsyZ diffs additive and small (§5), upstream where possible |
| Modern path accidentally feeds state back into logic | capture layer is write-only into the snapshot; state-parity hash tests in CI on every phase |
| Effort sink: reimplementing PS1 blend/modulation for the 2D overlay | reuse PsyZ's shader formulas verbatim (they are already validated against PCSX-derived tests) |

## A. What you get almost for free today

Independent of this plan: PsyZ already implements
`Psyz_VideoSetInternalResolution(n)` (up to 8×) — supersampled PS1 rendering
with zero architectural work. It stays affine-textured, OT-ordered and
logic-rate-locked, so it is not the goal of this document, but it is a useful
interim option and a good stress test of the present path.

## B. Raytracing (now scheduled — see §B.1 for the plan of record)

- SDL3 GPU does not expose raytracing; on macOS this means a **native Metal**
  path (`MTLAccelerationStructure`, Metal RT from Metal 3). The modern
  renderer therefore keeps its backend surface small (device, buffers,
  pipelines, passes) so a Metal-native backend can slot in beside the SDL3
  GPU one.
- Everything RT needs is already in the plan's data model: `RageScene` is a
  renderer-agnostic scene graph; model/terrain banks are static GPU buffers
  (→ BLAS, built once per bank); per-frame draws are instances (→ TLAS
  refit per frame); the texture cache provides RGBA materials.
- Realistic scope is **hybrid**: raster primary visibility + RT shadows /
  reflections (car paint, wet track) / AO. PS1-era geometry (low poly, heavy
  texture detail) makes full path tracing stylistically questionable anyway.
- Prerequisite phases: R2 (scene on GPU) and R3 (materials stable). Nothing
  earlier should bend for RT beyond keeping `RageScene` renderer-agnostic.

### B.1 Plan of record

The guardrails above survived contact with the code. What the capture layer
already records is BLAS/TLAS-shaped, so no phase needs redesigning:

- `RageCaptureFace.pos[4][4]` holds object-space `SVECTOR`s, not clip space.
  Clip space is produced later, inside the renderer.
- `RageCaptureModelDraw.bankId` already identifies the active model/course
  bank — the key a BLAS is built and cached against.
- `RageCaptureModelDraw.gte.rot` is the composed rotation and translation,
  which is the instance transform a TLAS wants.

Backend decision: **Metal first**. It is the only raytracing backend that can
be run and tested on the development machine, and Metal 3 exposes what is
needed (`MTLAccelerationStructure`, intersection queries from a fragment or
compute shader). Vulkan RT follows for Windows and Linux; until then those
platforms keep the SDL3 GPU path with no raytraced effects. The SDL3 GPU
backend stays — Metal is added beside it, not in place of it.

PsyZ is a staging dependency, not a fixture. The contact surface shrinks over
time and rendering is simply the first area to leave it; game logic follows
later. Treat every PsyZ call as a seam with a replacement due, not as a
boundary the port is built against.

Rendering starts from a good position here. `RageCaptureGteState` is plain
numbers, so the renderer already consumes camera and lighting state without
touching PsyZ — only the producer in `scene_capture.c` calls
`Psyz_GteCtrlRead`. That leaves the device, the swapchain and VRAM as the
renderer's remaining borrowings, and each has a step below that removes it.

Steps, each landing on its own:

1. **Material cache.** R2 deviated to sampling PsyZ's live VRAM with CLUT
   indirection. A ray that hits a triangle cannot do that indirection cheaply,
   so decoded RGBA materials become a prerequisite rather than an option.
   This is also the largest single piece of PsyZ coupling the renderer has.
2. **Geometry banks.** Vertices are streamed per frame today. Keyed by
   `bankId`, they become persistent buffers built once — the BLAS inputs.
3. **Metal backend.** Device, buffers, pipelines and passes behind the same
   small surface the SDL3 GPU backend sits on, plus its own swapchain.
4. **Acceleration structures.** BLAS per bank, TLAS refit per frame from the
   captured instance transforms.
5. **Effects.** Hybrid: raster primary visibility, then rays for shadows,
   car-paint and wet-track reflections, and AO. Full path tracing stays out of
   scope — PS1 geometry is low-poly with the detail in textures, so it would
   spend its budget on the wrong thing.

Steps 1 and 2 are backend-agnostic and improve the SDL3 GPU path on their own,
so they are worth landing whatever happens to steps 3 onwards.
