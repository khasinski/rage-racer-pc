#ifndef PORT_ENVIRONMENT_VIEW_H
#define PORT_ENVIRONMENT_VIEW_H
#include "game/environment.h"
#include "render/render_world.h"

/* Copy this race's environment into a camera without changing its pose,
 * clipping, panorama asset/layout or any source state. No GPU publication. */
static inline void ApplyEnvironment(RenderCamera *camera, const Environment *env) {
    if (!camera || !env) return;
    Vec3 *colors[] = {&camera->fogColor, &camera->skyTopColor, &camera->skyColor,
                      &camera->skyHorizonColor, &camera->skyBottomColor};
    for (unsigned slot = 0; slot < 5; ++slot) {
        const GameEnvColor color = env->colors.fields.slots[slot].cur;
        *colors[slot] = (Vec3){color.bytes.r / 255.0f, color.bytes.g / 255.0f,
                               color.bytes.b / 255.0f};
    }
    camera->skyCloudRow = (uint32_t)env->skyRowBase;
    /* Imported geometry is in four retail units per world unit. */
    camera->fogNear = env->fogNear * 0.25f;
    camera->fogFar = camera->fogNear * 5.0f;
}
#endif
