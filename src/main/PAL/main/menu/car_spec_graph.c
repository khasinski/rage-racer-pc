#include "game/asset.h"
#include "game/menu.h"

enum {
    CAR_SPEC_BAR_COUNT = 4,
    CAR_SPEC_BAR_MAX = 0x60,
    CAR_SPEC_FLOOR_DELAY = 0x10,
    CAR_SPEC_COLOR_DELTA = 0x40
};

static const CVec s_carSpecGraphColors[CAR_SPEC_BAR_COUNT] = {
    {0xA6, 0x35, 0xAC, 0},
    {0x7D, 0x27, 0x96, 0},
    {0x54, 0x1C, 0x94, 0},
    {0x2C, 0x12, 0x83, 0},
};

static void ApproachPerformanceRating(s32 *value, s32 target) {
    if (*value < target && *value < CAR_SPEC_BAR_MAX) {
        (*value)++;
    } else if (*value > target && *value > 0) {
        (*value)--;
    }
}

static void UpdateCarSpecValues(u32 tireGrade) {
    s32 i;

    for (i = 0; i < 3; i++) {
        ApproachPerformanceRating(&g_CarSpecBars[i],
                                  g_CarModelAsset->performanceRatings[i]);
    }
    if (tireGrade < 5) {
        ApproachPerformanceRating(&g_CarSpecBars[3],
                                  10 + (s32)tireGrade * 20);
    }
}

static u8 Lighten(u8 value) {
    s32 result = value + CAR_SPEC_COLOR_DELTA;

    return result < 0x100 ? (u8)result : 0xFF;
}

static u8 Darken(u8 value) {
    return value > CAR_SPEC_COLOR_DELTA ? value - CAR_SPEC_COLOR_DELTA : 0;
}

static void DrawCarSpecFloorLine(void *ot, s32 lineStep) {
    s32 y = 0x13E - lineStep;

    DrawPolyLine3(ot, 0x52, y, 0x61, y, 0x99, y + 0x38, 0xB4, 0xB4,
                  0xB4, 0xFF);
    DrawPolyLine3(ot, 0x52, y + 1, 0x61, y + 1, 0x99, y + 0x39, 0xB4,
                  0xB4, 0xB4, 0xFF);
}

static void DrawCarSpecFloor(void *ot, s32 progress) {
    s32 lineStep;

    for (lineStep = 0; lineStep < progress; lineStep += 0x10) {
        DrawCarSpecFloorLine(ot, lineStep);
    }
    DrawCarSpecFloorLine(ot, progress);
}

static void DrawCarSpecBar(void *ot, s32 index, s32 revealedHeight) {
    const CVec *color;
    s32 offset;
    s32 height;
    s32 baseX;
    s32 baseY;
    s32 rightX;
    s32 backY;
    s32 topY;
    s32 farX;
    s32 shadowOffset;

    if (revealedHeight == 0) {
        return;
    }

    height = revealedHeight < g_CarSpecBars[index]
                 ? revealedHeight
                 : g_CarSpecBars[index];
    offset = index * 0xC;
    baseX = 0x66 + offset;
    baseY = 0x144 + offset;
    rightX = 0x6E + offset;
    backY = 0x14B + offset;
    farX = 0x71 + offset;
    topY = baseY - height;
    color = &s_carSpecGraphColors[index];

    DrawFlatQuad(ot, baseX, baseY, baseX, topY, rightX, backY, rightX,
                 topY + 7, color->r, color->g, color->b, 0, 0xFF);
    DrawFlatQuad(ot, baseX, topY, rightX, topY + 7, 0x69 + offset,
                 topY - 4, farX, topY + 3, Lighten(color->r),
                 Lighten(color->g), Lighten(color->b), 0, 0xFF);
    DrawFlatQuad(ot, rightX, backY, rightX, topY + 7, farX, 0x147 + offset,
                 farX, topY + 3, Darken(color->r), Darken(color->g),
                 Darken(color->b), 0, 0xFF);

    shadowOffset = height / 4;
    DrawFlatQuad(ot, baseX, baseY, baseX - shadowOffset,
                 baseY + shadowOffset, rightX, backY,
                 baseX - shadowOffset + 8, baseY + shadowOffset + 8, 0x20,
                 0x20, 0x20, 1, 0);
}

/* The four animated performance bars on the CUSTOMIZE car panel. */
void DrawCarSpecGraph(s32 step, u32 tireGrade) {
    s32 revealed[CAR_SPEC_BAR_COUNT];
    s32 floorProgress;
    s32 i;
    void *ot;

    if (step == 0) {
        g_CarSpecGraphProgress = 0;
        return;
    }

    UpdateCarSpecValues(tireGrade);
    g_CarSpecGraphProgress += step;
    if (g_CarSpecGraphProgress > CAR_SPEC_BAR_MAX) {
        g_CarSpecGraphProgress = CAR_SPEC_BAR_MAX;
    } else if (g_CarSpecGraphProgress < 0) {
        g_CarSpecGraphProgress = 0;
    }

    for (i = 0; i < CAR_SPEC_BAR_COUNT; i++) {
        revealed[i] = g_CarSpecGraphProgress - CAR_SPEC_BAR_MAX +
                      g_CarSpecBars[i];
        if (revealed[i] < 0) {
            revealed[i] = 0;
        }
    }
    floorProgress = g_CarSpecGraphProgress - CAR_SPEC_FLOOR_DELAY;
    if (floorProgress < 0) {
        floorProgress = 0;
    }
    if (g_CarSpecGraphProgress == 0 || g_MenuAltLayout != 0) {
        return;
    }

    ot = RENDER_OT_BASE_AS(OT_TYPE) + 3;
    DrawSprite(ot, 0x4A, 0x166, 4, 8, 0xF0, 8, 0, 0, 0, 0x26C, 1, 0,
               0x19);
    DrawSprite(ot, 0x4A, 0x173, 4, 8, 0xF4, 8, 0, 0, 0, 0x26C, 1, 0,
               0x19);
    DrawSprite(ot, 0x4A, 0x180, 4, 8, 0xF8, 8, 0, 0, 0, 0x26C, 1, 0,
               0x19);
    DrawSprite(ot, 0x4A, 0x18D, 4, 8, 0xFC, 8, 0, 0, 0, 0x26C, 1, 0,
               0x19);
    DrawSprite(ot, 0x50, 0x165, 0x34, 0xC, 0, 0xE8, 0, 0, 0, 0x244, 1, 1,
               0x3A);
    DrawSprite(ot, 0x50, 0x172, 0x38, 0xC, 0x38, 0xE8, 0, 0, 0, 0x244, 1,
               1, 0x3A);
    DrawSprite(ot, 0x50, 0x17F, 0x24, 0xC, 0x70, 0xE8, 0, 0, 0, 0x244, 1,
               1, 0x3A);
    DrawSprite(ot, 0x50, 0x18C, 0x10, 0xC, 0x98, 0xE8, 0, 0, 0, 0x244, 1,
               1, 0x3A);

    DrawCarSpecFloor(ot, floorProgress);
    for (i = 0; i < CAR_SPEC_BAR_COUNT; i++) {
        DrawCarSpecBar(ot, i, revealed[i]);
    }
}
