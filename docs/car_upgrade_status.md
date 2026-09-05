# Car upgrade work in progress

Goal remains `goal.md`. Base Erriso and Abeille player bodies and their rival
representations are integrated. Other player grades and cars retain their
original geometry; the full set is still in progress.

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

Keep original data and the unrelated `.claude/` and `imgui.ini` files.
Commit verified stages locally; do not push or publish.
