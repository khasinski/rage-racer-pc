#include "game/car_render_rules.h"

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

enum {
    CAR_CLOSE_DISTANCE = 0xD00,
    CAR_CULL_DISTANCE = 0x2500,
    CAR_MODEL_BANK_FALLBACK = 1,
    MIRROR_PANEL_HIDDEN_Y = -44,
    MIRROR_PANEL_VISIBLE_Y = 18,
};

s32 CarRenderManhattanDistance(s32 x, s32 z, s32 viewX, s32 viewZ) {
    int64_t deltaX = (int64_t)x - viewX;
    int64_t deltaZ = (int64_t)z - viewZ;
    int64_t distance;

    if (deltaX < 0) deltaX = -deltaX;
    if (deltaZ < 0) deltaZ = -deltaZ;
    distance = deltaX + deltaZ;
    return distance > INT_MAX ? INT_MAX : (s32)distance;
}

s32 AdvanceMirrorPanelY(s32 currentY, int enabled) {
    if (enabled) {
        return currentY < MIRROR_PANEL_VISIBLE_Y ? currentY + 1 : currentY;
    }
    return currentY > MIRROR_PANEL_HIDDEN_Y ? currentY - 1 : currentY;
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
    const int64_t requestedBank = (int64_t)baseBank + offset;

    return requestedBank >= 0 && requestedBank < bankCount
        ? (s32)requestedBank
        : CAR_MODEL_BANK_FALLBACK;
}

MirrorBadgeStyle ResolveMirrorBadgeStyle(
    s32 carIndex, const MirrorBadgeStyle *styles, s32 carCount) {
    MirrorBadgeStyle style;

    if (styles == NULL || carCount <= 0 || (u32)carIndex >= (u32)carCount) {
        return MIRROR_BADGE_STYLE_NARROW;
    }
    style = styles[carIndex];
    return style < MIRROR_BADGE_STYLE_COUNT
        ? style
        : MIRROR_BADGE_STYLE_NARROW;
}
