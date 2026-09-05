# Car upgrade work in progress

Goal remains `goal.md`. Base Erriso, Abeille, Pegase, Esperanza, Acceron, Bayonet,
Hijack, Fatalita, Istante, Ghepardo, Vainqure, and Bulshade player bodies and
their rival representations are integrated. Other player grades and cars retain their
original geometry; the full set is still in progress.

The remaining audit includes duplicate rival representations: body 15 in
banks 88/90/92/96/98/100 has Esperanza-like geometry (90 faces, bounds
[-144,-140,-497] to [144,31,138]) and remains original. Inspect its livery
and role before selecting an authored replacement; body 0 coverage alone
does not prove the whole fleet is upgraded.

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

Keep original data and the unrelated `.claude/` and `imgui.ini` files.
Commit verified stages locally; do not push or publish.
