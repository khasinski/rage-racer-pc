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

## Integrated Acceron sources

Open `assets/cars/acceron.blend`, scene `Acceron player`, and export only
`Acceron_player_body` to `acceron-body.obj`. The rival source is
`acceron-rival.blend`, scene `Acceron rival`, object `Acceron_rival_body`,
exported to `acceron-rival.obj`. Both use the same baked export settings and
retain hidden originals and bevel construction references. Saved-source
exports reproduce the embedded native meshes byte for byte.

Player bank 38 and rival body 5 in banks 96/98/100 are replaced. The three
retail rival bodies have identical geometry, UVs, and material ordering.
Original wheels 7/8 and far body 9 remain. The player shares Esperanza's
texture identities and cached slot numbers, with its own bank's images.
Rival slots for courses 0/1/2 are 1/0/2. The Acceron scenario contains a
mixed grid to exercise the shared bank's three authored bodies. For a close
front view, use course 1, camera 2, grid 0/2/1 repeating, and rival points
119/121.

## Integrated Bayonet sources

Open `assets/cars/bayonet.blend`, scene `Bayonet player`, and export only
`Bayonet_player_body` to `bayonet-body.obj`. The rival source is
`bayonet-rival.blend`, scene `Bayonet rival`, object `Bayonet_rival_body`,
exported to `bayonet-rival.obj`. The same baked OBJ export settings apply;
hidden originals and construction meshes remain. Both saved sources reproduce
the embedded native meshes byte for byte.

Player bank 46 and rival body 5 in even banks 102–110 are replaced. All five
retail rival bodies have identical geometry, UVs, and material ordering.
Original wheels 7/8 and far body 9 remain. Player texture identities and
cached slots match Esperanza's, with the images supplied by Bayonet's bank.
Rival slots for courses 0/1/2/3 are 1/0/2/2. The scenario uses a mixed grid
with Abeille and Esperanza; a close front view uses course 1, camera 2,
grid 0/2/1 repeating, and rival points 119/121.

## Integrated Hijack sources

Open `assets/cars/hijack.blend`, scene `Hijack player`, and export only
`Hijack_player_body` to `hijack-body.obj`. Rival sources are
`hijack-rival.blend` / `Hijack_rival_body` and
`hijack-rival-alternate.blend` / `Hijack_rival_alternate_body`, exported to
their matching OBJ filenames. The same baked settings apply; originals and
construction meshes remain hidden. All three saved sources reproduce the
embedded native meshes byte for byte.

Player bank 52 and rival body 5 in banks 94 and even banks 112–126 are
replaced. Banks 94 and 120–126 have different front/side UVs and cached
material ordering from banks 112–118, requiring separate authored sources.
Original wheels 7/8 and far body 9 remain. Rival slots for courses 0/1/2/3
are 1/0/2/2. The scenario uses a mixed Hijack/Pegase/Esperanza grid; a close
front view uses course 1, camera 2, grid 0/2/1 repeating, and points 119/121.

## Integrated Fatalita sources

Open `assets/cars/fatalita.blend`, scene `Fatalita player`, and export only
`Fatalita_player_body` to `fatalita-body.obj`. The rival source is
`fatalita-rival.blend`, scene `Fatalita rival`, object `Fatalita_rival_body`,
exported to `fatalita-rival.obj`. Both use the same baked export settings
and retain hidden construction references. Saved-source exports reproduce
the native meshes byte for byte.

Player bank 56 and rival body 15 in even banks 102–110 are replaced. All
five retail rival bodies have identical geometry, UVs, and material ordering.
Original wheels 17/18 and far body 19 remain. Rival slots for courses
0/1/2/3 are 3/3/3/0. The mixed grid exercises four authored bodies in each
bank. For a close front view, use course 1, camera 2, grid 3/0/2/1 repeating,
and rival points 119/121.

## Integrated Istante sources

Open `assets/cars/istante.blend`, scene `Istante player`, and export only
`Istante_player_body` to `istante-body.obj`. The rival source is
`istante-rival.blend`, scene `Istante rival`, object `Istante_rival_body`,
exported to `istante-rival.obj`. The same baked export settings apply;
original and construction meshes remain hidden. Both saved sources reproduce
the embedded native meshes byte for byte.

Player bank 62 and rival body 15 in even banks 112–118 are replaced. All four
retail rival bodies have identical face data and material ordering. The
player needs its own palette mapping: slots 5–12 differ from Esperanza.
Original wheels 17/18 and far body 19 remain. Rival slots for courses
0/1/2/3 are 3/3/3/0. The mixed grid exercises Istante, Hijack, Pegase, and
Esperanza together. A close front view uses course 1, camera 2, grid 3/0/2/1
repeating, and rival points 119/121.

## Integrated Ghepardo sources

Open `assets/cars/ghepardo.blend`, scene `Ghepardo player`, and export only
`Ghepardo_player_body` to `ghepardo-body.obj`. The rival source is
`ghepardo-rival.blend`, scene `Ghepardo rival`, object `Ghepardo_rival_body`,
exported to `ghepardo-rival.obj`. The same baked settings apply; hidden
originals and construction meshes remain. Saved sources reproduce the
embedded meshes byte for byte. Their orthographic cameras use scale 780
to include the whole long body in the comparison renders.

Player bank 66 and rival body 15 in banks 94 and even banks 120–126 are
replaced. All five retail rival bodies have identical face data and material
ordering. Player palette slot 5 differs from Esperanza's. Rival materials
include palette 0x7909 on both pages 12 and 13; retain both identities.
Original wheels 17/18 and far body 19 remain. Rival slots for courses
0/1/2/3 are 3/3/3/0. The mixed scenario includes Hijack, Pegase, and Esperanza;
a close front view uses course 1, camera 2, grid 3/0/2/1 repeating, and
points 119/121.

## Integrated Vainqure sources

Open `assets/cars/vainqure.blend`, scene `Vainqure player`, and export only
`Vainqure_player_body` to `vainqure-body.obj`. The rival source is
`vainqure-rival.blend`, scene `Vainqure rival`, object `Vainqure_rival_body`,
exported to `vainqure-rival.obj`. The same baked export settings apply;
originals and construction references remain hidden. Both saved sources
reproduce the native meshes byte for byte.

Player bank 68 and rival body 10 in even banks 128–134 are replaced. All four
retail rival bodies have identical geometry, UVs, and material ordering.
The rival retains its original wingless silhouette. Original wheels 12/13
and far body 14 remain. Player textures use page 11 palettes 0x382f–0x39ef;
rivals use page 12 palettes 0x7840–0x7844. Rival slots for courses 0/1/2/3
are 2/1/0/3. Use course 1, camera 2, grid 1/2/0/3 repeating, and rival points
119/121 for a close front view.

## Integrated Bulshade sources

Open `assets/cars/bulshade.blend`, scene `Bulshade player`, and export only
`Bulshade_player_body` to `bulshade-body.obj`. Rival sources are
`bulshade-rival.blend` / `Bulshade_rival_body` and
`bulshade-rival-alternate.blend` / `Bulshade_rival_alternate_body`, exported
to matching OBJ filenames. The same baked settings apply. All three saved
sources reproduce the embedded meshes byte for byte.

These bodies use a narrower 0.6-unit bevel: the 1.6-unit draft produced
downward spikes at acute lower corners beyond the four-unit bounds margin.
The narrower model passes the unchanged bounds test and retains hidden
originals and construction references.

Player bank 70 and rival bodies 0/5 in even banks 128–134 are replaced.
The red and green rivals have distinct UVs and material identities; both
are required in each bank. Original rival wheels 2/3 and 7/8 and far bodies
4/9 remain. Course slots for red are 0/2/1/1, for green 1/0/2/2. A close
front view uses course 1, camera 2, lead slot 2 or 0, and rival points 119/121.

## Integrated Squaldon sources

Open `assets/cars/squaldon.blend`, scene `Squaldon player`, and export only
`Squaldon_player_body` to `squaldon-body.obj`. The rival source is
`squaldon-rival.blend`, scene `Squaldon rival`, object `Squaldon_rival_body`,
exported to `squaldon-rival.obj`. Both use the established 1.6-unit bevel
and baked export settings, retain originals, and pack their reference textures.
Saved sources reproduce the embedded meshes byte for byte.

Player bank 72 and rival body 15 in even banks 128–134 are replaced.
Original rival wheels 17/18 and far body 19 remain. All four rival banks
have identical face data and material ordering. Player slots 1–14 use page
11 palettes 0x382f–0x3b6f. Rival slots 8–14 use page 12 palettes
0x7900–0x7906; slots 19/20 use page 13 palettes 0x7907/0x7908.
Rival course slots are 3/3/3/0. A close front view uses course 1, camera 2,
lead slot 3, and rival points 119/121.

## Additional Esperanza rivals

Three separate sources preserve the remaining first-class bodies:
`esperanza-rival-slot1.blend` / `Esperanza_rival_slot1_body` (white/blue 30),
`esperanza-rival-slot2.blend` / `Esperanza_rival_slot2_body` (red 25), and
`esperanza-rival-duplicate.blend` / `Esperanza_rival_duplicate_body` (green
84 reference with tall wing). Scene names use the corresponding `Esperanza
rival-slot1`, `Esperanza rival-slot2`, and `Esperanza rival-duplicate` names.
Export each selected baked object to the matching OBJ basename using the
established settings. All three saved sources reproduce the embedded meshes
byte for byte; the retail textures and construction references are packed.

Bodies 5/10 are replaced in banks 88/90/92; body 15 is replaced in
88/90/92/96/98/100. Each keeps its own original wheels and far body.
The latter three banks use different vertex/normal indices for body 15,
but resolved positions, normals, UVs, colors, and texture identities agree.
Its source material slots 12/13/17 map to cache slots 15/16/20 in banks
96/98/100, while native disc import resolves by page and palette.
For course 1 close front captures, bodies 5/10/15 use grid slots 0/1/3.
Live race palettes can produce liveries different from the reference PNGs;
compare the same scenario before and after to verify preservation.

## Early compact rivals

The three non-selectable compact bodies use descriptive source names Compact
A/B/C. Open `compacta-rival-early.blend`, `compactb-rival-early.blend`, or
`compactc-rival-early.blend`. Scenes are `CompactA rival-early` and likewise
B/C; export only `CompactA_rival_early_body` (or B/C) to the matching OBJ.
These sources retain original references and packed textures, use the standard
1.6-unit bevel, and reproduce the embedded meshes byte for byte.

They replace parts 20/25/30 respectively in banks 88/90/92/96/98/100.
Original wheel and far-body parts remain. Model slots 4/5 share A with two
palette variants, 6/7 share B with two, and 8/9/10 share C with three.
These slots are unchanged by the course permutation. The three texture-page
14 palette families and page-10 details retain full native material metadata.
Banks 96/98/100 have separate cache-slot mappings. Use
`tests/scenarios/authored_compact_early.ini` to exercise all seven variants.

Later compact sources require separate integration because geometry and UVs
differ. Do not apply the early sources to other banks.

## Middle and late compact rivals

Use `compact{a,b,c}-rival-middle.blend` for banks 102–118 (even), and
`compact{a,b,c}-rival-late.blend` for 94/120/122/124/126. Scene and object
names follow the early sources, with `middle` or `late` in place of `early`.
Export the selected baked body to the matching OBJ basename. All six saved
sources reproduce the embedded meshes byte for byte. Reference textures and
original/construction geometry are packed in each source.

Middle sources originate in bank 102; banks 112–118 have a separate cache
slot map. Late sources originate in 94 and share that mapping across their
five banks. B/C differ between middle and late in six faces' UVs; A also
has different positions and normals. The separate sources preserve these
differences. Parts 20/25/30 and model palette slots 4–10 follow the same
assembly convention as the early sources. Original wheels and far parts remain.
