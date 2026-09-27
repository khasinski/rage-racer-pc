/* The race sky as the native modern renderer draws it (modern_native_gpu.c
 * ModernNativeBuildSky / ModernNativeEnsureSkyTexture with the
 * native_sky.frag.glsl shader): the environment gradient, plus the disc's
 * cloud panorama placed on the retail screen-space tile grid. */
#ifndef WEB_SKY_H
#define WEB_SKY_H

#include <stddef.h>
#include <stdint.h>

#include "client_race.h"
#include "render/render_world.h"

enum {
    WEB_SKY_WIDTH = 512,
    WEB_SKY_HEIGHT = 256,
    /* top, middle, horizon, bottom, gridOrigin, gridBasis, gridParams. */
    WEB_SKY_FLOATS = 28,
};

/* The panorama identity and the grid DrawSkyBackground measures for this
 * camera (no mirrored course), as render_world_game.c's
 * GameRenderWorldBuildCamera publishes them. `cameraY` and the angles are
 * the PS1 view values of the main camera; `mirrorPass` selects the grid the
 * rear-view mirror pass measures from them. */
void WebSkySetCamera(RenderCamera *camera, const ClientRace *race, s32 cameraY,
                     s32 pitch, s32 yaw, s32 roll, int mirrorPass);

/* ModernNativeBuildSky's uniform block; gridParams.w (the target height in
 * pixels) is left for the browser, which knows its drawing buffer. */
void WebSkyUniform(const RenderCamera *camera, float aspect, float out[WEB_SKY_FLOATS]);

/* client_frame.c DecodeFrameSky: the 512x256 panorama from the track's
 * texture page and the current environment palette. */
int WebSkyDecode(const ClientRace *race, const RenderCamera *camera, int page,
                 uint8_t *rgba, size_t size);

#endif
