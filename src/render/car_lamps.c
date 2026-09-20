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
        /* Inner rear-fender lenses share UVs, but have separate emitters. */
        {5, {71, 85, 75, 89}, {-114, 4.03590f, -92.69744f}, LAMP_TAIL_STOP, 1},
        {5, {71, 85, 75, 89}, {115.5f, 3.63942f, -92.58494f}, LAMP_TAIL_STOP, 1},
    };
    static const Lamp compact[] = {
        {3, {204, 46, 212, 57}, {-64.88971f, 41.61765f, 342.73235f}, LAMP_HEAD, 1},
        {3, {204, 118, 212, 129}, {64.42647f, 41.61765f, 342.91765f}, LAMP_HEAD, 1},
        /* Upper red lenses only; leave the white and amber sections unlit. */
        {0, {110, 13, 113, 17}, {74.35294f, 47, -48.73529f}, LAMP_TAIL_STOP, 0},
        {0, {174, 13, 177, 17}, {-74.13333f, 47, -47.5f}, LAMP_TAIL_STOP, 0},
    };
    static const Lamp coupe[] = {
        /* White inner lenses; the amber outer corners are indicators. */
        {3, {15, 8, 33, 13}, {58.925f, 41.2f, 416.225f}, LAMP_HEAD, 1},
        {3, {62, 8, 80, 13}, {-58.925f, 41.2f, 416.225f}, LAMP_HEAD, 1},
        /* Lower outer rear lenses, excluding the white reversing lights. */
        {0, {102, 15, 110, 20}, {107.25127f, 48.48477f, -51.85787f}, LAMP_TAIL_STOP, 1},
        {0, {177, 15, 185, 20}, {-107.09167f, 47.95f, -51.76667f}, LAMP_TAIL_STOP, 1},
    };
    static const Lamp sport[] = {
        {3, {7, 3, 16, 11}, {-49.55208f, 33.20833f, 381.82292f}, LAMP_HEAD, 1},
        {3, {80, 3, 89, 11}, {50.60417f, 33.16667f, 381.27083f}, LAMP_HEAD, 1},
        {0, {100, 6, 109, 11}, {102.78723f, 39.55319f, -74.76596f}, LAMP_TAIL_STOP, 1},
        {0, {178, 6, 187, 11}, {-101.87805f, 39.58537f, -74.78049f}, LAMP_TAIL_STOP, 1},
    };
    *lamps = NULL;
    if (body->component == 0 && body->assetSet == RAGE_RENDER_ASSET_MODEL_BANK &&
        body->assetKey == 24 && body->mesh == 0) {
        *lamps = sport;
        return sizeof(sport) / sizeof(*sport);
    }
    if (body->component == 0 && body->assetSet == RAGE_RENDER_ASSET_MODEL_BANK &&
        body->assetKey == 18 && body->mesh == 0) {
        *lamps = coupe;
        return sizeof(coupe) / sizeof(*coupe);
    }
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
