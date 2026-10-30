# Duel and CPU Chase

These optional race rules are read from `rage-port.ini` at startup. Both are
disabled by default; no save data or car parameters are changed.

```ini
[duel]
enabled = false
rival_slot = 0

[cpu_chase]
mode = 0
rival_slot = 0
far_distance = 50
near_distance = 8
speed_percent = 115
```

## Duel

Set `enabled = true` to leave only the opponent in `rival_slot` on the starting
grid. `rival_slot` is a CPU grid index from 0 through 10, independent of the
car's model. The selected car uses the last native CPU starting position for
the course and direction; it keeps its own model and driving parameters.
The course-select screen displays **DUEL**, and the race HUD shows a field of
two. You must finish first; second place follows the normal loss/retry path.
If the chosen grid slot has no active car, the ordinary field and result rule
remain in effect.

## CPU Chase

`mode = 0` disables the boost. `mode = 1` boosts only `rival_slot` (0 through
10); `mode = 2` boosts every active CPU rival. To boost the duel opponent with
mode 1, set both `rival_slot` values to the same number.

The boost scales each rival's course target speed, while its model, AI and
normal steering remain intact. `speed_percent = 100` means no boost; `115`
means a target 15 percent higher at full strength. The supported range is
100 through 300.

Distances are in metres relative to the player. The rival gets the full
boost when it trails by at least `far_distance`. The boost fades as it gets
closer and ends at `near_distance`. A negative `near_distance` lets the
boost continue after the rival passes: for example, `-10` ends it only when
the rival is more than 10 metres ahead. `near_distance` accepts -999 through
999; `far_distance` accepts 1 through 1000 and should be greater than
`near_distance` for a useful fade range.
