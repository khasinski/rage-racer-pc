#include "common.h"
#include "game/asset.h"
#include "game/menu.h"
#include "game/render.h"
#include "game/asset_internal.h"
#include "game/scratchpad.h"
#include "game/state.h"
#include "game/vector.h"


/* The five-position tire-compound slider of the CUSTOMIZE screen. */
void DrawTireCompoundSlider(u8 x, s32 useFlag) {
    OT_TYPE *ot;
    OT_TYPE *scratch;
    s32 gray;
    s32 alpha;
    s32 angle;
    s32 color;
    s32 zero;
    s32 xTest;
    s32 yLarge;
    s32 ySmall;

    scratch = SCRATCH_OT_BASE_AS(OT_TYPE);

    switch ((u8)x) {
    case 4:
        x = 0xB8;
        break;
    case 3:
        x = 0xC7;
        break;
    case 2:
        x = 0xD7;
        break;
    case 1:
        x = 0xE7;
        break;
    case 0:
        x = 0xF7;
        break;
    }

    if (useFlag != 0) {
        if (g_AnimTimer & 2) {
            alpha = 0xFF;
        } else {
            alpha = 0x60;
        }
    } else {
        angle = g_TireSliderPulsePhase;
        color = rsin(angle % 0x1000);
        if (color < 0) {
            color += 0x3F;
        }
        alpha = (color >> 6) - 0x41;
    }

    gray = 0xB4;
    ot = scratch + 2;

    DrawSprite(ot, 0xBC, 0x50, 0x14, 0x10, 0, 0xB4, 0, 0, 0, 0x244, 1, 1, 0x3A);
    DrawSprite(ot, 0xE0, 0x72, 0x14, 0x10, 0x14, 0xB4, 0, 0, 0, 0x244, 1, 1, 0x3A);

    xTest = (u8)x;
    zero = 0;
    if (xTest != 0xB8) {
        s32 green = (u8)alpha;

        DrawLine(ot, (s16)((u8)x - 1), 0x4C, (s16)((u8)x - 1), 0x84, zero, green, zero, 0xFF);
        DrawLine(ot, (s16)((u8)x - 3), 0x60, (s16)((u8)x - 3), 0x68, zero, green, zero, 0xFF);
        DrawLine(ot, (s16)((u8)x - 5), 0x60, (s16)((u8)x - 5), 0x68, zero, green, zero, 0xFF);
        DrawLine(ot, (s16)((u8)x - 7), 0x60, (s16)((u8)x - 7), 0x68, zero, green, zero, 0xFF);
        DrawFlatTriangle(ot, (s16)((u8)x - 13), 0x64, (s16)((u8)x - 8), 0x5E, (s16)((u8)x - 8), 0x6A,
                      zero, green, zero, zero, 0x80);
    }

    if (xTest != 0xF7) {
        s32 green = (u8)alpha;

        DrawLine(ot, (s16)((u8)x + 1), 0x4C, (s16)((u8)x + 1), 0x84, zero, green, zero, 0xFF);
        DrawLine(ot, (s16)((u8)x + 3), 0x6A, (s16)((u8)x + 3), 0x72, zero, green, zero, 0xFF);
        DrawLine(ot, (s16)((u8)x + 5), 0x6A, (s16)((u8)x + 5), 0x72, zero, green, zero, 0xFF);
        DrawLine(ot, (s16)((u8)x + 7), 0x6A, (s16)((u8)x + 7), 0x72, zero, green, zero, 0xFF);
        DrawFlatTriangle(ot, (s16)((u8)x + 14), 0x6E, (s16)((u8)x + 9), 0x69, (s16)((u8)x + 9), 0x73,
                      zero, green, zero, zero, 0x80);
    }

    DrawRectOutline(ot, 0xB8, 0x48, 0x40, 0x40, gray, gray, gray, 0xFF);
    yLarge = 0x85;
    ySmall = 0x60;
    DrawLine(ot, 0xC7, 0x4A, 0xC7, yLarge, gray, gray, gray, ySmall);
    DrawLine(ot, 0xD7, 0x4A, 0xD7, yLarge, gray, gray, gray, ySmall);
    DrawLine(ot, 0xE7, 0x4A, 0xE7, yLarge, gray, gray, gray, ySmall);

    DrawFlatTriangle(ot, 0xB9, 0x87, 0xF7, 0x48, 0xF7, 0x87, 0x1E, 0x8E, 0x95, zero, 0x80);
    DrawSolidRect(ot, 0xB8, 0x48, 0x40, 0x40, 0x95, 0x25, 0x1E, 0xFF);

    g_TireSliderPulsePhase += 0x60;
}

/* The two side browse arrows, each lit only when that direction has somewhere to go. */
void DrawBrowseArrows(s32 step, s32 wide, s32 drawLeft, s32 drawRight) {
    u32 *ot;
    s32 halfWidth;
    s32 y;
    s32 phase;
    s32 wave;
    s32 leftX;
    s32 intensityBias;
    s16 callY;
    u32 flags;
    s16 leftEdge;
    s16 rightEdge;
    u8 intensity;

    ot = SCRATCH_OT_BASE_AS(void);
    if (step == 0) {
        g_BrowseArrowsFade = 0;
        return;
    }

    if (step < 0) {
        g_BrowseArrowsFade += step;
        if (g_BrowseArrowsFade < 0) {
            g_BrowseArrowsFade = 0;
        }
    }

    intensityBias = 65;
    if (wide != 0 || g_MenuAltLayout != 0) {
        halfWidth = 0x58;
    } else {
        halfWidth = 1;
    }

    y = 0x119;
    if (wide != 0) {
        y = 0x144;
    }
    phase = g_BrowseArrowsFade - 11;
    if (phase >= 0) {
        u32 slidePhase;

        if (phase >= 11) {
            phase = 10;
        }

        slidePhase = phase;
        leftX = ((slidePhase * 0x250) >> 5) + 0xFFE7;
        wave = rsin(g_BrowseArrowsPulsePhase % 0x1000);
        intensity = (wave / 64) - intensityBias;
        leftEdge = leftX - halfWidth;
        g_BrowseArrowsPulsePhase += 0x60;
        callY = y;
        flags = 0x19;

        DrawSprite(ot, leftEdge, callY, 0x10, 0x20, 0x48, 0xB8,
                      0, 0, 0, 0x25A, 1, 0, flags);
        rightEdge = 0x1BF - leftX;
        DrawSprite(ot, rightEdge, callY, 0x10, 0x20, 0x58, 0xB8,
                      0, 0, 0, 0x25A, 1, 0, flags);

        if (drawLeft != 0) {
            DrawSolidRect(ot, leftEdge, callY, 0x10, 0x20,
                           0, intensity, 0, 0xFF);
        }
        if (drawRight != 0) {
            DrawSolidRect(ot, rightEdge, callY, 0x10, 0x20,
                           0, intensity, 0, 0xFF);
        }
    }

    if (step > 0) {
        halfWidth = step;
        g_BrowseArrowsFade += halfWidth;
        if (g_BrowseArrowsFade >= 26) {
            g_BrowseArrowsFade = 25;
        }
    }
}

typedef struct CarSpecGraphColors {
    CVec colors[4];
} CarSpecGraphColors;

const CarSpecGraphColors g_CarSpecGraphColors = {{
    {0xA6, 0x35, 0xAC, 0},
    {0x7D, 0x27, 0x96, 0},
    {0x54, 0x1C, 0x94, 0},
    {0x2C, 0x12, 0x83, 0},
}};

/* The four animated performance bars on the CUSTOMIZE car panel. */
void DrawCarSpecGraph(s32 step, u32 tireGrade) {
    s32 revealed[4];
    CarSpecGraphColors colors;
    const CarSpecGraphColors *sourceColors;
    void *ot;
    u16 markerClut;
    u8 baseR;
    u8 baseG;
    u8 baseB;
    u8 lightR;
    u8 lightG;
    u8 lightB;
    u8 darkR;
    u8 darkG;
    u8 darkB;
    s32 *scanBar;
    s32 *scanRevealed;
    s32 *revealedBar;
    s32 *bar;
    s32 revealedValue;
    CVec *color;
    s32 revealBase;
    s32 floorProgress;
    s16 lineX;
    s32 lineNearX;
    s32 lineStep;
    s16 lineFarX;
    u8 lineColor;
    u8 lineAlpha;
    s32 i;
    s32 offset;
    s32 height;
    s32 topY;
    s32 topRightY;
    s32 rightX;
    s32 backY;
    s32 leftX;
    s32 topLeftY;
    s32 farX;
    s32 topFarY;
    s32 farY;
    s32 lightAdjustment;
    s32 darkAdjustment;
    s32 lightRValue;
    s32 lightGValue;
    s32 lightBValue;
    s32 darkRValue;
    s32 darkGValue;
    s32 darkBValue;
    s32 shadowHeight;
    s32 quarterHeight;
    s32 shadowX;
    s32 shadowY;

    if (step == 0) {
        g_CarSpecGraphProgress = 0;
        return;
    }

    ot = SCRATCH_OT_BASE_AS(OT_TYPE) + 3;
    sourceColors = &g_CarSpecGraphColors;
    colors = *sourceColors;
    markerClut = 0x26C;

    {
        s32 *value = &g_CarSpecBars[0];
        if ((*value < g_CarModelAsset->performanceRatings[0]) && (*value < 0x60)) {
            (*value)++;
        } else if ((g_CarModelAsset->performanceRatings[0] < *value) && (*value > 0)) {
            (*value)--;
        }
    }

    {
        s32 *value = &g_CarSpecBars[1];
        if ((*value < g_CarModelAsset->performanceRatings[1]) && (*value < 0x60)) {
            (*value)++;
        } else if ((g_CarModelAsset->performanceRatings[1] < *value) && (*value > 0)) {
            (*value)--;
        }
    }

    {
        s32 *value = &g_CarSpecBars[2];
        if ((*value < g_CarModelAsset->performanceRatings[2]) && (*value < 0x60)) {
            (*value)++;
        } else if ((g_CarModelAsset->performanceRatings[2] < *value) && (*value > 0)) {
            (*value)--;
        }
    }

    switch (tireGrade) {
    case 0:
        if (g_CarSpecBars[3] < 10) {
            g_CarSpecBars[3]++;
        } else if (g_CarSpecBars[3] > 10) {
            g_CarSpecBars[3]--;
        }
        break;
    case 1:
        if (g_CarSpecBars[3] < 30) {
            g_CarSpecBars[3]++;
        } else if (g_CarSpecBars[3] > 30) {
            g_CarSpecBars[3]--;
        }
        break;
    case 2:
        if (g_CarSpecBars[3] < 50) {
            g_CarSpecBars[3]++;
        } else if (g_CarSpecBars[3] > 50) {
            g_CarSpecBars[3]--;
        }
        break;
    case 3:
        if (g_CarSpecBars[3] < 70) {
            g_CarSpecBars[3]++;
        } else if (g_CarSpecBars[3] > 70) {
            g_CarSpecBars[3]--;
        }
        break;
    case 4:
        if (g_CarSpecBars[3] < 90) {
            g_CarSpecBars[3]++;
        } else if (g_CarSpecBars[3] > 90) {
            g_CarSpecBars[3]--;
        }
        break;
    }

    if (step > 0) {
        g_CarSpecGraphProgress += step;
        if (g_CarSpecGraphProgress >= 0x61) {
            g_CarSpecGraphProgress = 0x60;
        }
    } else {
        g_CarSpecGraphProgress += step;
        if (g_CarSpecGraphProgress < 0) {
            g_CarSpecGraphProgress = 0;
        }
    }

    i = 0;
    scanRevealed = revealed;
    scanBar = g_CarSpecBars;
    revealBase = g_CarSpecGraphProgress - 0x60;
    do {
        *scanRevealed = revealBase + *scanBar;
        if (*scanRevealed < 0) {
            *scanRevealed = 0;
        }
        scanRevealed++;
        i++;
        scanBar++;
    } while (i < 4);

    floorProgress = g_CarSpecGraphProgress - 0x10;
    if (floorProgress < 0) {
        floorProgress = 0;
    }
    if (g_CarSpecGraphProgress == 0 || g_MenuAltLayout != 0) {
        return;
    }

    DrawSprite(ot, 0x4A, 0x166, 4, 8, 0xF0, 8, 0, 0, 0, markerClut, 1, 0, 0x19);
    DrawSprite(ot, 0x4A, 0x173, 4, 8, 0xF4, 8, 0, 0, 0, markerClut, 1, 0, 0x19);
    DrawSprite(ot, 0x4A, 0x180, 4, 8, 0xF8, 8, 0, 0, 0, markerClut, 1, 0, 0x19);
    DrawSprite(ot, 0x4A, 0x18D, 4, 8, 0xFC, 8, 0, 0, 0, markerClut, 1, 0, 0x19);

    DrawSprite(ot, 0x50, 0x165, 0x34, 0xC, 0, 0xE8, 0, 0, 0, 0x244, 1, 1, 0x3A);
    DrawSprite(ot, 0x50, 0x172, 0x38, 0xC, 0x38, 0xE8, 0, 0, 0, 0x244, 1, 1, 0x3A);
    DrawSprite(ot, 0x50, 0x17F, 0x24, 0xC, 0x70, 0xE8, 0, 0, 0, 0x244, 1, 1, 0x3A);
    DrawSprite(ot, 0x50, 0x18C, 0x10, 0xC, 0x98, 0xE8, 0, 0, 0, 0x244, 1, 1, 0x3A);

    if (floorProgress > 0) {
        s32 loopLineColor;
        s32 loopLineFarX;
        s32 loopLineAlpha;

        loopLineColor = 0xB4;
        loopLineFarX = 0x99;
        loopLineAlpha = 0xFF;
        lineStep = 0;
        do {
            lineNearX = 0x52;
            lineX = 0x13E - lineStep;
            DrawPolyLine3(ot, lineNearX, (s16)lineX, 0x61, (s16)lineX,
                                loopLineFarX, (s16)(lineX + 0x38),
                                loopLineColor, loopLineColor, loopLineColor,
                                loopLineAlpha);
            DrawPolyLine3(ot, 0x52, (s16)(lineX + 1), 0x61,
                                (s16)(lineX + 1), loopLineFarX,
                                (s16)(lineX + 0x39),
                                loopLineColor, loopLineColor, loopLineColor,
                                loopLineAlpha);
            lineStep += 0x10;
        } while (lineStep < floorProgress);
    }

    lineFarX = 0x99;
    lineColor = 0xB4;
    lineAlpha = 0xFF;
    lineNearX = 0x52;
    lineX = 0x13E - floorProgress;
    DrawPolyLine3(ot, lineNearX, (s16)lineX, 0x61, (s16)lineX,
                        lineFarX, (s16)(lineX + 0x38),
                        lineColor, lineColor, lineColor, lineAlpha);
    DrawPolyLine3(ot, 0x52, (s16)(lineX + 1), 0x61,
                        (s16)(lineX + 1), lineFarX,
                        (s16)(lineX + 0x39),
                        lineColor, lineColor, lineColor, lineAlpha);

    {
        s32 baseYHalf;
        s32 baseY, baseX;

        i = 0;
        lightAdjustment = 0x40;
        darkAdjustment = 0x40;
        
        
        baseY = 0x144;
        baseX = 0x66;
        offset = 0;
        do {
        revealedBar = &revealed[i];
        revealedValue = *revealedBar;
        if (revealedValue != 0) {
            bar = g_CarSpecBars + i;
            height = revealedValue;
            if (*bar < height) {
                height = *bar;
            }

            color = &colors.colors[i];
            rightX = offset + 0x6E;
            backY = offset + 0x14B;
            leftX = offset + 0x69;
            baseR = color->r;
            lightRValue = baseR + lightAdjustment;
            topY = baseY - height;
            topRightY = topY + 7;
            topLeftY = topY - 4;
            farX = offset + 0x71;
            topFarY = topY + 3;
            farY = offset + 0x147;
            if (lightRValue >= 0x100) {
                lightRValue = 0xFF;
            }
            lightR = lightRValue;

            baseG = color->g;
            lightGValue = baseG + lightAdjustment;
            if (lightGValue >= 0x100) {
                lightGValue = 0xFF;
            }
            lightG = lightGValue;

            baseB = color->b;
            lightBValue = baseB + lightAdjustment;
            if (lightBValue >= 0x100) {
                lightBValue = 0xFF;
            }
            lightB = lightBValue;

            darkRValue = baseR - darkAdjustment;
            if (darkRValue < 0) {
                darkRValue = 0;
            }
            darkR = darkRValue;

            darkGValue = baseG - darkAdjustment;
            if (darkGValue < 0) {
                darkGValue = 0;
            }
            darkG = darkGValue;

            darkBValue = baseB - darkAdjustment;
            if (darkBValue < 0) {
                darkBValue = 0;
            }
            darkB = darkBValue;

            DrawFlatQuad(ot, (s16)baseX, baseYHalf = (s16)baseY,
                            (s16)baseX, (s16)topY,
                            (s16)rightX, (s16)backY,
                            (s16)rightX, (s16)topRightY,
                            color->r, color->g, color->b, 0, 0xFF);
            DrawFlatQuad(ot, (s16)baseX, (s16)topY,
                                  (s16)rightX, (s16)topRightY,
                                  (s16)leftX, (s16)topLeftY,
                                  (s16)farX, (s16)topFarY,
                                  lightR, lightG, lightB, 0, 0xFF);
            DrawFlatQuad(ot, (s16)rightX, (s16)backY,
                                  (s16)rightX, (s16)topRightY,
                                  (s16)farX, (s16)farY,
                                  (s16)farX, (s16)topFarY,
                                  darkR, darkG, darkB, 0, 0xFF);

            shadowHeight = *revealedBar;
            if (*bar < shadowHeight) {
                shadowHeight = *bar;
            }
            quarterHeight = shadowHeight / 4;
            shadowX = baseX - quarterHeight;
            shadowY = quarterHeight + baseY;
            DrawFlatQuad(ot, (s16)baseX, baseYHalf,
                            (s16)shadowX, (s16)shadowY,
                            (s16)rightX, (s16)backY,
                            (s16)(shadowX + 8), (s16)(shadowY + 8),
                            0x20, 0x20, 0x20, 1, 0);
        }
        offset += 0xC;
        i++;
        baseY += 0xC;
        baseX += 0xC;
        } while (i < 4);
    }
}
