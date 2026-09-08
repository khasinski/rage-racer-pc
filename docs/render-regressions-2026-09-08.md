# Attract-mode car geometry and enhanced-classic seams

The first 0.6.4-alpha manual-test packages failed acceptance on these two
regressions. They remain unchanged as baselines; no release is approved.

## Wrong modern car bodies after interrupting attract mode

Track/asset transitions released CPU mesh templates but retained resident GPU
vertex/index buffers. The resident cache used the template address as its key.
When the allocator reused an address in the GP prologue or race, the renderer
could bind a previous car's GPU geometry to the new car's materials and parts.
The semantic scene and CPU draw dump remained correct, obscuring the defect.

`ModernNativeGpuPrepare` now releases resident geometry before releasing its
source templates on track or asset generation changes. Existing synchronization
drains preceding GPU work before this invalidation.

The compiled game's `attract_geometry` CMake regression drives title -> attract
-> interrupted title -> GP prologue -> race. Four GPU screenshots (two from
the prologue, two from the race) must equal the reference path that expands
geometry without the resident-buffer cache. It requires a legal disc through
`RAGE_PORT_DISC_CUE`; absent data is reported as a skip.

macOS Metal: all four screenshots match byte for byte. A captured pre-fix race
shows malformed bodies; its fixed counterpart matches the reference exactly.

## Holes between enhanced-classic polygons

Classic interpolation matched each emitted GP0 polygon independently. A face
whose subdivision or material identity changed was held at the previous logic
tick, while a matching neighbour moved toward the next tick. Their shared edge
separated, exposing the background on intermediate presentation frames.

Matching now checks connected projected surfaces. Shared vertices must have
consistent motion, and a held child holds its connected component. Separate
model instances retain independent correspondence. This deliberately retains
logic-rate motion for a component during incompatible topology changes; it
does not claim continuous interpolation through every subdivision change.

`classic_motion` covers adjacent edges, subdivision changes, conflicting vertex
motion, transitive hold propagation, course surfaces across source draws, and
independent overlapping model instances. The new adjacent-edge assertion fails
against the previous implementation and passes with the fix. Release and
ASan/UBSan builds pass. The high-FPS presentation test now captures an actual
intermediate frame; the inspected Overpass reverse frame has a torn road before
the change and a connected road afterward. The macOS 1920x1080/120-FPS route
test passes with 969 distinct intermediate presentations in the measured run.

These checks reproduce the reported mechanisms; they do not replace manual
route testing or establish that every possible classic-renderer seam is gone.
The initial fixes were validated on macOS. Subsequent Linux and Windows
results and their limits are recorded in the
[darwine validation report](validation-darwine-2026-09-08.md).
Nothing has been published.

## NTSC-U speedometer units

The US tachometer artwork says mph, but the port still supplied the PAL
numeric conversion (`internalSpeed * 160 / 1168`). Inspection of the user's
SLUS_004.03 executable, file offsets `0x23e54..0x23ec4`, confirms that retail
NTSC-U first truncates that value and then multiplies it by 100 and divides
by 160. PAL SCES_006.50, `0x24110..0x24150`, omits the extra conversion.
The port now follows that regional display rule, including its two rounding
steps, before clamping the three-digit readout. Disc identity determines the
unit independently of frame-rate/timing overrides. NTSC-J and PAL retain km/h.

This does not rescale player physics, gearing, AI targets, or elapsed time.
Across all 24 runtime track packs (88..134), PAL and US rival configuration
blocks are byte-identical. Speed-key tables differ in five packs, with small
regional balance adjustments (for example US 149 versus PAL 148), rather than
an mph conversion. The runtime uses the mounted disc's tables directly.

The tachometer test now checks actual emitted digit values in all regions,
unknown/null region handling, negative/overflow inputs, retail rounding,
conversion before clamping, and preservation of the entire player state.
`regional_speed_units` runs the real rival initialization, target-speed update
and acceleration for 300 ticks per region: every rival state matches while
1168 internal units displays as 160 km/h or 100 mph. This checks the unit
boundary; it is not a claim of identical regional race balance or a lap-time
benchmark. The selected nine HUD, AI, drivetrain and timing tests pass on
macOS. Original-disc analysis artifacts are local under
`build/regression-speed-units/` and are not included in release archives.
