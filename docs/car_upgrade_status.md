# Car upgrade work in progress

Goal remains `goal.md`. The first author/export/integration/game/inspection
cycle is verified for base Erriso and its class-2 rival representation.
Other player grades and cars retain their original geometry.

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

Keep original data and the unrelated `.claude/` and `imgui.ini` files.
Commit verified stages locally; do not push or publish.
