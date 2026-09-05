# Car mesh authoring

`rage-mesh-obj` is a C tool built with the normal CMake project. It bridges
native meshes and Blender's standard Wavefront OBJ importer/exporter:

```sh
cmake --build build/release --target rage-mesh-obj mesh_obj_tests
ctest --test-dir build/release -R '^mesh_obj$' --output-on-failure
build/release/rage-mesh-obj export bank.rmesh 0 body.obj
build/release/rage-mesh-obj import body-edited.obj body.rmesh
```

Export selects one native submesh. Import produces a one-submesh native mesh;
it does not install it in a game bank. The export also writes `body.obj.mtl`
with neutral materials. Texture images remain owned by the disc importer.
Assign reference textures in Blender for inspection without renaming the
`rage_N` materials. Their numeric suffix is the **entire** native material
word, including face metadata and the untextured sentinel. Taking just the
low 16 bits discards depth-bias information.

Use Blender OBJ import/export axes **-Z forward, Y up**, scale **1**. Export
only the intended object with **UVs, normals, colors, materials, modifiers,
and triangulation enabled**. The tool rejects faces without UV/normal
indices, polygons larger than triangles, non-finite attributes, out-of-range
indices, and unknown material names. It accepts positive and relative OBJ
indices. Object/group/smoothing records are accepted; material sidecar paths
are ignored on import because materials come from the game.

Coordinates and winding stay in native Y-up space. OBJ V is inverted relative
to native texture V. Blender writes sRGB OBJ vertex colors, whereas native
vertex multipliers are linear; the converter handles that transformation.
The regression checks all 256 channel values and an indexed mesh round trip,
including normals, UVs, winding, color, and textured/untextured materials.
Export currently requires opaque vertex alpha; it fails rather than dropping
non-opaque alpha. Geometry inspection and in-game validation remain necessary.

The existing assetbrowser glTF exports are useful textured references, but
they omit native face metadata. Start production geometry from the native
OBJ export when that metadata must survive.

## Integrated Erriso source

Open `assets/cars/erriso.blend` and select `Erriso_body` in scene `Scene`.
Export selected objects to `assets/cars/erriso-body.obj` using the options
above. Keep the modifier stack enabled: bevel, weighted normals, vertex-color
clamp, and transfer of the original panel normals. The clamp is necessary
because bevel interpolation can overshoot the legal color range. Reference
textures are packed in the blend file. The original body and wheel sources
remain available as hidden objects.

The normal CMake build converts the OBJ to native bytes and embeds them in
the executable. End users need neither Blender nor an additional asset file.
Modern rendering replaces base Erriso's body submesh while retaining its
wheel bank, textures, paint behavior, and gameplay data. Disc-imported
material slots are resolved by their texture-page/palette identity; the
prebuilt cache uses its existing slot numbering. `modern.authored_cars=0`
is a development-only before/after comparison override. The default is on.
For the rival, select only `Erriso_rival_body` in scene `Erriso rival` and
export to `assets/cars/erriso-rival.obj` with the same settings and modifier
stack. Its body replaces submesh 10 in banks 96, 98, and 100; far body 14 and
wheel submeshes 12/13 remain original. Player preview wheel objects are for
inspection only and must not be included in the body export. Other player
grades retain original geometry.

`tests/scenarios/authored_erriso.ini` provides a repeatable live comparison.
Rival model slots are course-specific: use slot 2 on course 0, slot 1 on
course 1, and slot 0 on course 2. These are not showroom car IDs. Set
`modern.authored_cars=0` for the original geometry and use camera 1 (chase)
or 2 (trackside) with the same custom start for comparable views.

## Integrated Abeille source

Open `assets/cars/abeille.blend`, scene `Abeille player`, and export only
`Abeille_body` to `assets/cars/abeille-body.obj`. The rival has a separate
`assets/cars/abeille-rival.blend`, scene `Abeille rival`; export only
`Abeille_rival_body` to `assets/cars/abeille-rival.obj`. Use the same OBJ
settings above. CMake embeds both automatically. Player bank 18 and rival
banks 102, 104, 106, 108, and 110 are covered; other player grades are original.

These export objects contain the final baked mesh. Hidden original and bevel
source objects retain the construction references. Color attributes were
converted to the CORNER domain before welding coincident vertices, preserving
different panel colors at a shared position. After beveling, UVs and original
corner normals were projected from source triangles with the same material
and compatible face direction. An unrestricted nearest-face transfer picked
adjacent panels and visibly distorted the windshield. Export the final body,
not the hidden bevel experiment or its disabled UV-transfer modifier.
The final export mesh splits vertices at color discontinuities and converts
colors back to POINT. This is essential: Blender's OBJ writer otherwise
averages corner colors into a shared vertex. The embedded-asset test checks
both dark and white native colors at one bumper/body seam.

`tests/scenarios/authored_abeille.ini` uses class 2 and course 0. Rival model
slots for courses 0/1/2/3 are 2/1/0/3. Camera 2 uses the retail camera node,
which can itself select a chase view; check the resulting image rather than
assuming a different camera number always provides a different angle.
Use the release executable for modern screenshots. The smoke executable's
`capture.path` reads the compatibility framebuffer, although its execution
and asset-loading logs remain useful for regression checks.

## Integrated Pegase source

Open `assets/cars/pegase.blend`, scene `Pegase player`, and export only
`Pegase_player_body` to `assets/cars/pegase-body.obj`. For the rival, open
`pegase-rival.blend`, scene `Pegase rival`, and export `Pegase_rival_body` to
`pegase-rival.obj`. The settings above and the normal CMake build apply.
The final bodies are baked; original and bevel-source objects remain hidden.

The bevel source retains an integer FACE attribute, `RetailFace`, identifying
each original triangle. UVs and normals are projected from that triangle,
excluding zero-area source triangles. Keeping face identity avoids crossing
an atlas seam even when adjacent triangles use the same material. Vertices
are split at color seams before exporting POINT colors, as for Abeille.
The hood decal retains its original two triangles and material metadata;
its coplanar appearance in Blender does not reproduce the native renderer's
depth treatment. The embedded-asset test verifies its positions and UVs.

Player bank 24 and rival body 10 in banks 94 and 112–126 (even IDs) are
replaced. Banks 94 and 120–126 use a different prebuilt material ordering
from banks 112–118. `authored_car_data.h` records both mappings; live import
resolves texture-page/palette identities. Original wheels and far body 14
remain. `authored_pegase.ini` uses class 3, course 0, rival slot 2.
For a close front rival view, use course 1, rival grid slots all 1, camera 2,
and `start.rival_track_points=119`.

## Integrated Esperanza sources

Open `esperanza.blend`, scene `Esperanza player`, and export only
`Esperanza_player_body` to `esperanza-body.obj`. Rival sources are
`esperanza-rival.blend` / `Esperanza_rival_body` and
`esperanza-rival-late.blend` / `Esperanza_rival_late_body`; export to their
matching OBJ filenames. Files are under `assets/cars/`. The same settings,
baked meshes, hidden construction references, and `RetailFace` transfer apply.

Player bank 28 and rival body 0 in even banks 88–126 are replaced. Early
banks 88/90/92/96/98/100 retain their distinct spoiler; the other banks use
the raised-wing source. Far body 4 and wheels 2/3 remain original. Five
per-bank cached slot layouts are recorded in `authored_car_data.h`.
`authored_esperanza.ini` alternates Esperanza and Erriso in class 1, course 0.
Esperanza rival slots for courses 0/1/2/3 are 0/2/1/1. For a close front view,
use course 1, camera 2, grid slots 2/1 alternating, and rival points 119/121.
