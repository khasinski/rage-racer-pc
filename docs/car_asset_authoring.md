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
