# Rage Racer 0.6.6-alpha

This alpha fixes the Music Player presentation path and makes the car catalog
safe to edit.

`cars.toml` and `cars.ntscj.toml` ship with every platform archive and inside
the macOS app bundle. They are copied to the user configuration directory on
first import. Each specified value overrides the matching retail value decoded
from the selected disc's `rage.bin`; omitted values remain retail. A malformed
catalog is logged and ignored, so it cannot stop the game reaching its menu.

The Music Player now uploads the selected track's texture asset and waits for
its track data before creating the presentation scene. This fixes stale or
broken skyboxes, cameras and scenery after entering the player.

Direct race startup now loads the selected player model before entering the
race. Automated renderer validation includes a native player-car material
check, so a carless race cannot pass it.

The release remains an alpha. Supply a legally obtained Rage Racer CUE or
Track 01 BIN at first launch; generated native assets are then prepared by the
game.
