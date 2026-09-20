#ifndef CAR_LAMPS_H
#define CAR_LAMPS_H
#include "render_world.h"
typedef enum LampKind { LAMP_HEAD, LAMP_TAIL, LAMP_STOP, LAMP_TAIL_STOP } LampKind;
typedef struct Lamp {
    uint32_t material;
    float bounds[4]; /* Atlas texels, x0 y0 x1 y1. */
    Vec3 position;   /* Imported body-local coordinates. */
    LampKind kind;
    int round;
} Lamp;
/* Some banks reuse a body and atlas with shifted material indices.
 * materialOffset may be NULL when only positions/intensities are needed. */
unsigned CarLamps(const RenderMeshInstance *body, const Lamp **lamps,
                  uint32_t *materialOffset);
float CarLightDaylight(Vec3 sky, Vec3 horizon);
float CarLampIntensity(const CarLights *state, LampKind kind);
void RenderCarSpotLights(RenderWorld *world);
#endif
