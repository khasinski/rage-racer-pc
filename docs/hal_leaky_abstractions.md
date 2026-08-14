# HAL leaky-abstraction audit

Goal of this audit: identify every place where game logic / physics is coupled
to the hardware abstraction layer (PsyQ/PSY-Z: GPU, GTE, SPU, CD, pad), as a
prerequisite for separating simulation from rendering, audio and assets, and
for making the game fully moddable and extensible.

Baseline facts that shape the work:

- The port keeps PsyQ semantics end to end. `src/port/include/psyq/*` and
  `include/psyq/*` re-export SDK types (`MATRIX`, `RECT`, `POLY_*`, `CdlLOC`)
  directly into game code; `include/game/vector.h` defines the "neutral"
  vector types *as* GTE shapes.
- Per the port decision of 2026-08-10 (PORT_BRIEF §5), the PS1 byte match is
  no longer a release gate. The regression oracle is the host characterization
  test suite. Refactoring game code is therefore permitted, gated on
  characterizing each subsystem first.

The audit is organized by separation axis. Within each axis, findings are
ranked worst first. Every citation was verified against the source.

---

## 1. Frame model: update and render are fused

**1.1 One scene handler does everything.** `MainLoop`
(`src/main/PAL/main/boot/main_loop.c`) calls a single
`g_SceneHandlers[g_SceneId]()` per frame. That handler advances physics, AI,
audio *and* emits GPU packets into the ordering tables. There is no
`update()` / `render()` split anywhere in the game tier; interleaving is the
norm (e.g. `RunRaceIntroCamera` in `car/race_car_update.c:696-726` computes
camera state from car physics, then calls `SelectModelBank(0)` and
`DrawPlayerCarModel(car)` in the same branch).

**1.2 Physics is locked to the display rate.** The loop waits on
`VSync(1) < g_FrameSyncThreshold` and steps the simulation once per displayed
frame (PAL 25 Hz with `g_FrameSyncThreshold = 0x80`). There is no fixed
timestep decoupled from presentation; changing refresh rate changes physics.

**1.3 Double-buffer assumptions leak upward.** Logic-tier code resolves the
current ordering table through `g_DrawBuffer` / `g_FrameContexts[2]`
(`GamePrimaryOrderingTable`, `include/game/render_internal.h:108-111`), so
every caller hard-codes the PS1 two-frame OT model.

## 2. Simulation ↔ rendering (GTE/GPU)

**2.1 Player physics integrates motion through the GTE.**
`car/update_player_car.c:287-341`: yaw/pitch/roll matrices are built, run
through `MulMatrix2` (a COP2 op), and `ApplyMatrix(&m2, &sv1, &car->motionX)`
writes the GTE result registers straight into the car struct. The car's
corner/track-limit extents (`:331-341`) are also computed on the GTE. The
physics step cannot advance one frame without the geometry coprocessor.

**2.2 Rival/AI physics does the same for all 11 cars.**
`car/race_car_update.c:242-259` and `:486-503` repeat the pattern
(`ApplyMatrix(..., &car->motionX)`), so every AI car's world velocity is a GTE
output.

**2.3 Collision response is bound to GTE + scratchpad layout.**
`car/car_track_state.c:278-300, 477-478` build a rotation matrix into a
scratchpad work struct and transform the wall-knockback correction on the
GTE; `car/track_contact.c:50-52` projects the car into segment-local space
with `BuildRotMatrixY` + `ApplyMatrix` while sampling surface height — pure
simulation running on render hardware.

**2.4 Sim state is stored in GTE-ABI shapes.** `include/game/car.h`:
`motionX/Y/Z` are deliberately laid out as an `LVec`/`Vec4` so `ApplyMatrix`
can write them; `GameCarRuntimeAddress` (`car.h:186-194`) unions the car
pointer to `LVec */Vec4 *`; `SetPlayerPosition`/`CopyPlayerBodyRotationToModel`
(`car.h:703-733`) reinterpret sim state as GTE vectors, and `_Static_assert`s
(`car.h:649-682`) lock the offsets. Position/velocity/orientation have no
engine-neutral representation.

**2.5 A single global GTE view matrix is shared by logic and renderer.**
`track/update_route_scenery.c:88-95`, `track/update_shuttle_scenery.c:118-126`
(and the `draw_*scenery.c` twins) mutate `SCRATCH_VIEW_MATRIX_GTE` — the
fixed scratchpad slot `&g_RageScratchpadState.matrix`
(`include/game/scratchpad.h:197`). Camera logic is GTE-resident:
`track/update_camera.c:135-624` ends every path in `SetCameraRotMatrix()`;
even `car/car_ai.c:570-571, 644-645` loads GTE rotation registers from AI
decision code.

**2.6 Logic allocates GPU packets directly.**
`race/draw_speed_digits.c:27,39` bump-allocates from
`SCRATCH_PRIM_CURSOR_AS(u8)` and queues into the OT;
`race/race_runtime.c:264` calls `AddPrim` on the primary OT;
`race/race_hud_text.c:367-593` and `race/draw_wrong_way_warning.c:69-318`
hand-build `POLY_F4`/`POLY_FT4`/`SPRT` packets. The menu tier is saturated
with the same pattern (76 CLUT/tpage/`DrawSync`/`SetDispEnv` hits across 12
files).

**2.7 VRAM/CLUT/screen coordinates embedded in game data.**
`race/race_runtime.c:255-261` hard-codes `clut = 0x780B` and screen positions
in race code; `g_HudGlyphClut` lives in `car.h:857`; `CarTachometerSpec`
(`car.h:372-389`) stores screen-space HUD coordinates inside per-car spec
data — gameplay tuning and framebuffer layout share one struct.

## 3. Simulation ↔ audio (SPU)

**3.1 The physics update calls the SPU voice engine directly.**
`car/update_player_car.c:603-614`: `UpdateLoadedAudioVoices(g_EngineRpm +
g_EngineRpmJitter, …)` every frame, with a hand-derived engine-sound bank
selector built from clutch/throttle/redline state (`:585-597`). The shared
globals `g_EngineRpm`, `g_EngineRpmJitter`, `g_EngineRpmSnapshot`
(written here and in `car_orientation.c:246-248`) are the sim→audio channel.

**3.2 The RPM→voice transfer function is hardwired to SPU slots.**
`audio/loaded_audio_voices.c:83-132` normalizes RPM against
`g_EngineSoundState.maxRpm`, interpolates per-voice tone/pitch curves and
writes 6 fixed SPU slots (`SetSoundSlotTone(index, …)`); it also drives VAB
bank crossfades. Any non-PsyQ backend must reimplement this mapping.

**3.3 SPU voice numbers are the API.** Hardcoded voice indices encode game
meaning: `effect_voice_runtime.c:96-118` (voices `0x14`/`0x15`), `:518-527`
(music channels `8+i`); `sound_cues.c:357-369` (special cues `0x12`–`0x17`),
`:489-501` (voices `0x16`/`0x17`); `audio_runtime_control.c:196` and
`stop_sound_slot_voice.c:4` (`slot + 0xE`). Voice-count reconfiguration is
scene-coupled (`SsSetVoiceCount(0x12/8/0xA)` in `loaded_audio_voices.c:140,150`,
`sound_runtime.c:124,152`).

**3.4 SPU RAM addresses and VAB bank indices in load logic.**
`audio/audio_slot_loading.c:150` (`0x6A000`), `sound_runtime.c:129`
(`0x1000`), positional `g_SoundScale.vabIds[vab]` addressing throughout
`sound_cues.c` / `effect_voice_runtime.c`. Cue-parameter data carries hardware
bank indices.

**3.5 Reverb registers driven from track logic.** `race/race_runtime.c:386-422`
sets reverb depth from the car's track position (`SetReverbDepth`, magic
depths `0x28`/`0x46`); more direct calls in `race_scene.c:482,699`,
`waypoint_race.c:150`, `replay_cars.c:135,160`, `race_end_screens.c:49`.
Environmental acoustics are expressed as SPU register writes, not "entered
tunnel" events.

**3.6 CD-audio transport reached from scene code.** Disc-layout arithmetic in
menu logic (`bgm_select.c:69-141`, `race_scene.c:371`: `track + 3`), and raw
`CdControl(9,0,0)` pause commands from `race/record_entry.c:545,552`,
`menu/frontend.c:123`, `fmv/fmv_scene.c:18`, bypassing even the thin request
layer.

**Existing seam worth keeping:** `PlaySoundCue(id)` already is an abstract
event API (cue IDs looked up in `g_SoundCueParams*`); callers in race/AI code
are clean. The leak is only *below* `PlaySoundCue`.

## 4. Logic ↔ assets (CD layout)

**4.1 Asset identity is a disc offset.** `asset/asset_loader.c:22-35`:
`LoadAsset(assetIndex, dst)` indexes a fixed 135-entry `(sectorOffset, size)`
table (`g_AssetCdEntries[135]`, `src/port/native_game_state.c:26`); the host
bridge (`src/port/legacy_platform.c:532-546`) reads sector 0 of `RAGE.BIN`
as that table and rejects `count > 135`. No names, no handles.

**4.2 Disc-layout arithmetic compiled into logic.** Index formulas encode the
retail layout: car assets `(idx*2)+0xA` (`asset/car_assets.c:82,131`,
`race_assets.c:62`), track packs `(class*8)+(course*2)+0x57/0x58`
(`race_assets.c:100,140,264`), GP round screens `series*6+class+0x4A`
(`race_assets.c:235-237`). "4 courses × 6 classes" and "32 car halves" are
multiply-add constants; a 5th course cannot exist because `course*2` sits
inside a per-class stride of 8 that exactly fills `[0x57..0x86]`
(`include/game/asset.h:53-79`).

**4.3 Fixed compile-time content counts.** `g_CarModelSlots[2]`
(`native_game_state.c:25`; `SetCarModelSlot` rejects `index >= 2`,
`model_banks.c:155`), `g_CarModelBaseIndex[16]` (`src/port/host_state.c:229`),
`g_CarImageSlots[56]` (`host_state.c:1154`), `g_StreamCdEntries[11]` with
hardcoded FMV sector offsets (`native_game_state.c:451-463`), fixed per-car
RAM budget `CAR_MODEL_SLOT_SIZE 0x20000` (`asset.h:304`), and bank/terrain
limits in `asset.h:377-444`. Paint colors silently stop applying at
`carIndex >= 10` (`car_assets.c:93,146`).

**4.4 VRAM placement per asset.** Every car texture uploads to the single
fixed rect `g_CarImageRect = {0x2C0, 0, 64, 256}`
(`native_initialized_state.c:55`, used in `model_banks.c:144-145`);
`image_upload.c:8-41` reads target VRAM x/y out of the asset bytes
themselves, so textures embed their own framebuffer coordinates;
`image_upload.c:76` moves a CLUT to literal coords `(0x3F0, 0xE2)`.

**4.5 Asset ↔ hardware slot welding.** Menu car bank forced to model-bank
slot `0xE` (`car_assets.c:56`), player car to bank 0, track scenery to banks
1–2 (`race_assets.c:161,176`); audio slots fixed per load phase
(`StartAudioSlotLoad(0..3, …)`).

**4.6 Modding today means re-mastering a disc.** The host bridge is 1:1 with
the ISO: `RageHostFindArchive` parses ISO9660 for `RAGE.BIN` and all reads go
through `archive_sector + offset/2048` (`legacy_platform.c:455-546`). Adding
one car requires authoring the packed serialized format, inserting entries in
the middle of `RAGE.BIN` (shifting every later index used by the formulas in
4.2), widening the fixed tables, and rebuilding the `.bin/.cue` image. There
is no loose-file, name-addressed path.

## 5. Shared mutable state across all axes

- `src/port/native_game_state.c` stores scene state in hardware types:
  `Matrix` light/color matrices, `CdlLOC g_CdTrackLocs[18]` (CD sector
  positions as game state), `g_VabSpuAddress[4]` (SPU RAM addresses),
  `TILE`/`RECT` buffers — and resolves UI scripts by retail RAM address
  (`0x8007fb50`).
- The emulated PS1 scratchpad (`include/game/scratchpad.h`) holds the camera
  (`SCRATCH_VIEW_*`), GPU packet cursor, GTE matrix and model-bank pointers in
  one block written by both logic and renderer.
- `Random15` (`random/random15.c`) is a clean LCG, but seeding paths run
  through frame counters, so determinism depends on the frame loop.

**Input is the one clean boundary already:** the port maps SDL keys onto the
retail pad bitfield (`PadState`, `include/game/state.h`), and all game code
consumes `PAD_*` bits. This is the model the other axes should follow.

---

## 6. Recommended cut points (ordered)

Each step is gated on characterization tests per PORT_BRIEF §5.

1. **Software GTE math for simulation.** Replace
   `ApplyMatrix`/`MulMatrix2`/`RotMatrix` in `update_player_car.c`,
   `race_car_update.c`, `car_orientation.c`, `car_track_state.c`,
   `track_contact.c`, `car_ai.c` with fixed-point software equivalents
   (`render/matrix_apply.c` already contains a neutral `MatrixApplyVector`).
   Semantics must match GTE truncation exactly; characterize first. After
   this, the sim tier no longer touches COP2.
2. **Engine-neutral sim state.** Introduce explicit position/velocity/
   orientation fields (still fixed-point) and remove the `Vec4`/`LVec`
   aliasing unions and offset `_Static_assert`s in `car.h`. The renderer
   reads sim state through accessors instead of shape-punning.
3. **Update/render split per scene.** Factor each scene handler into
   `Update*()` (no OT, no scratchpad cursor, no GTE) and `Render*()`
   (reads sim state, owns all packet emission). Start with the race scene;
   the fused functions to split first are `RunRaceIntroCamera`,
   `DrawWaypoints`, and the `track/update_*scenery` pairs. Then decouple the
   sim step from `VSync` with a fixed timestep and render interpolation.
4. **Audio event bus.** Keep `PlaySoundCue` as the model; add
   `AudioSetEngineState(rpm, load, bank)` to replace the direct
   `UpdateLoadedAudioVoices` call, and `AudioSetEnvironment(zone)` to replace
   scattered `SetReverbDepth` calls; route BGM through
   `AudioPlayMusic(trackId)` (no `+3`, no `CdControl` from scenes). Everything
   below the bus (`audio/*`, `cd/*`) becomes the swappable backend.
5. **Virtual asset filesystem.** Behind `RageHostLoadAsset`, add a manifest
   (name/handle → bytes) with the packed `RAGE.BIN` TOC as the fallback
   provider and a loose-file override directory for mods. Replace the index
   formulas of 4.2 with data-driven tables (`carAssets[model][grade]`,
   `trackAssets[class][course]`) generated from the manifest; make the fixed
   counts dynamic. Allocate VRAM rects / model-bank / audio slots at load
   time instead of per-asset hardcoding.
6. **Mod surface.** Once 4–5 land, the moddable set is: manifest-addressed
   assets (cars, tracks, textures, sounds), data-driven car specs (already
   mostly tabular once `CarTachometerSpec`'s screen coordinates move to a
   renderer-owned layout table), and cue/music IDs. Extensibility limits
   (car count, course count) then live in one manifest instead of a dozen
   compiled constants.

The end state: `src/main` holds a deterministic, headless-simulatable game
core consuming abstract input bits, emitting draw/audio events and asset
handles; `src/port` + PSY-Z hold the only code that knows what a GPU packet,
an SPU voice or a CD sector is.
