> Historical local candidate notes for refresh7.232. The distributed preview is
> now 0.1.0-preview.1; see [HD distribution](hd-distribution.md). These older
> acceptance notes do not describe the current archive or delivery method.

# Texture Refresh 0.1 — release candidate scope

This is a local texture-pack preview for Mythical Coast and Esperanza, including
the refreshed sky and previously accepted HUD and menu artwork. It is not a
replacement for every original texture. Unchanged original surfaces remain.

Asset additions are frozen at pack `0.6.24-refresh7.232`.

- Archive: `Faithful-Refresh-v7.232.rrhd`
- SHA-256: `6b6bf533d7d786f759c47f63a3b9748b71e5a1547db431df1b76e125ab6fbbe4`
- Delivery: local pack; server downloads are outside this release.
- Further tracks, cars and artwork belong to later releases. Changes to this
  candidate must address a failed acceptance condition.

## Known coverage limits

- Original textures remain, including within partially refreshed atlases.
- Other tracks and cars are outside the preview's supported visual scope.
- The reported Lakeside Gate waterfall flicker is not resolved by the Mythical
  Coast captures and remains an outstanding issue.
- Esperanza HUD comparisons cover day and night at 72 km/h in second gear,
  class 0. They do not establish correctness for every car, class or gear.

## Verification still required before release

This candidate is **not yet release-ready**.

- Complete a normal full-race check in day and night, including continuous
  inspection for flicker. Sampled full-route captures and short consecutive
  waterfall windows have been reviewed, but autopilot bypasses normal physics.
- Check the native file chooser and pack switching in the running game. Full
  archive installation and persistence across processes have passed; restarted
  GPU-rendered frames respect the saved enabled/disabled state.
- Check clean packaged double-click launches on every supported platform,
  automatic modern-asset generation from only a legal CUE or Track 01 BIN,
  and startup in the modern renderer.
- Reconcile this document with the final acceptance results and include it
  alongside the distributed local pack.

The authoritative acceptance criteria are in
[HD texture packs](hd_texture_packs.md#texture-refresh-01-release-boundary).
Detailed candidate evidence is recorded in
`output/texture-refresh/v7/release-0.1-route-review/readiness.json`.
