#include "common.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/render_internal.h"

#include <stdio.h>
#include <string.h>

Matrix g_SceneColorMatrix;
Matrix g_SceneLightMatrix;
Matrix g_TrackColorMatrix;
Matrix g_TrackLightMatrix;
s16 g_TrackZoneCode;

static Matrix s_colorSet;
static Matrix s_lightSet;
static s32 s_colorCalls;
static s32 s_lightCalls;
static s32 s_back[3];
static s32 s_far[3];
static s32 s_fog[2];

void SetColorMatrix(MATRIX *matrix) {
    memcpy(s_colorSet.m, matrix->m, sizeof(s_colorSet.m));
    s_colorCalls++;
}
void SetLightMatrix(MATRIX *matrix) {
    memcpy(s_lightSet.m, matrix->m, sizeof(s_lightSet.m));
    s_lightCalls++;
}
void SetBackColor(long r, long g, long b) {
    s_back[0] = (s32)r; s_back[1] = (s32)g; s_back[2] = (s32)b;
}
void SetFarColor(long r, long g, long b) {
    s_far[0] = (s32)r; s_far[1] = (s32)g; s_far[2] = (s32)b;
}
void SetFogNear(long distance, long projection) {
    s_fog[0] = (s32)distance; s_fog[1] = (s32)projection;
}

#define CHECK(condition) do {                                                  \
    if (!(condition)) {                                                        \
        fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #condition);\
        return 1;                                                              \
    }                                                                          \
} while (0)

static void FillMatrix(Matrix *matrix, s16 value) {
    s32 row;
    s32 column;
    memset(matrix, 0, sizeof(*matrix));
    for (row = 0; row < 3; row++) {
        for (column = 0; column < 3; column++) {
            matrix->m[row][column] = value;
        }
    }
}

int main(void) {
    Matrix light;

    FillMatrix(&g_SceneColorMatrix, 1000);
    FillMatrix(&g_TrackLightMatrix, 600);
    FillMatrix(&light, 200);
    g_TrackZoneCode = 1;
    ApplyZoneLighting(128, &light);
    CHECK(s_colorCalls == 1);
    CHECK(s_colorSet.m[0][0] == 625 && s_colorSet.m[2][2] == 625);
    CHECK(light.m[0][0] == 200 && light.m[2][2] == 200);

    g_TrackZoneCode = 0;
    ApplyZoneLighting(128, &light);
    CHECK(s_colorSet.m[0][0] == 1000);
    CHECK(s_colorSet.m[1][1] == 750);
    CHECK(s_colorSet.m[2][2] == 625);
    CHECK(light.m[0][0] == 400 && light.m[2][2] == 400);

    RestoreColorMatrix();
    CHECK(s_colorCalls == 3 && s_colorSet.m[0][0] == 1000);

    FillMatrix(&g_TrackColorMatrix, 300);
    FillMatrix(&g_TrackLightMatrix, 700);
    InitTrackLighting();
    CHECK(g_SceneColorMatrix.m[1][1] == 300);
    CHECK(g_SceneLightMatrix.m[1][1] == 700);
    CHECK(s_colorCalls == 4 && s_lightCalls == 1);
    CHECK(s_back[0] == 0x20 && s_back[1] == 0x20 && s_back[2] == 0x20);
    CHECK(s_far[0] == 0x80 && s_far[1] == 0x80 && s_far[2] == 0x80);
    CHECK(s_fog[0] == 0x1770 && s_fog[1] == 0x140);

    puts("zone lighting preserves blends and track initialization");
    return 0;
}
