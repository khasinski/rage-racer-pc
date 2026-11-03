# Texture Refresh 0.1 — final acceptance session

Use frozen pack `0.6.24-refresh7.232`. Its SHA-256 is
`6b6bf533d7d786f759c47f63a3b9748b71e5a1547db431df1b76e125ab6fbbe4`.
Record the game build hash, OS, GPU, results and evidence for each check. Do not
replace missing observations with a passing unit test. No new artwork is needed
to perform this session.

## 1. Local pack controls

In the running game, open Esc → Mods. Use the native chooser to install the local
archive. Verify it, then disable and re-enable HD textures without restarting the
process. Observe an actual texture change in both directions, confirm the menu
reports success, and check that the game remains responsive.

Restart once with HD disabled and once with HD enabled. Each start must preserve
the selected state and display the corresponding textures. Save screenshots of
the settings and rendered scene. Installer worker tests and separately launched
reference captures already exist; this session covers the native chooser and
live renderer integration they do not exercise.

## 2. Normal race coverage

Drive one complete Mythical Coast lap in Esperanza, class 0, by day and one by
night with the installed pack enabled. Use normal physics and steering. Record
continuous video, with start/finish evidence establishing the complete lap.
Inspect missing surfaces, conspicuous new seams, style mismatches and flicker.
Also inspect tachometer identity, speed/gear placement and digit 4 when visible.

The debug route autopilot and an accelerator-only run do not satisfy this check.
Do not create an AI driver as an additional prerequisite for this preview.

For a suspected regression, retain its frame/time and location, then compare
that location with HD disabled. Existing original-renderer behavior must not be
silently attributed to new artwork. A newly discovered asset idea is later work;
a confirmed failure of the agreed acceptance criteria is a release blocker.

## 3. Clean game release

On each supported OS, extract the intended game release into a fresh location
and use a clean user profile without generated game assets. Launch by
double-click. Provide only a legally obtained Rage Racer CUE or Track 01 BIN
through the normal selection flow. Confirm automatic native asset generation
and startup in the modern renderer. No separate conversion command, developer
directory or classic-renderer fallback is acceptable.

Record the release artifact hash, startup log and renderer evidence. A staged
macOS executable has started using Metal and the C importer; that does not prove
this clean-profile, native-selection, double-click flow or other platforms.

## Stop rule

After these observations pass, reconcile the results with
`output/texture-refresh/v7/release-0.1-route-review/readiness.json`, update the
[scope and limitations](texture_refresh_0.1.md), and include them with the local
pack. Ship the frozen preview once all five criteria in
[the release boundary](hd_texture_packs.md#texture-refresh-01-release-boundary)
are met. Further tracks, cars and optional polish remain later work.

Automated game runs remain hidden and silent. Visible interactive testing is
performed by the player, or by an agent only when explicitly requested.
