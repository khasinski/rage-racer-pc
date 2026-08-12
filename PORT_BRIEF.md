# Rage Racer — Port Brief

You are starting a native port of Rage Racer (PS1, PAL `SCES_006.50`) to modern
platforms, beginning with **macOS on Apple Silicon (arm64)**, using
[PSY-Z](https://github.com/Xeeynamo/psyz) as the SDK replacement.

Everything in this brief was measured against the actual tree, not estimated.
Where a number appears, it came from a command you can re-run.

---

## 1. The goal, and the hard constraints

**Immediate goal:** the retail game, running natively, identical in behaviour
to the PS1 original.  That compatibility renderer remains a permanent oracle,
not the architectural ceiling of the port.

**Renderer direction (decision 2026-08-12):** keep two explicit paths.  The
`compat` path retains the original game transforms, clipping, ordering tables,
GP0 packets and PS1 raster behaviour so emulator comparisons remain meaningful.
The later `enhanced` path consumes semantic scene submissions before PS1 screen
projection and is allowed to use a depth buffer, arbitrary resolution/aspect,
independent mirror render targets, extended draw distance, configurable model
LOD, replacement meshes/materials/sprites and modern effects.  PsyZ is a
transitional compatibility backend, not the public API of the enhanced
renderer.

**Hard constraints — do not violate these:**

1. **1:1 first. No improvements, no modifications, no "while I'm here" fixes**
   until the game is 100% playable start to finish. No widescreen, no higher
   internal resolution, no re-textures, no framerate changes, no bug fixes —
   even for bugs you are certain are bugs. Original bugs are in scope to
   reproduce, not to fix.  This constrains the current compatibility milestone,
   not the design of the later opt-in enhanced renderer.
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
then add the signed byte at `stride - 3` to the OT position for all four record
types.  Earlier diagnostics incorrectly labelled byte 29 of GT4 as a non-bias
field while investigating an experimental near-plane clipper.  The DMA/OT
oracle disproves that conclusion directly: at timer 270 six model-0 GT4 faces
have projected depths 62..64 and byte 29 `0xee` (-18); retail links them
exactly 18 buckets earlier.  Model-15 F4 face 5 likewise has depth 67 and byte
13 `0xf0` (-16), and retail selects slot `128 + 67 - 16`.  Apply the bias only
after validating the un-biased depth, just like the terrain path.  The
temporary near-plane triangle clipper turned the close Grand Prix camera shot
into malformed GT3 packets; retail does not take that path for model faces.

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
and color matrices rather than assigning one flat placeholder color.  The two
record types do not use the same GTE operations: the retail GT4 emitter at
`0x80029298` issues `NCT` (`0x4AD80420`) for the first three normals and at
`0x80029304` issues `NCS` (`0x4AC8041E`) for the fourth.  These operations do
not apply the scratch base RGB.  G4 retains the colour-modulating
`NCCT`/`NCCS` path.  Preserve that distinction when backporting the emitter or
GT4 vertices can differ by a lighting unit and produce unstable texture seams.

The apparent global `g_HudLapHighlightRow` at retail address `0x8009E836` is
not independent storage.  It aliases `g_PlayerCar.drive.hudLapHighlightRow`
inside the full player-car object and is initialized to `-1` by game code.
Generating a separate zero-filled host BSS symbol makes lap row zero appear
selected, changing its CLUT from retail `0x78CC` to `0x780F`.  Reads must use
the embedded field (or the host data layout must retain the alias).

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

For raw VRAM diagnosis, set `RAGE_PORT_CAPTURE_DRAW_PAGE=1` and
`RAGE_EMU_CAPTURE_DRAW_PAGE=1`. These make both capture paths read the VRAM
page selected by the current drawing area instead of the presented front
buffer. Do not use that mode as the visual acceptance image at VBlank: after
buffer exchange, draw area already selects the next page, while the completed
OT belongs to the page just presented. Use the default front-buffer captures
for PSX/native image scoring, normally with `--visual-refine 3`; reserve draw
page captures for texture/CLUT inspection while rendering is stopped inside a
known packet phase. Mixing the two produces plausible but false texture and
coverage conclusions.

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

Once a batch bundle identifies a credible pixel, `rage_gp0_bundle.rb` can now
replay both sides with every relevant layer of evidence in one command:

```sh
ruby tools/rage_gp0_bundle.rb --bundle /tmp/compare/frame-pair \
  --pixel 120,141 --texel --ot --resync-window 200
```

The first pass discovers the exact replay frame on both clocks and compares
the GP0 stream; the second pass applies those discovered frame numbers to
packet coverage and actual PSX texel/CLUT tracing.  `--ot` retains DMA-node
ownership and `--resync-window` distinguishes a local insertion from a
divergent suffix.  Outputs are self-contained as `gp0-{psx,native}.log`,
`pixel-{psx,native}.log`, and `gp0-diff.txt` beside the image bundle.  Do not
guess the trace frame from the original capture filename after loading a
savestate: replay frame numbering is relative to the checkpoint.

`rage-gp0-replay` is the lower-level deterministic renderer harness. It loads
a raw 1024x512 RGB5551 VRAM image, consumes the emulator's canonical
`gp0-command` records for one frame while preserving every packet boundary,
and writes PsyZ's 320x240 draw page as PPM. The PsyZ diagnostic API exposes
`Psyz_GpuReplayBegin/Packet/End`, so variable polylines are never reparsed from
an ambiguous flattened word stream. `tools/psx-ruby/bin/rage-state-vram`
extracts the VRAM payload from a validated emulator savestate.

The VRAM snapshot must represent the exact instant before the selected DMA
chain. A generic earlier `replay-pre.psxstate` is useful for bootstrapping
textures and CLUTs, but commands executed between that state and the target
DMA can change either VRAM page; its replay image is therefore diagnostic, not
a pixel oracle. The final capture path must snapshot VRAM at the first traced
GP0 command and pair that snapshot with the same frame's command records.

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

For new deterministic paths prefer state-triggered input.
`RAGE_PORT_STATE_INPUT_SCRIPT` and `RAGE_EMU_STATE_INPUT_SCRIPT` accept
comma-separated `SCENE@TIMER:BUTTON` events, for example
`4@0:START,8@20:CROSS`. Each event arms only after observing the requested
scene (and phase, when supplied) at or below the requested timer, then fires
once when that timer is reached or crossed. This accommodates an emulator
callback that can cross a game tick between observations without allowing a
new scene ID paired briefly with the previous scene's larger timer to fire. The
optional `SCENE@TIMER@PHASE:BUTTON` form also gates asynchronous substates;
phase names `g_FrontendState` for scene 4, `g_MenuScreen` for scene 8 and
`g_PrologueStep` for scene 32. Thus `4@150@2:DOWN` waits for the interactive
main menu, `8@300@4:CROSS` waits for car selection, while `32@169@3:CROSS`
cannot fire while the same scene and timer still belong to its CD-loading
step. This distinction is required for repeatable native/emulator routes.
The
native harness injects `g_PadPressed`; the emulator holds the raw active-low
pad level across two VBlanks so the game's polling cadence observes exactly
one rising edge. This keeps both implementations on the same menu
route even when BIOS, FMV or presentation consumes a different number of
VBlanks. Frame-number scripts remain useful for held race controls and old
cached checkpoints.
For screens that poll less frequently, add an emulator hold duration after the
timer, for example `32@169+8:CROSS`. The default remains two VBlanks; extending
the level does not create another rising edge. Native injects the edge directly
and ignores this emulator-only duration suffix.
When phase and duration are both present, use `32@169@3+8:CROSS`.

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
regions: `road`, `mirror`, `mirror-road`, `mirror-frame`, `rank`, `record`,
`tacho`, `time` and `hud`. `mirror-road` isolates the lower 16 lines of the
mirror so divergent rival positions and upper scenery do not dominate terrain
diagnosis. For example, use `--preset tacho --jobs 8 --top 10` to compare the
cached sequence in parallel and print only the ten worst frames. The complete
machine-readable `summary.json` and all per-frame bundles are still written,
so limiting console output does not discard evidence.

The primary presets are diagnostic profiles, not merely RMSE crops. `road`
enables main-visible-cell equality plus clear/black road detectors and ranks by
clear coverage; `mirror` and `mirror-road` enable the corresponding mirror-mask
gate and detectors; `tacho` enables the red-needle mask/ranking; `hud` enables
the lower-band black detector. Explicit detector/rank options still override a
preset. Stdout always includes raw animation-timer delta and main/mirror mask
equality, so a zero count cannot be mistaken for a detector that never ran or a
pair aligned only in the wrong render pass.

`tools/rage_visual_run.rb` is the one-command repeatable path. It starts the
Ruby/YJIT emulator and native smoke capture in parallel, captures presented
front buffers at all timer phases, writes separate logs, then invokes the selected
diagnostic preset. `run.json` records cwd, argv, environment, timer range and a
shell-escaped reproduction command for all three stages. Use `--dry-run` to
audit the route without starting either game; repeat `--match-arg=VALUE` for
batch tolerances. After the expensive capture, use `--compare-only --output DIR`
without a checkpoint to rerun matching, profiles, bundle generation and budgets
against the existing `psx/capture-manifest.csv` and
`native/capture-manifest.csv`. This makes detector and alignment iteration take
seconds and guarantees that it is operating on the identical captured frames;
the runner fails if either manifest is missing instead of silently starting a
new route. Compare-only metadata goes to `compare-run.json`, preserving the
original capture recipe in `run.json`. Pass `--draw-page` only for raw VRAM/packet diagnosis; those
images are not visual acceptance frames. `--alignment-only` performs the same
captures and state gates but skips ImageMagick, heatmaps and per-frame bundles;
on a cached sequence the manifest scan takes about 0.06 seconds. Use it to find
strictly aligned candidates in long routes before generating detailed bundles.
The runner uses `--visual-refine 3` for presented front buffers because the
VBlank manifest state can describe the newly submitted frame while the display
page still contains the preceding timer. Draw-page captures use refinement 0
so packet traces remain tied to their exact submission phase.
Zero eligible pairs are a successful discovery result: `summary.json` still
contains every rejected row and gate reason, while normal image-comparison mode
continues to fail when it has nothing valid to compare. A 701-phase PSX versus
351-frame native scan found 279 exact-projection/mask pairs, but their scene and
animation timers were consistently seven to nine ticks apart. Large black-mask
scores on those pairs followed shifted barrier texture segments and are not
renderer evidence. Changing the native accelerator start did not remove that
offset; synchronize game-update cadence before using that long cache for UV or
HUD conclusions.

`RAGE_GPU_DIGEST=1` provides the next stage after alignment discovery. Both the
Ruby GPU and PsyZ hash the decoded primitive stream with the same FNV-1a format:
command, normalized PS1 tpage (`& 0x9ff`), CLUT, vertex count, and every
little-endian `x/y/u/v/rgba` field. One `gpu-digest` record per completed frame
contains the total and a command histogram; it does not enable the verbose
per-primitive trace. `RAGE_GPU_DIGEST_GROUPS=1` additionally groups counts by
`code/tpage/clut`. `tools/rage_gpu_digest_compare.rb` joins these records to an
alignment-only summary by the relative `-fNNNNN-` filenames, reporting total,
hash, and primitive-class deltas. The workflow is therefore: capture once with
digest enabled, run alignment-only, compare digests, then enable the full packet
trace only around the first differing class/frame.

The exact-projection timer-868 pair gives a concrete baseline. Every primitive
class except `POLY_FT4` has the same count; retail submits 1,135 FT4 packets and
native 1,257. The largest grouped delta is tpage `0x0005`, CLUT `0x7943`
(`763` retail versus `834` native), followed by smaller tpage `0x001b` road
groups. This proves that the mismatch exists in the submitted/decoded primitive
stream rather than being only a final framebuffer-rasterization artifact. The
Optional digest groups also include the active draw area, separating the full
main pass from the mirror scissor. At timer 868, tpage `0x0005` / CLUT `0x7943`
is `570` retail versus `573` native in the main pass, but `193` versus `261` in
the mirror pass. Thus 68 of the 71 excess packets are specifically mirror-pass
geometry, not a VBlank boundary or a general main-road/rasterization mismatch.
Retail disassembly at `0x80028a9c..0x80028ad4` confirms the portable child
backface expression including its zero boundaries: main accepts when the first
`NCLIP > 0` or the second `< 0`; mirror negates the first result and accepts
when the original first `< 0` or the second `> 0`. Therefore the 68-packet
mirror excess is not evidence for reversing that condition. Compare the child
`NCLIP` distribution near zero under the remaining three-unit camera
translation before changing culling.

For example:

```sh
ruby tools/rage_visual_run.rb \
  --checkpoint /tmp/rage-cont-700/sync-00199-t00700-s12.psxstate \
  --output /tmp/rage-road-866 --profile road \
  --timer-min 866 --timer-max 870 --psx-frames 342 --native-frames 2240 \
  --psx-input '0-500:CROSS' \
  --native-input '400:START,500:START,650:CROSS,950:CROSS,1100:CROSS,1200:CROSS,1469-2240:CROSS' \
  --match-arg=--max-position-distance --match-arg=8 \
  --match-arg=--max-view-distance --match-arg=8 \
  --match-arg=--max-projection-delta --match-arg=16 --top 5
```

The end-to-end regression route reproduces timer 868 with equal animation and
main/mirror visible-cell hashes, position/view distance 3, zero native-only
clear pixels and ten isolated native-only black pixels.
Use `--profile all` to reuse one expensive capture for `road`, `mirror-road`,
`tacho`, and `hud`. Each profile gets its own comparison directory, while the
top-level `summary.json` records maxima and the complete state delta of the
frame producing each maximum. A large raw mirror/needle count is therefore
always accompanied by translation, RPM, animation, and mask evidence needed to
decide whether it is a renderer regression.

The runner can also turn those measurements into an explicit regression gate.
Repeat `--budget PROFILE.METRIC=VALUE`; supported maximum metrics are `clear`,
`black`, `needle`, and `rmse`, while `matched_min` is a minimum.  The budgets
are recorded in `run.json`, their results are written under `validation` in the
top-level summary, and a failed check exits nonzero with the offending worst
frame.  Thresholds deliberately belong to a checkpoint and diagnostic route,
not to a hidden universal tolerance.  For the synchronized timer-868 route,
the established non-loss assertions can be expressed as:

```sh
  --profile all \
  --budget road.clear=0 --budget hud.black=0 \
  --budget road.matched_min=1
```

Do not gate the raw mirror or needle maximum until rival pose/translation or
RPM is equally aligned; those profiles retain the relevant state on their
worst-frame records precisely so phase drift is not mislabeled as rendering
loss.

State-relative taps can be passed as matching `--psx-state-input` and
`--native-state-input` strings. Do not yet use scene-timer durations to drive a
long route: the checkpointed PS1 observes roughly two VBlanks/car updates per
race timer while native smoke normally advances one update per loop. Equal
timer durations therefore do not mean equal numbers of game updates; this is
part of the outstanding native cadence bug, not an input-script tolerance.

The black detector follows the same area rule as the clear-colour detector.
`raw_count` retains every isolated native-only black pixel for raster-edge
diagnosis, while `count` requires both a horizontal and vertical neighbour and
therefore represents the interior of a real two-dimensional hole.  In the
synchronized timer-868 road pair the old value of ten is `raw_count=10` but
the regression value is `count=0`; none of those samples form a black triangle.

For geometry below the framebuffer level, the Ruby emulator accepts
`RAGE_GTE_TRACE_PC`, `RAGE_GTE_TRACE_TIMER`, `RAGE_GTE_TRACE_OPCODES`, and
`RAGE_GTE_TRACE_LIMIT`.  Each selected command records all 32 data and control
registers before and after execution.  `tools/rage_gte_replay.c` restores the
captured input into PsyZ and executes the same COP2 command, providing a direct
runtime comparison of SXY/SZ/OTZ/MAC/FLAG rather than inferring GTE behaviour
from pixels.  The emulator uses the PS1 UNR reciprocal table/refinement instead
of ordinary integer division: on 5,000 real timer-868 terrain commands this
removed every SXY/SZ/OTZ/MAC difference.  Sixteen remaining differences are
only FLAG bit 15 (MAC0 negative overflow); Rage's terrain decision consumes
bit 31, whose relevant seam-line decision remains equal.  This evidence rules
out transform, projection, NCLIP, and AVSZ data as the source of a large hole
in that checkpoint, but does not generalize to a route which has not been
captured.

Race captures include the X/Z positions of all four active rivals as well as
the player state. New manifests also retain each rival's speed, progress, body
yaw, lateral offset, collision flag and active flag. Position matching adds
their aggregate distance to its score
and records it in each report. This matters especially for the narrow mirror:
two frames can have an identical player pose while a rival differs by tens of
world units, shifting its silhouette by a pixel and dominating mirror RMSE.
The manifest may also contain opaque diagnostic columns such as hex-encoded
raw car structures. `rage_visual_batch.rb` converts only its documented scalar
matching fields to integers and leaves unknown fields untouched, so adding a
capture diagnostic cannot break the visual-comparison pipeline.

The interactive-race car update had the same portable-C defect previously
fixed in the attract-car path: it assigned only X/Z in a stack `Vec4` and then
copied all four words into the car. Retail's MIPS stack happened to contribute
zero for `positionW`, whereas the 64-bit host reproducibly contributed one.
`UpdateRaceCars` now preserves the existing Y/W and changes only X/Z. This is
a game-code 32/64-bit compatibility fix to backport to the decompilation, not
a HAL workaround; `positionW` is subsequently read by camera/render code.
Regenerate old cached manifests when diagnosing cars in the mirror; the batch
tool remains backward compatible with caches that lack these columns.
Use `--max-rival-distance N` for mirror investigations to reject pairs whose
aggregate four-car X/Z displacement is too large; the selected distance is
printed beside player and camera alignment. A value around 180 retains enough
samples in the current 837..987 dense run while excluding the most misleading
car-silhouette comparisons.

One game timer can span several VBlanks, and rival updates need not be visible
in the first VBlank carrying that timer. `RAGE_EMU_CAPTURE_ALL_PHASES=1` and
`RAGE_PORT_SMOKE_CAPTURE_ALL_PHASES=1` preserve every occurrence as
`timer-T-fFRAME-sSCENE.ppm` instead of deduplicating by timer. This is required
for collision and mirror-car diagnosis: at timers 854..857, comparing only the
first PSX occurrence reports about 372 aggregate rival-position units, while
the updated occurrence of the same timer reduces it to about 149. Neither path
sets a collision flag; the larger jump is capture phase, not game physics.
In `--match timer` mode, `rage_visual_batch.rb` groups these names by scene and
game timer, then selects the lowest-RMSE PSX/native VBlank combination inside
the requested comparison region. Relative host frame numbers intentionally do
not need to match.

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

New captures additionally fingerprint the renderer state that directly controls
projection: the nine signed 12-bit-fixed rotation-matrix entries, viewport
`x0/y0/x1/y1`, and the main/mirror ordering phase. On retail these live at
scratchpad offsets `0x28`, `0x78..0x7e`, and `0x68`; the host capture reads the
named `SCRATCHPAD` members instead. Do not share raw structure offsets between
the two builds because native pointers change the layout on 64-bit hosts.
`rage_visual_batch.rb --max-projection-delta N` rejects candidates whose
aggregate matrix/viewport difference exceeds `N`, and always requires the same
ordering phase when both manifests provide it. Old manifests remain usable.

Candidate limits are applied before choosing the lowest-score state. Applying
them only after `min_by` allowed an ineligible frame with an attractive
projection score to hide another candidate that satisfied every requested
limit. In the synchronized timer-880 sample, the corrected matcher finds a
state with `projection_delta=0`, position distance 9, and equal animation phase;
the remaining 50-unit scratch-view difference is a camera-update phase and the
best displayed native image is still two timer phases earlier. Keep simulation
state, projection state, and front-buffer image phase separate when deciding
whether a road patch is genuinely missing.
When no pair passes, the batch tool prints up to five nearest PSX/native rows
and every failed gate (`position`, `view`, `speed`, `angle`, `lateral`, rivals,
projection, tachometer RPM, projection phase, animation phase, or RNG). Use
this report to tune the deterministic input route; do not widen unrelated
limits until a misleading image happens to pass. In the current acceleration
offset scan, native `CROSS` starts 1469/1471 produce a best aggregate state
error of 34 (mostly three-unit position/view/speed/lateral differences), while
1468/1470 score 56 with eight-to-ten-unit differences. This two-frame parity
effect is simulation/input cadence, not visual rendering.
For that exact-projection road pair, both area-filtered detectors report zero
native-only clear and zero native-only black pixels. This disproves the earlier
missing-road diagnosis for that sample only; it is not evidence that every
in-race hole is fixed. Expand the synchronized capture window and require the
projection gate before classifying further candidates as culling or draw-range
failures.

Mirror comparisons additionally record all nine entries of
`g_MirrorViewMatrix` (retail `0x8019cb18`) and accept
`--max-mirror-projection-delta N`. The restored scratch matrix normally
describes the main pass, so its equality alone cannot validate rear-view
geometry. In the strict timer-868 draw-page pair, both main and mirror matrices
are exactly equal; player/view translation differs by three world units and
speed by four. Retail writes mirror pixel `(220,42)` from a thin `tpage 0x05`,
`CLUT 0x7943` quad, while native submits the corresponding strip one to two
screen pixels higher/lower and leaves the sample clear. Shifting native CROSS
to frame 1469/1471 and emulator CROSS to relative VBlank 0/1 is the closest
reachable input cadence; the next parity jumps by 17 player-X and 62 view-X
units. Therefore the 118-pixel mirror-road clear mask in this pair is explained
by unresolved sub-tick camera translation, not evidence for changing PsyZ or
Rage Racer culling. Require both the mirror-matrix gate and tighter translation
before treating such a thin-strip displacement as a renderer defect.

The same strict timer-868 pair is useful for the remaining HUD/road audit.
Across the full lower HUD band native has zero native-only black pixels. The
largest main-road black sample at `(120,140)` is also not a missing primitive:
both builds submit matching `tpage 0x1b`, CLUT `0x7c51/0x7c45` quads, with a
one-pixel vertex displacement from the residual translation; native also
submits a `tpage 0x15`, CLUT `0x7c03` strip over that coordinate. Do not classify
that isolated black texel as a triangle hole.

At timer 868 the sampled tachometer inputs are 6698 RPM on retail and 6733 on
native. Native trace maps these nearby values to needle angles around 866 and
879, enough to move the narrow tip by a pixel. Comparing the PSX needle against
the adjacent native 6692-RPM draw page reduces mismatch from 41 pixels (IoU
0.880) to three pixels (IoU 0.991). Thus the current needle geometry is correct;
HUD comparisons must select the displayed/draw phase by `tacho_rpm`, not just
car speed or scene timer.

Use display-page captures for user-visible acceptance and draw-page captures
for packet-level diagnosis. A timer-841 display-page pair appeared to contain a
four-pixel clear hole at `(115..116,138..139)`: retail submitted two textured
quads there, while the visually refined native image submitted neither. The
same timer compared on the active draw page has zero native-only clear pixels,
proving that candidate was a cross-buffer phase mismatch. Conversely, the
draw-page timer-880 candidate at `(23..26,128..130)` is not yet a PsyZ
rasterization case: retail submits two quads using CLUTs `0x7c49/0x7c4b`, while
native submits a differently projected quad using `0x7c03`. That divergence is
already present in Rage Racer's submitted geometry/visible-cell selection and
must be aligned or traced before changing the HAL rasterizer.

Do not gate the visually refined image by its own manifest row. The manifest
samples current globals at VBlank while the display-page image can be an older
buffer; doing so forced `display_timer_delta=0` and increased a known road case
from zero to 57 native-only clear pixels. Instead use draw-page captures with
`--visual-refine 0` whenever the manifest and submitted packets must describe
the same render. A previously documented exact timer-903 route is also not a
permanent golden state: after later fixes the regenerated runs differed by
position `(29,-9)`, speed 42, and body roll 5. Always prove exactness from the
current manifests rather than relying on old frame offsets.
For scenery-heavy regions such as the full mirror, add
`--require-random-seed`. Position equality is not enough there: animated
track objects consume `Random15`, so unequal seeds can produce different
object quads even when player, camera, rivals and timer all match. The option
rejects such pairs instead of ranking a simulation divergence as a renderer
error. Road, static HUD and mirror-road comparisons can remain useful without
this gate, but their reports still expose `random_seed_equal`.
Collision frames are the exception: require the RNG gate even for
`mirror-road`. Player contact calls `StartCarBodyKick(2)`, which selects the
sign of the body kick from `Random15() & 0x80`; the chase camera copies that
body roll into `SCRATCH_VIEW_ANGLE_Z`, and `SetCameraRotMatrix` consequently
changes the mirror matrix. In the synchronized timer-889 sample the player
positions differ by only `(3,-2)`, but unequal inherited seeds produce roll
`-9` versus `-10` and mirror-matrix entries `-56` versus `-62`. That is
simulation/RNG divergence, not evidence of a mirror viewport or shader defect.
Use a pre-contact frame with matching roll, or require equal RNG state.
To find the first cause of an RNG divergence, enable
`RAGE_PORT_RANDOM_TRACE=1` on native and `RAGE_EMU_RANDOM_TRACE=1` on the Ruby
runner. Both log the call index, frame, resulting seed/value and caller
location (`caller_delta` from `Random15` natively, retail return PC in the
emulator). Start both from equivalent checkpoints and compare the first
unequal call rather than inferring a cause from a later manifest seed.
`ruby tools/rage_random_align.rb PSX_LOG NATIVE_LOG` aligns the logs by their
32-bit LCG states and reports the dominant call-index offset. A constant offset
means the two runs inherited a different number of earlier calls (commonly a
different frontend route); it is not evidence that `Random15` itself or the
race renderer diverged. A changing offset identifies the interval containing
the first extra/missing runtime call.

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

For larger dark surface losses, pass the comparison rectangle as `--region`
and use `--rank surface`.  The detector requires a large RGB error, a native
luminance deficit of at least 48, a native colour absent from the nearby PSX
reference, and two-axis interior support.  This is still a discovery rank,
not proof of a missing primitive: repeating textures and near-plane projection
can create a large coherent component even when both renderers submit the same
face.  `--surface-error N` controls the RGB-distance threshold (default 96).

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
`RAGE_GPU_TRACE_TEXEL=1` now also emits `gpu-raster` records from the Ruby
emulator's actual polygon and sprite inner loops. These include the final
windowed UV, palette result and whether the texel wrote VRAM. Use these rather
than the geometric `gpu-cover` estimate when a narrow mirror polygon lies on a
scanline edge; the coverage helper and the reference scanline rasterizer can
legitimately disagree at such an edge.
The runner assigns its relative trace frame inside the emulator at the exact
VBlank transition. Do not set the trace frame only before a large host-side
`run(steps:)` batch: one batch may cross a VBlank and would then label packets
from two phases with the previous frame number. Capture filenames, manifests,
and `gpu-raster frame=N` now share the same VBlank index.

At native resolution, PS1-compatible hardware renderers place integer polygon
vertices at pixel centres. Texture coordinates come from a fixed-point DDA and
the sampled coordinate is its integer part: it must be truncated, not rounded
to nearest. PsyZ previously used `roundEven(rawUV)`, which coherently selected
an adjacent palette index across large triangles and frequently landed on
index zero, producing black wedges. The SDL_GPU and GL shaders now use
`floor(rawUV)`. On the exact timer-896 state this reduced road RMSE from
0.070685 to 0.033125 and native-only black pixels from 29 to zero; timer-894
road RMSE fell from 0.081101 to 0.019267. The game submitted identical packets
and vertices in both runs, so this is a HAL rasterization correction and does
not require a Rage Racer game-code backport.

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

For sequence comparison, pass `--needle-region 250,160,45,42 --rank needle`.
New manifests record `tacho_rpm = g_EngineRpm + g_EngineRpmJitter`, the exact
value passed to `DrawTachometer`; use `--max-tacho-rpm-delta N` so similar car
speed does not masquerade as an equal needle state. An explicitly requested
RPM gate now rejects either manifest when that column is absent and reports
`missing_tacho_rpm`; it must never treat an unavailable diagnostic as a
zero-delta match. Regenerate older caches before using them as tachometer
geometry evidence. When `--needle-region` is
present, visual refinement now minimizes the binary red-needle silhouette
rather than full-region RMSE. The dial, speed digits, and gear glyph otherwise
outweigh the thin needle and can select the wrong front buffer. In the
timer-841 reference this changes the selected native display phase from timer
839 to 845 and reduces the needle mismatch from 92 pixels (IoU 0.766) to five
pixels (IoU 0.986). The needle is therefore present and geometrically correct
in this sample; its prior apparent failure was capture-phase selection.
The report extracts the same strongly red silhouette in both images and records
pixel counts, mismatch count and intersection-over-union independently of the
transparent dial background. In the synchronized timer-837..987 run the needle
IoU is normally `0.95..0.98`; timer 895 is `0.981`. Packet tracing at that state
also finds the retail `POLY_F4` colour `c0,18,00` and the same four vertices in
native, proving that remaining full-tachometer RMSE is not a missing needle.

Position matching can use `--skip-unmatched` for captures where the emulator
records more display/VBlank phases than native. It only skips manifest rows
which have no eligible scene/lap state or fail the normal state gates; it does
not relax any accepted pair. The JSON summary retains every rejection and its
reason. Pair it with a `matched_min` visual budget so a mostly unmatched run
cannot pass silently.

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

`native_only_clear` reports both `raw_count` and an area-filtered `count`.
The filtered count requires a candidate pixel to belong to horizontal and
vertical runs. A one-pixel diagonal contour is the normal result of a small
camera/front-buffer displacement and must not rank as a missing triangle; a
real triangular hole retains a two-dimensional interior and survives this
filter. Use the raw count only when deliberately examining raster-edge rules.

The synchronized pre-contact timer-882 candidate illustrates the required
packet-level check. Native showed a 5x3 clear-colour patch at `(10..14,130..132)`,
but both renderers submitted a GT4 covering pixel `(12,131)`. Retail actually
wrote palette index `0xf`, colour `0x8441`, from windowed UV `(75,104)`; native's
corresponding packet reached `(71,104)`. Thus this candidate was texture/projection
phase drift, not culling, draw-distance loss, or an absent terrain packet.

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

`--max-anim-timer-delta N` exposes a wrapped 7-bit animation tolerance; its
default is zero. A small nonzero value is appropriate only for geometry/clear
coverage, because UV animation cannot create an uncovered framebuffer pixel.
Keep it at zero for black-texel and texture-content diagnosis. On the corrected
turn route, allowing three ticks yields eleven tightly aligned draw-page pairs
and every one reports zero area-filtered native-only clear pixels, including
timer 997/994 with position distance 3.2, equal speed, lateral delta 1, and an
identical projection matrix. The 285..332 black-pixel counts in other pairs
coincide with two-to-three-tick texture-animation differences and are not valid
missing-triangle evidence.

Input scripts for an emulator checkpoint use relative VBlank frames, not game
timer ticks. From the timer-700 checkpoint the emulator observes roughly two
VBlanks per race timer. Matching native LEFT at smoke frame 2164 therefore
requires emulator LEFT at relative VBlank 200, not 268. The incorrect 268 start
already differed at timer 900 by about 1278 position units and 429 yaw units;
the corrected start reduces the closest early-turn differences to single-digit
translation and tens of fixed-angle units. Always derive these offsets from
manifested scene/timer transitions rather than subtracting timer numbers from
host frame numbers.

Capture manifests also hash the complete 32-word (`1024`-bit) main and mirror
visible-cell masks using bytewise FNV-1a. Retail reads the fixed arrays at
`0x8019c86c` and `0x8019c7e4`; native hashes the named globals. Normalize hashes
to unsigned 32-bit before comparison because the host CSV can print a value as
signed while retail prints the identical bit pattern unsigned. Use
`--require-main-visible-cells` for road/scenery and
`--require-mirror-visible-cells` for rear-view geometry; the older
`--require-visible-cells` requires both and is intentionally stricter.

On the corrected turn, five close pairs with identical main-pass masks all
have zero area-filtered native-only clear pixels. One close pair also has an
identical mirror-pass mask and zero mirror-road clear pixels. This verifies that
the original visibility code selects the same terrain cells and PsyZ preserves
their coverage in those samples. Earlier 118-pixel mirror clear regions occurred
when the mirror masks themselves differed after a few world units of camera
translation; they are not evidence of post-submit triangle loss or a shorter
native draw distance.

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

### Terrain subdivision and NCLIP oracle

Do not infer a terrain culling fix from the number of black pixels.  At the
exact timer-868 state pair, semantic GPU digests differ only in textured quads:
the main `tpage=0x05, clut=0x7943` pass is 570 retail versus 573 native, while
the mirror is 193 versus 261.  The excess therefore exists in submitted game
packets before PsyZ rasterization.

Retail GTE traces at `0x80028a88` and `0x80028ac0` also prove the portable
two-triangle winding test is algebraically correct.  Main terrain accepts the
first positive NCLIP or the second negative one; mirror terrain accepts the
first negative NCLIP or the second positive one.  Zero-area child triangles
are common in retail and are not by themselves a host bug.

An unfiltered child trace at timer 868 narrows the systematic discrepancy to
subdivision before child culling.  Retail projects 1284 main and 700 mirror
children; native creates 1314 main and 1208 mirror children.  The matching
visible-cell masks and near-matching main count rule out draw distance and the
cell builder.  Audit the parent OTZ/fog result and the `stream[22/23] - OTZ`
LOD calculation against the retail dispatcher before changing NCLIP signs or
adding rasterizer workarounds: each one-bit LOD error multiplies the number of
children.

Use `RAGE_PORT_TERRAIN_TRACE_TIMER` on native and
`RAGE_TERRAIN_LOD_TRACE=1 RAGE_TERRAIN_LOD_TIMER=N` in the Ruby emulator to
record the parent `OTZ`, record bytes 22/23, reduced levels and final step
counts at the retail decision point (`0x800284a4`).  Capture from a state saved
before `SubmitTerrainCells`: a state made inside the subdivision emitter can
resume after the parent decision and silently produce no LOD records.  GTE
trace limits can likewise truncate before the mirror parents, so compare
complete per-pass counts rather than treating a prefix as a distribution.
The emulator probe covers all four equivalent post-LOD PCs (`0x800284a4`,
`0x80028554`, `0x80028600`, and `0x80028758`); tracing only the first silently
misses the other terrain dispatch modes.

Visible-cell mask equality is necessary but not sufficient for LOD diagnosis.
The mask records which cells were selected, while each 16-byte list entry also
carries its camera-relative translation, which feeds GTE OTZ.  Capture
manifests therefore hash the complete 1024-byte main and mirror lists as
`main_visible_list_hash` and `mirror_visible_list_hash`.  The existing
`--require-main-visible-cells` / `--require-mirror-visible-cells` gates require
mask equality only, which is appropriate for aligning images whose camera
translation may differ by one unit.  Use the separate
`--require-main-visible-list` / `--require-mirror-visible-list` gates for a
bit-exact LOD oracle that also requires equal per-cell translations.  Mixing
these contracts can reject visually comparable states merely because their
active list hashes retain the legitimate one-unit camera delta.

Canonicalize inactive visible-list entries before hashing: `BuildVisibleCells`
sets only `w = -1` on rejection and deliberately leaves the old x/y/z words in
place.  Hashing those stale words compares unused history, not renderer input.
The canonical hash substitutes `(0,0,0,-1)` while retaining every active entry
and its order.

The mirror LOD bug was a native scratchpad aliasing regression.  In retail,
`BeginMirrorPass` stores mode 9 at scratch offset `0x6c`, and the terrain
dispatcher reads that same word as the variable shift in
`record[22/23] - (OTZ >> shift)`.  `EndMirrorPass` restores 10 at the same
address.  The typed host scratch representation had separate `mode` and
`faceOtShift` members, so only mode changed and mirror terrain incorrectly kept
shift 10.  Explicitly setting `faceOtShift` to 9/10 reproduces the original
alias without reintroducing overlapping native fields.

At timer 866, before the fix native mirror parents had only 100 unsplit 1x1
faces versus retail's 136 and promoted many faces into 1x2, 2x2 and deeper
subdivision.  After the fix every subdivided step class matches retail exactly;
native has 131 unsplit faces, with the remaining five-parent difference
explained by the approximately 11-unit state offset.  The native semantic GPU
digest fell from the earlier 1346-primitives class to 1263 in the nearby
fixed capture, removing 83 spurious packets.  This is a game/decompilation
scratch-layout fix, not a PsyZ renderer heuristic.

The deterministic `race_start` regression now enables the terrain LOD trace at
timer 562 and requires both contracts in one rendered frame: main entries use
shift 10 and mirror entries use shift 9.  The earlier Grand Prix timer-56
frame predates the active rear-view pass and can only assert the main shift.
That same race regression already verifies nonzero speed glyphs, a
nondegenerate red needle, bounded clear-colour road wedges, and textured mirror
road, so it is the fast host gate for this group of symptoms.

After the mirror-shift fix, reuse the 701-frame retail cache covering race
timers 700..1050 and regenerate only the 351-frame native side.  With position
distance at most 12, equal animation phase, projection delta at most 32 and
one-frame display refinement, sixteen state pairs survive.  Every mirror pair
has zero native-only clear and zero native-only interior-black pixels.  Fifteen
of sixteen main-road pairs also have zero clear pixels; timer 880 has 21 pixels
forming a one-pixel road/barrier seam, not a filled triangular hole, and all
main pairs have at most one interior-black pixel.  This longer route is the
coverage evidence for the fix; the semantic digest/LOD trace explains why it
works, while a single framebuffer does not.

Missing-geometry discovery must rank connected area, not a raw count of black
pixels.  `rage_visual_compare.rb` reports the largest four-connected component
after its two-axis interior filter, including its bounding box, and the batch
tool's `--rank black` sorts by that component before total pixel count.  This
keeps a real triangular hole ahead of scattered legal black texels and
one-pixel phase contours.  The road preset stops at scanline 199: lines
200..204 contain the lower HUD/world boundary and previously produced a long
false-positive component unrelated to terrain.

Treat even a large black component as discovery evidence, not proof.  In the
timer-1670 long-route capture, a 276-pixel component was the legal black half
of the yellow/black barrier shifted by a one-unit camera delta.  Pixel tracing
showed the same textured packet, tpage `0x1a`, CLUT `0x7c80`, texture window,
UV neighbourhood and VRAM contents on both sides.  Exposed dark-blue clear
colour remains the stronger missing-coverage signal.  Run summaries therefore
retain maxima and frame identities for both clear and black connected
components so automated scans can prioritize clear coverage failures and use
black results only for packet-level follow-up.

Reject fully black 320x240 emulator readbacks before state pairing.  The long
timer-1600..2400 capture contained transient blank reference frames at timers
2070 and 2280; accepting them produced impossible tachometer IoU zero and the
two worst mirror RMSE results even though neither was a rendered retail frame.
`rage_visual_batch.rb` records these rows as `blank_reference`, so every visual
profile shares the same guard instead of rediscovering the capture failure.

The red-pixel tachometer mask is a discovery metric, not a geometry oracle: the
needle can connect to red dial markings and digits.  At synchronized timer
1900 both sides record 6208 RPM.  Retail packet trace and native
`RAGE_PORT_TACHO_TRACE` independently produce the identical untextured
`POLY_F4`, colour `c0,18,00`, with vertices
`257,194 / 290,179 / 259,198 / 291,181`.  Any residual silhouette difference
for that state is a PsyZ polygon raster-edge question, not missing game state
or a malformed tachometer quad.  Prefer exact packet evidence when the image
mask overlaps the surrounding dial.

Do not compare that packet trace directly with the display-page screenshot.
Rage Racer double-buffers VRAM: the trace describes primitives submitted to
the current draw page, while the normal capture reads the page selected by the
display environment.  At timer 1900 this phase mismatch made `(270,190)` look
like a rasterizer failure even though it merely compared the newly submitted
needle with the preceding displayed frame.  Use draw-page captures for packet
coverage and display-page captures for user-visible regressions, and record
which page supplied every oracle.  A standalone PsyZ polygon test must be
derived from a controlled PS1 GPU capture, not from this mixed-page pair.
Capture manifests label this explicitly as `capture_surface=draw|display`.
The batch comparator rejects mixed surfaces before state matching, so a packet
diagnostic cannot silently enter a user-visible framebuffer regression.
Keep the native CSV format signed for projection and mirror-matrix elements;
only the four FNV hashes and random seed are unsigned.  A misplaced `%u`
turned negative mirror coefficients into values near `2^32`, defeating the
mirror-projection alignment gate even when both matrices differed by one unit.
The two capture strides have different clocks: the Ruby argument samples
emulated VBlanks, while `RAGE_PORT_SMOKE_CAPTURE_TIMER_STRIDE` samples the game
timer.  `rage_visual_run.rb` therefore exposes `--psx-capture-stride` and
`--native-capture-stride` separately.  For sparse route scans use PSX stride 1
and a larger native timer stride; giving both the same value can accidentally
leave only a handful of timer intersections.

A dense draw-page scan over race timers 1600..1800 uses PSX VBlank stride 1,
native timer stride 5, equal main/mirror visible-cell masks, projection deltas
at most 64 and camera/view distance at most 12.  Forty timer groups survive.
Every road and mirror-road pair has zero native-only clear pixels and zero
connected native-only black area.  HUD pairs likewise have no native-only
black component, and the speed digits plus tachometer needle are present on
the native draw page.  Residual RMSE hotspots follow moving rivals, road-edge
phase and one-unit camera differences.  This interval is useful as a negative
control for culling: do not manufacture a terrain or HUD fix from it; search a
different route interval when looking for a real missing-primitive oracle.

Animated course scenery is another state gate, not renderer output. A dense
timer-200..800 scan initially ranked timer 410 with a 504-pixel connected black
road component despite equal player/camera/projection state and visible-cell
hashes. Pixel tracing showed identical screen geometry but PSX used course
model 7 (`V=96..127`) while native used model 9 (`V=160..191`). Both runs had
the same course/class/mode, texture-page state, course-object count and byte
hash. The actual difference was `g_AnimSceneryVariant`: retail RNG had selected
0 and native RNG 2, and `DrawAnimatedScenery` deliberately submits
`variant + 7`. Capture manifests therefore include both animated-scenery
variants, course selectors, texture-page state and the course-object hash. The
normal race matcher gates `g_AnimSceneryVariant`, which selects this path's
models; `g_AnimScenery2Variant` is retained for replay/attract diagnosis but is
not a normal-race gate. Do not diagnose
UV, CLUT, clipping or missing faces from a pair that fails this gate.  The
batch summary includes aggregate `rejection_counts`, and a zero-pair error
prints them in descending frequency. Inspect these before relaxing any
tolerance: dominant scenery-variant, rival, or visible-list rejection counts
mean the runs are different simulation states, not that the renderer lost a
face.

Use `--save-psx-states` for discovery scans.  It enables the emulator's
per-capture save states, and the batch comparator copies the matching file to
each result bundle as `reference.psxstate` while recording its path in
`summary.json`.  A ranked hotspot can therefore go straight into pixel,
packet, GTE or VRAM tracing without replaying the full route and hoping to hit
the same VBlank phase again.

`reference.psxstate` is the post-capture state and is authoritative for the
captured VRAM contents, but it is too late to replay the packets that produced
that page.  When earlier per-frame states exist, the comparator also copies
the nearest one as `replay-pre.psxstate` and records `psx_replay_frames`.
Start packet/GTE tracing from that pre-state and advance exactly the recorded
number of VBlanks; starting from `reference.psxstate` traces the following
buffer and can falsely attribute a full-screen clear to the ranked image.

GPU trace lines include the active draw area.  This matters because Rage
normalizes the two VRAM pages into screen coordinates for diagnostics: a quad
whose normalized vertices cover a road pixel may actually be clipped to the
rear-view rectangle and cannot overwrite it.  At canonical draw-page timer
868 the clear detector found a 3x2 component at `(63..65,138..139)`.  Both
retail and native submit the two textured road quads at that location; retail
vertices are `68,128 / 67,149 / 65,128 / 62,149`, while native's three-unit
camera delta produces `67,128 / 66,148 / 64,128 / 60,148`.  The flagged pixels
sit on that shifted edge.  The later apparent full-screen clear has draw area
`86,18..233,53` and is the mirror pass.  Therefore this component is an
alignment seam, not a missing primitive or evidence for changing culling.

The largest `--rank surface` result in the canonical timer-840..900 scan is
timer 890.  Packet tracing identifies native terrain cell 162, face 50,
subdivision child 2,2/4,8 at OT depth 127.  Retail and native use the same
texture page 15 and CLUT 7c03, while their projected vertices are respectively
`121,98 / 120,165 / -52,82 / -54,203` and
`111,97 / 110,166 / -68,82 / -70,203`.  The projection matrices match, but
the aligned captures retain a roughly 2.8-unit camera-position delta and the
face crosses the near-camera projection boundary, amplifying that small state
difference into a 10--16-pixel screen shift.  This is a useful near-plane
alignment stress case, not evidence of a missing child, wrong texture lookup,
or a PsyZ rasterizer stretching an identical packet.

For a decision-level terrain comparison, enable
`RAGE_TERRAIN_DECISION_TRACE=1` plus `RAGE_TERRAIN_DECISION_TIMER=N` in the
Ruby emulator and the corresponding `RAGE_PORT_TERRAIN_DECISION_TRACE=1` /
`RAGE_PORT_TERRAIN_DECISION_TIMER=N` variables on native.  Both logs contain
the ordered texture page, CLUT, main/mirror pass, two NCLIP results, OTZ/depth,
and final submit/reject decision.  Compare them with
`tools/rage_geometry_trace_compare.rb`; optional clip/depth tolerances apply
only to numeric drift and never hide a different texture, pass, or final
decision.  The emulator samples NCLIP immediately after the COP2 command,
before the R3000A delayed MFC2 result reaches a CPU register.
Normalize the diagnostic PC to its physical `0x0002xxxx` address before the
range check, because a restored state may execute through either cached or
uncached aliases.  Mixing a masked PC with `0x8002xxxx` bounds silently
produces `na` NCLIP fields even though emulation itself remains correct.
Install the hook in the inlined COP2 fast path as well as `execute_cop2`:
normal game execution under YJIT uses the former, so a probe placed only in
the cold method never observes terrain commands.

At aligned timer 890 the first 500 records have identical texture/CLUT order;
without comparing NCLIP, all 500 semantic decisions initially appeared equal.
The full trace exposes four submit/reject differences, but this capture retains
the documented roughly 2.8-unit camera delta and is therefore a sensitivity
case rather than fix evidence.  Re-run the decision oracle at an exact state
match (the timer-903 route is known to provide one) before changing clipping.
This prevents both image alignment and trace instrumentation errors from being
misdiagnosed as a game renderer bug.

With corrected raw NCLIP capture, the first semantic difference in that
non-exact timer-890 pair is record 81 (`tpage=1a`, `clut=7b10`).  Retail has
NCLIP `190,-159` and projected X coordinates `350,321,354,324`, all beyond its
right bound 320, so it rejects the parent as off-screen.  Native's displaced
camera produces NCLIP `188,-188` and submits it.  This difference adds native
geometry rather than making a hole and is fully explained by the screen-edge
state delta; it is not evidence for weakening or removing the bounds test.

A separate retail timer-903 GTE capture containing 5,000 consecutive terrain
RTPT/NCLIP/RTPS/AVSZ commands replays through PsyZ with zero mismatches.  This
rules out the HAL GTE transform and clipping arithmetic for that captured
route.  Continue above that boundary in the portable asset decoder, parent LOD
and packet emitter, while keeping the GTE replay as the fast bit-exact gate.

A freshly regenerated draw-page cache over race timers 200..260 provides 40
state-aligned pairs with exact player position/speed, animation, visible-cell
masks and many exact camera/projection phases.  Across those pairs the road has
zero native-only clear pixels; the mirror-road region has zero native-only
black pixels and no black component.  The largest road black component is only
six pixels, a 2x3 block at timer 250 around `(142..143,127..129)`, and the
surface detector rejects it as a missing area.

Pixel tracing at `(142,128)` proves both sides submit the same final textured
quads, including identical XY, UV, tpage 1c and CLUT 7bc0.  Retail's scanline
rasterizer writes palette index 1 / colour c232 from the final shared-edge
quad, while the native hardware rasterizer leaves the prior black texel.  This
is a small triangle-edge fill-rule difference, not missing game geometry,
wrong clipping or draw distance.  Removing SDL_GPU's half-pixel vertex offset
was tested and rejected: timer-250 road RMSE rose from 0.0511 to 0.1388 and
coherent surface components grew to 15 pixels.  Keep the established +0.5
pixel-centre convention rather than trading a 2x3 seam for broad regressions.

The same cache cannot prove tachometer geometry from equal timer/speed alone:
no pair has equal `tacho_rpm`, because the smoothed `g_EngineRpm` retains prior
update history even when the complete current car record is identical.  The
worst 37-pixel needle mismatch corresponds to 6972 versus 6948 RPM.  Require
an exact RPM phase before classifying it as a HUD emitter failure.

For physics/render-boundary diagnosis the capture manifest also records the
complete 0x19c-byte player runtime as `player_raw`, alongside the existing four
rival records.  This is diagnostic evidence rather than a default alignment
gate: several cached/output-only fields legitimately differ even when the
observable player position, speed and progress match.  A checkpoint replay at
timers 499..525 produced identical player x/z, speed and progress throughout;
the previously suspected timer-508 physics divergence was a pairing/run-phase
artifact.  Compare named scalar state first, then use byte offsets in
`player_raw` to locate the first internal writer when a real divergence remains.

The native SDK boundary now canonicalizes every ordering-table element into a
dense sequence of 32-bit GP0 words before dispatch, using the same code on
32-bit and 64-bit hosts.  Set `RAGE_GPU_GP0_TRACE=1` to print stable
`chain`/`packet`, opcode, length and word payload records.  Pointer-sized OT
links and host struct padding never enter this stream, so it is the correct
compatibility contract for a future software oracle or renderer backend.  A
`code=00 length=4` record containing four zero words is four legal GP0 NOPs,
not an uninitialized geometry primitive; classify packets by their GP0 opcode
before treating zero payload as missing HUD or terrain.

The Ruby retail GPU accepts the same `RAGE_GPU_GP0_TRACE=1` switch and emits
decoded `gp0-command` records with DMA chain/node and RAM-address provenance.
Compare a controlled pair with `tools/rage_gp0_compare.rb --psx PSX.LOG
--native NATIVE.LOG`.  It flattens packet grouping (for example PsyZ's one
nine-word drawing-environment packet versus the retail GPU's seven decoded
commands) and reports the first changed 32-bit word plus both enclosing
packets.  Do not compare unrelated boot and checkpoint logs: unlike image
alignment, this oracle deliberately performs no heuristic state matching.

Use `tools/rage_gp0_bundle.rb --bundle COMPARE/FRAME` for the repeatable
second-stage diagnosis.  It reads the matched PSX/native capture filenames,
surface, `replay-pre.psxstate`, replay VBlank count and original commands from
the visual bundle and its parent `run.json`; it then leaves `gp0-psx.log`,
`gp0-native.log` and `gp0-diff.txt` beside the heatmap.  Each ordering-table
submission is tagged with the game's `scene` and `timer`; bundle replay filters
both runtimes by those values and then requires the same E3 draw page.  It
aborts instead of falling back to an unrelated frame.  This is stronger than
selecting by presentation-frame number: an earlier display-page `frame-1`
rule selected stale geometry (for example timer-263 commands for a timer-265
image), fabricating colour and HUD differences.  The comparator normalizes
only physical draw-page state (E3/E4/E5 and fill Y), never primitive XY, which
is already relative to E5 on both sides.

A saved checkpoint can sit on either side of the VBlank/DMA boundary.  A
one-VBlank replay therefore does not imply that commands are tagged as local
frame zero (and in practice may submit on frame two).  Bundle replay leaves the
short checkpoint trace unfiltered and selects the nearest local frame which
actually contains GP0 commands.  This prevents valid one-step visual bundles
from producing an empty retail trace while retaining the chosen frame in
`gp0-diff.txt` packet provenance.

When more than one local retail frame contains commands, bundle replay also
matches the first GP0 E3 draw-area word against the selected native frame.
Choosing merely the numerically nearest local frame can land on the opposite
VRAM page.  For the timer-265 bundle, draw-area matching selects retail local
frame 2 and native frame 1629; this removes the stale `575757` colour and leaves
the first real difference at word 105, one projected X coordinate (`122` on
retail, `123` native).  Texture, CLUT, tpage, colour and the other vertices are
already equal at that point.

That remaining one-pixel coordinate is not a GTE mismatch.  The extended
decision trace identifies the same asset vertex indices on both sides
(`6816,6812,6817,6814`), but the actual GTE translation registers differ:
retail `-5957,-21237,43871`, native `-5959,-21231,43971`.  The resulting SXY is
retail `105,123 / 102,122 / 103,120 / 101,120` versus native
`105,123 / 102,123 / 103,120 / 101,120`.  Treat this bundle as a coverage
discovery pair, not a bit-exact projection oracle.  Terrain decision records
now include source vertex indices and the real TRX/TRY/TRZ control registers;
`rage_geometry_trace_compare.rb` treats either input difference as structural
and reports it before clip/depth output is interpreted.

The first application of this oracle found real portable terrain-emitter bugs.
Direct retail windowed faces submit `reset+set` before the FT4 and `reset+NOP`
after it.  A subdivided face instead brackets the complete visible-child group
once: `set+NOP`, every child FT4, then `reset+NOP`.  The portable emitter used
to bracket every child independently, inserting E2 commands between adjacent
children and disturbing the OT stream.  Keep the direct and grouped paths
separate when backporting; one universal packet layout makes one class match
by breaking the other.  Because PS1 ordering tables are LIFO, link the group
reset before the first child and the set command after the last child.

Subdivision guide lines have a related OT rule.  Retail adds the record's
signed byte-21 depth bias to the parent terrain bucket before the guide emitter
adds its fixed 64-bucket band.  Passing the unbiased face depth placed every
guide from such a record in the wrong bucket.  At timer 270 the representative
record `0x80149100` uses raw depth 3807, parent slot 249 (base 128 + depth 118 +
bias 3), and guide slot 313.  Applying the bias moved the strict GP0 mismatch
from word 2506 to 4123; grouping the texture-window commands moved it again to
5486.  Both are game-emitter fixed-point/OT semantics, not PsyZ workarounds.

The same trace exposed a missing game-side far-face conversion.  When
`rawDepth >= 0x800` and face flag bit 0 is set, retail halves all UV bytes and,
for windowed dispatches, transforms the packed texture window as
`((window >> 1) & 0x7bfff) | 0x210` (original instructions at
`0x80028608..0x80028620` and `0x80028760..0x80028778`).  The portable decoder
halved UV but left the window unchanged: timer 270 therefore produced native
`E20C0300` where retail produced `E2060390`, despite identical FT4 XY and UV.
Applying the original conversion removes that mismatch.  This belongs in the
game's `SubmitTerrainCells` port, not in PsyZ or a modern renderer.

Course geometry has two related but distinct rules.  `EmitCoursePolyFT4` and
its subdivided sibling store `set+NOP` in their pre-polygon `DR_TWIN`; copying
the direct-terrain `reset+set` layout inserts an extra GP0 word.  In addition,
the original emitter adds the 32-bit scratchpad render mode at `scratch+0x84`
to the first packed UV/CLUT word before emitting every textured course face.
The portable course decoder omitted this for both type 1 and windowed type
2/3 records.  On timer 270, model 8 therefore selected CLUT `0x7a0c` instead
of retail `0x7a0d`; applying the original packed-word addition fixes the
whole emitter rather than special-casing that model.

Course OT depth is also not an `AVSZ4` average.  The original dispatcher
projects three vertices with RTPT and the fourth with RTPS.  Its transform
stage quantizes every GTE Z into a 16-bit scratch-table entry first; the face
loop adds the first and fourth saved entries and only then shifts the sum by
three (`0x8002a37c..0x8002a43c`).  `RotTransPers` exposes a value eight times
larger than that saved entry, so the portable equivalent is
`((z0 >> 3) + (z3 >> 3)) >> 3`.  Do not replace it with
`(z0 + z3) >> 6`: quantization and addition do not commute.  At timer 270,
model 57 type-2 face 28, retail saves `865` and `869` and selects bucket 216;
the wider-value average selected 217 despite identical source indices and
screen XY.  Correcting the quantization order moves the strict GP0 stream's
first mismatch from word 988 to word 2528, removing the earlier course/terrain
interleave error.  This fixed-point semantic belongs to the game-code port and
must be preserved on both 32- and 64-bit targets when backporting.

For repeatable course-depth diagnosis, `RAGE_COURSE_FT4_TRACE` in the Ruby
emulator now hooks both direct emitters and `SubmitCourseSubdividedFaces`.
Each record includes its source pointer, four vertex indices, four saved Z
values, pre-shift sum and selected depth.  The native course trace prints its
four corresponding `RotTransPers` depths.  Filtering both with timer 270 turns
an OT-ordering symptom into a record-for-record fixed-point comparison without
interpreting screenshots manually.

Variable-length GP0 polylines need the same treatment in the packet oracle.
The emulator originally executed `0x48` packets through its terminator-driven
polyline path without calling the ordinary command logger.  Native traces then
appeared to contain extra subdivision guides even though timer 270 has the same
12 line pairs on retail and native.  The emulator trace now records the full
packet including `0x55555555`/`0x50005000`; a regression test covers that
special path.  Do not diagnose a missing or extra primitive from a command
stream until all variable-length GP0 families are represented in the trace.

GP0 traces on both sides now carry the packet address, and the native model,
course and terrain diagnostics carry their allocation cursor.  Correlating
those fields is substantially safer than guessing ownership from a primitive's
XY payload: the same small F4/FT4 geometry can occur several times in one OT.
For example, the apparent timer-270 FT4-versus-F4 mismatch at strict word 5486
maps the native F4 to `SubmitModel` model 15 face 5.  The retail emitter reports
the identical face at depth 67.  The nearby terrain face (cell 151 face 2) has
base depth 66 and bias -1 on both builds; retail proves its final OT pointer is
slot 193 (`128 + 66 - 1`).  Therefore this case does not justify removing the
mode-0 terrain bias.  Use address/owner correlation before changing an asset
decoder in response to a stream-order symptom.

`rage_gp0_compare.rb --resync-window N` complements the strict first-word
failure by finding the nearest identical packet after a local divergence.  It
classifies the gap as native-extra, PSX-extra, or divergence on both sides and
reports both packet identities.  At word 5486 a window of 100 finds the retail
FT4 eight native packets later; this proves a local OT interleave instead of an
FT4-to-F4 conversion.  The emulator's optional `RAGE_GPU_OT_TRACE=1` records
every DMA node including zero-length OT links, so the exact bucket boundary can
then be inspected without changing emulated memory or registers.

With `RAGE_GPU_OT_TRACE=1` enabled on both builds, `rage_gp0_compare.rb
--nodes` reconstructs retail's several GP0 commands back into their original
DMA node and retains empty ordering-table links.  This exposed the ordinary
model bias bug above: fixing all four record types moved the strict timer-270
oracle from word 5486 to 6045.  Runtime GTE tracing then identified the exact
retail GT4 `NCT`/`NCS` sequence described above and moved the mismatch to word
10602.  Raw-texture GP0 commands ignore the low 24 RGB bits of their command
word, so the visual oracle normalizes those bytes while `--raw` still compares
them exactly.  The next real difference was the lap-highlight CLUT caused by
the false standalone global.  After reading the embedded player-car field,
the complete normalized timer-270 stream matches: 10809 GP0 words.

A strict draw-page replay over timers 260..280, gated on exact player/view
position, speed, projection, tachometer RPM and both visible-cell masks, leaves
13 matched frames.  The timer-280 tachometer packet is identical on retail and
native: `280018c0,00c80107,00a40107,00c7010a,00a30108`.  The apparent
33-pixel red-mask difference is therefore not a missing needle or malformed HUD
geometry; the mask also includes red dial artwork and PS1/native edge coverage.
In the same strict cache the mirror has no native-only black pixels, the road
has no clear-colour holes, and its largest black component is ten pixels with
no corresponding surface-divergence component.  Keep these small edge signals
separate from the earlier systemic missing-texture failures.

The apparent next command-stream mismatch, a one-level fog-colour difference,
was also caused by that stale native-frame selection.  For the concrete terrain
cell 168 face 7, native receives base RGB `3f3f3f`, IR0 `1512`, and far colour
`128,128,128`; `DpqColor` produces the retail `565656`, and the correctly
aligned canonical packet contains the same value.  A standalone PsyZ regression
records this DPCS case.  Do not compensate for the discarded `575757` result in
the game emitter—it belonged to an earlier submitted frame.

After removing blank references, the timer-1600..2400 mirror-road scan has no
native-only clear pixels and no connected native-only black area.  Its worst
valid frame is timer 2030 (region RMSE 0.136) with a one-unit view-position
delta and projection fingerprint delta 14; both images contain the mirror
frame, road and distant geometry.  The residual is a shifted high-contrast
texture/raster edge, not evidence of mirror culling or draw-distance loss.

For deterministic backend work, `rage_gp0_bundle.rb` now stores exact
`gp0-pre.vram` and `gp0-post.vram` RGB5551 images around the traced retail GP0
frame.  `rage-gp0-replay VRAM GP0 FRAME OUTPUT` loads the former and submits
the canonical commands to PsyZ while preserving packet boundaries.  It derives
the destination VRAM page from GP0(E3) before readback; otherwise Rage's double
buffering compares the newly drawn page with the page displayed before it.
Timer 868 supplies a stable 11805-word reproducer in which PsyZ retains the HUD
and nearby scenery but initially appeared to lose large world regions.
The replay's optional `LAST_PACKET` and `ONLY_PACKET` arguments reduce that
frame to a packet prefix or one primitive plus its E1--E6 state.  At screen
pixel `(50,150)`, retail primitive order 520 is FT4 packet 909
(`address=1c90a4`): retail samples palette texel `94a5` and dithers it to
`9084`.
For an authoritative one-packet oracle, `rage_gp0_bundle.rb
--dump-psx-packet N` writes `gp0-before-NNNN.vram` immediately before retail
executes packet N.  Capture N and N+1, then use `rage_vram_delta.rb` to report
the exact changed-pixel count, bounds and an optional pixel value.  Packet 909
changes 22 pixels in `(39..59,390..391)` and changes `(50,390)` from `1800` to
`9084`.  This packet exposed a 64-bit bug in the diagnostic replay API:
canonical GP0 words are 32-bit, but `Psyz_GpuReplayPacket` cast their buffer to
PsyZ's pointer-sized `u_long *`, making `DispatchPackets` read every other word
on 64-bit hosts.  Expanding every input word into a real `u_long` fixes the
harness without changing game data.  The isolated PsyZ packet then matches 21
of retail's 22 changed pixels bit-for-bit, including `(50,390)`, and full
timer-868 replay RMSE falls from about 0.489 to 0.0282.  The remaining
one-pixel difference is ordinary raster edge coverage; the earlier large
missing-world replay was not evidence for changing game culling or LOD.
The same packet-boundary oracle isolated tachometer FT4 packet 454
(`address=1c8584`).  Retail changes 31 pixels in `(246..253,354..360)`;
PsyZ changes 29, but initially only 9 values match bit-for-bit.  GLSL matrices
are column-major, so the row-major PS1 dither table must be sampled as
`ditherMatrix[dx][dy]`, not `[dy][dx]`.  After that correction the isolated
packet matches 23 of the 31 retail writes; the three missing writes are edge
coverage and the remaining five are interpolation/raster precision.  This
also reduces full timer-868 RGB RMSE from 0.0282 to 0.0207.

A fresh native timer-1600..2400 capture after the dither fix was compared
against the retained retail capture.  With position, camera, projection,
animation-phase and mirror-visible-cell gates, every accepted mirror pair has
`native_only_clear=0`, `native_only_black=0`, and no black connected component.
The remaining mirror RMSE is not a strict content oracle: rival-position
deltas are 474--840 world units, and requiring a 64-unit aggregate rival gate
correctly rejects every pair.  Do not diagnose mirror texture loss from those
moving-car residuals.

The worst fresh tachometer pair (timer 1900, retail frame 600/native frame
3264) reports a large red-mask mismatch, but the actual needle GP0 packet is
bit-identical on both sides:
`280018c0,00c20101,00b30122,00c60103,00b50123`.  The broad mask includes dial
artwork and raster-edge coverage; it is not evidence of a missing needle.  A
road comparison requiring the complete main visible-cell list finds no
eligible pairs, so this capture cannot prove course-surface pixel parity
without first synchronizing simulation visibility state.

`RAGE_EMU_VISIBLE_CELLS=1` and `RAGE_PORT_SMOKE_VISIBLE_CELLS=1` now write the
same per-capture `visible-cells.log`: 64 main records, 64 mirror records and 32
masks for each pass.  `rage_visible_cells_compare.rb PSX.LOG NATIVE.LOG TIMER`
reports exact differences and separates membership, active-coordinate and mask
changes.  At timer 1900 the old full-list hashes differ in 66 records, but
membership differences are zero and all masks are identical; 25 active records
only contain small coordinate deltas caused by the one-unit view-position
difference.  Requiring byte-identical full lists was therefore too strict for
surface comparison even though requiring the visible-cell masks remains valid.

With that corrected gate, the fresh road scan finds apparent native-only black
components at timers 1670 and 1735.  Visual inspection of the worst timer-1670
component (`x=24..83, y=55..68` in the road preset) places it inside the legal
black arrow texture on the barrier, shifted by the one-unit camera delta; it is
not missing road geometry.  Keep the black detector sensitive, but confirm a
candidate with membership/mask logs and packet/pixel tracing before treating a
textured black region as a dropped triangle.

Timer 1750 provides a second, packet-level check of the same barrier.  The
large surface-divergence mask is not an UV, CLUT or TPAGE mismatch: at hotspot
`(164,57)` both renderers select the wall FT4 with physical `tpage=0x15`,
`clut=0x7c03`, and nearly identical interpolated UV (`89.08,2.46` retail versus
`88.71,2.47` native).  Its projected vertices are shifted by three to four
pixels, so the narrow repeating black-arrow texture legitimately covers a
different set of screen pixels.  The complete GP0 stream first differs even
earlier at word 103, again only in projected XY; that packet's colour, UV,
CLUT and TPAGE words agree.  Therefore neither the timer-1670/1735 black mask
nor the timer-1750 surface mask is evidence for changing terrain culling,
texture selection or PsyZ sampling.  A genuine missing-geometry candidate must
survive visible-cell membership/mask comparison and show either a missing game
packet or a different result when replaying the same packet from identical
VRAM.

Long-route visual scans need two additional alignment safeguards.  Position
alone is ambiguous on a circuit (and while the scripted car is held against a
barrier): the 2400--3400 scan initially matched three reference frames to
native states 129 game-timer ticks later, with otherwise plausible camera
positions but rival deltas around 28000.  Use `--max-timer-delta N` (normally
1 or 2 for adjacent VBlank phases) to exclude those loop aliases.  Animated
scenery variants remain an equality requirement by default, but
`--ignore-scenery-variants` permits an explicit static-road or HUD audit when
the cached reference inherited a different variant; do not use that opt-out to
judge moving scenery itself.  With `--max-timer-delta 2`, the scan retains 13
state-valid pairs and none contains a connected native-only black region.
Visual inspection also shows that the acceleration-only input has left the car
at roughly 33 km/h against the same barrier, so this route cannot substantiate
the user's later in-race missing-triangle report; the next capture must include
deterministic steering or start from a manually obtained later checkpoint.

`rage_gp0_bundle.rb` now caps each replay just after the requested filename
frame (`frame + 2` VBlanks/host frames) when it does not have a closer
per-capture checkpoint.  Previously it reused the original capture's full
limit, so the second pixel/texel pass could keep Ruby at 100% CPU for minutes
after the useful trace had already been written.  Per-capture checkpoint
replays retain their existing `psx_replay_frames + 2` boundary tolerance.

The acceleration-only route is not state-comparable indefinitely.  Retail and
native remain within tens of world units through timer 2700, then diverge
sharply between 2740 and 2800: retail stays near progress 25220 against the
barrier while native advances by roughly 1800 track units.  At timer 3400 the
position delta is already about 7900, so later image pairing is invalid even
when the scenery looks superficially similar.  A three-frame native LEFT
impulse near host frame 4100 delays but does not remove the divergence; input
compensation is not an acceptable renderer oracle or a collision fix.

Use `RAGE_PORT_CAR_TRACK_TRACE=1` plus
`RAGE_PORT_CAR_TRACK_TRACE_TIMER=N` on native, and `RAGE_CAR_TRACK_TRACE=1`
plus `RAGE_CAR_TRACK_TRACE_TIMER=N` in the Ruby emulator, to log the player
entry/exit state around the original `UpdateCarTrackState`.  The trace records
position, track point, speed, progress, lateral offset, hull limits, computed
track geometry, boundary result and knockback state.  A synchronized retail
checkpoint at timer 2741 shows that the manifest's retail timer phase normally
pairs with native `N-1`; compare state, not equal textual timer labels.  At the
2742-retail/2741-native pair both calls return `knockback=0`, but their entry
position, lateral offset and existing motion timer are already different.
Therefore the long-route desynchronization precedes this track-clamp call and
must be traced backward through the prior knockback/motion integration before
using post-2800 captures for visual acceptance.

Range tracing resolves that divergence further.  Both runtimes repeatedly hit
the same left boundary: every clamp ends at `x=20235`, lateral `-162`, mode 1,
motion timer 30 and velocity `(-58,-1)`.  Retail events occur at timers
2613/2634/2655/2676/2703/2724 and native at 2610/2631/2652/2673/2700/2721/2742;
the three-tick phase offset is stable and the clamp geometry is not the bug.
For 18 updates after the last aligned clamp, X and speed are bit-identical and
Z differs by only one unit.  At native timer 2759,
`CollidePlayerWithCars` alone changes the active knockback from `(-5,0)` to
`(-62,-184)`; `ApplyCarKnockback` applies the resulting `(62,184)` jump on the
next update.  The collision is opponent index 5, region 4, at player
`(20311,14574)` and opponent `(20379,14481)`.  The retail opponent is nearby
but offset by roughly 74 world units, and the original retail collision
routine reports no player-car hit anywhere from timer 2741 through 2790.

Instrumenting the retail `IsPointInQuad` entry at `0x8002D2E8` resolves the
remaining ambiguity: during the corresponding retail interval the player-car
collision routine never reaches a point-in-quad call. Its earlier proximity
filter rejects opponent 5. Native reaches the exact half-plane test and gets a
valid region-4 hit for its already different opponent position. Thus there is
no evidence for changing `NormalClip` or `IsPointInQuad`; doing so would hide
the upstream AI-state divergence and change valid collisions.

This is a reproducible false native car-car collision, not a terrain clamp,
renderer, draw-distance or visual-alignment failure.  `RAGE_PORT_CAR_MOTION_TRACE`
marks pre-integration, post-position, post-progress, post-knockback,
post-track and post-car-collision phases; `RAGE_PORT_CAR_COLLISION_TRACE`
identifies the struck opponent/region.  The Ruby equivalents
`RAGE_CAR_STATE_TRACE` and `RAGE_CAR_COLLISION_TRACE` read the retail runtime
without modifying game RAM.  Keep post-2800 visual captures excluded until
opponent 5 is synchronized or the collision test is compared from an identical
input state.

Use two routes for renderer work until that gameplay divergence is fixed. A
Time Attack route, which has no opponent collision updates, is the long-lived
reference for road geometry, textures, sky and the common HUD. Grand Prix
captures remain necessary for the rear-view mirror, but must stay before the
first state divergence (or use a later independently synchronized checkpoint).
Do not accept a visually similar post-divergence Grand Prix frame as renderer
evidence. Both routes retain the original PS1 LOD and draw-distance policy
while compatibility is being established; extended distance and forced
highest-detail models belong to a separate modern-renderer mode after the
baseline agrees.

`rage_visual_run.rb --route time-attack` selects Time Attack during the fresh
native boot and holds acceleration for the long capture. The supplied emulator
checkpoint must already be a Time Attack scene-12 state; `--route` deliberately
does not mutate retail RAM or reinterpret a Grand Prix checkpoint. Use
`--route grand-prix` (the historical default input path) for mirror captures.
Acceleration starts at native host frame 1264, the observed scene-12 entry,
not at the countdown end: delaying it to frame 1470 left native at idle RPM
while the checkpointed retail car was already near 7050 RPM. With the corrected
route, all 54 available timer-200 onward reference frames pass state alignment
without a rejection (using the explicit scenery-variant opt-out while the
fresh boots retain different RNG history).

One HUD failure was unrelated to drawing. `InitEffectVoiceRuntime` reproduced
retail pointer arithmetic from `g_AudioLoadedSlotMask` to the right-volume
field of each `MusicChannel`. On PS1 those globals occupy one fixed contiguous
work area; independently linked host globals do not. The first write landed on
`g_BestTotalTimes[0][0][0]`, replacing the initialized 310765 ms record with
zero exactly when a race was entered, so Time Attack displayed `0'00"000`.
Write `g_MusicChannels[i].volRight.value` directly. This preserves the retail
field update on both 32- and 64-bit targets and is a game-code compatibility
fix to backport to the decompilation, not a PsyZ workaround. The synchronized
timer-530 native capture now displays the retail value `5'10"765`.
`ResetSoundState` contained the same unsafe expression and must use the same
named-field write. The `audio_state_layout` regression checks both reset paths
so a future matching-oriented rewrite cannot silently restore the PS1 linker
layout dependency.

The drivetrain had another split-symbol dependency. On retail,
`g_TorqueBandStart` is the halfword immediately before `g_TorqueBandEnd[]`, so
for RPM band `b > 0`, `(&g_TorqueBandStart)[b]` means
`g_TorqueBandEnd[b - 1]`; the loss curve uses the same representation. Native
globals have no such adjacency guarantee. Use the predecessor entries of the
two end tables explicitly. This retains the original curve search ranges while
removing another 32/64-bit and linker-layout dependency from game code.

`g_BestSectorTimes[2][4][3]` was another truncated host object: its declaration
requires 96 bytes, while `host_state.c` allocated only eight and treated the
following two retail labels as separate globals. Reads therefore returned zero
or unrelated state, and the save loader's 96-byte copy could overwrite later
host globals. Allocate the complete table and initialize all three sectors of
each course from the same default lap time used by retail before an optional
memory-card load. At synchronized Time Attack timer 315, `SECTION TIME` now
shows retail's `1'40"765` rather than `0'00"000`. With this fixed, the complete
normalized GP0 stream for that frame matches exactly: 10097 words. Its residual
82-pixel road-region component is therefore PsyZ raster precision, not a
missing game triangle or different texture packet.

The scripted camera state had the same truncated/split-object problem. The
three `g_CamPathOffset*` arrays require 12 bytes each and the three
`g_CamPathAngle*` arrays require 16, but every host backing object was only
eight bytes. In addition, retail `g_ChaseYawPrev` is exactly
`g_CamPathAngleDelta[CAMPATH_YAW]` at `0x8009B1EC`, not independent state.
Allocate the complete arrays and express that alias in `track.h`. This prevents
camera-path writes from escaping their objects and keeps chase-camera and path
camera users on the same retail word, which is essential before diagnosing
projection, clipping, road bending or mirror-matrix differences.
