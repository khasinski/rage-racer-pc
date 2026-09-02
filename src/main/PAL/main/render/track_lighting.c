#include "game/race.h"
#include "game/render_internal.h"

enum {
    LIGHTING_BLEND_MAX = 0x100,
};

static void ScaleColorRow(Matrix *output, const Matrix *source, s32 row,
                          s32 scale) {
    s32 column;

    for (column = 0; column < 3; column++) {
        output->m[row][column] =
            source->m[row][column] * scale / LIGHTING_BLEND_MAX;
    }
}

static void BlendLightMatrix(Matrix *matrix, s32 blend) {
    s32 originalWeight = LIGHTING_BLEND_MAX - blend;
    s32 row;
    s32 column;

    for (row = 0; row < 3; row++) {
        for (column = 0; column < 3; column++) {
            matrix->m[row][column] =
                matrix->m[row][column] * originalWeight / LIGHTING_BLEND_MAX +
                g_TrackLightMatrix.m[row][column] * blend /
                    LIGHTING_BLEND_MAX;
        }
    }
}

/* Darkens the scene colour matrix by GetTrackZoneBlend's 0..0x100 ramp. */
void ApplyZoneLighting(s32 blend, Matrix *lightMatrix) {
    Matrix colorMatrix;
    s32 row;

    if (blend < 0) {
        blend = 0;
    } else if (blend > LIGHTING_BLEND_MAX) {
        blend = LIGHTING_BLEND_MAX;
    }

    if (g_TrackZoneCode != 0) {
        s32 scale = LIGHTING_BLEND_MAX - (blend * 3) / 4;

        for (row = 0; row < 3; row++) {
            ScaleColorRow(&colorMatrix, &g_SceneColorMatrix, row, scale);
        }
    } else {
        ScaleColorRow(&colorMatrix, &g_SceneColorMatrix, 0,
                      LIGHTING_BLEND_MAX);
        ScaleColorRow(&colorMatrix, &g_SceneColorMatrix, 1,
                      LIGHTING_BLEND_MAX - blend / 2);
        ScaleColorRow(&colorMatrix, &g_SceneColorMatrix, 2,
                      LIGHTING_BLEND_MAX - (blend * 3) / 4);
        BlendLightMatrix(lightMatrix, blend);
    }
    SetColorMatrix(&colorMatrix);
}

void RestoreColorMatrix(void) { SetColorMatrix(&g_SceneColorMatrix); }

void InitTrackLighting(void) {
    g_SceneColorMatrix = g_TrackColorMatrix;
    g_SceneLightMatrix = g_TrackLightMatrix;
    SetColorMatrix(&g_SceneColorMatrix);
    SetLightMatrix(&g_SceneLightMatrix);
    SetBackColor(0x20, 0x20, 0x20);
    SetFogNear(0x1770, 0x140);
    SetFarColor(0x80, 0x80, 0x80);
}
