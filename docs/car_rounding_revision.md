# Car geometry and surface revision

The first pass was too conservative: it retained the original wheels and only
slightly beveled the bodies. This revision establishes a more visible baseline
on **base Esperanza, player bank 28**. Other cars and Esperanza upgrades still
use the previous pass; their completion in the fleet report does not mean they
have received this stronger revision.

## Implemented

- Four circular wheel arches, replacing the original straight upper segments
  with subdivided arcs of radius 51 model units. Original body bounds, panel
  texture identities, and corner colors are retained.
- All twenty wheel/axle submeshes in this bank use 64 angular segments,
  rounded tire shoulders, and recessed rim geometry. Radius 43, axle widths,
  steering, rotation, tire selection, and blur-bank selection remain tied to
  the original game assembly. Physics and collision data are unchanged.
- Glass, paint, rubber, and rim metal have independent renderer materials.
  Glass is dark, smooth and nonmetallic; paint is less glossy than the previous
  default; rubber is rough; rims retain specular lighting. Glass remains opaque
  tinted glazing, with no newly modeled interior or transmission pass.
- The normal compiled build embeds these meshes. The C importer supplies the
  original disc textures and paint variants automatically; classic is unchanged.

## Saved source and export

Open `assets/cars/esperanza-rounded.blend`, scene `Esperanza player`.
Export `Esperanza_rounded_body` to `esperanza-rounded.obj`, and
`Esperanza_rounded_wheel_2` through `_21` to `esperanza-wheel2.obj` through
`esperanza-wheel21.obj`. Export each object separately with the existing
-Z forward, Y up, UV/normal/color/material/triangulation settings. Preview
wheel copies are positioned for inspection and must not be exported.

The source includes original geometry and packed reference images. The final
body preserves material assignment through edge subdivision; nearest-face
material reassignment was rejected because it introduced an atlas seam at the
rear arch. Split export vertices preserve corner colors; explicit normals
smooth the arches and tire circumference while retaining panel creases.

Blender materials carry a `surface` custom property. Their `rage_N` numeric
word retains all native face metadata. A textured source slot is encoded as
`original_slot + 64 * surface`, where 1/2/3/4 mean glass/paint/rubber/metal.
The runtime resolves the original page/palette identity and stores a separate
material as `resolved_slot + 4096 * surface`. Texture and paint loading use
the original slot. Untextured sentinel faces retain their original behavior.
Material response is centralized in `src/render/authored_car_surface.c`.

## Verification

- Eight focused C tests passed (`rounded-tests-final.log`), including asset
  bounds, wheel radius and outward normals, surface classes, material registry,
  OBJ conversion, mesh replacement, and car assembly.
- Repeat-race transition passed (`rounded-transition.log` and
  `rounded/authored-transition-3.log`).
- All 21 meshes re-exported from the saved blend reproduce their embedded
  native bytes exactly after C conversion.
- Blender front/rear and release front/chase views were inspected. The final
  front capture confirms the corrected rear-arch texture seam.

Evidence is in `build/car-upgrade-evidence`:

- [In-game detail before/after](../build/car-upgrade-evidence/esperanza-rounded-final-detail-comparison.png)
- [Final front view](../build/car-upgrade-evidence/esperanza-rounded-final-front.png)
- [Blender rear view](../build/car-upgrade-evidence/esperanza-rounded-blender-rear-final.png)

The comparison uses the same scenario, camera, and capture time; original is
left. The detail image only crops and enlarges those actual game captures.
The known environment atlas problem in custom track starts is still present.

## Wheel-well interiors, hood, and glass follow-up

Base Esperanza now includes 316 dark liner triangles: the curved inner walls
and an inboard closure at each of its four wheel openings. The original
cross-car strips were removed because they intersected the new curved wells.
The bottom remains open for the tire. The inner closures sit at lateral
positions ±74, clear of the original wheel assembly and its steering sweep.
They use a matte untextured material with the native metadata flag and the
untextured sentinel (`0x2000ffff`); a bare `0x0000ffff` is not a drawable
untextured material in the native draw builder.

Original panel normals were restored, including the hood. Its positions
had not changed, but recomputed smooth normals had changed its appearance.
The original hood silhouette and UVs remain, with the separate paint response.

All glass now uses one tint. The original window atlas painted different
fixed reflection gradients onto front and side windows; the C material loader
replaces those RGB values only for explicitly authored glass, preserving alpha.
The shader supplies view-dependent reflections, so different window angles can
still reflect different parts of the environment. Paint and other textures
are unaffected by this glass operation.

Eight focused tests passed again (`well-liners-tests-final.log`). New checks
cast sample rays through all four openings to require inward closures with
correct winding, verify their dark vertex colors and native material tag,
check the restored hood normal, and check uniform glass RGB with unchanged
alpha. Re-exporting the saved body through C reproduces the embedded mesh.
Blender inspections include a low camera, 25-degree front steering, the other
side, and a view with front wheels hidden to expose the closures. A final
release capture verifies the native material correction in the actual game.

- [Latest game comparison](../build/car-upgrade-evidence/esperanza-liners-glass-final-comparison.png)
- [Low view with steering](../build/car-upgrade-evidence/esperanza-well-liners-low-steered.png)
- [Wheel-well closure inspection](../build/car-upgrade-evidence/esperanza-well-liners-open-inspection.png)
