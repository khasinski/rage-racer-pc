# Kontrakty i backlog techniczny

Proponowane API/schemata — nie istniejąca implementacja. [Plan](plan.md).
Nazwy funkcji są krótkie i robocze; nie tworzymy pliku lub klasy na każdą operację.

## 1. Najmniejsze sensowne granice

| Właściciel | Dane | Operacje | Nie posiada |
| --- | --- | --- | --- |
| `Catalog` (C) | Immutable source index, decoded descriptors, references, bounds | Open/read/describe/resolve source binding | UI selection, active game global state |
| `Project` (existing JS service) | Source ID, overrides, files, history, dirty/recovery | Edit/reset/undo/reload, save transaction | Binary layout parser, second gameplay resolver |
| `Build` (C + transaction coordinator) | Frozen input, compiled records, file inventory, hash, diagnostics | Validate/compile snapshot; commit handled by service | Mutable current menu/course selection |
| Preview (existing WebGL module) | Scene instances, selected local element, camera | Pick, transform gizmo, compare, material/texture view | Independent authoritative car/route parameter defaults |
| Runtime asset session | One resolved immutable snapshot/revision | Load resolved content, release after session | Draft authoring/history, editor profile format |

Private command protocol: bounded request over stdin, one JSON result/error and
progress stream. Handles/IDs rather than arbitrary renderer-supplied paths.
File picker grants happen in main process. Reuse existing job cancellation/limits.

Minimum operations before expanding UI:

- `catalog`: source fingerprint/region/revision + named records, capabilities.
- `read`: selected semantic ID → decoded original/effective data + field origins.
- `scene`: all parts/instances + transforms/material references for car or course.
- `import`: input file + target role map → staged candidate + diagnostics.
- `validate`: frozen project delta + source → diagnostics with asset/field/context.
- `build`: frozen valid project → immutable runtime snapshot + exact inventory.

No general arbitrary command execution, plugin scripts or scriptable setters into
game globals. Pure C codecs are called in these commands and in tests without boot.

## 2. Record identity and resolved fields

A proposed entity identity is semantic **within a source family**:
`car/esperanza/grade/2`, `track/mythical-coast`, `interface/main-menu/custom`,
`audio/menu/confirm`. The source fingerprint namespaces identities; it is not a
hash baked into every UI label. Explicit mappings reference original bank/index
and context. Existing manifest IDs remain adapter bindings for old packages.

Descriptors need:

- kind/name/thumbnail, known role, source address/range (details only);
- contexts: region/revision, grade, class, series, renderer;
- owned components versus shared references and instance transforms;
- capabilities with reasons: view/edit/replace/roundtrip, original/native/classic;
- provenance: disc, built-in, selected catalog, installed pack, project;
- unknown/reserved serialized spans preserved for roundtrip.

Resolved field reports `{value, source, overridden, canEdit, units, validRange}`.
A central compiled table supplies car field descriptors from actual C layouts.
No UI `max=1` guessed for signed/non-normalized legacy physics fields. Canonical
integers stay exact; display-unit conversion is reversible and shares game code.
Unknown role names can be project aliases; binding identity remains stable.

## 3. Project and build consistency

`project.rrproject` holds schema/version, source fingerprint, revision, name,
author/package identity and relative document references. Typed car/track/audio
records are deltas; they never duplicate all original geometry by default.

Files are content-addressed or immutable after staging. Editing a mesh/PNG
publishes a new reference. History command holds old/new reference or old/new
field value; gesture grouping lives in Project, not one class per numeric field.
History has a byte/count budget and never evicts references needed by committed
snapshot/recovery. Garbage collection runs after transaction completion.

Input freeze includes enabled packages and conflict decisions, then validates
against the exact bytes used by compile. No build based on cached UI TOML and
then copy from differently edited files. `Build` emits:

1. schema/source requirements and renderer compatibility;
2. resolved car parameter records / track records / runtime media bindings;
3. generic meshes/materials/instances references with bounds;
4. chosen provider list and human-readable change summary;
5. complete file inventory and hashes.

Game activation validates source/version and all required records before
publishing a session view. Invalid snapshot retains original content with a
clear error and is shown as failed activation in the editor. No partial mixing
of incompatible meshes/parameters, no modern-assets fallback to classic.

Atomic active pointer and immutable builds solve runtime activation, not every
cross-directory transaction by magic. Journal recovery distinguishes completed
project save from failed activation; states are truthful. Source images/saves
not selected as copy destinations remain untouched.

## 4. Track codecs and validation dependencies

First implementation can use existing `.2ND` encoders for gameplay blocks that
fit recovered layouts, retaining unknown bytes. For native appearance add generic
scene data independent of PS1 grid/texture atlas. Do **not** write arbitrary new
mesh data into old face streams just to reuse an archive writer.

Track document contains:

- variant context and inherited geometry references, direction-specific data;
- instances and named geometry/face groups with material bindings;
- centerline/curve model, widths, pitch/cross-slope, surface references;
- grid/AI keys, finish/speed cues, zones, camera paths, scenery/event timelines;
- sky/env palettes/timeline and source-render sections;
- visibility/bounds generation policy and applicable renderer capabilities.

Validation order before commit:

1. Parse full document and referenced files, bound allocation/counts.
2. Resolve IDs/roles/indices, cycle/shared-reference ownership, context availability.
3. Geometry finite/transforms, winding/normals/UV/material and texture dimensions.
4. Closed route, positive segment lengths, valid arcs/surface/width, supported units.
5. Grid/AI/event/camera route interval/reference validation per direction.
6. Terrain/world placement scale and visibility/bounds, ray-scene participation.
7. Runtime adapter fit: old integer limits for legacy gameplay writer; no truncation.
8. Compiled snapshot reread using the runtime decoder, then native scene/race check.

Build road geometry once on edit commit/build, not per game frame. Bounds and
spatial visibility index likewise build once. Track rendering should use tested
existing scene culling; new geometry must not rely on accidental old masks.

## 5. Dodatkowe auta i tory — pełne rozszerzenie po podstawowym editorze

Nie wystarczy dołożyć pozycji w pickerze. Te zmiany są jawnie poza małą konsolidacją
UI i poza podstawowym oszacowaniem czasu w planie.

### Większy roster

- Data-driven car registry: family ID, Grade variants, appearance assembly,
  handling record, shop/unlock rules. Existing 13 families become built-in IDs.
- Separate selected appearance from handling donor, including actual rival bodies.
- Session stores references/handles, not “model >= 13 implies rival”. Active race
  car count independent from number of available cars; preserve final Class rules.
- Car select enumerates registry; record/save labels resolve IDs, fallback honest
  unknown custom car rather than use slot 0 without explanation.
- Retail save binary remains readable/writable. New ownership/custom IDs require
  versioned native save extension tied to the base save (identity/checksum), or
  a new native save container retaining retail payload. Choose after migration
  spike; do not squeeze custom IDs into unused-looking reserved bytes.
- Gate: buy/custom race/replay/restart/upgrade/record across registry reorder;
  missing/disabled mod yields named missing reference, no OOB/null dereference.

### Dodatkowe trasy

- Native track registry: semantic ID, display name, supported classes/directions,
  road/scene/media/race data; original 4 slots remain legacy bindings.
- Replace public selector identity derived from `course & 3` / `course >> 2` with
  explicit ID + direction. Legacy loader adapters can still use CourseSlot.
- Records/prizes/GP progression distinguish custom standalone track from original
  championship slot. Custom tracks do not silently reuse an original record table.
- Native route representation uses adequate 32-bit positions/counts, supported
  curves/surface queries and dynamically allocated validated arrays. Legacy
  export has constraints; new native track is not limited to 32×32 cells.
- Bounds/spatial index from actual world geometry rather than 10-bit packed cell
  identity. AI and cameras reference route IDs/progress rather than table address.
- Gate: two custom tracks loaded successively, long route/wrap, reverse, AI full
  lap, native replay, records/save restart and all scene/cache identities distinct.

### Classic native geometry adapter

- Keep classic visual style/ordering/fog behavior, but consume generic native
  triangles/material/instance transforms via a dedicated adapter.
- Explicitly choose supported texture/material subset; no pretense of PBR parity.
- Per-face depth hint and transparency must preserve ordering and Pause/HUD layering.
- New style adapter has fixture tests with glass, wheel arches, terrain seams,
  negative scales, large models and native UI images. Real GPU performance measured.
- If adapter is absent, a native appearance package remains declared Modern-only.
  Gameplay/PCM edits still work with either supported renderer.

### Videos

- Spike bundled compiled codec licensing/size/platform and playback path, with
  decode cancellation/AV sync/aspect/skip/loop tests before exposing Import MP4.
- Resolve named intro/class/ending media bindings like audio; extracted original
  STR remains a supported source. Do not transcode all sources needlessly each launch.

## 6. Pierwsze małe commity po zatwierdzeniu kierunku

1. Add source-aware compiled Catalog API, decode actual one car + one full track;
   fixture tests and original/authored distinction. No new UI abstraction.
2. Extract shared typed car fields/resolver, fix catalog precedence; one price/AT
   delta verified in game, no parallel raw spec writer in normal editor flow.
3. Introduce Project drafts/history/reference staging and transactional Save & apply;
   shared auto-discovery, source compatibility and failed-write tests.
4. Replace raw-centric car workflow with one embedded viewport and named parts.
   Remove replaced handlers/UI immediately after equivalent coverage.
5. Add generic track scene resolver/placement writer; first named building/PNG
   edit → game. Then GLB roundtrip and driveable route iteration.

Każdy commit zawiera usunięcie zastąpionej ścieżki, odpowiedni test i krótką
notatkę zakresu. Nie rozciągać publicznego „Car editor” na niedziałające przyciski.
