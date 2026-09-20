#include "car_lamps.h"
#include "render_instance_transform.h"
#include <stddef.h>

unsigned CarLamps(const RenderMeshInstance *body, const Lamp **lamps) {
    /* Centers projected through model 0/material 3's UV triangles. Keeping
     * the patch and its emitter together prevents independent placement drift. */
    static const Lamp special[] = {
        {3, {12, 131, 28, 139}, {91.88095f, 19.41667f, 434.88889f}, LAMP_HEAD, 0},
        {3, {85, 131, 98, 139}, {-92.98106f, 19.41667f, 434.88889f}, LAMP_HEAD, 0},
        /* Both rear corners reuse this circular lens in the atlas. */
        {1, {74, 222, 80, 229}, {104.73947f, 43.53947f, -77.00053f}, LAMP_TAIL_STOP, 1},
        {1, {74, 222, 80, 229}, {-104.77895f, 43.53947f, -76.81105f}, LAMP_TAIL_STOP, 1},
    };
    static const Lamp rival[] = {
        {7, {29, 160, 42, 165}, {-64.43902f, 35.7f, 386.50244f}, LAMP_HEAD, 0},
        {7, {29, 160, 42, 165}, {64.43902f, 35.7f, 386.50244f}, LAMP_HEAD, 0},
    };
    static const Lamp compact[] = {
        {3, {204, 46, 212, 57}, {-64.88971f, 41.61765f, 342.73235f}, LAMP_HEAD, 1},
        {3, {204, 118, 212, 129}, {64.42647f, 41.61765f, 342.91765f}, LAMP_HEAD, 1},
    };
    *lamps = NULL;
    if (body->component == 0 && body->assetSet == RAGE_RENDER_ASSET_MODEL_BANK &&
        body->assetKey == 10 && body->mesh == 0) {
        *lamps = compact;
        return sizeof(compact) / sizeof(*compact);
    }
    if (body->component == 0 && body->assetSet == RAGE_RENDER_ASSET_MODEL_BANK &&
        body->assetKey == 68 && body->mesh == 0) {
        *lamps = special;
        return sizeof(special) / sizeof(*special);
    }
    if (body->component == 0 &&
        body->assetSet == RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1 &&
        body->assetKey == 128 && body->mesh == 0) {
        *lamps = rival;
        return sizeof(rival) / sizeof(*rival);
    }
    return 0;
}

float CarLampIntensity(const CarLights *state, LampKind kind) {
    switch (kind) {
    case LAMP_HEAD: return state->headlights;
    case LAMP_TAIL: return state->tail;
    case LAMP_STOP: return state->stop;
    case LAMP_TAIL_STOP: return fmaxf(state->tail, state->stop);
    }
    return 0;
}

void RenderCarSpotLights(RenderWorld *world) {
    for (uint32_t i = 0; i < world->instanceCount; ++i) {
        const RenderMeshInstance *body = &world->instances[i];
        const Lamp *lamps;
        if (body->pass != RAGE_RENDER_PASS_MAIN) continue;
        unsigned count = CarLamps(body, &lamps);
        if (!count) continue;
        RenderInstanceTransform transform = RenderPrepareInstanceTransform(&body->transform);
        for (unsigned j = 0; j < count; ++j) {
            const Lamp *lamp = &lamps[j];
            float strength = CarLampIntensity(&body->lamps, lamp->kind);
            if (strength <= 0) continue;
            int front = lamp->kind == LAMP_HEAD;
            SpotLight light = {0};
            light.position = RenderTransformInstancePoint(&transform, lamp->position);
            light.direction = RenderRotateInstanceVector(&transform,
                (Vec3){0, -0.06f, front ? 1.0f : -1.0f});
            light.range = front ? 1200.0f : 160.0f;
            light.innerCos = front ? 0.96f : 0.75f;
            light.outerCos = front ? 0.80f : 0.25f;
            light.color = front
                ? (Vec3){strength * 5, strength * 4.7f, strength * 4}
                : (Vec3){strength * 1.5f, strength * 0.025f, strength * 0.01f};
            RenderWorldSubmitSpotLight(world, &light);
        }
    }
}
