# Car catalog profiles

`cars.toml` is the default catalog for PAL and NTSC-U. `cars.ntscj.toml` is
the catalog for both Japanese retail revisions. Each file is a complete,
editable template. At runtime it is an overlay: every key that is absent from
a record keeps the retail value decoded from the selected disc's `rage.bin`.

Every `[[cars]]` record describes one retail model and grade. `id` is a stable,
ASCII identifier; display names are ordinary catalog data and may differ by
profile. A record may override its price, next-grade price, unlock class,
maker, class, transmission policy, and any driving-specification field.

`manual_only = true` prevents the automatic transmission from being selected
and forces an existing save entry back to manual. The automatic fields remain
editable so a mod can change that flag without having to invent a drivetrain
setup. For these cars, `automatic_acceleration_scale` and `shift_points` are
reconstructed port defaults; the generated file marks them with comments.
The shift points are internal speed thresholds calculated from the peak-torque
RPM and each gear ratio, with a 25% downshift hysteresis.

`automatic_acceleration_scale` is a per-thousand multiplier applied to the
calculated acceleration only while automatic is selected: `1000` preserves the
unscaled value and `985` retains 98.5%. This follows the matching retail
drivetrain decompilation. Some manual-only retail packs contain the values `3`
or `6` at the same offset. They are inactive because retail never selects
automatic for those cars, so the profile replaces them with the reconstructed
`985` mod default rather than exposing a non-working automatic setup.

The runtime copies both shipped templates into the user configuration directory
after the first successful disc import. It selects the Japanese file only for
a disc identified as `NTSC-J`; PAL, NTSC-U, and an unrecognised disc select the
default file. A malformed selected catalog is logged and ignored before the
first menu; the game then uses its untouched retail data.
