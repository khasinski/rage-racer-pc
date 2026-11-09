#ifndef RAGE_ATMOSPHERE_GROUND_H
#define RAGE_ATMOSPHERE_GROUND_H
#include "render/render_world.h"
#include <math.h>

/* Course coordinates have no common sea-level zero. Anchor the local haze
 * layer to the driving surface represented by a car root, shared by both
 * views. Camera altitude alone changes during intros and mirror rendering. */
static float ModernAtmosphereGroundHeight(const RenderWorld *world,
                                         const RenderCamera *camera) {
    float ground = camera->transform.position.y - 64.0f;
    float nearest = INFINITY;
    for (uint32_t i = 0; i < world->instanceCount; ++i) {
        const RenderMeshInstance *item = &world->instances[i];
        if (item->entity > RAGE_RENDER_PLAYER_CAR_ENTITY || item->component != 0 ||
            !isfinite(item->transform.position.y)) continue;
        if (item->entity == RAGE_RENDER_PLAYER_CAR_ENTITY)
            return item->transform.position.y;
        float x = item->transform.position.x - camera->transform.position.x;
        float z = item->transform.position.z - camera->transform.position.z;
        float distance = x*x + z*z;
        if (distance < nearest) {nearest = distance;ground = item->transform.position.y;}
    }
    return ground;
}
#endif
