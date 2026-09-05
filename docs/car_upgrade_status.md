# Car upgrade work in progress

Goal remains `goal.md`. No upgraded car is installed or accepted yet.

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

## Drafts, not accepted assets

`assets/cars/erriso.blend` and `erriso-body.obj` are local authoring experiments
from the textured glTF reference, not installed replacements. The body has a
1.6-unit, three-segment angle bevel and weighted normals. Wheels are still
original. This draft loses the glTF-omitted native face metadata; rebuild it
from the C-exported native OBJ before accepting it. The native OBJ was
successfully imported into a separate Blender scene `Interchange regression`;
its `rage_N` materials retain full metadata words. It is currently present
in Blender memory, not in the saved authoring file.

The first two in-game capture attempts used the bumper camera and do not
verify the player body. `start.camera` is applied by custom-start handling;
set `start.player_track_point=120` as well. Use camera 1 for chase, 2 for track.
The capture writer emits PPM even if given a `.png` suffix; convert with
ImageMagick for viewing. Do not count these early captures as model validation.

## Next required work

Finish Erriso's native-preserving authoring, add repeatable export/build
integration, and load it by default in modern rendering for player and rivals.
Inspect before/after views from multiple cameras, wheels, textures, culling,
shadows, and performance. Verify classic rendering, race transitions,
attract mode, and clean disc import. Only then proceed to other cars.
Keep original data and the existing unrelated `.claude/` and `imgui.ini` files.
Commit verified stages locally; do not push or publish.
