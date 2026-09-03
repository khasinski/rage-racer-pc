#include "common.h"
#include "game/race.h"
#include "game/render.h"
#include "game/track_internal.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

s32 g_RaceSeries;
s32 g_CourseIndex;
s16 g_RacePhase;
s32 g_CourseModelCount;
GameRenderState g_RenderState;
Vec4 g_StartGridSceneryPos[2];
StartGridSceneryStep g_StartGridSceneryStep[2];
s32 g_StartGridSceneryAngle[2];

static s32 g_Submissions;
static s32 g_Model;
static s32 g_X;
static s32 g_Z;

void BuildRotMatrixY(void *matrix, s32 angle) {
    (void)angle;
    memset(matrix, 0, sizeof(Matrix));
}

#undef MulMatrix2
MATRIX *MulMatrix2(MATRIX *left, MATRIX *right) {
    (void)left;
    return right;
}

void SetGteObjectMatrix(LVec *position, Matrix *rotation) {
    (void)position;
    (void)rotation;
}
void SubmitCourseModel(void *renderState, s32 model) {
    (void)renderState;
    (void)model;
}
void GameRenderWorldSubmitDynamicCourseObject(
    u32 entity, s32 model, s32 x, s32 y, s32 z,
    const s16 rotation[3][3], int fogged, int mirrorPass) {
    (void)entity;
    (void)y;
    (void)rotation;
    (void)fogged;
    (void)mirrorPass;
    g_Submissions++;
    g_Model = model;
    g_X = x;
    g_Z = z;
}

static int Expect(s32 timer, s32 submissions, s32 model, s32 x, s32 z) {
    g_Submissions = 0;
    DrawStartGridScenery(timer);
    if (g_Submissions != submissions ||
        (submissions != 0 &&
         (g_Model != model || g_X != x || g_Z != z))) {
        printf("FAIL timer %d: submissions=%d model=%d pos=(%d,%d)\n",
               timer, g_Submissions, g_Model, g_X, g_Z);
        return 0;
    }
    return 1;
}

int main(void) {
    g_StartGridSceneryPos[0] = (Vec4){100, 200, 300, 400};
    g_StartGridSceneryPos[1] = (Vec4){1000, 2000, 3000, 4000};
    g_StartGridSceneryStep[0] = (StartGridSceneryStep){10, 20};
    g_StartGridSceneryStep[1] = (StartGridSceneryStep){100, 200};
    g_CourseModelCount = 64;
    g_RacePhase = 1;

    if (!Expect(80, 0, 0, 0, 0) ||
        !Expect(81, 1, 0x28, 100, 300) ||
        !Expect(93, 1, 0x29, 100, 300) ||
        !Expect(135, 1, 0x28, 110, 320)) {
        return 1;
    }

    g_RaceSeries = 1;
    g_CourseIndex = 3;
    if (!Expect(135, 1, 0x28, 1100, 3200 + 0x5000)) {
        return 1;
    }

    g_CourseModelCount = 40;
    if (!Expect(81, 1, 1, 1000, 3000 + 0x5000)) {
        return 1;
    }

    g_RacePhase = 2;
    if (!Expect(135, 0, 0, 0, 0)) {
        return 1;
    }

    g_RacePhase = 1;
    g_RaceSeries = 0;
    g_CourseIndex = 0;
    g_CourseModelCount = 64;
    g_StartGridSceneryPos[0].x = INT_MAX;
    g_StartGridSceneryPos[0].z = INT_MIN;
    g_StartGridSceneryStep[0] =
        (StartGridSceneryStep){INT16_MAX, INT16_MAX};
    if (!Expect(INT_MAX, 1, 0x34, -1813523841, -1813523840)) {
        return 1;
    }

    puts("start-grid scenery behavior preserved");
    return 0;
}
