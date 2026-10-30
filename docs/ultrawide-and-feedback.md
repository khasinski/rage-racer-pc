# Ultrawide displays and controller feedback

`video.aspect` accepts `4:3`, `16:9`, `21:9` and `32:9`; `auto` retains the
original 4:3 framing. The editor exposes all four ratios. Wider ratios reveal
more of the world horizontally with the same vertical field of view. The
classic enhanced and modern renderers share this layout. Menus remain centered
at their original proportions. `hud.anchor=edges` moves race HUD elements to
the new edges; `center` retains their original positions.

Force feedback uses SDL's steering-axis constant-force effect with hardware
gain set to full strength where supported. The game's `input.ffb_gain` controls
the actual force, so it is not multiplied by an unknown driver gain. Effects
are held only during a race, released on exit, and cleaned up if playback fails.
Gamepad rumble follows the active pad, stops the previous pad on a switch, and
refreshes sustained effects. Engine rumble remains off by default; impacts,
road motion and grip loss produce feedback. The pause-menu FFB setting is
remembered and can override the initial INI setting.

Automated game tests use `PSYZ_HEADLESS=1` and `SDL_AUDIODRIVER=dummy`. Hidden
windows retain the GPU backend so image captures still test actual rendering.
The smoke executable defaults to both settings; CTest also applies both to
production executable tests. Hardware-specific wheel/8BitDo confirmation still
requires testing on the relevant controller and OS.
