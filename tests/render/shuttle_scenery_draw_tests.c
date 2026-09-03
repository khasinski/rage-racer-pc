#include "common.h"
#include "game/race.h"
#include "game/render.h"
#include "game/track_internal.h"

#include <stdio.h>
#include <string.h>

GameShuttleScenery g_ShuttleScenery[2];
GameRenderState g_RenderState;
s32 g_CourseIndex;
s32 g_CourseModelCount;

static int g_Visible;
static s32 g_ClassicSubmissions;
static s32 g_WorldSubmissions;
static s32 g_ClassicModel;
static s32 g_WorldModel;
static u32 g_WorldEntity;

int TrackCellVisible(s32 x, s32 z) {
    (void)x;
    (void)z;
    return g_Visible;
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

void SetGteObjectMatrix(const LVec *position, Matrix *rotation) {
    (void)position;
    (void)rotation;
}

void SubmitCourseModel(void *renderState, s32 model) {
    (void)renderState;
    g_ClassicSubmissions++;
    g_ClassicModel = model;
}

void GameRenderWorldSubmitDynamicCourseObject(
    u32 entity, s32 model, s32 x, s32 y, s32 z,
    const s16 rotation[3][3], int fogged, int mirrorPass) {
    (void)x;
    (void)y;
    (void)z;
    (void)rotation;
    (void)fogged;
    (void)mirrorPass;
    g_WorldSubmissions++;
    g_WorldEntity = entity;
    g_WorldModel = model;
}

static int ExpectDraw(const char *label, s32 instance, s32 submissions,
                      s32 model) {
    g_ClassicSubmissions = 0;
    g_WorldSubmissions = 0;
    DrawShuttleScenery(instance);
    if (g_ClassicSubmissions != submissions ||
        g_WorldSubmissions != submissions ||
        (submissions != 0 &&
         (g_ClassicModel != model || g_WorldModel != model ||
          g_WorldEntity != (u32)(0x110 + instance)))) {
        printf("FAIL %s: classic=%d/%d world=%d/%d entity=%u\n",
               label, g_ClassicSubmissions, g_ClassicModel,
               g_WorldSubmissions, g_WorldModel, g_WorldEntity);
        return 0;
    }
    return 1;
}

int main(void) {
    memset(g_ShuttleScenery, 0, sizeof(g_ShuttleScenery));
    g_CourseModelCount = 64;

    g_CourseIndex = 0;
    g_Visible = 0;
    if (!ExpectDraw("culled", 0, 0, 0)) return 1;

    g_Visible = 1;
    if (!ExpectDraw("course zero model", 1, 1, 0x3F)) return 1;

    g_CourseModelCount = 63;
    if (!ExpectDraw("missing model fallback", 0, 1, 1)) return 1;

    g_CourseIndex = 2;
    g_CourseModelCount = 61;
    g_Visible = 0;
    if (!ExpectDraw("course two culling bypass", 1, 1, 0x3C)) return 1;

    g_CourseIndex = 6;
    if (!ExpectDraw("other series still culled", 0, 0, 0)) return 1;

    g_Visible = 1;
    if (!ExpectDraw("negative instance", -1, 0, 0) ||
        !ExpectDraw("past-last instance", SHUTTLE_INSTANCE_COUNT, 0, 0)) {
        return 1;
    }

    puts("shuttle scenery drawing preserved");
    return 0;
}
