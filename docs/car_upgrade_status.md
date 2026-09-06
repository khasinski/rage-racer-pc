# Car upgrade work in progress

Goal remains `goal.md`. Base Erriso, Abeille, Pegase, Esperanza, Acceron, Bayonet,
Hijack, Fatalita, Istante, Ghepardo, Vainqure, Bulshade, and Squaldon player bodies and
their rival representations are integrated. Other player grades and cars retain their
original geometry; the full set is still in progress.

The duplicate rival audit identified three additional Esperanza bodies;
their integration is documented below. Coverage of the first four rival
models is tracked separately from the nineteen remaining player upgrades
and from any extra bank objects beyond those four models.

## Integrated and inspected on 2026-09-05

`assets/cars/erriso.blend` contains the player and rival sources, original
references, and packed reference textures. A 1.6-unit three-segment bevel,
weighted normals, color clamp, and 85% transfer of retail panel normals soften
edges while retaining the silhouette and panel shading. The player body has
1948 triangles; the rival body has 1694. Original wheels, far rival LOD,
physics, collision, and classic rendering remain unchanged.

The normal CMake build converts the OBJ sources with the C interchange tool
and embeds their bytes. Modern rendering automatically replaces body 0 in
player bank 10 and body 10 in rival banks 96, 98, and 100. Disc-imported
materials are resolved by texture-page/palette identity. Players need no
Blender installation or separate asset-generation command.

## Verification

Evidence is in `build/car-upgrade-evidence/` (local build artifacts):

- Full release build passed: `final-rival-build.log`.
- All 375 non-e2e regression tests passed with assertions enabled in Release:
  `full-regression.log`. Coverage includes native parsing/cache/loading, OBJ
  interchange, preservation of wheel data and face metadata, and both
  embedded bodies. Release tests previously compiled out their assertions;
  the test directory now explicitly enables them.
- Real direct-boot tests passed for all three course banks:
  `direct-grid-0.log`, `direct-grid-1.log`, `direct-grid-2.log`. These also
  verify the fix for custom grids being applied too late during direct boot.
- A finish/repeat cycle entered two races with the authored player and rival
  models: `authored-transition.log`. Classic rendering also completed the
  scenario: `erriso-classic.png` and `erriso-classic-output.log`.
- Natural attract mode reached scene 30 and loaded the authored rival bank:
  `attract-after.log` and `attract-after.png`. Its screenshot follows another
  car; it demonstrates attract rendering, not a close view of Erriso.
- Comparable live-game chase views: `erriso-grid-before.png` and
  `erriso-grid-after.png`; trackside views: `erriso-trackcam-before.png` and
  `erriso-trackcam-after.png`. They show the cream player and blue Erriso
  rivals, including original wheels, textures, and visible panel edges.
- Blender rival views: `erriso-rival-blender-before.png` /
  `erriso-rival-blender-after.png` and `erriso-rival-blender-front-before.png` /
  `erriso-rival-blender-front-after.png`. Player wheel assemblies were checked
  in `erriso-blender-assembly-front.png` and `erriso-blender-assembly-rear.png`.
  The earlier player chase pair is `erriso-current-before.png` /
  `erriso-after-smooth.png`.

Late CPU submission samples were about 0.24–0.25 ms before versus 0.38 ms after
in chase, and 0.41–0.42 versus 0.52–0.53 ms trackside. Build time was about
0.008 ms. Some captures overlapped other CPU work: these are limited CPU
samples, not an isolated GPU/FPS benchmark. Far LOD remains original.

## Known limits and remaining work

The frozen custom start at track point 120 shows incorrect environment atlas
imagery on a rock wall. This is also present with `modern.authored_cars=0`
and in the classic capture; it predates the authored replacement. Investigate
environment issues after the car work. Natural attract tunnel rendering was
visually intact. Additional cars and player grades still require authoring,
integration, and the same visual checks; the overall goal is not complete.

Use `tests/scenarios/authored_erriso.ini` for repeatable comparisons. Course
0/1/2 use rival model slots 2/1/0 respectively. Early `erriso-traffic-*`
captures preceded the grid fix and do not validate the requested rival grid.
The capture writer emits PPM regardless of the filename suffix; convert with
ImageMagick before viewing. See `car_asset_authoring.md` for repeatable export.

## Abeille integration, 2026-09-05

Player bank 18 uses 2025 triangles; rival body 10 in banks 102, 104, 106, 108,
and 110 uses 1702. Each course has a separate replacement cache. Source files
are `assets/cars/abeille.blend` and `abeille-rival.blend`, with the corresponding
OBJ exports embedded by CMake. Original wheels and far rival body 14 remain.

UV and normal transfer is restricted to original triangles with the same
material and compatible face direction. This corrected the first bevel's
windshield corner artifact. Final export vertices are split at color seams:
Blender otherwise averages corner colors when writing OBJ. The asset test
checks a bumper/body seam retains both native multipliers, 12 and 255.

Build and focused asset checks pass (`abeille-color-build.log`,
`abeille-tests.log`, `abeille-seam-test.log`). Real-game bank checks cover all
five rival banks (`abeille-bank-102.log` through `abeille-bank-110.log`). The
repeat-race test enters scene 12 twice (`authored-transition-1.log`). These
extend the earlier full 375-test regression run; that full suite has not been
repeated for these model edits.
The final registered e2e selection also passes 4/4 (`abeille-final-e2e.log`):
Erriso direct grid and transition, plus Abeille banks and transition. Its
fresh per-bank logs are under `build/release/car-upgrade-evidence/`.

Inspected Blender pairs include `abeille-blender-front-before.png` /
`abeille-blender-front-material-normals.png`, `abeille-blender-rear-before.png`
/ `abeille-blender-rear-after.png`, and the rival's
`abeille-rival-blender-{front,rear}-{before,after}.png` views. Final live modern
comparisons are `abeille-game-c1-0.png` / `abeille-game-c1-1.png` (chase) and
`abeille-track-0.png` / `abeille-track-1.png` (course 1, camera 2, player front).
`abeille-rival-track-0.png` / `abeille-rival-track-1.png` add a close front
three-quarter rival view, with original wheels clearly visible. Reproduce
this pair on course 1 with grid slots all 1, camera 2, and
`start.rival_track_points=119`.
The first course-0 camera-2 pair repeated chase and is not a separate angle.

Late chase CPU submission samples were about 0.25–0.26 ms before versus
0.35–0.36 ms after. The course-1 front view sampled about 0.70 versus
0.86–0.87 ms. These are local CPU timings with other work on the machine,
not an isolated GPU/FPS benchmark; far LOD geometry is unchanged.

Smoke `capture.path` images read the compatibility framebuffer; they are not
evidence of modern geometry even when authored-bank loading appears in the
log. In particular, `abeille-lookbehind-*` and `abeille-course*-probe` images
only helped locate cameras. Use the release executable for modern screenshots.

## Pegase integration, 2026-09-05

Player bank 24 uses 2559 triangles. Rival body 10 uses 2182 triangles in nine
banks: 94, 112, 114, 116, 118, 120, 122, 124, and 126. Original rival far body
14 and wheel submeshes remain. Source files `pegase.blend` and
`pegase-rival.blend` each re-exported to byte-identical native meshes during
the saved-source round trip check.

Construction preserves original triangle identity through the bevel using
the `RetailFace` attribute. UV projection excludes zero-area source triangles
and stays within the original panel, fixing the first experiment's stretched
hood logo and non-finite UVs. The original hood decal's positions, UVs, and
full material word are covered by an embedded-asset test. Blender's coplanar
decal preview differs from the native renderer's depth treatment.

The renderer now uses an explicit replacement table with texture identities
and per-bank cache slots. This handles Pegase's two different cached-material
orderings and permits multiple authored bodies in a shared bank. A sequential
replacement test confirms adding another body preserves the first one's
indices and material metadata.

Full build and 7 focused asset tests pass (`pegase-final-build.log`,
`pegase-tests.log`). All six registered car e2e checks pass after the table
change (`pegase-e2e.log`), covering all three cars' bank loading and repeat-race
transitions. This extends the earlier 375-test full regression run.
Additional cache-mode bank checks pass for 94, 112, and 120
(`pegase-cache-tests.log` and `pegase-cache-bank-*.log`), covering both cached
material orderings. `pegase-cache-track-0.png` / `pegase-cache-track-1.png`
verify the alternate ordering on bank 122 visually, including its blue/yellow
rival livery. These cached views do not show the atlas imagery seen in the
earlier frozen live-import views, a useful lead for later environment work.

Inspected Blender references are `pegase-{player,rival}-{front,rear}-before.png`
and final `pegase-{player,rival}-{front,rear}-refined.png`. The earlier
`*-after.png` files show the rejected first projection experiment.
Live release comparisons are `pegase-chase-0.png` / `pegase-chase-1.png` and
`pegase-track-0.png` / `pegase-track-1.png`. The latter shows a close red rival
and the yellow player from the front, including the original wheels and
exposed front fenders. No new visible gaps or culling failures were observed
in these views. The existing custom-start environment atlas issue remains.

Late CPU submission samples were 0.25–0.26 ms before versus about 0.43 ms
after in chase, and about 0.75 versus 0.92–0.95 ms trackside. These are local
CPU samples, not an isolated GPU/FPS benchmark. Other player grades and the
remaining cars still need work; overall acceptance remains open.

## Esperanza integration, 2026-09-05

Player bank 28 uses 1438 triangles. Rival body 0 has two distinct retail
shapes: the early spoiler uses 1435 triangles in banks 88, 90, 92, 96, 98,
and 100; the raised rear wing uses 1453 triangles in bank 94 and even banks
102–126. Original wheel submeshes 2/3 and far body 4 remain. All three
saved Blender sources re-exported to byte-identical embedded native meshes.
Five explicit cached material layouts preserve each bank's texture ordering.
These replacements coexist with the previously authored body 10 in shared
banks; the bank checks verify both installation messages.

The full build and seven focused asset tests pass (`esperanza-final-build.log`,
`esperanza-tests.log`). All five e2e tests pass (`esperanza-e2e.log`), covering
all 20 Esperanza banks and repeat-race transitions for all four integrated
cars. Additional cache checks pass for banks 88, 96, 94, 104, and 112,
covering all five material layouts (`esperanza-cache-tests.log` and
`esperanza-cache-bank-*.log`). Blender comparisons are
`esperanza{,-rival,-rival-late}-blender-comparison.png`, each showing front
and rear before/after. Inspected release screenshots are
`esperanza-{chase,track}-{1,2}-comparison.png`, with original left and authored
right. They cover both rival variants and the player, preserving liveries,
wheels, and spoiler shapes without new visible gaps in these views. The
previously observed custom-start environment atlas issue occurs before and
after this change.

Late class-2 CPU submission samples were 0.25–0.26 ms before versus
0.36–0.37 ms after in chase, and 0.73–0.74 versus 0.85–0.86 ms trackside.
These are local CPU samples, not an isolated GPU/FPS benchmark. Other player
grades and remaining cars still use original geometry.

## Acceron integration, 2026-09-05

Player bank 38 uses 1705 triangles; rival body 5 uses 1337 triangles in
banks 96, 98, and 100. These retail rival bodies have identical face data,
including UVs and materials. Original wheels 7/8 and far body 9 remain.
Both saved Blender sources re-exported to byte-identical embedded meshes.
The shared bank now installs Acceron, Erriso, and Esperanza together.

The full build and seven focused asset tests pass (`acceron-build.log`,
`acceron-tests.log`). Both e2e tests pass (`acceron-e2e.log`), covering all
three banks and a finish/repeat transition. Cache-mode checks also pass for
all three banks (`acceron-cache-tests.log`, `acceron-cache-bank-*.log`). Each
bank check requires installation of all three authored rival bodies.
Inspected Blender comparisons are
`acceron-blender-comparison.png` and `acceron-rival-blender-comparison.png`,
each containing front and rear before/after. Release comparisons are
`acceron-chase-comparison.png` and `acceron-track-comparison.png`, originals
on the left. The close front rival view retains the paired hood stripes,
lights, bumper, wheels, and side decals. No new visible gaps or culling
failures appeared in these views. Existing environment atlas imagery is
present with both original and authored cars.

Late CPU submission samples were 0.26–0.27 ms before versus 0.35–0.36 ms
after in chase, and about 0.75 versus 0.86–0.88 ms trackside. These are local
CPU samples, not an isolated GPU/FPS benchmark. Other player grades and
remaining cars continue to use original geometry.

## Bayonet integration, 2026-09-05

Player bank 46 uses 1586 triangles; rival body 5 uses 1543 triangles in
even banks 102–110. All five original rival bodies have identical face data,
including UVs and materials. Original wheels 7/8 and far body 9 remain.
Saved Blender player and rival sources re-exported to byte-identical embedded
native meshes. Bayonet shares track banks with authored Abeille and Esperanza.

The full build and seven focused asset tests pass (`bayonet-build.log`,
`bayonet-tests.log`). Both e2e tests pass (`bayonet-e2e.log`), covering all
five banks and a finish/repeat transition. Cache-mode checks also pass for
all five banks (`bayonet-cache-tests.log`, `bayonet-cache-bank-*.log`). Each
bank check requires installation of Bayonet, Abeille, and Esperanza together.
Inspected Blender comparisons are
`bayonet-blender-comparison.png` and `bayonet-rival-blender-comparison.png`,
each with front and rear before/after. Inspected release comparisons are
`bayonet-chase-comparison.png` and `bayonet-track-comparison.png`, originals
on the left. They retain the long nose, hood recesses, center stripe, lights,
engine cover, exhausts, and wheels without new visible gaps or culling
failures in these views. Existing environment atlas imagery occurs before
and after the change.

Late CPU submission samples were 0.25–0.26 ms before versus about 0.37 ms
after in chase, and about 0.74 versus 0.85–0.86 ms trackside. These are local
CPU samples, not an isolated GPU/FPS benchmark. Other player grades and
remaining cars continue to use original geometry.

## Hijack integration, 2026-09-05

Player bank 52 uses 1831 triangles. Rival body 5 uses 1253 triangles in banks
94 and even banks 112–126. Two authored rival sources preserve the retail
UV differences between banks 112–118 and banks 94/120–126, including the
front panel and side liveries. Each uses its own cached material map.
Original wheels 7/8 and far body 9 remain. All three saved Blender sources
re-exported to byte-identical embedded meshes.

The full build and seven focused asset tests pass (`hijack-build.log`,
`hijack-tests.log`). Both e2e tests pass (`hijack-e2e.log`), covering all nine
banks and a finish/repeat transition. Cache checks pass for banks 94, 112,
and 120, covering both material layouts (`hijack-cache-tests.log` and
`hijack-cache-bank-*.log`). Bank checks require installation of Hijack,
Pegase, and Esperanza together. Inspected Blender comparisons are
`hijack-blender-comparison.png`, `hijack-rival-blender-comparison.png`, and
`hijack-rival-alternate-blender-comparison.png`, each containing front and
rear before/after. Other player grades and remaining cars remain original.

Inspected release comparisons are `hijack-{chase,track}-{3,4}-comparison.png`,
with originals on the left. These cover both rival UV variants from the
front and rear and the green player, preserving liveries, bed, braces,
lights, and wheels. No new visible gaps or culling failures appeared in
these views. Existing environment atlas imagery is present before and after.
Late class-4 CPU submission samples were 0.26–0.27 ms before versus about
0.37 ms after in chase, and about 0.76 versus 0.89 ms trackside. These are
local CPU samples, not an isolated GPU/FPS benchmark.

## Fatalita integration, 2026-09-05

Player bank 56 uses 2146 triangles; rival body 15 uses 1622 triangles in
even banks 102–110. All five retail rival bodies have identical face data,
including UVs and materials. Original wheels 17/18 and far body 19 remain.
Both saved Blender sources reproduce the embedded native meshes byte for
byte. These banks now contain four authored bodies: Fatalita, Bayonet,
Abeille, and Esperanza.

The full build and seven focused asset tests pass (`fatalita-build.log`,
`fatalita-tests.log`). Both e2e tests pass (`fatalita-e2e.log`), covering all
five banks and a finish/repeat transition. Cache-mode checks pass for all
five banks (`fatalita-cache-tests.log`, `fatalita-cache-bank-*.log`). The bank
checks require all four authored bodies to be installed together.
Inspected Blender comparisons are
`fatalita-blender-comparison.png` and `fatalita-rival-blender-comparison.png`,
each containing front and rear before/after. Inspected release comparisons
are `fatalita-chase-comparison.png` and `fatalita-track-comparison.png`,
originals on the left. They preserve front and rear lighting, bumper,
spoiler, side intakes, exhausts, and wheels without new visible gaps in
these views. The existing custom-start environment atlas issue and dark
rival wheels occur with original and authored bodies.

Late CPU submission samples were 0.26–0.27 ms before versus about 0.37 ms
after in chase, and about 0.72 versus 0.87 ms trackside. These are local CPU
samples, not an isolated GPU/FPS benchmark. Other player grades and remaining
cars still need work.

## Istante integration, 2026-09-05

Player bank 62 uses 1804 triangles; rival body 15 uses 1500 triangles in
even banks 112–118. The four retail rival bodies have identical geometry,
UVs, and materials. The player uses its own palette map. Original wheels
17/18 and far body 19 remain. Both saved Blender sources re-exported to
byte-identical embedded meshes.

The full build and seven focused asset tests pass (`istante-build.log`,
`istante-tests.log`). Both e2e tests pass (`istante-e2e.log`), covering all
four banks and a finish/repeat transition. Cache-mode checks also pass for
all four banks (`istante-cache-tests.log`, `istante-cache-bank-*.log`). Each
bank check requires Istante, Hijack, Pegase, and Esperanza together.
Inspected Blender comparisons are
`istante-blender-comparison.png` and `istante-rival-blender-comparison.png`,
each containing front and rear before/after. Release comparisons are
`istante-chase-comparison.png` and `istante-track-comparison.png`, originals
on the left. They retain the wedge silhouette, hood panels, spoiler, lamps,
rear exhausts, liveries, and wheels without new visible gaps or culling
failures in these views. The existing environment atlas issue remains.

Late CPU submission samples were 0.26–0.27 ms before versus about 0.37 ms
after in chase, and 0.76–0.77 versus 0.87–0.88 ms trackside. These are local
CPU samples, not an isolated GPU/FPS benchmark. Other player grades and
remaining cars still need work.

## Ghepardo integration, 2026-09-05

Player bank 66 uses 1929 triangles; rival body 15 uses 1422 triangles in
banks 94 and even banks 120–126. All five retail rival bodies have identical
face data, including UVs and materials. Explicit mappings preserve palette
0x7909 on both pages 12 and 13. Original wheels 17/18 and far body 19 remain.
Both saved Blender sources re-exported to byte-identical embedded meshes.

The full build and seven focused asset tests pass (`ghepardo-build.log`,
`ghepardo-tests.log`). Both e2e tests pass (`ghepardo-e2e.log`), covering all
five banks and a finish/repeat transition. Cache checks also pass for all
five banks (`ghepardo-cache-tests.log`, `ghepardo-cache-bank-*.log`). Bank
checks require Ghepardo, Hijack, Pegase, and Esperanza together.
Inspected Blender comparisons are
`ghepardo-blender-comparison.png` and `ghepardo-rival-blender-comparison.png`,
each containing front and rear before/after with the whole body in frame.
Release comparisons are `ghepardo-chase-comparison.png` and
`ghepardo-track-comparison.png`, originals on the left. The long nose,
cockpit, spoiler, lights, rear outlets, liveries, and wheels remain without
new visible gaps or culling failures in these views. The existing environment
atlas issue remains.

Late CPU submission samples were 0.19–0.20 ms before versus 0.28–0.29 ms
after in chase, and 0.60–0.61 versus about 0.71 ms trackside. These are local
CPU samples, not an isolated GPU/FPS benchmark. Remaining special cars,
other player grades, and duplicate rival representations still need work.

## Vainqure integration, 2026-09-05

Player bank 68 installs 2284 triangles; rival body 10 installs 2156 triangles
in even banks 128–134. The four retail rival bodies have identical face data
and material ordering. The player retains its large wing and the rival its
original wingless shape. Original wheels 12/13 and far body 14 remain.
Both saved Blender sources reproduce the embedded meshes byte for byte.

Special-class starters use activeFlag 0; only -1 marks an inactive slot.
The custom-start scenario incorrectly tested the flag as a boolean, skipping
these starters and attempting to place inactive slots. Placement and freeze
now use the retail -1 sentinel. The special-class bank test requires all
four active starters to be positioned and no inactive slot to be touched.

Full build and nine focused regression tests pass
(`vainqure-final-build.log`, `vainqure-regression.log`), including starting
grid and lap progress alongside the asset checks. Inspected Blender comparisons
are listed below. All four e2e tests pass (`vainqure-e2e.log`): ordinary
direct grid and repeat-race checks, all four special banks, and a special-class
repeat race. Cache-mode checks pass for all four special banks
(`vainqure-cache-tests.log`, `vainqure-cache-bank-*.log`). Blender comparisons
are `vainqure-blender-comparison.png` and
`vainqure-rival-blender-comparison.png`, each showing front and rear before/after.
Accepted release comparisons are `vainqure-fixed-chase-comparison.png` and
`vainqure-fixed-track-comparison.png`, originals on the left. They preserve
the cockpit, wing, front fenders, lights, exhausts, and wheels without new
visible gaps in these views. Earlier images without `fixed` demonstrate the
rejected scenario placement and do not verify close rival geometry.

Late local CPU submission samples were about 0.23 ms before versus
0.21–0.26 ms after in chase, and 0.73–0.74 versus 0.75–0.76 ms trackside.
These fluctuating samples are not an isolated GPU/FPS benchmark. The existing
environment atlas issue remains; remaining cars and variants still need work.

## Bulshade integration, 2026-09-05

Player bank 70 installs 3173 triangles. Red rival body 0 and green rival
body 5 each install 2665 triangles in even banks 128–134, alongside Vainqure.
The two rival variants retain separate UVs and palette mappings. Each variant's
retail face data is identical across the four banks. Original wheels 2/3 and
7/8, and far bodies 4/9, remain. All three saved Blender sources reproduce
the embedded meshes byte for byte.

The initial 1.6-unit bevel produced acute fender spikes outside the existing
four-unit bounds tolerance. It was rejected. A 0.6-unit bevel passes the
unchanged bounds validation. Full build and seven focused asset tests pass
(`bulshade-refined-build.log`, `bulshade-refined-tests.log`). Accepted Blender
front/rear comparisons are `bulshade-blender-refined.png`,
`bulshade-rival-blender-refined.png`, and
`bulshade-rival-alternate-blender-refined.png`.

Both e2e tests pass (`bulshade-e2e.log`): all four special banks load both
rival variants, and a finish/repeat cycle loads the authored cars again.
Cache-mode checks also pass for all four banks (`bulshade-cache-tests.log`,
`bulshade-cache-bank-*.log`). Bank tests require Vainqure to remain installed
alongside both Bulshade bodies.

Inspected release comparisons are `bulshade-red-chase-comparison.png`,
`bulshade-red-track-comparison.png`, `bulshade-green-chase-comparison.png`,
and `bulshade-green-track-comparison.png`, originals on the left. They retain
the cabin, grille, fenders, rear panel, liveries, and wheel placement without
new visible gaps or missing surfaces in these views. The existing environment
atlas issue remains visible before and after.

Late local CPU submission samples were 0.23–0.24 ms before versus about
0.29 ms after in chase, and 0.73–0.74 versus 0.79–0.80 ms trackside.
These are limited CPU samples, not an isolated GPU/FPS benchmark. Squaldon,
other player grades, and duplicate rival representations still need work.

## Squaldon integration, 2026-09-05

Player bank 72 installs 1340 triangles; rival body 15 installs 1149 triangles
in even banks 128–134. All four retail rival bodies have identical face data
and material ordering. Original wheels 17/18 and far body 19 remain.
Both saved Blender sources reproduce the embedded meshes byte for byte.

Full build and seven focused asset tests pass (`squaldon-build.log`,
`squaldon-tests.log`). Inspected Blender front/rear before/after comparisons
are `squaldon-blender-comparison.png` and
`squaldon-rival-blender-comparison.png`. Release comparisons are
`squaldon-chase-comparison.png` and `squaldon-track-comparison.png`, originals
on the left. The long nose, cabin, lamps, rear panel, liveries, and wheels
remain without new visible gaps or missing surfaces in these views.
The existing environment atlas issue remains visible before and after.

Late local CPU submission samples were about 0.24 ms before versus
0.29–0.30 ms after in chase, and about 0.73 versus 0.79 ms trackside.
These are limited CPU samples, not an isolated GPU/FPS benchmark.
All thirteen base player cars now have authored bodies; other player grades,
duplicate rival representations, and the final fleet audit remain incomplete.
The catalog contains 32 player variants: base indices are
0,4,7,9,14,18,21,23,26,28,29,30,31. The nineteen non-base variants still need
inspection and integration; base-car coverage does not establish completion.
Their retail face records have distinct hashes and often different face counts;
do not replace them blindly with a base body. Direct-boot scenarios currently
have no model-variant override, so grade verification also needs a tested way
to select the intended variant before loading car assets.

Both e2e checks pass (`squaldon-e2e.log`), covering all four banks and a
finish/repeat cycle. Each bank requires Squaldon, Vainqure, and Bulshade to
remain installed together.
Cache-mode checks also pass for all four banks (`squaldon-cache-tests.log`,
`squaldon-cache-bank-*.log`).

The remaining body-15 audit now has an inspected original Blender view:
`esperanza-rival-duplicate-front-before.png`. Despite that provisional filename,
it shows a green number-84 touring sedan with a large wing; its identity must
be compared against the established car sources before authoring or naming it.
The original is imported in the temporary `Esperanza rival-duplicate` scene.

## Additional Esperanza integration, 2026-09-05

Three additional authored sources preserve the white/blue number-30 body 5,
red number-25 body 10, and green number-84 reference body 15 with its tall
wing. Bodies 5/10 install 1421/1453 triangles in banks 88/90/92. Body 15
installs 1434 triangles in banks 88/90/92/96/98/100. Its latter bank group
uses different vertex and normal indices, but resolved positions, normals,
UVs, colors, and texture identities agree. Separate cache maps account for
different material slot numbers. All original wheel and far-body parts remain.

Full build and seven focused tests pass
(`esperanza-duplicate-final-build.log`, `esperanza-duplicate-final-tests.log`).
All three saved Blender sources reproduce the embedded meshes byte for byte.
Inspected Blender comparisons are
`esperanza-rival-duplicate-blender-comparison.png`,
`esperanza-rival-slot1-blender-comparison.png`, and
`esperanza-rival-slot2-blender-comparison.png`, each showing front/rear
before and after. They preserve the distinct wings, panel colors, decals,
and lamps. Runtime race palettes can differ from the reference PNGs.

Inspected release comparisons are `esperanza-duplicate-chase-comparison.png`,
`esperanza-duplicate-track-comparison.png`,
`esperanza-duplicate-class0-track-comparison.png`, and the chase/track pairs
`esperanza-slot1-*-comparison.png` and `esperanza-slot2-*-comparison.png`.
Originals are on the left. They preserve the distinct wings, lamps, panel
textures, liveries, and wheel placement without new visible gaps or missing
surfaces in these views. The body-15 track views cover both first-class
number 84 and second-class number 92 palettes. Its initial chase pair predates
the body-5/10 additions; the other first-class pairs show all four authored
bodies installed together. The environment atlas problem remains.

Late local CPU submission samples for the final four-body bank were about
0.26 ms before versus 0.35 ms after in chase, and 0.74–0.75 versus
0.86–0.87 ms trackside. The earlier body-15 pair was 0.26–0.27 versus
0.31 ms chase and 0.78–0.79 versus 0.86–0.87 ms trackside. Some captures
overlapped Blender work; these are limited CPU observations, not an isolated
GPU/FPS benchmark.

A registry audit against parts 0/5/10/15 of every even track bank 88–134
found no remaining uncovered entries after these additions. This proves
registration coverage only. Extra bank objects beyond the first four models,
the nineteen other player variants, and final fleet verification remain open.
Special bank 128 part 20 is a helicopter, identified visually in
`special-bank-extra-front-before.png`; special banks have 23 total parts.
Ordinary banks have 38 parts: three additional car assemblies at 20/25/30,
then the helicopter at 35. The early bank-88 originals are imported as
`EarlyExtra20 reference`, `EarlyExtra25 reference`, and `EarlyExtra30 reference`.
Their inspected `early-extra-{20,25,30}-front-before.png` views show distinct
compact hatchback rivals (numbers 91/16/44), which still require authoring.
Later banks have different face counts for the first two of these bodies;
do not assume all ordinary banks share the early version.
Resolved face-data hashes (positions, normals, colors, UVs, texture identity)
split each extra body into three groups: 88/90/92/96/98/100;
102/104/106/108/110/112/114/116/118; and 94/120/122/124/126.
Inspect the actual differences before deciding whether separate geometry,
palette maps, or both are required.

All three e2e tests pass (`esperanza-duplicate-e2e.log`): six banks and
finish/repeat transitions in both the first and second classes. Transition
checks explicitly require four and two authored Esperanza rival bodies,
respectively, alongside the player.
Cache-mode checks also pass for all six banks
(`esperanza-duplicate-cache-tests.log`, `esperanza-duplicate-cache-bank-*.log`).

## Early compact rival integration, 2026-09-05

Three descriptive compact source names cover the non-selectable early
hatchbacks: Compact A/B/C replace parts 20/25/30 in banks
88/90/92/96/98/100 with 1172/1092/1038 triangles. Original wheels and far
bodies remain. Slots 4/5 share A with two palette variants, 6/7 share B
with two, and 8/9/10 share C with three. Two cache-map layouts preserve
the original page-10 details and page-14 palettes across the six banks.

The full build and seven focused asset tests pass (`compact-early-build.log`,
`compact-early-tests.log`). Saved Blender sources reproduce all three
embedded meshes byte for byte. Inspected Blender front/rear comparisons
are `compacta-early-blender-comparison.png`,
`compactb-early-blender-comparison.png`, and
`compactc-early-blender-comparison.png`.

Release comparisons cover all seven model slots from the trackside camera:
`compact-early-model{4,5,6,7,8,9,10}-track-comparison.png`. Additional chase
pairs are `compact-early-model{4,6,8}-chase-comparison.png`. All ten pairs
were inspected, originals on the left. They preserve the distinctive fronts,
rear lamps, spoilers, liveries, and wheel placement without new visible gaps
or missing surfaces in these views. The environment atlas issue remains.

Late local CPU submission observations were 0.25–0.28 ms before versus
0.32–0.34 ms after in chase. Trackside samples ranged 0.68–0.78 before
versus 0.81–0.84 ms after. Captures overlapped some Blender work and these
are limited CPU observations, not an isolated GPU/FPS benchmark.

The six later compact sources have been authored and inspected in Blender
only: `compact{a,b,c}-rival-{middle,late}.blend` and matching OBJ/MTL files
are local drafts. Middle references come from bank 102 (`102_OVAL2_2ND_b3`),
late from 94. Each A draft reports 1092 triangles, B/C 1106. Their
`compact{a,b,c}-{middle,late}-blender-comparison.png` views were inspected.
All six draft OBJs pass the C converter and their saved-source round trips
produce byte-identical native files (`compact{a,b,c}-{middle,late}-draft.rmesh`
and matching roundtrip files in the evidence directory).
They still need native bounds/material validation, integration,
live-game comparisons, and bank/transition/cache tests. Do not claim these
later banks are upgraded yet. The nineteen other player variants and final
fleet audit also remain incomplete.

Both early-compact e2e tests pass (`compact-early-e2e.log`). The bank check
places all seven palette variants and requires the player, Esperanza rivals,
and all three compact bodies. The repeat-race check requires all three compact
bodies and the player to load successfully.
All six cache-mode bank checks pass (`compact-early-cache-tests.log`,
`compact-early-cache-bank-*.log`).

## Later compact integration, 2026-09-05

Six sources preserve the middle and late versions of Compact A/B/C. Each
group installs 1092/1104/1105 native triangles at parts 20/25/30. Middle
sources cover even banks 102–118; late sources cover 94/120/122/124/126.
Three cache layouts preserve material slot differences. Original wheel and
far-body parts remain. Saved-source round trips match all six embedded meshes.
The seven focused asset checks passed (`compact-later-tests.log`).

All twenty release comparisons were inspected: for both `middle` and `late`,
`compact-{group}-model{4,5,6,7,8,9,10}-track-comparison.png` covers all seven
palettes, and `compact-{group}-model{4,6,8}-chase-comparison.png` covers each
body from behind. Originals are on the left. The distinct fronts, wings,
lamps, liveries, and wheel placement remain without new visible gaps or
missing surfaces in these views. The six Blender comparisons were inspected
in the preceding stage. The environment atlas issue remains.

Representative late local CPU submission samples for model 4 were about
0.26 ms before versus 0.32–0.34 ms after in chase; trackside was
0.75–0.77 versus about 0.84–0.85 ms. Captures overlapped some Blender work;
these are limited CPU observations, not an isolated GPU/FPS benchmark.

Scenario option `race.variant` now selects a zero-based player asset variant
before loading. It is bounded by the selected car's catalog range; omission
preserves the normal setup and invalid indices produce a diagnostic.
This enables verification of the nineteen non-base player assets without
editing saves or silently selecting another car's bank.

## Player upgrade variants, 2026-09-05

All nineteen player upgrade variants now have OBJ/MTL/Blender sources:
`erriso-grade{1,2,3}`, `abeille-grade{1,2}`, `pegase-grade1`,
`esperanza-grade{1,2,3,4}`, `acceron-grade{1,2,3}`, `bayonet-grade{1,2}`,
`hijack-grade1`, `fatalita-grade{1,2}`, and `istante-grade1`.
All original and before/after Blender views were inspected, and every saved
source re-exports to a byte-identical C-converted native mesh. These player
upgrades are embedded, bounds/material validated, and visually checked in
the actual release executable. All bank, transition, and cache tests passed.
Fatalita's original wing has coincident opposite-facing faces with different
UVs; its Blender comparison exposes the top logo after beveling. Both variants'
modern front/rear comparisons preserve the visible wing surfaces without new
holes in those views.
The final fleet audit remains outstanding.

The current later-compact build passed all 375 non-E2E regression tests
(`compact-later-regression.log`, 1217.60 seconds). All four E2E tests passed
(`compact-later-e2e.log`, 274.56 seconds): fourteen live-import banks,
seven variant-selector cases, and repeat-race transitions in classes 2 and 4.
The fourteen extracted-cache banks also passed (`compact-later-cache-tests.log`,
individual `compact-later-cache-bank-*.log` traces). A registry audit confirms
all seven car bodies in each ordinary bank 88–126 and all four in each special
bank 128–134 are registered exactly once; the remaining submeshes are wheels,
distance models, and the helicopter.

Player upgrade integration is complete in commit `674b71ef8`. All nineteen
OBJ sources are embedded and mapped to their individual player banks; seven
focused asset/pipeline tests passed (`player-grades-tests-triangulated.log`).
The tests caught a bevel overshoot in Esperanza grade 3 (reduced from 1.6 to
1.2 units) and undefined Hijack grade 1 corner normals (saved as explicit
triangles). Both corrected saved sources re-export identically through C.
All 38 real-game comparison pairs (76 captures) were inspected. Accepted comparison
filenames use `*-gradeN-{front,chase}-verified-comparison.png`; the front view
uses course 1, camera 2, player track point 118, timer 350, and distant rivals.
Earlier timer-120 track views are unsuitable because of the starting lights.
The player-grade bank and repeat-race E2E tests passed (174.10 and 615.01
seconds; `player-grades-e2e.log`). All nineteen extracted-cache banks passed
as well (`player-grades-cache-tests.log` and `player-grade-cache-bank-*.log`).

Across the nineteen variants, the last two CPU submission samples per capture
were 0.827–0.878 ms before and 0.946–1.015 ms after in the front view;
chase samples were 0.247–0.282 ms before and 0.356–0.436 ms after. These compare
all original versus all authored cars in the scene, including rivals, and
are CPU submission observations rather than isolated GPU/FPS measurements.

## Final local application audit

The fleet work is complete. The short delivery report is
[car_upgrade_summary.md](car_upgrade_summary.md). The final release build passed
(`player-grades-release-build.log`), and the tested clean copy is
`build/package-audit/Rage Racer.app`. Its executable links only macOS system
libraries/frameworks; Blender and Python are not runtime dependencies.

LaunchServices (`open -W -n`) started this bundle without an adjacent native
asset cache, without renderer or asset-source overrides, and using the
remembered CUE. The log confirms modern rendering and automatic C generation;
the inspected `fleet-attract-clean.png` shows the natural tunnel attract scene
with authored rivals. Evidence: `fleet-attract-clean.log`. The same bundle
also loaded a standalone Track 01 BIN, installed Erriso grade 3 (bank 16) and
all seven rival bodies in bank 96, and reached scene 12, timer 350.
The modern capture `fleet-track01-modern.png` was inspected. Classic completed
the same scenario (`fleet-track01-classic.log`) without authored replacements.
The release capture hook only supports modern, so the classic run has no
frame capture; a desktop capture attempt returned black and is not visual
evidence.

The original remembered CUE was in Downloads, where macOS blocked the app's
file-open request. The provided image was cloned to the ignored real directory
`build/launch-disc`, and the remembered CUE setting now points there. Original
disc files remain unchanged. The old setting is backed up in
`build/car-upgrade-evidence/disc-cue-path-before-app-audit.txt`. No system privacy
permissions were changed. Interactive picker selection was not automated;
these launches validate the remembered-disc path and explicit Track 01 input.

Custom mid-course starts still show the existing environment atlas issue with
both original and authored cars. The natural clean-import attract view looked
correct, which does not establish correctness for every course or viewpoint.
This remains outside the completed car geometry work. Windows and Linux were
not run on this Mac. Local commits only; no push or publication. Unrelated
`.claude/` and `imgui.ini` remain untouched.
