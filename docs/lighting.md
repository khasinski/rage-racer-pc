# Modern renderer lighting

Lighting in the modern renderer is rasterized and runs on every GPU backend.

- **Sun.** `RenderDirectionalLightFromSky` derives the sun direction, colour and
  ambient from the scene's sky colours. Surfaces keep their authored vertex
  colours; the sun term and ambient term modulate them.
- **Sun shadows.** Four nested, texel-snapped orthographic cascades follow the
  player (`render_shadow.c`). They share one depth atlas laid out two by two:
  4096² at `shadows = high` (roughly 0.5, 2, 6 and 16 world units per texel) or
  2048² at `shadows = standard` (twice those sizes). `shadows = off` draws
  none. Every opaque
  span casts, except coplanar overlays. The fragment shader
  (`native_sun_shadow.glsl`) picks the finest cascade that contains the fragment,
  filters with a tent-weighted 3×3 grid of hardware comparisons, offsets the
  receiver along its normal to avoid acne, and fades into the next cascade near a
  tile edge. Cascades are centred on the player rather than fitted to the view
  frustum, so they do not shimmer and the rear-view mirror sees the same coverage.
- **Ambient occlusion.** `modern_ssao.c` darkens creases and contact areas of the
  3D scene. It reads the main view depth buffer, takes 16 hemisphere taps
  around each pixel in view space (rotated per pixel by interleaved gradient
  noise), blurs with a depth-aware filter so occlusion never crosses an edge,
  and multiplies the result onto the scene before the 2D layers. It fades out
  with distance so fog carries no contact shadow. `ssao = off` skips it.
- **Local lights.** Up to 72 spot lights (headlights, tail and brake lamps) are
  evaluated per fragment without occlusion.

Casters are the spans drawn in the main view plus shadow-only spans: scenery
and terrain outside the camera frustum (behind it, beside it or past the far
plane) whose bounds reach the outer cascade. They are drawn into the shadow
map alone, so a shadow does not pop in when its caster enters the frame.
Terrain and shadow-only instances are built whole (no camera-dependent
back-face or hidden-quad culling), and each span carries a bounding sphere so
a cascade draws only the spans that reach it.

## Ray tracing

An experimental ray-traced sun-shadow and reflection path (per-fragment BVH
traversal in the fragment shader) shipped in `0.6.8-alpha-raytracer` and was
removed. It showed self-shadowing patterns, ignored alpha cutouts and
back-face culling, and cost too much at high internal resolutions. A
replacement will be designed from scratch for a later version: half-resolution
compute visibility with denoising, alpha-aware, built on the same `RenderWorld`
scene contract.

## In-game options

`Esc` pauses the game and opens settings (`graphics_overlay.c`). Graphics
lists the renderer, resolution, aspect ratio, frame rate, texture filtering,
anti-aliasing, colour grading, shadows, ambient occlusion and draw distance.
Controls covers analog steering, the wheel, steering feel and force feedback.
Audio covers music and effects. View covers the HUD and the chase camera.
Graphics changes apply at once: anything but the renderer choice rebuilds the
presentation targets through `ModernApplyConfig` while the game is held, and
the renderer choice is a switch. Graphics, steering, the HUD and the chase
camera last until the game closes. Music and effects are the save-game
volumes, and force feedback is the choice the pause menu remembers. `[video]`
in `rage-port.ini` sets the values used at start-up. Keyboard bindings stay
there.
