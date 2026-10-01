/* What rage_web.c lends the other parts of the browser bridge (web_showroom.c):
 * the prepared race and drawing a scene of its cars. */
#ifndef WEB_BRIDGE_H
#define WEB_BRIDGE_H
#include "client_race.h"
#include "render/render_world.h"

/* The race prepared last (or the garage preview's one-car race), or NULL. */
ClientRace *WebRace(void);
/* Draws the course around the field's cars as they stand, from a camera at
 * `eye` (game coordinates) looking along pitch/yaw, into the frame buffers
 * the renderer reads (as rw_build_frame does). Returns the vertex count, or -1. */
int WebDrawFieldScene(float aspect, Vec3 eye, s32 pitch, s32 yaw, float verticalFovDegrees);
/* The bounding box of the cars drawn in the last frame (drawn coordinates);
 * 0 when none was drawn. */
int WebDrawnCarBounds(float low[3], float high[3]);
#endif
