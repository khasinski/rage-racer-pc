# Rounded fleet revision

The stronger Esperanza style now covers all 13 base cars, all 19 upgrades,
and all 28 distinct rival bodies across the 24 rival banks. The registry
replaces 188 body bindings and 952 wheel/axle bindings. Distant models,
shadows, physics and collision data retain their original behavior.

## Geometry and materials

Every player bank has all twenty wheel/axle variants, including tire selection
and blurred wheels. Tires have 64 angular segments with rounded shoulders;
rims use recessed metal geometry. Original radii, widths, axle positions,
rotation and steering remain the assembly reference.

Wheel openings have circular upper arcs and dark inner walls with inboard
closures. Existing cross-car strips and paint intrusions are removed.
Wide fenders are subdivided and bent together with their lip so that the
liner cannot protrude through a folded flare. The center of the hood keeps
its original geometry; local fender shaping falls off toward surrounding
panels. Original UVs, corner colors and normals are interpolated locally.
Squaldon keeps its original closed rear aero skirts, with liners behind them.
Pegase and Bulshade retain their exposed front-fender design.

Low hoods require a tapered well: its outer lip follows the fender, while
the inboard roof drops to one model unit above the original tire radius.
Pegase, Bulshade and the Ghepardo rival also limit the inboard depth to the
wheel's inner edge plus four units. This removes the gray protrusions without
raising the hood or leaving the wheel cavity open.

The follow-up well-fit correction uses the actual triangulated liner to cut
the surrounding paint, including its changing width along the arch. A cone
with one assumed outer width removed valid paint from Vainqure's fender
shoulders. Vainqure also limits its inboard wall to the wheel's inner edge;
Bulshade bends the surrounding rear fender together with the rounded lip.
The correction covers both player bodies, all their rival variants, and
the same cutting defect in Pegase and the Ghepardo rival.

Paint, glass, rubber, metal and hood artwork have independent surface classes.
The special cars' glazing is split inside their shared body atlases, retaining
painted roof sections and frames. Glass uses a common tint. Player name/logo
refresh only targets the actual page-10 marking materials; it does not overwrite
the special cars' page-11 body atlases.

The hood logo is a depth-tested decal displaced outward from the hood. Its
atlas is isolated from neighboring wheel artwork, and the name sunstrip is
preserved by glass tinting. Direct scenarios upload the editable logo and name
just like normal course selection. A blank fresh canvas remains blank;
`race.logo_sample=5` selects a retail sample for reproducible screenshots.

## Sources and build

[The catalog](../assets/cars/rounded/catalog.json) identifies each saved scene,
body, wheel, source bank and original wheel dimensions. The 59 new Blender
files contain packed references, hidden retail bodies and assembled wheel
previews. The approved Esperanza baseline stays in
[esperanza-rounded.blend](../assets/cars/esperanza-rounded.blend).

There are 59 new body OBJs and 342 shared wheel OBJs. Identical wheel exports
are deduplicated; bank-specific page/palette/cache mappings remain distinct.
Normal CMake builds import and embed every OBJ through the C mesh tool.
Playing needs neither Blender nor a separate asset-generation command.

Export only the named body or individual wheel object, with -Z forward, Y up,
UVs, normals, colors and triangulation. Preview copies must not be exported.
Material names for the selected export must be canonical `rage_N` words;
wheel export words use source slot zero plus the rubber/metal class.
The saved-source round-trip check normalizes these names before each export,
because Blender requires distinct names for separate preview materials.

## Verification and evidence

- All 735 newly authored body/wheel objects reproduced their expected native
  bytes when re-exported from saved Blender files through the C importer.
- The asset test checks every new body against its original bounds, every
  distinct wheel's radius, width, circular circumference and outward tire
  normals, material mappings, and paint/liner intersections.
- A separate flare-join check catches the wide-fender defect even when no
  triangle intersections remain. The old Esperanza fender still fails the
  intersection negative control.
- Vertical clearance checks across all 59 new bodies find no well roofs
  exposed above the hood. The compiled asset test also guards the original
  Vainqure failure on both sides of the player and rival bodies.
- Clearance alone missed holes where the cutter had removed the paint.
  The expanded compiled coverage test checks 194 positions across Vainqure
  and Bulshade player/rival bodies, including both fender shoulders and the
  inboard edge. All 99 objects in the nine revised source scenes round-trip
  to their expected native bytes. Follow-up evidence and comparisons are in
  `build/car-well-fit`.
- The broad regression pass had 374/375 passing tests. Its sole failure
  exposed floating-point drift between 0° and 360° in the inspection stage.
  Normalizing complete turns fixed it; both stage tests subsequently passed.
  Final focused geometry, material, mesh conversion and stage tests pass.
- All 17 transition tests passed, including the test that repeats races with
  each of the 19 upgraded player banks.
- Each of the 59 new bodies has front/rear renderer inspections; front wheels
  are turned 25°. The inspection PNGs use gamma 1.8 to expose dark well interiors.
  Actual race screenshots preserve the rendered brightness and include every
  base car and every upgrade.

[Open the comparison gallery](../build/car-upgrade-evidence/rounded-fleet-gallery.html).
Original/rounded base-car pairs use the same camera, course and capture timer.
The gallery also contains upgraded players and every rival-body inspection.

Evidence logs live in `build/car-upgrade-evidence`, including
`fleet-regression-tests.log`, `fleet-stage-tests.log`,
`fleet-final-focused-tests.log`, `fleet-flare-join-tests.log`,
`fleet-transition-tests.log`, `fleet-final-transition-tests.log`,
`fleet-hood-clearance-audit.json`, and `fleet-fender-negative-control.log`.

[The low-hood comparison](../build/car-upgrade-evidence/well-protrusion-game-comparison.png)
shows Vainqure before and after the inboard-well correction in the actual game.

The clean application is [build/fleet/Rage Racer.app](../build/fleet/Rage%20Racer.app).
LaunchServices started it without an adjacent native cache or asset/renderer
overrides. The log confirms modern rendering, automatic C asset generation
from the remembered CUE, and installation of the authored rivals.

Validation here is on macOS. The existing environment-atlas defect at some
custom mid-track starts is visible with original cars as well; these car
comparisons do not establish complete track-rendering coverage.
