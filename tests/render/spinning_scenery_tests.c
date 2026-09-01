#include "common.h"
#include "game/race.h"
#include "game/render.h"
#include "game/track.h"

#include <stdio.h>
#include <string.h>

s32 g_CourseIndex;
s32 g_CourseModelCount;
GameRenderState g_RenderState;
ObjectMatrixWork g_ObjectMatrixWork;
SpinningSceneryPlacement g_SpinningSceneryPlacements[4];
s16 g_SpinningSceneryAngle[4];
u16 g_SpinningSceneryRate[4];

static s32 g_SubmissionCount;
static s32 g_Models[4];
static u32 g_Entities[4];
static s32 g_RandomValues[2];
static s32 g_RandomIndex;

s32 Random15(void) {
    return g_RandomValues[g_RandomIndex++];
}

void BuildRotMatrixY(void *matrix, s32 angle) {
    (void)angle;
    memset(matrix, 0, sizeof(Matrix));
}

void BuildRotMatrixZ(void *matrix, s32 angle) {
    (void)angle;
    memset(matrix, 0, sizeof(Matrix));
}

#undef MulMatrix2
MATRIX *MulMatrix2(MATRIX *left, MATRIX *right) {
    (void)left;
    return right;
}

void SetGteObjectMatrix(ObjectMatrixWork *work, LVec *position,
                        Matrix *rotation) {
    (void)work;
    (void)position;
    (void)rotation;
}

void SubmitCourseModel2(void *renderState, s32 model) {
    (void)renderState;
    g_Models[g_SubmissionCount++] = model;
}

void GameRenderWorldSubmitDynamicCourseObject(
    u32 entity, s32 model, s32 x, s32 y, s32 z,
    const s16 rotation[3][3], int fogged, int mirrorPass) {
    s32 index = g_SubmissionCount;

    (void)model;
    (void)x;
    (void)y;
    (void)z;
    (void)rotation;
    (void)fogged;
    (void)mirrorPass;
    g_Entities[index] = entity;
}

static void ResetSubmissions(void) {
    g_SubmissionCount = 0;
    memset(g_Models, 0, sizeof(g_Models));
    memset(g_Entities, 0, sizeof(g_Entities));
}

int main(void) {
    memset(g_SpinningSceneryAngle, 0, sizeof(g_SpinningSceneryAngle));
    g_SpinningSceneryRate[0] = 32;
    g_SpinningSceneryRate[1] = 64;
    g_CourseModelCount = 63;

    g_CourseIndex = 0;
    ResetSubmissions();
    DrawSpinningScenery(1, 1);
    if (g_SubmissionCount != 1 || g_Entities[0] != 0x100 ||
        g_Models[0] != 0x3E || g_SpinningSceneryAngle[0] != 32 ||
        g_SpinningSceneryAngle[1] != 0) {
        puts("FAIL: single spinner course");
        return 1;
    }

    g_CourseIndex = 1;
    g_CourseModelCount = 62;
    ResetSubmissions();
    DrawSpinningScenery(1, 1);
    if (g_SubmissionCount != 3 || g_Entities[0] != 0x101 ||
        g_Entities[1] != 0x102 || g_Entities[2] != 0x103 ||
        g_Models[0] != 1 || g_Models[1] != 1 || g_Models[2] != 1 ||
        g_SpinningSceneryAngle[1] != 64 ||
        g_SpinningSceneryAngle[2] != 64 ||
        g_SpinningSceneryAngle[3] != 64) {
        puts("FAIL: three spinner course");
        return 1;
    }

    g_SpinningSceneryAngle[1] = 0xFFF;
    ResetSubmissions();
    DrawSpinningScenery(2, 0);
    if (g_SpinningSceneryAngle[1] != 0xFFF) {
        puts("FAIL: frozen spinner angle");
        return 1;
    }

    g_RandomValues[0] = 0x7F;
    g_RandomValues[1] = 0xAA;
    g_RandomIndex = 0;
    DrawSpinningScenery(512, 1);
    if (g_RandomIndex != 2 || g_SpinningSceneryRate[0] != 0x1F ||
        g_SpinningSceneryRate[1] != 0x2A) {
        puts("FAIL: periodic spinner rate update");
        return 1;
    }

    puts("spinning scenery behavior preserved");
    return 0;
}
