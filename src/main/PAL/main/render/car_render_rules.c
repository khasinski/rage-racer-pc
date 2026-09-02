#include "game/car_render_rules.h"

enum {
    CAR_CLOSE_DISTANCE = 0xD00,
    CAR_CULL_DISTANCE = 0x2500,
    CAR_MODEL_BANK_FALLBACK = 1,
};

static s32 Absolute(s32 value) {
    return value < 0 ? -value : value;
}

s32 CarRenderManhattanDistance(s32 x, s32 z, s32 viewX, s32 viewZ) {
    return Absolute(x - viewX) + Absolute(z - viewZ);
}

CarRenderRange ClassifyCarRenderRange(s32 viewDepth, s32 distance) {
    if (viewDepth < 0) {
        return CAR_RENDER_BEHIND;
    }
    if (distance < CAR_CLOSE_DISTANCE) {
        return CAR_RENDER_CLOSE;
    }
    if (distance < CAR_CULL_DISTANCE) {
        return CAR_RENDER_FAR;
    }
    return CAR_RENDER_CULLED;
}

s32 ResolveCarModelBank(s32 baseBank, s32 offset, s32 bankCount) {
    const s32 requestedBank = baseBank + offset;

    return requestedBank >= 0 && requestedBank < bankCount
        ? requestedBank
        : CAR_MODEL_BANK_FALLBACK;
}
