# Local HD texture packs

In the game press Escape, open **Mods**, then **Install local pack** and choose
an `.rrhd` file. The panel shows installation/verification progress. **HD
textures** switches the installed pack on or off; **Verify pack** checks every
file, and **Cancel task** cancels work before publication. Server discovery and
downloading are deliberately absent in this first version.

Installation checks the disc region, archive limits, PNG dimensions and decode,
manifest references, safe file names, and each file's SHA-256. It stages a new
immutable version, then atomically replaces `active.txt`. Cancellation or invalid
input preserves the previous active version. Previous versions remain on disk.
SHA-256 detects corruption; it is not a publisher signature.

Installed packs live in the user configuration directory under `hd-packs`.
`hd.store` overrides that location for isolated previews/tests. The included PoC
uses `output/imagegen-poc/v4/installed` so its toggle does not affect other runs.

The HD layer takes priority over ordinary texture/material overrides. Switching
it clears prepared materials and GPU presentation resources while keeping the
native imported assets and running game. Existing disc import and renderer
startup remain unchanged. No network service or Python helper is required.

HUD textures use semantic keys such as `ui.hud.page.9.clut.30924`. They replace
supported unwindowed race/replay sprite draws; coordinates, UV rectangles,
clipping, vertex colour and packet order come from the game. Unsupported PS1
blend/window operations retain their native VRAM draw. The tachometer needle,
speed, gear, timers, warning logic and positions remain game-controlled. This PoC
covers ten atlas/palette combinations, not every HUD effect or every car palette.

## Texture refresh 0.1 release boundary

Version 0.1 is a local-pack preview of Mythical Coast, Esperanza, the existing
refreshed sky, and the already accepted HUD/menu artwork. It does not promise
replacement of every texture in the game. Original surfaces may remain where
they do not create a conspicuous mismatch.

Asset additions are frozen at pack 0.6.24-refresh7.232, including the finished
temple wall materials 137 and 138 and their source-matched aliases. Further
tracks, cars, and replacement artwork belong to a later release. After the
freeze, changes must fix a failing acceptance condition below. The broader
remaining-texture backlog stays open independently of this release milestone.

The stopping rule is to ship this frozen scope once the five acceptance items
below pass. Asset counts, complete game coverage, and further visual polish are
not additional release gates. Record non-blocking findings for a later release;
only failures of these acceptance items may expand the remaining 0.1 work.

Release acceptance requires recorded evidence for every item:

1. Review a complete Mythical Coast run in both day and night with the final
   installed pack. Check missing textures, conspicuous new seams, style mismatch,
   and flicker. Spaced screenshots and material-load logs alone do not prove
   temporal correctness. Record the pack hash, build hash, environment, and
   limitations; a physics-bypassing autopilot is not a normal race test.
2. Verify Esperanza's tachometer identity and speed/gear placement, and compare
   the width of the digit 4 with the other speed digits. Preserve accepted HUD,
   logos, and menu artwork. Do not infer these fixes from unrelated texture tests.
3. Verify in-game local installation, verification, enable/disable, and state
   persistence after restart. The released pack must match its recorded hash.
4. A clean release must launch by double-click on each supported platform. Given
   only the player's legally obtained CUE or Track 01 BIN, it must generate all
   modern-renderer assets automatically and start the modern renderer without
   a separate command or silent classic-renderer fallback.
5. Publish a short scope/known-limitations list with the local pack. Any deferred
   issue must be compatible with the conditions above; missing evidence keeps
   an item open. Server delivery is outside 0.1.

Release readiness is tracked separately from asset counts in
`output/texture-refresh/v7/release-0.1-route-review/readiness.json`. No number of
bindings or successful isolated-material probes makes this checklist complete.

The native renderer prepares texture banks (including mipmaps and GPU uploads)
before their first presentation, using the complete published course world rather
than only the camera-visible draws. Both track image banks and the actual car
palette/paint identities are included. Immutable mesh templates and resident GPU
geometry are prepared at the same boundary. This increases initial preparation
time in exchange for avoiding first-use texture uploads at later corners. A track
asset revision or HD-pack generation change invalidates this preparation.

## Authoring

Car body overrides may opt into the selected in-game paint using a `.paintable`
suffix, for example `car.9.material.0.paintable` in `[textures]`. The original
palette mask colors those regions in the HD image; artwork outside the mask is
retained. Prepare those regions for the retail mask and inspect them in game.
Ordinary texture keys remain finished-color artwork. A pack layer takes
precedence over ordinary mods for both kinds of key.

A source folder contains `pack.json`, `mod.toml`, and referenced flat
`textures/*.png` files. `pack.json` has `format: 1`, `name`, `version`, and `region`
(`PAL`, `NTSC-U`, or `NTSC-J`). The existing semantic mod manifest supplies textures
and material properties. Local HD packs cannot supply meshes, audio or dependencies.

The native game binary also exposes the installer for build tools:

```
Rage\ Racer --set tools.hd_pack_create=pack-folder --set tools.hd_pack_output=pack.rrhd
Rage\ Racer --set tools.hd_pack_install=pack.rrhd --set hd.store=installed --set hd.region=PAL
Rage\ Racer --set tools.hd_pack_verify=true --set hd.store=installed --set hd.region=PAL
```

`.rrhd` v1 is an eight-byte `RRHD001\n` header, little-endian uint32 record count,
then records: uint16 name length, uint32 payload size, 64 ASCII SHA-256 digits,
name bytes, payload bytes. `pack.json` and `mod.toml` are the first two records.
Limits: 1026 records, 64 MiB per payload, 1 GiB total payload, PNGs at most 4096
pixels per dimension. The installed integrity index contains the same record
headers without payloads; the active pointer also binds its SHA-256.

The workflow follows Parasite Eve PC's native installer principles: validate a
complete staged version, atomically activate, retain the old version on failure,
and refresh presentation independently from game state. Its server/API/archive
format is not required here.

## Scene-scoped UI atlases

Ordinary captured UI sprites also accept
`ui.scene.<scene>.page.<page>.clut.<clut>` PNG overrides. The page number includes
texture depth bits, with the blend-mode bits removed (`tpage & 0x19f`). Scene
scoping avoids applying an unrelated atlas after VRAM pages are reused. Texture
caches retire on scene changes and HD-layer generation changes. Existing
`ui.hud.page.*` keys remain a fallback for the original 4-bit race/replay HUD.

Explicit UI atlas scenes can render at 240 or 480 logical rows. The latter uses
the original 320x480 coordinates and drawing area, fitted to the presentation
output; glyphs are not stretched using the 240-row transform. Source rectangles,
clip areas, draw order and colour modulation remain game-controlled.

For semi-enabled sprites, RGBA artwork supplies new colour and coverage while a
second sampler reads the original VRAM texel's STP bit. This retains opaque and
semi-transparent texels in the same primitive, including half, additive and
quarter-additive blending. Reverse-subtractive primitives and texture-window
operations currently retain the native VRAM draw. The artwork must preserve any
intentional background plate as part of its alpha/colour design.

## Individual sprite regions

Append `.region.<x>.<y>.<width>.<height>` to a scene or HUD atlas key to replace
only that original source rectangle. For example:

```toml
[textures]
"ui.scene.23.page.31.clut.32576.region.0.72.60.24" = "textures/trophies.png"
```

Here `trophies.png` contains the local 60×24 rectangle at a uniform integer
scale, for example 240×96. It is not a 256×256 atlas. The complete drawn sprite
must fit inside the region; crossing sprites and uncovered neighbours retain
their native draw or another matching override. Coordinates must fit inside the
original 256×256 page. Preserve transparent margins within the original cell.

A matching region takes priority over a full atlas in the same layer. Later
matching regions win within that layer; the HD layer precedes ordinary mods.
The original UV position is retained for STP classification even though the
new image uses local coordinates. Full atlas keys continue to use 256×256
source coordinates. Region tests cover clipping at boundaries, fallback,
480-row presentation, neighbouring unmodified labels and GPU semi-transparency.

Use narrow regions around static art when a page also contains mutable
player-authored content. Coverage of those editing screens is still pending.
The binding support does not establish that the replacement artwork or all
scenes are complete.

Car-specific HUD art uses `ui.hud.car.<model>.page.<page>.clut.<clut>`
(with the same optional `.region.x.y.w.h` suffix). For example,
`ui.hud.car.0.page.10.clut.13224` replaces only model 0’s dial atlas.
These bindings precede shared `ui.hud.page.*` bindings. A shared page/CLUT
is not a car identity: different cars upload different dial layouts there.
Keep dial backgrounds, logos and the speed/gear cutouts scoped to their car.

## Cloud atlases

`track.<bank>.sky.atlas` replaces the original four-column, two-row cloud page
used by the captured sky polygons. Supply a square RGBA PNG at an integer
multiple of 256×256 (for example 1024×1024). Keep the eight 64×128 source
cells in their original places and preserve the original coverage: transparent
cloud pages retain their empty areas, while opaque sky pages retain blue sky.
The renderer
retains the original tile map, source UVs, roll/pitch, clipping, colour tint
and blend mode. It never reconstructs the cloud grid as rectangular UI.

The existing `track.<bank>.sky` strip/panorama key remains the native panorama
input, including the mirror. A pack covering both paths should provide both
forms derived from the same artwork. The optional atlas is scoped to its
track/environment bank, not applied indiscriminately to unrelated cloud art.
When random time of day installs another environment profile, both sky keys
follow that profile's bank; terrain and car asset identities remain unchanged.
The `sky_environment_runtime` test checks that a base-class atlas cannot
override the selected profile and that the selected profile's atlas renders.

## Native material references

Manifests accept up to 16,384 texture assignments and 16 MiB of TOML, including
shared-image aliases across track banks. The native pack reader and ordinary
mod loader use the same byte bound. Existing schema-1 packs remain compatible.
`RageModManifest` is a large bounded value and must live on the heap or in
static storage. The parser capacity test also runs successfully with a 256 KiB
process stack; the pack test exercises create/install/verify at the full entry
limit, including a manifest larger than the previous one-MiB pack-reader bound.

A material with a texture window can reuse fresh artwork from its full-page
source when the original page and palette match. First crop the native window,
repeat it over the decoded page, and compare every RGBA pixel with the native
windowed material. Only after that comparison is identical, crop the same
scaled rectangle from the authored page and repeat it with the same layout.
This preserves the window's offset, period and the shared artwork. Record the
native comparison and verify the resulting material hash in the renderer.
Texture-cache loading is not evidence that the material was drawn in a captured
view. Track native source coverage, successful GPU loading and visible-use
comparisons separately, including the sampled texture-page variant.

`diagnostics.material_usage=true` writes `native-material-use:` JSON records
when a textured colour draw is submitted, once per cached texture and view
(`main` or `mirror`). The records contain the actual texture identity and car
paint variant; cache warming and shadow-only draws do not emit them.
`diagnostics.material_usage_start_frame=N` starts recording at world frame N,
which can exclude loading and initial race setup. A submitted draw can still
be hidden by depth testing or cover no visible pixels, so retain screenshot
comparisons for visual acceptance. `material_usage_runtime` checks cache-only
exclusion, deduplication, main/mirror records and identical pixels with tracing
enabled and disabled.

The compiled `rage-racer-smoke` binary can export original decoded material
images independently of HD replacements. Set `dump.material_png` to an output
path, `dump.material_asset_key` to the loaded track bank, `dump.material_asset_set`
to the native set (1 course, 2 terrain), `dump.material_index` to the first
material, and `dump.material_variant` to the requested texture-bank/palette
variant. `dump.material_count` defaults to 1; 0 exports every remaining material
in the loaded bank. For a larger count, the exporter
appends `.<material>.png` to the supplied path and reports each source texture
window as `native-material` JSON on stdout. The requested range must exist.

`report.material_uv=true` reports the selected material's unique native UV
corners. `report.material_locations=true` reports terrain instances containing
that material, their world-space centers (`world_xyz`), axis-aligned bounds
(`world_bounds_min`/`world_bounds_max`), and the nearest track points. Coordinates
use renderer space: Y points up and Z is the negative game-world Z. The legacy
`world_xz` field is retained. Nearest track points use X/Z distance only and can
select another road height at an overlap. These centers and bounds help choose
review cameras; they do not establish visibility or prove
that a replacement matches the game. Verify an actual before/after render.

The report also includes `texture_page2_sections`, the currently loaded race's
native half-open section range `[lo, hi)` used by `TrackTexturePageForSection`.
Keep the exported asset key aligned with that race. An empty range (`lo >= hi`)
cannot select page 2 for any section, including finish-camera sections. A
nonempty range does not prove that the selected material is drawn there; check
the actual material-use records and pixels separately.
`track_length` is also reported. For the retail car-progress rules, section
values are bounded above by `track_length / 256` (including the reverse-course
endpoint); finish cameras retain a car's section. A range wholly above that
bound is therefore inactive during these normal updates, even if nonempty.

For a close material review independent of the car's road height, explicitly set
`diagnostics.review_camera=true` and all six integer settings:
`diagnostics.review_target_x`, `diagnostics.review_target_y`,
`diagnostics.review_target_z` (renderer coordinates), `diagnostics.review_distance`
(1–65536), `diagnostics.review_azimuth` (−360–360 degrees), and
`diagnostics.review_elevation` (−89–89 degrees). The view orbits that target using
the existing render-stage camera. Invalid or incomplete settings leave the normal
camera intact and emit a diagnostic. This changes only the modern race-world
view, disables the driver mirror, and preserves the game's camera and physics.
Use a nearby normal track start to retain relevant scenery. These diagnostic
views validate materials; the original screen-space sky packets and HUD remain
from the game camera and are not valid sky/HUD-layout review evidence.
`report.sky_layout=true` reports all eight cloud UV quads and the original
tile map. Atlas row boundaries are not necessarily panorama boundaries: inspect
the map before authoring seamless joins.

Run these game probes with `PSYZ_HEADLESS=1 SDL_AUDIODRIVER=dummy
SDL_MAC_BACKGROUND_APP=1`, retaining the native GPU renderer for image review.
For reproducible random time-of-day reviews, the smoke binary accepts
`start.random_seed` (0 through 2147483647) before initial race setup. Scene-level
`sync.random` runs later and cannot control an environment already selected
during race initialization. `report.hud_layout` also reports the actual
environment mode, whether random time of day is enabled, and
`sky_override_asset_key` (zero when the original environment is retained).
