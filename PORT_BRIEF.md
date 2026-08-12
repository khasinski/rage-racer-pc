# Rage Racer — Port Brief

You are starting a native port of Rage Racer (PS1, PAL `SCES_006.50`) to modern
platforms, beginning with **macOS on Apple Silicon (arm64)**, using
[PSY-Z](https://github.com/Xeeynamo/psyz) as the SDK replacement.

Everything in this brief was measured against the actual tree, not estimated.
Where a number appears, it came from a command you can re-run.

---

## 1. The goal, and the hard constraints

**Goal:** the retail game, running natively, identical in behaviour to the PS1
original.

**Hard constraints — do not violate these:**

1. **1:1 first. No improvements, no modifications, no "while I'm here" fixes**
   until the game is 100% playable start to finish. No widescreen, no higher
   internal resolution, no re-textures, no framerate changes, no bug fixes —
   even for bugs you are certain are bugs. Original bugs are in scope to
   reproduce, not to fix.
2. **Windowed, original resolution scaled ×2** (integer scale, nearest
   neighbour). PAL Rage Racer runs 320×240-class modes; scale the final
   framebuffer ×2 so it is visible on a modern display. Do not change the
   internal render resolution.
3. **Input from a config file**, keyboard by default. A plain text config
   mapping game buttons to keys, read at startup, with a sane built-in default
   if the file is missing. No hardcoded key handling scattered through the code.
4. **Behaviour must remain stable through characterization tests.** The native
   port is the only supported build; continued PS1 compilation and byte matching
   are not requirements. Capture observable behaviour in host-side tests before
   changing a subsystem, then keep those tests passing throughout the port.

---

## 2. What you have

### Layout

```
~/Projects/rage-port/
  <clone of khasinski/rage-racer-decomp @ 5052c568>
  external/psyz/     PSY-Z SDK (Xeeynamo/psyz @ be9c9b9)
  PORT_BRIEF.md      this file
```

The decomp clone is the upstream project. It is **still under active
development by another agent** — expect to rebase. Do not fork away from it;
keep pulling.

### The source, split by destiny

| Path | Size | What happens to it |
|---|---|---|
| `src/main/PAL/main/` | 178 files, **40 951 lines** | **This is the port.** Game logic: car physics, AI, track, render, menus, race flow, save, audio policy, FMV. |
| `src/main/PAL/lib/` | 167 files, 14 266 lines | **Delete from the port build.** This is a faithful reimplementation of PSY-Q (libgpu, libspu, libsnd, libcd, libgte, libapi, libpress). PSY-Z replaces it. |
| `include/game/` | — | Game headers. Port these. |
| `include/psyq/` | — | PSY-Q declarations. Replace with PSY-Z's `psyz/include`. |

The current state builds byte-identical to retail:
`make check VERSION=PAL` → `build/PAL/main.exe: OK`, SHA-1
`2913e15648eddef40821c5f666460abc04155ee6`. Progress is 1089/1089 functions and
409 400/409 400 code bytes — 100% plain C, apart from 50 documented
`HANDWRITTEN_ASM` routines.

### The game↔SDK boundary is clean

The game calls **126 distinct PSY-Q functions** and touches no GPU/SPU register
directly — there is not a single GTE macro or MMIO literal in `main/` that
belongs to the rendering path. That means the SDK swap is a real seam, not a
theoretical one.

Coverage against PSY-Z as of `be9c9b9`: **103 of 126 present, 23 not.**

Of the 23, roughly a third are not PSY-Q at all — they are internal names this
decomp invented (`KernelCallbackSlot3`, `CdPosToInt_Local`, `SsSeqCloseWrapper`,
`StCdInterrupt`, `SetDMAInterruptState`). Those come from `lib/` and either
disappear with it or move into your platform layer.

The genuinely missing PSY-Q API you will have to provide (and should upstream to
PSY-Z):

```
ApplyMatrixSV   MulMatrix2      Intpl           CdReadBreak
SpuTransferStatus               SpuVmDamperStep
SsSetSpuInputAttr               SsSetVoiceCount
SsStartSoundTickMode1           SsStopSoundTick
```

Plus BIOS file I/O used by the memory-card path: `BiosFileOpen/Close/Read/
Write/Seek`, `BiosFirstFile`, `BiosNextFile`, `BiosFormatDevice`. In the decomp
these are hand-assembled instruction-word stubs; on the host they become
ordinary file operations against a save directory.

By subsystem, the 126 break down as: libgpu 40, libsnd 33, libcd 12, libgte 10,
libspu 2, other 29.

---

## 3. Measured blockers

Re-runnable numbers, game code only (`src/main/PAL/main`):

| Blocker | Count | Note |
|---|---:|---|
| Register pins `register x asm("$4")` | 209 | **Invalid on arm64** — `unknown register name` |
| Empty barriers `asm("")` | 133 | Harmless but pointless off-PS1 |
| Hardcoded KSEG addresses `0x8xxxxxxx` | 30 | PSY-Z: *"guaranteed to crash at runtime"* |
| MMIO literals `0x1F80xxxx` | 34 | Same |
| Units still containing real assembly | 3 | See below |
| `u_long` in game code | **0** | Good — see §4 |

Whole-tree totals including `lib/`: 306 pins, 196 barriers. Those in `lib/`
disappear with `lib/`.

**The 3 game units with real assembly:**
- `main/boot/_start.c` — boot entry. PSY-Z supplies `main()`; this goes away.
- `main/render/matrix_apply.c`
- `main/render/terrain_submission.c`

Only the last two need real reimplementation in C, and they are GTE matrix and
model-face submission code — mechanical to write once you accept you do not
need a byte match for the host target.

**Scratchpad.** Two symbols live in PS1 scratchpad RAM and are assigned by the
linker script, outside any segment (`linkers/PAL/undefined_syms_manual.txt`):

```
g_FinalSkyOrderingTable = 0x1F800004;
g_ScratchRenderMode     = 0x1F800084;
```

On the host these must become ordinary objects. Note that scratchpad on PS1 is
1 KiB of fast RAM; nothing about its behaviour matters here except that the
addresses are fictional off-hardware.

**Host-compile reality check.** Running `clang -fsyntax-only` over the 178 game
files today:

| Condition | Compiles |
|---|---:|
| As-is | 23 / 178 |
| + PSY-Q header struct-size checks satisfied | 80 / 178 |
| + register pins neutralised | 97 / 178 |

The remaining failures are implicit function declarations and missing returns —
mechanical. The headers fail first because `include/psyq/gpu.h` carries C89
static assertions (`typedef char DrawEnvSizeCheck[sizeof(DrawEnv)==0x5C?1:-1]`)
that trip when `long` becomes 64-bit. Replacing `include/psyq/` with PSY-Z's
headers removes that whole class.

---

## 4. PSY-Z rules that apply to this codebase

Read `external/psyz/PORTING_GUIDE.md` in full. The parts that bite here:

- **`u_long` is pointer-sized in PSY-Z** (8 bytes on 64-bit). Keep `u_long` for
  anything holding an address. If a `u_long` holds a plain 32-bit value, change
  it to `unsigned int`. Game code currently has **zero** `u_long`, so this is
  almost entirely a `lib/` and `include/psyq/` concern — i.e. it evaporates with
  the SDK swap.
- **Never store a pointer in `s32`/`u32`.** It will be truncated. The upstream
  agent has been migrating integer-offset arithmetic to real byte pointers
  (`saveAddress.offset` → `saveAddress.bytePointer`); that work is directly
  aligned with this rule — do not undo it.
- **Ordered tables:** `u_long ot[]` must become `OT_TYPE ot[]`.
- **Custom GPU primitives:** a leading `u_long tag` must become `O_TAG tag`.
- **Include order:** `#include <psyz.h>` first, then `<libgpu.h>`, `<libgte.h>`
  etc. with angle brackets. Add `-Ipsyz/include`.
- **Never include `<fcntl.h>`.** Use PS1 flags from `<romio.h>` (`FWRITE`,
  `FCREAT`, …). PSY-Z renames `open` → `psyz_open` and friends.
- **No overlapping symbols.** A decomp can legally declare `s32 D_800A1230[2]`
  and `s32 D_800A1234` overlapping; that breaks under any porting library.
  Audit for this.

---

## 5. The verification strategy — read this twice

> **Port decision (2026-08-10):** the PS1 build and retail byte match may be
> broken by porting work. They remain useful as a one-time reference where
> available, but are not release gates. The required regression oracle is a
> growing host-side characterization test suite.

Today the project has an exceptionally strong oracle: `make check` compares the
linked image byte-for-byte against retail. It catches **any** semantic change,
to the byte. The moment you fork the source for the port, that oracle normally
dies — and 40 951 lines of car physics, rival AI and race scoring have no other
test.

When preserving the PS1 build is cheap, PSY-Z's dual-target technique remains
available:

```c
#ifdef __psyz
#define REG(r)
#else
#define REG(r) asm(r)
#endif

register s32 i REG("a2");     /* pin on PS1, plain local on host */
```

I applied this to `car_ai.c`, rebuilt for PS1, and `make check` still returned
`build/PAL/main.exe: OK`. The macro expands before parsing, so the PS1 codegen is
bit-identical while the host build never sees a MIPS register name. Do the same
for barriers (`BARRIER()` → `asm("")` on PS1, empty on host).

**Therefore the rule is:** characterize a subsystem before changing it. Pure
logic gets table-driven input/output tests, stateful logic gets deterministic
scenario tests, and host platform code gets focused integration tests. Preserve
integer-width, overflow, wraparound and ordering semantics explicitly. The PS1
byte match is informative, not mandatory.

Where the match genuinely cannot cover you (host-only code: window, input,
audio backend, file I/O), write tests. The 120 game files that carry no pins,
no barriers, no GTE and no MMIO (17 748 lines) compile in isolation and are the
natural first tranche for unit tests: compile the translation unit against a
harness that supplies its globals, and assert on outputs. Good early candidates
are the pure ones — `race/time_conversion.c`, `random/random15.c`,
`car/angle_math.c`, `track/blend_angle.c`, `track/interpolate_track_angle.c`.

---

## 6. Phased plan

Do these in order. Do not start a phase before the previous one is verified.

**Phase 0 — scaffolding, no behaviour**
- CMake project targeting arm64 macOS, linking `psyz`, defining `__psyz`.
- Add the host characterization-test harness and baseline tests for pure game
  logic.
- Register pins and barriers may be removed for the host-only port.
- Verify: all characterization tests pass and the host build compiles the first
  game translation unit.

**Phase 1 — it links**
- Swap `include/psyq/` for PSY-Z headers in the host build.
- Exclude `src/main/PAL/lib/` from the host build.
- Stub every one of the 23 missing SDK functions with a loud
  `not-implemented` abort, so you get a runtime map of what the game actually
  reaches.
- Verify: host binary links; characterization tests still pass.

**Phase 2 — it boots to a window**
- Window at original resolution ×2, nearest-neighbour, windowed.
- Get `main()` → boot scene → first frame presented, even if it is garbage.
- Verify: something renders; PS1 still matches.

**Phase 3 — assets and the disc**
- The game reads from the CD. Back `libcd`/`libds` with the real disc image
  (`~/Downloads/Rage Racer (Europe)/…`) or an extracted file tree. Do not
  repack or convert assets — read them as the game expects.
- Verify: title screen and FMV path reach real data.

**Phase 4 — input**
- Config-file key mapping, keyboard default, mapped onto the PS1 pad bitfield
  the game already consumes. The game's own pad handling stays untouched.

**Phase 5 — playable**
- Menus → car select → race → results → save. Fill in stubbed SDK functions as
  the game hits them.
- Audio last: `libsnd` is 33 of the 126 calls and the sequenced-music engine is
  the single largest risk in the project.

**Only after 100% playability:** consider anything else. Not before.

---

## 7. Things that will trip you up

### Scratchpad pointer portability (backport to the decompilation)

The first two PS1 scratchpad words are a primitive-packet cursor at `+0x00`
and an ordering-table pointer at `+0x04`. Several more pointer slots occur at
`+0x48` through `+0x60`. They are adjacent because a PS1 pointer is four
bytes. Do not model these slots on the host by casting a byte array at the
retail offsets to native pointer types: on a 64-bit target each store is eight
bytes and overlaps the following slot.

This was observed in the prologue path. Writing the packet cursor at `+0x00`
overwrote half of the ordering-table pointer at `+0x04`; `DrawSkyBackground`
then passed an invalid ordering-table address to `AddPrim`.

A second instance was the menu/prologue camera setup. Decompiled routines used
`s32 *scratch = &scratchWord0` and then accessed `scratch[2]` through
`scratch[9]`. On PS1 those indices are the camera position, angles and depth;
after word 0 and word 1 become native pointers, `scratch[2]` and `scratch[3]`
are instead the low and high halves of the OT pointer. The portable form uses
the named `SCRATCH_VIEW_*` fields. Where a large recovered camera algorithm is
still naturally expressed using the retail indices, it operates on a local
`ScratchLegacyViewWords`, then copies words 2..9 to the typed state with
`LoadScratchLegacyView`/`StoreScratchLegacyView`. The legacy word array must
never alias the native pointer fields.

Ordering-table pointers must also retain their element type. PSY-Z defines
`OT_TYPE` as the native word used by its primitive links; declaring the same
pointer as `u32 *` happens to work on PS1 but makes indexed entries use a
four-byte stride on a 64-bit host. `SkyRenderScratchpad.orderingTable` is
therefore `OT_TYPE *`, not `u32 *`.

Likewise, obtain the first OT from
`frameContext->layout.orderingTables[0]`; do not reproduce the retail
`frame + 0xCC` offset. Native `DRAWENV`/`DISPENV` and OT entries may be larger,
so the typed member is the only representation valid on both word sizes.

The packet tag itself has the same requirement. The retail `P_TAG` packs a
24-bit address and 8-bit length into one 32-bit word. Under `__psyz`, game
packet declarations use PSY-Z's native-width `addr` and `len` fields so that
`AddPrim`, `nextPrim`, and `DrawOTag` agree on one representation. Keeping the
retail bitfield in game headers while linking against the 64-bit PSY-Z
implementation produces valid-looking packets with truncated chain links.
Special packets follow the same rule: the host `DrawPacket` used for
`SetDrawMode` mirrors PSY-Z's `DR_MODE` (`tag`, `len`, two command words), while
the PS1 build keeps its original three-word packed layout.

Packet cursors must advance with `sizeof(POLY_...)`, never retail constants
such as `+0x24` or `+0x28`. Native primitive tags grow with native links, so a
fixed PS1 increment makes consecutive packets overlap and corrupts the OT
chain. This affected all four manual increments in `DrawSkyBackground`.

The portable game-side representation is `GameScratchpadRenderState` in
`include/game/scratchpad.h`. It stores pointers as native pointer fields and
keeps scalar state as fixed-width fields. Named `SCRATCH_*` accessors refer to
those fields instead of assuming four-byte pointer offsets. The raw
`g_RageScratchpad` byte buffer remains only for genuinely byte-addressed work
areas. Any newly decompiled scratchpad access must be classified the same way:

- pointer or persistent render state: add/use a typed field;
- temporary byte-addressed workspace: use `RAGE_SCRATCH_ADDRESS`;
- never store `void *` through a retail `+4` byte offset.

This is a correctness fix for both 32-bit and 64-bit builds, not a host HAL
workaround. It should be backported with the decompiled renderer.

The same rule applies to address-arithmetic unions found outside the
scratchpad. Code such as `union { void *pointer; s32 offset; }`, followed by
writing `offset += index`, truncates the pointer on a 64-bit target. For normal
C arrays (for example `g_Font8x8Cells`) use typed pointer arithmetic such as
`&font[index]`. Keep offset/pointer unions only for serialized 32-bit asset
addresses, and resolve those addresses explicitly before treating them as
native pointers.

Serialized model banks are a concrete example. Their header is always
`{u32 count, s32 tableOffset, s32 normalsOffset, s32 modelOffsets[]}`. Do not
replace those four-byte offsets in place with native pointers: every 64-bit
write overwrites the following offset. `RegisterModelBank` now resolves the
file header into a separate `NativeModelBank`; the bytes loaded from the asset
remain unchanged. `SelectModelBank` publishes the native sidecar pointers to
the renderer. This separation belongs in the decompilation as the canonical
32/64-bit representation.

The same fixed-width/sidecar rule now applies to terrain-cell and course-model
tables. The native geometry path also bounds every emitted GPU packet against
the active frame's `primitiveBuffer`. This exposed the prologue crash: the
provisional terrain decoder emitted past that buffer and overwrote adjacent
native asset sidecars. Exhaustion remains a diagnostic until retail culling is
reproduced; truncation is a safety net, not the fidelity fix.

Terrain record modes are not direct jump-table indices. Retail maps modes
`0,1,2,3,4,5` to dispatch entries `0,2,0,1,2,3`, whose record strides are
`32,36,32,32,36,36`. Treating the mode as a linear 32/36-byte choice
desynchronizes the stream, interprets face bytes as headers, and was the root
cause of the missing track, giant polygons, and primitive-buffer overwrite.
For cells with translated Z at least `0xA000`, the retail far path additionally
skips records whose byte 20 has bit 1 set.

Terrain dispatch entries 0 and 2 run the face RGB through GTE `DPCS` using the
`IR0` produced by projection; entries 1 and 3 keep the record colour. PsyZ's
`SetFogNear` was an empty stub, so even a correct terrain emitter could not
match retail depth cueing. The HAL implementation now programs the values used
by Rage Racer's recovered libgte routine: `DQA = -(near * 320) / projection`
and `DQB = 0x1400000`. The native terrain emitter applies `DpqColor` only on
the two retail fogged dispatch paths.

Native PsyZ packets contain pointer-sized links (`POLY_FT4` is 56 bytes rather
than the retail 40), so preserving the retail `0x22000` byte capacity does not
preserve its packet capacity. `GameFrameContext` therefore sizes its native
packet arena for native records, and game code uses `sizeof(GameFrameContext)`
instead of the retail `0x237E8` stride when walking the two host contexts.

Retail terrain subdivision levels are the two bytes at record offsets 22/23,
reduced by `OTZ >> SCRATCH_FACE_OT_SHIFT` and clamped at zero. A level `n`
means a fixed-point step of `4096 >> n`, or `2^n` pieces on that axis.

The GTE trace settles what `EmitSubdividedTerrainQuad` and
`InterpolateSubdivRow` interpolate. The values loaded into `VXY0..VZ2` before
the `RTPT` at `0x80028D5C` are interpolated local XYZ coordinates, not packed
screen XY. The resulting SXY FIFO is then tested with `NCLIP` and written to a
POLY_FT4. The portable path must therefore interpolate the four local vertices
with the same separable `INTPL sf=1,lm=0` stages and project every child.
Screen-space interpolation happens to hide some gaps but bends near-camera
road faces and is not retail behavior.

The two subdivision axes are asymmetric in the hand-written loop. Record byte
22 controls the outer interpolation from `v0` to `v1` and `v2` to `v3`; byte
23 controls the inner row from those results toward the `v0`-`v2` /
`v1`-`v3` side. The emitted packet order is current, next outer, next inner,
then next outer+inner. Swapping the loop bounds changes a retail 4x8 face into
8x4 and produces incorrect textures and clipping. Children inherit their
parent's OT depth: retail runs `RTPT` and `NCLIP` for each child but no child
`AVSZ4` depth rejection.

Before the child loop, faces without a negative GTE/record sign bit also emit
two `LINE_F3` packets at `parent OT + 64`. Their vertex chains are
`v0-v1-v3` and `v0-v2-v3`, use the command/colour word at record offset 24,
and end in the standard `0x55555555` polyline terminator. These are the retail
seam cover: omitting them raised exposed clear-colour road pixels in the race
regression from 91 to 252.

### Reset paths must not depend on unloaded assets

`InitMenuMode` resets every menu widget before `RequestCarSelectAssets` has
loaded a car model. Consequently `DrawCarSpecGraph(0, ...)` is a pure reset:
it sets `g_CarSpecGraphProgress` to zero and returns before reading
`g_CarModelAsset`. A decompilation which updates the performance bars first
dereferences the zero-initialized BSS pointer during the prologue-to-menu
transition and would fail on PS1 as well as on the host. Preserve this early
return when backporting; it is recovered game control flow, not a host guard.

### Initialized prologue data

The host state must not emit zero storage for pointer-bearing retail
`.data`. `g_PrologueLines` is fourteen `{s16 x, s16 y, const u8 *text}` entries
at retail address `0x8007D6DC`; `g_PrologueCameraCuts` is eleven
`{s16 timer, s16 carIndex}` entries at `0x8007D74C`. Their scalar values and
strings are checked-in native C initializers in
`src/port/native_game_state.c`. Copying retail 32-bit string pointers into a
64-bit process is invalid; these definitions should be backported as real
initialized game data.

The proportional/8x8 font lookup tables are initialized `.data` too, not BSS.
In particular `g_Font8x8Cells` is the 192-byte `(u, v)` table at retail
`0x8007C2F8`. Leaving the generated host allocation zeroed makes every glyph
sample the same texture cell even though the strings and sprite packets are
otherwise correct.

Function-pointer tables require the same treatment. The retail
`g_MenuScreenUpdate[14]` and `g_MenuScreenDraw[14]` tables at `0x80082EB8` and
`0x80082EF0` contain 32-bit MIPS code addresses. Native definitions must name
the corresponding C functions so the linker emits valid host pointers; zero
storage from the host-state generator makes `UpdateMenuMode` call address zero
on its first frame. Backport the recovered table order to initialized game
data rather than adding a dispatcher in the HAL.

### Native model face submission

The retail `SubmitModelFaces` loop performs exactly one `NCLIP`, on the first
three vertices produced by `RTPT`; only after that test does it project the
fourth vertex with `RTPS`.  The native rewrite must therefore not accept a
quad because its second triangle happens to face the camera.  Doing so admits
twisted or back-facing asset records and creates large stray polygons in the
Grand Prix prologue.  This is recovered renderer/game behaviour and belongs
in the portable replacement for the hand-written GTE routine, not in the HAL.

Likewise, the plain model FT4 emitter forces command byte `0x2D`: raw textured,
opaque FT4.  It is neither the default modulated `0x2C` nor semi-transparent.
Before writing UV0/CLUT it also adds scratchpad word `0x84`
(`g_ScratchRenderMode`) to the record word.  Car bodies set this to
`lod[1] << 16`, selecting the model's palette row; dropping that addition can
make otherwise correctly projected cars sample a transparent or unrelated
CLUT.  Both details come directly from retail `EmitPolyFT4Raw` at `0x800290C8`.

Two scalar lookup tables used by `DrawCar` are initialized retail data, not
BSS: `g_CarModelBankTable` at `0x8007D380` (11 `{model, palette}` pairs) and
`g_CarModelByCourse` at `0x8007D3AC` (4 × 11 model ids). The host state must
include their fixed-width initialized bytes. Leaving either table zeroed silently selects model 0
and palette 0 for every opponent; this is a data migration bug, not HAL policy.

The five paint lookup tables immediately before them are initialized data as
well: `g_BodyColorPrimary` (`0x8007D30C`), `g_BodyColorSecondary`
(`0x8007D330`), `g_PaintSlots3StopA` (`0x8007D354`),
`g_PaintSlots3StopB` (`0x8007D368`) and `g_PaintSlots4Stop`
(`0x8007D378`).  Zero host definitions make `ApplyBodyColor1/2` overwrite the
car palette with black entries. The checked-in fixed-width initializers and
their declarations should be backported to the decompilation.

The default lap and total record tables are also initialized retail data:
eight `s32` values each at `0x8007D444` and `0x8007D464`. Treating their first
symbols as isolated zero-initialized scalars makes the Grand Prix intro print
`0` for the course record. The portable declarations are fixed-size initialized
arrays; backport their declarations and values.
`EnterRaceScene` must also index `g_BestLapTimes[series][course][mode]`
directly. Its original union/integer spelling constructs a 32-bit address and
therefore truncates a native pointer on a 64-bit host; that is a game-code
portability bug, not a HAL concern.
The same rule applies to `GetPathSceneryPositionKey` and
`GetPathSceneryRotationKey`: return `&data->keys[index]`. Their retail union
spelling adds an offset to a 32-bit integer view of the pointer and crashes in
`InitPathScenery` under ASan/64-bit hosts.

The HUD time formatter requires the initialized 12-byte template at
`g_TimeTextBuffer` (`0x8007DF04`, initially `0'00\"000`). A zero BSS buffer
retains only the first generated digit because byte 1 is already the string
terminator, producing the apparent `0`/`1` record in the Grand Prix intro.
Backport it as initialized data, alongside the other HUD tables.

Grand Prix car lighting exposed three byte-reversed GTE command constants in
PsyZ's high-level wrappers. `NormalColorCol`, `NormalColorDpq`, and `DpqColor`
must issue `0x4B08041B` (NCCS), `0x4AE80413` (NCDS), and `0x4A780010`
(DPCS). The previous `0x1B04084B`/`0x1304E84A`/`0x1000784A` values decode as
different commands and make modulated GT4 car faces nearly black. This is a
HAL correction; the game continues to emit retail opcode `0x3C` rather than
forcing raw textures.

The native `SubmitModelFaces` translation must follow the retail GTE loop:
project and clamp the original quad, cull it with NCLIP, compute AVSZ4, and
submit it without reading a made-up per-face depth bias. In particular, byte
29 of a GT4 record is not a depth bias. A temporary near-plane triangle
clipper turned the close Grand Prix camera shot into malformed GT3 packets;
retail does not take that path for model faces.

The same rule applies to `g_AtanTable` at `0x8007B664`: it contains 1026
signed halfwords used by the game's `Atan2` camera math.  A zero-filled host
definition reduces camera aiming to quadrant-sized steps.  In the prologue
this projected the camera car around x=470 on a 320-pixel display; restoring
the retail table brought the same model face back to x=164..182.  Keep this as
initialized fixed-width game data when backporting.

All six preset lighting matrices are initialized retail data too:
`g_TrackColorMatrix`/`g_TrackLightMatrix` (`0x8007C758/0x8007C778`),
`g_DefaultColorMatrix`/`g_DefaultLightMatrix` (`0x8007D548/0x8007D568`) and
`g_MenuColorMatrix`/`g_MenuLightMatrix` (`0x80082DFC/0x80082E1C`).  The game
copies these into the active scene matrices before issuing GTE `NCCT/NCCS`.
Zero host definitions make every correctly decoded normal shade black.

Native model G4 and GT4 records carry four normal indices at offsets
8/10/12/14.  Their emitters must run those normals through the active light
and color matrices (retail `NCCT` for the first three and `NCCS` for the
fourth), rather than assigning one flat placeholder color.

### Full-size car state is game data, not a symbol-map-sized placeholder

The PAL symbol annotation reports only `0x30` bytes for `g_Cars` and one word
for `g_PlayerCar`; those values are not the allocation sizes. Game code uses
eleven `GameCarRuntime` records and one `PlayerCarRuntime`, each `0x19c` bytes.
Generating 48 and 8 byte host arrays lets `BuildStartingGrid` overwrite the
following globals and makes `DrawCars` read `activeFlag`/`aiEnabled` from
unrelated storage. Native state therefore defines the objects with their game
types and excludes both symbols from generic host-state generation. Backport
the corrected symbol sizes/types to the decompilation metadata.

`UpdateAttractCars` must also preserve the existing Y and fourth position word
when advancing X/Z. Copying an uninitialized stack `Vec4` into the complete
car position is undefined C and changes with the ABI. Initialize the temporary
from the current car position before replacing X/Z.

### Never perform native pointer arithmetic through 32-bit union members

Several recovered address unions intentionally contain a PS1-width `s32/u32`
view. That view is valid for serialized offsets, but not for arithmetic on a
live pointer. In track-camera mode 4, rebuilding `&g_TrackCameras[index]`
through `GameTrackCameraNodeAddress.value` truncates a 64-bit host address;
use typed array indexing. The race-intro key cursor likewise uses
`&script->keys[index]`, and track points use `&g_TrackPoints[index]`.
These are game-code portability fixes and should be backported rather than
hidden behind host-only macros.

### Embedded race HUD packets follow the native frame layout

Retail constants `0x236cc`, `0x236d8`, and `0x236e4` address the two
tachometer draw-mode packets and face sprite after a frame buffer containing
four-byte OT links. Native links are pointer-sized, so those constants land in
the primitive arena and eventually turn geometry bytes into OT links. Access
`GameFrameContext.layout.raceHud` and `orderingTables[0][0]` by type instead.
This removes the deterministic crash at the first race HUD frames without
changing scene flow.

The embedded packets must also be initialized through that typed layout.
`BuildRaceHudPrims` rebuilds both draw modes (texture pages 9 and 10) and the
tachometer face sprite in each frame context. Merely moving the later reads to
typed fields leaves retail's packed 12-byte adjacency assumption behind: on a
64-bit host, the uninitialized packet bodies contain pointer-sized OT links,
which then appear as three unsupported GP0 commands per race frame. This is a
game-code 32/64-bit portability fix and should be backported with the layout.

The proportional/sprite text emitters have the same representation rule.
Advance their native cursor by `sizeof(SPRT)` and `sizeof(DrawPacket)`, and add
glyphs through `GamePrimaryOrderingTable(0)`. Retail's literal 20-byte sprite,
12-byte draw-mode, and `g_DrawBuffer + 0xCC` strides overlap native packets.
In the Grand Prix intro this made three adjacent OT pointers get decoded as
GP0 commands; their apparent opcode even varied with ASLR. Backport the typed
cursor/OT expressions—the literals remain correct only for the PS1 layout.
Menu effects follow it too: the light-burst `POLY_G4` cursor advances by
`sizeof(POLY_G4)`, not retail's 36-byte packet size.

PsyZ's native Gouraud staging path needs space for the four real vertices plus
its following-vertex scratch write. Reserving only four vertices lets the last
write cross `vertex_buf`; reserve five while retaining the same submitted
quad.

The race HUD descriptor tables are initialized fixed-width data as well:
12 `GameSpriteDesc` records at `0x8007DAF4` for Grand Prix and 11 at
`0x8007DBE4` for Time Trial. Zero host storage produces correctly linked
sprites whose position, UV and size fields are all zero. The two 12-entry grid
tables at `0x8007E074` and `0x8007E0A4` must likewise be copied rather than
treated as BSS.

Finally, `g_CarImageRect` at `0x8007C484` is the initialized
`RECT {704, 0, 64, 256}` used by `UploadCarImage`. A zero definition makes the
VRAM transfer a 0-by-0 no-op even though model geometry is emitted correctly.

Track texture page selection must also use a real prototype. The recovered
call sites already pass `trackSection`, but `RequestTrackTexturePage` and the
call inside `SetTrackTexturePageNow` used old-style declarations and silently
dropped it. Reading a leftover argument register is undefined on every C ABI;
pass the section explicitly in game code. This fix should be backported to the
decompilation even though the current native Grand Prix frame still has a
separate terrain rendering defect.

On 64-bit PsyZ, packet length must be read through `getlen()`, not directly
from `DR_ENV.len`: PSYQ's `setlen()` writes the low byte only. The backend was
therefore incorporating stale upper bytes into the length. In addition, after
flushing a full native command queue it must still copy the packet which
triggered the flush; advancing `queue_len` without that copy drops geometry.

The native terrain emitter must preserve the retail dispatch mapping and its
texture-window packet chain. Batch types 0 and 1 select an entry using
`type * 2 + envMode4`; types 2 through 5 use `type - 2`. Entries 2 and 3 carry
the extra word at record offset `0x20`. Emit it as GP0 E2 immediately before
the FT4 and emit a zero E2 immediately after it. Omitting this state makes the
Grand Prix road render as large white/black patches. Entries 1 and 3 also use
the adjacent CLUT row. `grand_prix_terrain_frame` characterizes this path so it
cannot silently regress.

Track texture swapping also depends on initialized data and exact retail
adjacency. `g_TrackTextureRowRect` is `{576, 0, 448, 1}`, not zeroed BSS, and
the shadow-page state is one contiguous 256-byte array. Modelling its last byte
as a separately aligned symbol makes the retail backwards reset loop overwrite
unrelated native globals. Native game state must copy the rectangle and reset
the typed array by index; this is a host representation fix, while the retail
layout must remain documented for the decompilation.

The live controller mapping is a single 16-element `u16` array. Retail symbols
such as accelerator, brake, shift and mirror masks are aliases into that array,
not independent variables. The two 64-element preset tables are initialized
retail data and must be copied on native startup. Keeping generated one-symbol
BSS placeholders leaves the accelerator mask at zero and prevents a race from
accepting input. The smoke-only input script also supplies controller type
`0x41` for held digital ranges so it exercises the unchanged game input path.

The BIOS receive area is likewise one 0x50-byte allocation: two 0x28-byte pad
buffers. `g_PadBufferType`, `g_PadBufferButtonsHigh`, and
`g_PadBufferButtonsLow` are byte aliases at offsets 1, 2, and 3, not standalone
host globals. `g_PadState` must be allocated as the complete typed structure;
an eight-byte generated placeholder lets `InitPAD` overwrite adjacent globals
and breaks interactive keyboard edges even though direct smoke injection
works. Initialize SDL input before resolving configurable key names. The
prologue regression now feeds raw digital-pad packets and proves the game's own
held-to-pressed edge calculation can skip START/CROSS screens. Backport the
correct buffer/state sizes and alias metadata to the decompilation.

The recovered game-facing names `InitPad`/`StartPad` are not disposable no-op
stubs: the native adapter must forward them to PsyZ `InitPAD`/`StartPAD`.
Without `InitPAD`, SDL still receives keyboard events but PsyZ has no BIOS
receive-buffer pointers, so `UpdatePadState` reads permanent zeroes. The SDL
backend also latches every non-repeat `KEY_DOWN` for one pad sample; an FMV can
otherwise pump both halves of a short tap together and lose the transition
when only `SDL_GetKeyboardState` is sampled.

The start-countdown strip is another fixed-width/native-width boundary.
Retail stores 512 TILE packets at a 0x10-byte stride, but a native TILE has a
pointer-sized tag. Index the native backing store by `sizeof(TILE)`, size both
512-entry banks accordingly, and advance the six following SPRT packets by
`sizeof(SPRT)` rather than the retail 20 bytes. The initialized tables at
`0x8007DEC0` (16 digit-pattern words) and `0x8007DF1C` (four CVec values,
including the TILE opcode byte) must be copied from retail data. Leaving them
as generated BSS turns every countdown cell into an invalid GPU command and
eventually corrupts OT traversal.

Replay storage deliberately spans two adjacent retail symbols. The nominal
`g_ReplayFrameBuffer` is `0x2EE0` bytes and the following countdown backing is
`0x5DC0`; together they form the `0x8CA0` arena required by 750 48-byte Grand
Prix frames (or 1285 28-byte Time Attack frames). Native storage must model the
full arena even though countdown TILEs use a separate expanded backing. Also
define `g_RankingRecords` and `g_TimeRecords` as their real 2x4x5 arrays:
zeroing 40 `RaceRecord`s through an eight-byte symbol-map placeholder wipes
the replay pointers during `InitRecordTables`.

`GetCamRow` retains the retail eight-byte sliding-row convention, but native
addressing must use `table + index * 8` directly. Overlaying a pointer with an
`s32 byteOffset` only happens to work for selected host addresses. Both car
draw paths must also initialize `clipHandle` before their visibility branches;
a culled car otherwise tests an indeterminate stack value and may restore a
lighting matrix that was never replaced.

`TriggerRaceCues` contained a genuine 32/64-bit game-code bug: it formed
addresses by adding byte offsets through the `s32` member of a pointer union.
As soon as the player car moved far enough to evaluate a speed cue, the host
pointer was truncated and the game crashed. Index `finish[series]` and
`speed[series][cue]` with typed pointer arithmetic instead. This change is
portable on both 32- and 64-bit targets and should be backported to the
decompilation. The `race_start` integration test now holds acceleration after
the countdown and requires positive speed plus a full digital input sample.

Retail text tables are pointer tables, not byte strings. In particular,
`g_GrandPrixNames` contains eleven pointers and `g_CourseNames` is the
four-pointer alias beginning at retail entry 11. Native startup must read each
32-bit retail address and resolve it to a host pointer. Copying the 44- and
16-byte symbol-map ranges into generated BSS leaves null/invalid native
pointers and crashes the Grand Prix result screen in `DrawText8x8Trans`.

The smoke binary has two result-path controls, both confined to the debug test
harness. `RAGE_PORT_SMOKE_FINISH_FRAME` repeatedly places the player just over
the current finish-line threshold, allowing the unchanged lap/finish logic to
advance every remaining lap. `RAGE_PORT_SMOKE_AUTO_CONFIRM_FRAME` supplies a
digital confirm every 60 frames after its start. Together they cover finish
camera, replay, result, prize-money, and next-round setup without weakening the
game code. `grand_prix_results` characterizes that complete transition.

Whenever the smoke executable is rebuilt, rebuild the ordinary interactive
game in the same invocation as well. Use the full
`cmake --build build-host -j4`, not an individual `rage-racer-smoke` target.
This keeps `rage-racer` available for manual testing in parallel with every
automated smoke/debug iteration and prevents the two executables from silently
drifting onto different object revisions.

Native memory-card I/O uses the existing PsyZ `bu00:` path mapping, but the
port must forward the game's `BiosFileOpen/Read/Write/Seek/Close`, directory
scan, and format calls instead of returning stub failures. The Unix
`psyz_open` flag conversion must assign `O_RDONLY/O_WRONLY/O_RDWR` to the host
flag while preserving PSYQ's `FCREAT`; mutating the input flag or only updating
the wrong local creates an empty file. The initialized `g_SaveTitleSjis` and
`g_SaveFilePath` ranges must be copied from retail data. Store/load table loops
must use typed pointers rather than the `s32` members of pointer unions. The
`save_roundtrip` test writes the original `0x1300`-byte file, clears a live
progress value, and proves that loading restores it from an isolated card
directory. Do not truncate host file-I/O paths to PS1's 20-byte `DIRENTRY`
field: the three retail filenames differ only in their final slot digit, beyond
that boundary, so truncation aliases every slot to one file. Truncate only the
name copied out by directory enumeration. The same regression reboots into the
title-screen memory-card scene and requires exactly slot 0 to be detected.

The graphical card menu has another retail pointer table:
`g_McMessageRows[19]` points into 24 serialized eight-byte rows whose first
word is a retail text address. Native state must expand each row to a host
pointer plus column byte, resolve its text address, and rebuild the 19-pointer
index. Its message/help strings, formatting strings, charset, column table and
slot labels are initialized data as well; zero BSS crashes the first prompt or
renders an empty menu.

The native sound globals must preserve retail aliasing. `g_VabIds` is the
halfword array inside `g_SoundScale`, and the fields reached through
`g_AudioRuntimeState` are the same live scalars/arrays used elsewhere; separate
generated BSS objects silently split the loader from playback. Game code now
uses the typed live globals directly. The initialized cue tables
`g_SoundCueParams`, `g_SoundCueParams2`, `g_EffectCueTable`,
`g_SpecialVoiceBits`, and `g_VabSpuAddress` also have to be copied from retail.
With those restored, the original VAB path produces non-zero PCM through the
emulated SPU; `audio_output` measures rendered frames and sample energy rather
than accepting successful SDL initialization as evidence of sound.

CD-DA has a similar aliasing requirement. Retail's two-entry
`g_CdTrackLocs` object is immediately followed by the sixteen BGM locations at
`g_CdBgmTrackLocs`; game requests deliberately index the former with track
numbers through 17. Native state therefore owns one 18-entry table and exposes
`g_CdBgmTrackLocs` as `&g_CdTrackLocs[2]`. Separate generated objects resolve
race BGM to zero/Track 01. The 18 initialized `g_CdTrackLoopPoint` entries at
`0x8007F5B0` must also be copied. In PsyZ, `CdlSetloc` marks CD-DA for a rewind
and the following `CdlPlay` must reopen the requested track even if another
track is currently playing. The `race_cdda` integration test proves the switch
from prologue Track 02 to an actual race music track.

- **Do not "fix" the code toward testability.** Byte-match forbids it while the
  PS1 target lives. Tests live *beside* the code, not inside it.
- **`make split` regenerates `asm/` and currently rewrites
  `asm/PAL/main/nonmatchings/main/boot/_start/D_800630B4.s` with weaker
  symbolisation** (raw `D_8009AED8` instead of `g_SpuRevAttrTable + 0x2A8`).
  This is a splat-version artefact — `requirements.txt` pins only
  `splat64[mips]>=0.39.1` and a newer splat emits more auto-symbols that shadow
  the curated names. It shows up as a spurious modified file and will block
  `git pull`. Discard it; do not commit it.
- **The upstream agent commits frequently** (867 commits in two days). Rebase
  often, and re-run `make check` *after* rebasing, not only before — a change
  verified against an older tree is not verified against the merged one.
- **`code_debt --check` passing means little right now.** All 18 categories sit
  exactly at their baseline because upstream ratchets the baseline down after
  each sweep. It means "no worse than what was just set", not "better".
- **Register pins are not noise.** A pin that resists removal is a signal that
  the C does not yet have the original's shape; they have been falling as the
  code gets properly typed. Do not strip them from the PS1 side to make life
  easier — use the macro.

---

## 8. First commands

```sh
cd ~/Projects/rage-port

# confirm the inherited state before changing anything
python3 -m venv .venv && .venv/bin/pip install -r requirements.txt
mkdir -p assets/PAL
# stage the game from your own copy, then:
make split VERSION=PAL && make build VERSION=PAL && make check VERSION=PAL
#   expected: build/PAL/main.exe: OK

# PSY-Z
cd external/psyz && make -j && cd ../..
```

Note the toolchain requirements inherited from the decomp: `mipsel-none-elf`
binutils on PATH, a GCC 2.6.3 PSX `cc1` in `build/toolchain/bin/`, and Python
≥3.10 for `maspsx` (the repo's `cc.sh` honours `$PYTHON`).

---

## 9. Definition of done for this stage

The game boots from the real disc data, shows the title screen, accepts keyboard
input mapped from a config file, lets you select a car and a course, race a full
event, see the results, and save — in a window at original resolution ×2, on
macOS arm64. Sound working. No graphical or logic differences from the PS1
original that you introduced.

The host characterization and integration test suite passes at every commit.

### Native global state

The host build must not synthesize every unresolved game symbol as zero BSS.
Retail data symbols now live in the checked-in `src/port/host_state.c`; true
BSS remains zero-initialized, while symbols from the initialized PS-EXE range
carry their original bytes in the C build. `src/port/native_initialized_state.c`
does the same for globals whose host storage was split out to preserve native
alignment or 64-bit safety. Neither file is generated by Python during the
build. Pointer-bearing tables use typed native C initializers. UI scripts retain
their serialized fixed-width bytes in `g_UiScriptData`; startup only relocates
their checked-in 32-bit offsets to native pointers. It does not open or read
`assets/PAL/main.exe`. BGM/course names, CD audio paths, and memory-card message
rows are ordinary typed C arrays rather than runtime-relocated PS-EXE pointers.

Several retail symbols name fields inside one packed object rather than
independent allocations. The proportional, word, high and sprite font tables,
cell scan offsets, prize table, sound tone grid, clock text, and scenery
position/yaw records therefore share backing arrays with explicit offset
aliases. Separating them lets the linker insert redzones or alignment gaps and
is a real 32/64-bit game-data bug. The ASan integration suite exercises these
paths so backports preserve the retail layout without depending on adjacent
linker placement.

The Time Attack defaults are a useful guard for this distinction. Retail has
one enabled car out of thirteen and initially selects car index 3. A zeroed
`g_SaveDefaults` produces `0/13`; `initial_state` asserts `1/13` and index 3.

### Host window size

`DEFAULT_FRONT_W/H` are logical window dimensions. On HiDPI displays SDL
creates a larger backing store automatically; dividing 640×480 by pixel density
created a visibly 320×240 window on Retina. The SDL HAL now requests logical
640×480 directly and tracks backing-store pixels separately. Set
`RAGE_PORT_WINDOW_DEBUG=1` to log logical size, pixel size, and density.

### Race update cadence

Do not remove the `g_FrameSyncThreshold` wait from `MainLoop`. The PAL game uses
`0x80` for menus and `0x180` after `InitRaceScene`: menu logic advances every
VBlank, while race physics and lap-frame counters advance every second VBlank.
`FramesToMilliseconds` makes the intended race rate explicit: 25 update frames
equal 1000 ms. Running every scene handler once per host VBlank therefore makes
the race physics and its displayed timers exactly twice as fast.

The native build keeps the original game loop. PsyZ implements `VSync(1)` in
1/256-VBlank units, including a deterministic LIMITLESS path for smoke tests.
The returned counter is quantized to hardware VBlank boundaries: it can move
from `0x100` to `0x200`, but must never report the continuous intermediate
`0x180`. Consequently a menu threshold of `0x80` completes on the first
VBlank, while a race threshold of `0x180` completes on the second. Returning
continuous wall-time fractions was the source of the too-fast native race.
This timing policy belongs in the HAL, not in special cases in the game code.

### Showroom ordering and model lighting (backport to the decompilation)

`DrawMenuCarView` temporarily adds 120 bytes to the word at PS1 scratchpad
address `0x1f800004` before submitting the showroom stand. That word is the
ordering-table base pointer, so the retail operation means a 30-bucket OT
bias. It must be expressed as `SCRATCH_OT_BASE_AS(OT_TYPE) += 30` on a native
target: keeping `D_1F800004` as an unrelated scalar makes the operation a
no-op and draws the stand over the car. This is a game-code 32/64-bit aliasing
fix and should be backported.

The native `SubmitModel` implementation must also use the initialized
`SCRATCH_GT4_R/G/B/CODE` values as the base passed to `NormalColorCol`.
`InitRenderState` deliberately sets GT4 RGB to `0xff`; a hard-coded `0x80`
halves the lighting result and turns weakly lit body textures into apparently
black or missing panels. This applies both to ordinary GT4 faces and the
near-plane clipping path.

Model and terrain backface tests must remain separate in a portable rewrite.
Retail `SubmitModelFaces` performs one `NCLIP`: the main pass accepts only
`clip > 0`, while the mirror pass accepts `clip >= 0`.  The terrain dispatcher
performs a second `NCLIP` after projecting its fourth corner and therefore has
a genuinely different two-half rule.  Reusing that terrain expression for a
model by setting `clip1 = clip0` reduces the rejection condition to
`clip0 == 0`; almost every back-facing model polygon is then emitted.  In the
starting-grid reference frame this submitted a dark untextured car quad after
the correctly lit body and produced the large black triangular panel.  Keep
the single-result model branches equivalent to the original MIPS when this
change is backported to another portable target.

For renderer comparisons, `RAGE_PORT_DUMP_VRAM=/path/vram.bin` writes the
final 1024x512 little-endian 16-bit VRAM image from the smoke executable.
This made it possible to prove that the car-select texture page and CLUT match
the emulator byte-for-byte before investigating rasterization differences.

For repeatable frame comparisons, stop both implementations by game state,
not host time or raw loop count. The smoke harness accepts
`RAGE_PORT_SMOKE_STOP_SCENE=12` and
`RAGE_PORT_SMOKE_STOP_SCENE_TIMER=220`; the Ruby runner has the matching
`RAGE_EMU_STOP_SCENE` and `RAGE_EMU_STOP_SCENE_TIMER` controls and saves both
a screenshot and a reusable savestate at that point. Keep a generous frame
limit only as a safety timeout. The PSX game advances this race timer once per
two PAL VBlanks while the current native build advances it once per loop, so
equal raw frame numbers are not equivalent states.

`tools/rage_visual_compare.rb` turns a cached emulator capture and a fresh
native capture into one debug bundle:

```sh
ruby tools/rage_visual_compare.rb \
  --psx /tmp/reference.ppm --native /tmp/native.ppm \
  --output /tmp/rage-compare --pixel 260,100 \
  --psx-log /tmp/reference.log --native-log /tmp/native.log
```

The bundle contains normalized inputs, an absolute difference image, an
amplified heatmap, a side-by-side view, normalized RMSE and the ordered GPU
packets that cover the selected screen pixel. `RAGE_GPU_TRACE_PIXEL=x,y`,
optional `RAGE_GPU_TRACE_FRAME=n`, and packet-wide `RAGE_GPU_TRACE_TPAGE` /
`RAGE_GPU_TRACE_CLUT` filters use the same record format in PsyZ SDL_GPU/GL and
the Ruby emulator. Coordinates use the page-local raster position
`packet XY + draw offset - drawing-page base`; the page base is selected from
the draw-area Y (0 or 240), never from the currently displayed page. Raw packet
coordinates lose the mirror's deliberately changed geometry offset, while
subtracting `display_start` compares the back buffer with the front buffer and
creates a false 240-line displacement. Coverage rejects degenerate triangles.
Configuration is parsed once, so disabled or frame-filtered tracing stays cheap
enough for repeated smoke runs.

Without `--pixel`, the comparison also reports separated maximum-error
hotspots with the PSX/native RGB values and writes marked copies of both
frames.  Use `--region X,Y,W,H` to keep animated HUD or scenery out of a road,
mirror, or dashboard investigation; `--hotspots` and `--hotspot-radius`
control the number and spacing of candidates.

For sequences, both runners can emit the same timer-based filenames.  Create
the output directories first, then set
`RAGE_EMU_TIMER_FILENAMES=1` on `rage-frame-capture`, and set
`RAGE_PORT_SMOKE_CAPTURE_DIR` plus
`RAGE_PORT_SMOKE_CAPTURE_TIMER_STRIDE` on the smoke binary.  The smoke capture
is restricted to `RAGE_PORT_SMOKE_STOP_SCENE` when that variable is present.
Use `RAGE_PORT_SMOKE_CAPTURE_TIMER_MIN/MAX` or the matching
`RAGE_EMU_CAPTURE_TIMER_MIN/MAX` bounds for dense one-timer captures around a
problem without writing the whole race.  Timer-named emulator captures are
deduplicated when two VBlanks observe the same game timer.
Long emulator runs can be split into sub-minute chunks by also setting
`RAGE_EMU_SAVE_CAPTURE_STATES=1`.  Each periodic PPM then receives a matching
`.psxstate`; load the last checkpoint with `RAGE_EMU_LOAD_STATE` and continue
to the next timer.  This retains deterministic emulated time while avoiding a
new BIOS, frontend, and race-intro run for every trace.

A fixed frame-number input script is not reusable from a fresh boot because
FMV cadence determines whether those presses occur at the title, menu, or
inside the movie. For the current local reference cache, resume the saved
scene-8 `NOW LOADING` state, issue a one-frame `CROSS` edge after loading has
settled, and save the first scene-12 checkpoint. The resulting race-intro
timer-141 state can be resumed with held `CROSS`; reaching timer 800 with
timer-named screenshots and per-capture states takes about two minutes with
Ruby 4/YJIT and avoids replaying BIOS, FMV, and menus thereafter.

This produces `timer-00150-s12.ppm`-style files without replaying the frontend
for every sample.  Pair the cached emulator sequence with any fresh native
sequence using:

```sh
ruby tools/rage_visual_batch.rb \
  --psx-dir /tmp/rage-psx-series --native-dir /tmp/rage-native-series \
  --output /tmp/rage-series-compare --region 0,55,250,150
```

For repeated renderer iteration, the batch tool also accepts stable named
regions: `road`, `mirror`, `mirror-frame`, `rank`, `record`, `tacho`, `time`
and `hud`. For example, use `--preset tacho --jobs 8 --top 10` to compare the
cached sequence in parallel and print only the ten worst frames. The complete
machine-readable `summary.json` and all per-frame bundles are still written,
so limiting console output does not discard evidence.

Race captures include the X/Z positions of all four active rivals as well as
the player state. Position matching adds their aggregate distance to its score
and records it in each report. This matters especially for the narrow mirror:
two frames can have an identical player pose while a rival differs by tens of
world units, shifting its silhouette by a pixel and dominating mirror RMSE.
Regenerate old cached manifests when diagnosing cars in the mirror; the batch
tool remains backward compatible with caches that lack these columns.
Use `--max-rival-distance N` for mirror investigations to reject pairs whose
aggregate four-car X/Z displacement is too large; the selected distance is
printed beside player and camera alignment. A value around 180 retains enough
samples in the current 837..987 dense run while excluding the most misleading
car-silhouette comparisons.

Each matched timer gets its own standard comparison bundle, while
`summary.json` and stdout rank frames by RMSE and identify the worst hotspot.
Both runners also write `capture-manifest.csv` with the filename, scene/timer,
player position, speed, progress, body pitch/yaw/roll, lateral track offset,
RNG seed, mirror position and active scratchpad view. This makes later
divergence explicit instead of attributing a shifted or collision-rolled camera
to the renderer. `rage_visual_batch.rb --match position` selects the
nearest same-scene/same-lap state and rejects matches farther than
`--max-position-distance` (64 by default). A wider limit is useful for
simulation-divergence diagnostics but is too permissive for pixel-level
renderer ranking. The displayed VRAM page can lag
the state sampled at VBlank because of double buffering; add
`--visual-refine 3` to test only a small timer window around the state match
and select its lowest-RMSE image.  On the timer-330..370 checkpoint series this
correctly pairs PSX timer 350 with native timer 348, only 49 world units apart,
instead of comparing it with a visibly different camera phase.

The state score also penalizes pitch, roll, lateral offset and unequal RNG
seeds when those newer manifest columns are present, while remaining compatible
with cached older captures. This matters immediately after contact: two frames
can have identical position, speed and yaw but different body roll, which the
chase camera copies into `SCRATCH_VIEW_ANGLE_Z` and therefore changes every
terrain and sky projection.

RMSE is often dominated by HUD digits, animated signs and a one-frame texture
phase even when the road is correct.  For terrain-hole searches pass the same
road rectangle as `--clear-region X,Y,W,H` and use `--rank clear`.  Each bundle
then records `native_only_clear.count` plus up to 32 sample coordinates where
native exposes Rage Racer's dark-blue clear colour (`r < 8, g < 8, b > 35`)
but the emulator does not.  On the coarse timer-600..800 series this ranked
timer 620 first with 58 pixels; a dense timer-610..630 refinement paired PSX
620 with native 617 and reduced the count to 26, proving that the apparent
wedge was alignment/raster drift rather than a missing terrain face.

Black textured artifacts need a separate signal from exposed clear colour.
Pass `--black-region X,Y,W,H` and `--rank black` to count pixels that are
near-black only in native while the PSX reference has visible surface colour.
Treat this as candidate ranking, not proof by itself: a one-tick camera shift
also moves legitimate shadows and dark texture edges. The report retains the
32 highest-error samples so a candidate can be followed with
`RAGE_GPU_TRACE_PIXEL` on both renderers. `--artifact-radius N` (default 2)
suppresses a candidate when the same clear/black class exists within N pixels
of the reference coordinate. This removes shifted texture edges without
erasing a solid triangular hole; use radius 0 when investigating exact raster
coverage.

For packet-level follow-up, the native terrain dispatcher accepts optional
`RAGE_PORT_TERRAIN_TRACE_TIMER`, `RAGE_PORT_TERRAIN_TRACE_CLUT`, and
`RAGE_PORT_TERRAIN_TRACE_TPAGE` filters. Matching faces report their cell and
face indices, dispatch mode, rejection reason, OT depth, colour, vertex
indices, cell translation, and projected coordinates. Subdivided children
also report all four interpolated local XYZ vertices before projection. This
allows a captured retail GTE `RTPT` input to be matched directly to a native
child, independently of later rasterization and texture sampling. The trace is inactive
unless at least one filter is present and does not alter assets or game state.

For example, the timer-826 road edge identified by the clear-pixel ranker has
the native local vertex `(3779,24000,-3103)`. Retail's GTE trace loads the same
values (`VXY=0x5dc00ec3`, `VZ=0xfffff3e1`) before the child projection. The
native packet also covers the investigated screen coordinate, so that case is
not evidence of a missing terrain face or an incorrect draw distance; its
remaining difference occurs after game geometry submission. A controlled
test removing PsyZ's derivative-based UV offset increased road-region RMSE
from 0.15657 to 0.15865 and was therefore rejected rather than retained as a
visual workaround.

Packet coverage alone is insufficient for black-polygon diagnosis. With
`RAGE_GPU_TRACE_PIXEL=x,y`, PsyZ also reports the selected triangle, affine UV
derivatives, rounded texel coordinate, and the coordinate after the active
texture window. The Ruby reference accepts `RAGE_GPU_TRACE_TEXEL=1` with the
same pixel/frame filters and reports the UV, palette index, and final 16-bit
CLUT colour actually used by its scanline rasterizer. At race timer 826 this
proved that the apparently missing road geometry was submitted, but the host
selected palette index zero, whose CLUT entry is opaque black (`0x8000`).

At native resolution, PS1-compatible hardware renderers place integer polygon
vertices at pixel centres and round interpolated texture coordinates to the
nearest even integer. PsyZ previously placed vertices on modern pixel
boundaries and tried to compensate in the fragment shader by subtracting only
`abs(dU/dx)` from U and `abs(dV/dy)` from V. The SDL_GPU shader now applies a
half-pixel vertex offset and `roundEven(rawUV)` instead. Across the synchronized
timer-800..920 road series this reduces native-only clear pixels from an
average 444.9 to 311.1 and the worst frame from 5713 to 5597. The Ruby
emulator's inclusive floating-point scanline rasterizer does not yet implement
the same edge convention, so edge-only black-pixel rankings remain a candidate
finder rather than an acceptance oracle.

PsyZ must not infer texture flipping from projected screen coordinates. Its
old `FixupFlipUV` compared XY and UV orientation and incremented every U or V
in a packet when they appeared reversed. The PS1 consumes the packet UVs
verbatim; orientation is already expressed by which UV belongs to each
vertex. At synchronized timer 877 the retail packet contained U `126..0`,
while this heuristic changed the identical native packet to `127..1` and
could move sampling onto palette index zero (`0x8000`, opaque black). Removing
the heuristic improves road-region RMSE, the maximum mirror black-pixel count,
and the maximum HUD black-pixel count without changing submitted geometry.

Timer labels are not identical render checkpoints across the two runtimes. In
the repeatable turning scenario, PSX timer 501 has the same car state as native
timer 500; comparing equal timer numbers advances native physics once and can
make the neighbouring terrain segment look like a huge bent-road wedge. Use
the capture manifests and `--match position --visual-refine N`, and verify
position, progress and view yaw before interpreting a polygon difference. The
smoke input parser accepts combinations such as `CROSS+LEFT`, matching
`RAGE_EMU_INPUT_SCRIPT`, so steering cases can now be replayed rather than
approximated by straight-line captures.

At synchronized race timer 220, the reference and native captures align in
car, HUD, start lights and perspective. Filtering for tpage `0x0005` and CLUT
`0x7943` proves that the apparently missing perspective quads are present in
both streams; after expressing both in draw-page-local coordinates their
geometry is close. Continue black-wedge investigation from a genuinely
differing pixel/packet rather than treating page-flip coordinates as a game or
clipping bug.

The terrain face record's byte at `+0x14` also controls retail UV reduction.
When OTZ is at least `0x800` and bit 0 is set, the hand-written dispatcher
halves all four UV pairs before direct emission or subdivision. The portable
dispatcher must do the same: otherwise a retail `0..127` tile becomes
`0..254` and samples unrelated parts of the 4-bit texture page. This decoding
belongs in the portable counterpart of `SubmitTerrainCells` and should be
backported with it.

The two triangle tests of a subdivided terrain child intentionally have
opposite winding. At `0x80028A9C..0x80028AD4`, retail accepts a main-view child
when the first `NCLIP` is positive or the second is negative; the mirror pass
accepts it when the first is negative or the second is positive. Treating both
triangles as if they had the same sign submits back-facing folded quads and
drops visible ones, producing large terrain wedges and holes. The three IR0
decrements at `0x80028D58..0x80028D64` prepare three adjacent vertices for
RTPT; they are not a four-cell packet stride. Portable subdivision must still
emit each adjacent grid interval. Backport both the opposite-winding test and
the one-interval iteration semantics.

Several apparent HUD globals are interior labels in the retail player object,
not independent storage. `g_PlayerSpeed` at `0x8009E778` is
`g_PlayerCar.speed` (`+0xA4`), and `g_PlayerGear` at `0x8009E806` is
`g_PlayerCar.drive.gear`. Native code must name those fields directly; separate
zero-initialized globals make the tachometer display `000`, gear `0`, and feed
zero speed to path scenery. The race-start framebuffer regression checks that
the deterministic moving state has distinct speed glyphs and a visible needle.

The mirror badge table has the same overlapping-symbol trap. Retail stores four
`{u,v,width}` triples at `0x8007C738`; the U, V and width symbols point at bytes
0, 1 and 2 and are indexed by `style * 3`. Native arrays cannot overlap, so
their indexed views explicitly contain U `{e8,50,78,a0}`, V `{30,10,10,10}`
and width `{10,28,28,30}` at indices 0, 3, 6 and 9. Treat this as a game-data
layout fix for the decompilation, not runtime asset copying.

### Terrain subdivision and rear-view cell selection (backport)

The retail terrain dispatcher performs the face `NCLIP` before subdivision and
then repeats a *two-triangle* test for every interpolated child.  At
`0x80028A74` it loads `(v0,v1,v2)` into SXY0..2 and runs `NCLIP`; when that
half fails, `0x80028AB4` replaces only SXY0 with `v3` and runs the FIFO test
again on `(v3,v1,v2)`.  In the main view the child survives when
`clip0 > 0 || clip1 < 0`; the mirror reverses both signs.  The earlier native
one-triangle approximation punched holes where an interpolated first half was
degenerate.  Removing child `NCLIP` entirely avoided those holes but emitted
hundreds of back-facing children per frame, allowing overlapping road pieces
to stretch across the near view.  `RageScreenQuadVisible` now reproduces both
retail tests before its four-corner screen-edge rejection.  At synchronized
timer 151 this reduces textured tpage-5 packets from 442 to 277, close to the
PSX stream's 293; the remaining difference follows the adjacent displayed
frame.  Smoke counters `terrain_child_reject` and `terrain_child_second` prove
both the rejection and second-half rescue paths execute.

`BuildVisibleCells` quadrant 3 uses opposite signs for the two passes. The main
view uses `sx = cx - dx`, `sy = cy + dy`; the reflected pass uses
`sx = cx + dx`, `sy = cy - dy`. Applying either pair globally makes one list
correct while reversing the other. In the synchronized frame the retail main
list starts with cell IDs `167,166,168,165`, while the mirror starts
`167,168,166,169`; the port now reproduces both orders. The wrong main-view
signs removed the stone overpass on the left even though the checked-in scan
table was byte-for-byte identical to retail. This is game-code control flow
and must be backported to the decompilation.

The rear-view terrain dispatcher reflects the first GTE rotation row. Because
the cell center has already been transformed by `BuildVisibleCells`, its X
translation must be reflected as well before `SetTransVector`; otherwise the
correct mirror terrain is displaced by tens of pixels. The synchronized visual
test checks both the road for clear-colour wedges and the mirror for a textured
road surface.

The terrain face loop is not allowed to reuse the model face's single-triangle
back-face test.  Retail runs `NCLIP` once after `RTPT(v0,v1,v2)`, projects the
fourth corner with `RTPS`, then runs it again on the resulting FIFO
`(v1,v2,v3)`.  The second triangle has the opposite winding in the stored quad
order; a main-view face survives when `clip0 > 0 || clip1 < 0` (with both signs
reversed in the mirror).  Testing only the first triangle discarded complete
terrain quads whenever that half became degenerate near the camera, producing
recurrent holes and an apparently bending road.  Keep the two-triangle rule in
the portable terrain dispatcher while retaining the original one-triangle
rule for models.  The smoke summary's `terrain_second` counter and race test
prove that the retail-only rescue path is exercised.

Do not add a screen-span near-plane heuristic to the direct terrain path.
Retail `SubmitTerrainCells` has no `maxX-minX`/`maxY-minY` rejection: after
the two `NCLIP` decisions it tests all four projected X coordinates against
the scratchpad bounds at `0x80028324..0x80028360`, repeats that for Y at
`0x8002837C..0x800283BC`, and then accepts OT indices 1..447 at
`0x800283C0..0x800283CC`.  A former native `640x512` span cutoff was an
undocumented substitute for PS1 projection behaviour and could discard the
large close-road faces whose subdivision is intended to make them safe.  The
portable path now uses only the retail NCLIP, screen-bound and OT-depth
decisions.

All three retail geometry dispatchers start from an ordering-table pointer
shifted by `0x200` bytes. On PS1 this is 128 four-byte OT entries: the model
dispatcher loads it at `0x80028ECC..0x80028ED0`, and the course dispatcher at
`0x8002973C..0x80029748`; terrain shares the model dispatcher setup. The
portable renderer must therefore submit model, course and terrain primitives
relative to `SCRATCH_OT_BASE + 128`, not the start of the frame OT.

The depth tests and the signed record bias have a deliberately asymmetric
order. Retail first validates the un-biased projected depth in `1..447`, then
adds the face bias to the already shifted OT pointer without a second range
rejection. Subdivided children inherit that biased parent OT position. A
second native check after applying the bias incorrectly discarded valid near
faces: at synchronized race timer 948, terrain cell 161 face 50 has parent
depth 3 and bias -3, and must be emitted at absolute OT entry 128. Dropping it
exposed a large clear-colour road wedge. This fixed OT base and validation
order are game-renderer semantics to backport, not a HAL workaround or a
32/64-bit compatibility heuristic.

Course-model faces use a related but distinct retail dispatcher and must not
be collapsed into the ordinary model FT4 path. Its four record modes are F4
(`0x10` bytes), FT4 (`0x1c`), subdivided FT4 (`0x20`) and scrolling,
subdivided FT4 (`0x20`). Modes 2 and 3 store the horizontal and vertical
subdivision levels at bytes 26/27 and a GPU texture-window word at byte 28.
The effective levels subtract `OTZ >> SCRATCH_FACE_OT_SHIFT`, clamp at zero,
and select the same two-stage GTE `INTPL` interpolation used by retail terrain.
Every child is emitted between texture-window set/reset packets. Mode 3 first
adds `g_AnimTimer & 0x7f` to the four packed UV pairs, retaining the retail
word/halfword carry behaviour. Ignoring these fields made large scenery quads
sample unrelated texture data and removed the perspective subdivision intended
near the camera.

Course faces also have their own winding convention. The parent accepts a
positive first-triangle `NCLIP` in the main view and a negative result in the
mirror. A subdivided main-view child survives when `clip0 < 0 || clip1 > 0`,
with both signs reversed in the mirror; the second triangle is the FIFO order
`(v3,v1,v2)`. The portable course dispatcher now reproduces these decisions,
the per-record OT bias, fogged `DPCS` colour path, texture windows and scrolling
UVs. This is fixed-width game asset decoding and should be backported to the
decompilation rather than implemented as a PsyZ heuristic.

PsyZ's `FixupFlipUV` now ignores edges whose screen or texture delta on the
tested axis is zero. Such an edge carries no flip direction; treating a
slightly sloped terrain edge with equal V as a flip added one to every V and
sampled the transparent border of the tile. Genuine flipped-XY and flipped-UV
quads retain the compatibility adjustment. `RAGE_GPU_TRACE_AREA=1` logs
draw-area changes and pending batch sizes when diagnosing mirror scissor
ordering.

The Ruby reference runner enables YJIT itself when the installed Ruby exposes
`RubyVM::YJIT.enable`, and prints `ruby_yjit=true` at startup. On the current
Apple Silicon host this reduces a 99-VBlank savestate capture from minutes to
about 12 seconds; a synchronized timer-593 race capture takes about 147
seconds. Keep the saved scene-12 state as the input for repeated packet traces,
which then need only one or two VBlanks.

`g_ShuttleScenery` is two contiguous `0x34`-byte records. The old host-state
definition used only the symbol map's `0x0e`-byte named prefix; writes to the
first record's position and angles therefore landed in separately allocated
globals, starting with `g_SkyRowBase`. This changed the retail value zero to
`0x51ac`, selected the textured-cloud sky branch and corrupted more state. The
host backing allocation now covers both complete records, while
`GameShuttleScenery` has a compile-time `0x34` layout assertion. Backport the
complete object boundary rather than preserving the split placeholder symbol.

The tachometer exposed three more overlapping-layout failures which must be
backported as game-code fixes:

- `g_PlayerTargetRpm` at `0x8009E808` is the interior word
  `g_PlayerCar.drive.engineRpm` (`+0x134`), not independent BSS.  Reading a
  detached host global pins `g_EngineRpm` to its 500-rpm lower clamp.
- `g_TachoNeedleQuad` spans all eight halfwords at `0x8019C7D4..0x8019C7E3`;
  the seven following `D_*` labels are interior coordinates, not separate
  globals.
- `BuildTachoNeedleQuad` must address the named draw-mode packets directly.
  Subtracting two PS1 packets from the `SPRT` pointer assumes the original
  contiguous BSS layout.  A native `DrawPacket` is 32 bytes because of its
  pointer-sized OT link, so retaining 12-byte PS1 backing objects also writes
  past their host allocations.  Allocate the native object sizes and never
  reconstruct cross-global aliases with pointer arithmetic.

`RAGE_PORT_TACHO_TRACE=1` prints RPM, angle, colour, source quad and emitted
vertices.  The race framebuffer regression now counts red needle pixels rather
than accidentally accepting the orange dial numerals.

Car collision had the same class of non-portable reconstruction.  The host
`TransformCollisionVector` was an identity stub even though retail executes a
rotation-matrix `MVMVA`; rival collision hulls therefore remained axis-aligned.
It now calls `ApplyRotMatrix`.  `CollidePlayerWithCars` also cast the address of
its first stack local to a synthetic structure covering every subsequent local.
That encoded the MIPS stack layout into C and reads unrelated storage on a
modern compiler.  Its polygon tests now use the actual named `playerGrid`,
`opponentSamples`, and `opponentCorners` arrays.  These are 32/64-bit game-code
fixes, not HAL behaviour.

The indexed sound-effect table had another split interior symbol:
`g_IndexedEffectBaseVolumes` is `g_IndexedEffects + 8`, not a separate object.
Keeping an eight-byte first allocation made normal indexed access walk past
the host object (and was caught by ASan).  The host now owns the complete three
`IndexedEffect` records as one 36-byte table.

The PS1 treats texture colour `0x0000` as a colour key: the GPU must leave the
destination pixel untouched for both opaque and semi-transparent textured
primitives. Writing RGBA `(0,0,0,0)` is not equivalent when the host blend
state is disabled, because it replaces the destination with black. PsyZ's GL
and SDL_GPU shaders therefore discard these fragments. The PsyZ regression
`opaque_texture_zero_texel_preserves_framebuffer` checks VRAM directly. This
is a HAL fix, not a change to Rage Racer's model or texture data.

When comparing an animated showroom capture, match `g_MenuViewAngle`,
`g_MenuViewOffset`, body/model yaw and menu state together. Matching yaw alone
can still compare the car while it is being eased vertically into view; in the
reference capture screen 4 starts at VBlank 323 and the offset reaches zero
later. Likewise, compare the race intro by `g_SceneTimer`, not by the smoke
loop's `g_FrameCounter`: LIMITLESS smoke mode consumes the required VBlank
units inside a loop iteration. At race timer 25, the native ROUND overlay and
background geometry match the emulator; the old port frame 1420 comparison
was already at race timer 56 and had legitimately faded the overlay away.

For race geometry comparisons, `--region` is also the RMSE domain used by
`rage_visual_batch.rb` during visual refinement, not merely a filter for the
reported hotspots.  A full-frame score can otherwise select a candidate by
the HUD, mirror, or an animated texture phase while the road is several ticks
out of alignment.  Use independent regions for the road (`0,100,240,100`),
mirror (`84,16,152,40`), and tachometer/HUD; do not interpret clear-colour
pixels or triangle-shaped diffs until the candidate was aligned in the same
region.  `report.json` retains both `normalized_rmse` and
`normalized_region_rmse` so this distinction is testable.

The `native_only_black` detector deliberately means true black (`RGB <= 2` in
every channel), not merely a dark texel. A threshold such as `< 12` marks the
course's legitimate `(0,8,0)` barrier stripes as holes whenever even a
one-pixel camera difference shifts their high-contrast pattern. Diagnose
uncovered framebuffer separately with `native_only_clear`; otherwise the
ranking is dominated by false positives rather than missing geometry.

The capture manifest must also include `g_AnimTimer`. Terrain mode 3 scrolls
its packed UV/CLUT words by `g_AnimTimer & 0x7f`; the simulation-state match
therefore requires equal phase modulo 128 and limits camera distance, speed,
body angles and lateral offset. The image selected by `--visual-refine` is a
separate concern: a VBlank manifest samples current globals while its front
buffer may still contain a neighbouring render and animation phase. Refinement
may select that adjacent displayed image, but reports both `timer_delta` for
the validated state and `display_timer_delta` for the chosen framebuffer. At
timer 930, wrongly forcing the image phase to equal the sampled state paired
PSX/native timer 930 and reported 2429 native-only black pixels; the actual
display match is native timer 929, reducing that false ranking to 246.

For the timer-800 race checkpoint used here, delaying the native continuous
CROSS input from smoke frame 1470 to 1471 gives an exact timer-903 state match:
player position, camera position, speed, progress, lateral offset and animation
phase are all identical. Starting LEFT at smoke frame 2164 similarly aligns
the first turn. Use those offsets for cached comparisons; do not weaken the
state tolerances to manufacture more pairs.

### FMV cadence

Do not pace every extracted MP4 at its tagged 15 fps. Retail blocks on frames
arriving from `RAGE.STR`, and MDEC/ring-buffer stalls mean that dividing the
double-speed CD extent by the final frame number is only a theoretical upper
bound. In a stable 493-PAL-VBlank emulator window the intro presents 204 new
frames, or about 20.69 fps; five independent 100-VBlank windows measure
20.62--20.92 fps. The host uses the compact 53/128-frame-per-VBlank cadence
(20.70 fps) plus the measured ten-VBlank pipeline fill.

This was verified from a pre-FMV savestate, not host wall time: the emulator
enters scene 5 at relative VBlank 313 and the port at 314. At emulator frame
850 and port frame 851 both display MP4 frame 218; full-frame RMSE is 0.83%.
The convenience `fmv00.mp4` contains 2000 frames, but frames 1800 onward are a
white tail outside the retail final-frame value, not content that should be
resampled into the movie. Class and ending streams retain their sector-derived
cadence until they receive the same per-stream emulator measurement. Emulator
checks must always compare emulated VBlank numbers, never host wall time.

### Fixed-width arithmetic audit

Run the complete scripted race under UBSan when geometry or physics diverges;
ordinary smoke screenshots do not expose host undefined behaviour. The first
audit found game-code UB in all three relevant paths. `BuildVisibleCells`
left-shifted negative camera-relative coordinates, `BlendAngle` relied on MIPS
32-bit signed multiply/add wrap, and the car-spec initializer deliberately
walked across the declared ends of `torqueLossRpm[9]` and `gearLoad[6]`. Those
last accesses are real packed-asset aliases: loss slot 9 is `gearLoad[0]`, and
load slot 6 is `gearRatio[0]`. Preserve the asset offsets (`0xA8` and `0xCC`)
and make the contiguous access explicit; do not enlarge fields and move every
following member. The collision knockback sign-extension idiom likewise uses
an unsigned shift followed by a signed 16-bit conversion on the host.

PsyZ's GTE had the same portability problem in the shared transform path:
negative translations and sine products were left-shifted as signed C values,
and colour registers were copied through potentially unaligned `unsigned int
*` casts. Multiplication by 4096 and `memcpy` retain the PS1 bit-level result
without depending on compiler or CPU behaviour. These fixes belong in PsyZ;
the packed car-table and wrapping arithmetic fixes belong in the game and
should be backported to the decompilation.
