#ifndef GAME_CAR_RENDER_RULES_H
#define GAME_CAR_RENDER_RULES_H

#include "common.h"

typedef u8 MirrorBadgeStyle;

enum {
    MIRROR_BADGE_STYLE_NARROW,
    MIRROR_BADGE_STYLE_WIDE,
    MIRROR_BADGE_STYLE_MEDIUM,
    MIRROR_BADGE_STYLE_LARGE,
    MIRROR_BADGE_STYLE_COUNT,
    MIRROR_BADGE_STYLE_STORAGE_COUNT = 16,
};

typedef struct MirrorBadgeSprite {
    u8 textureU;
    u8 textureV;
    u8 width;
} MirrorBadgeSprite;

_Static_assert(sizeof(MirrorBadgeSprite) == 3,
               "mirror badge sprite must remain a packed byte triple");

typedef enum CarRenderRange {
    CAR_RENDER_BEHIND,
    CAR_RENDER_CLOSE,
    CAR_RENDER_FAR,
    CAR_RENDER_CULLED,
} CarRenderRange;

s32 CarRenderManhattanDistance(s32 x, s32 z, s32 viewX, s32 viewZ);
CarRenderRange ClassifyCarRenderRange(s32 viewDepth, s32 distance);
s32 ResolveCarModelBank(s32 baseBank, s32 offset, s32 bankCount);
MirrorBadgeStyle ResolveMirrorBadgeStyle(
    s32 carIndex, const MirrorBadgeStyle *styles, s32 carCount);
s32 AdvanceMirrorPanelY(s32 currentY, int enabled);

#endif
