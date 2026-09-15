# Rage Racer 0.6.7-alpha

This alpha adds Custom Race and continues the reliability work begun in
0.6.6-alpha. Custom Race uses the familiar course and car selectors, exposes
every normal and reverse course, supports classes 1 through 6, and allows
player and rival car models. Custom races use the competitive race HUD,
positions and collisions and return safely after either a win or a loss.

Rival car previews now load their own bodies, wheels, glass and textures
without overwriting ordinary menu assets. Rival cars driven by the player use
the same steering state in both renderers, including visible front-wheel
steering. Replays record the complete active field and handle the smaller
final-class grid.

The in-race pause menu can switch between the classic and modern renderers.
The hand-off preserves the current frame, the classic PAUSE overlay and the
menu layout. Start-grid scenery, course faces and the Reiko presentation use
the corrected depth and face selection in both renderers.

Menu music is extracted with the rest of the native assets and played from
PCM. Menu sound cues, transitions and CD audio state share one lifecycle,
reducing stale audio when moving between the title screen, selectors, races
and replays.

The third-person camera can be adjusted in `rage-port.ini` or the settings UI:

```ini
[camera]
chase_height = 1
chase_distance = 1
chase_pitch = 0
```

Height and distance accept multipliers from 0.25 to 4. Pitch accepts -45 to
45 degrees. The default values preserve the original chase-camera presets.

Internal menu, race, replay, audio, memory-card and camera state has been
grouped by lifetime. Dead compatibility paths, obsolete mono handling and
unused developer probes were removed. The complete native unit and functional
gates are built from a clean tree before packaging.

The release remains an alpha. Supply a legally obtained Rage Racer CUE,
Track 01 BIN or CHD at first launch. The game imports the required native
assets and starts with the modern renderer; no separate extraction command is
required.
