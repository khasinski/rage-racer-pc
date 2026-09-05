# Car upgrade work in progress

Goal remains `goal.md`. Base Erriso's upgraded player body is installed by
default. No car has passed the complete acceptance scope yet.

## Verified on 2026-09-05

- Branch `re-asset`; Blender MCP connected (Blender 5.2).
- Erriso is car 0, base asset 10 (`010_CAR_00_1ST`). Its bank has
  22 submeshes: body 0, obsolete flat shadow 1, wheel variants 2–21.
- Player submission uses this bank. Rivals use the course's shared track
  model bank, with a separate far model. Both paths need integration.
- Original game starts with `modern.assets=disc`, imports asset 10, and
  captures a modern-renderer frame. Evidence and logs are under
  `build/car-upgrade-evidence/`.
- C interchange tool and tests added; `mesh_obj`, `rmesh`, `rmesh_index`,
  `rmesh_cache`, and `native_asset_loader` pass (5/5). The loader test target
  needed building before its first invocation.
- Blender inspection of the original body and a narrow bevel experiment:
  `erriso-blender-before.png`, `erriso-blender-bevel.png`. Weighted normals
  corrected the first experiment's faceted shading.

## Integrated player prototype, not a fully accepted car

`assets/cars/erriso.blend` and `erriso-body.obj` now use the C-exported native
source, preserving full material metadata. The body has a 1.6-unit,
three-segment bevel, weighted normals, a color clamp, and an 85% transfer of
retail panel normals. It contains 1948 triangles. Reference textures are
packed in the blend. Original wheel geometry and gameplay data are unchanged.
CMake converts and embeds this model automatically. The disc importer remaps
authored material slots to live texture identities. Base player asset 10 is
replaced; rivals and other grades are still original.

The new body was rendered and inspected in Blender and the release game.
Comparable current-build chase captures are `erriso-current-before.png` and
`erriso-after-smooth.png`; Blender's latest view is
`erriso-native-blender-after-smooth.png`. An earlier bevel version had a sharp
roof highlight, corrected by the panel-normal transfer. More viewing angles
and close inspection of panel joins remain required. The current-build
baseline also shows incorrect environment atlas imagery on the rock wall;
this occurs with `modern.authored_cars=0` too and is not introduced by the car
replacement. Investigate it before final acceptance.

Seven focused checks now cover native parsing/cache/loading, OBJ interchange,
submesh replacement, and the actual embedded model. Replacement tests verify
that wheel vertices and indices are preserved and that material remapping
does not strip face metadata. Latest logs: `integration-tests.log` and
`asset-tools-build.log` under the evidence directory. At the final frozen
chase view, logged submission CPU time was approximately 0.27 ms with either
body; this is a narrow sample, not a complete performance assessment.

The first two in-game capture attempts used the bumper camera and do not
verify the player body. `start.camera` is applied by custom-start handling;
set `start.player_track_point=120` as well. Use camera 1 for chase, 2 for track.
The capture writer emits PPM even if given a `.png` suffix; convert with
ImageMagick for viewing. Do not count these early captures as model validation.

## Next required work

Finish Erriso's visual refinement and integrate its rival representation.
Player body authoring, repeatable export/build integration, and default
modern loading are implemented for the base grade.
Inspect before/after views from multiple cameras, wheels, textures, culling,
shadows, and performance. Verify classic rendering, race transitions,
attract mode, and clean disc import. Only then proceed to other cars.
Keep original data and the existing unrelated `.claude/` and `imgui.ini` files.
Commit verified stages locally; do not push or publish.
