#ifndef GAME_CAR_RENDER_RULES_H
#define GAME_CAR_RENDER_RULES_H

#include "common.h"

typedef enum CarRenderRange {
    CAR_RENDER_BEHIND,
    CAR_RENDER_CLOSE,
    CAR_RENDER_FAR,
    CAR_RENDER_CULLED,
} CarRenderRange;

s32 CarRenderManhattanDistance(s32 x, s32 z, s32 viewX, s32 viewZ);
CarRenderRange ClassifyCarRenderRange(s32 viewDepth, s32 distance);
s32 ResolveCarModelBank(s32 baseBank, s32 offset, s32 bankCount);
s32 ResolveMirrorBadgeSpriteIndex(s32 carIndex, const u8 *styles,
                                  s32 carCount);
s32 AdvanceMirrorPanelY(s32 currentY, int enabled);

#endif
