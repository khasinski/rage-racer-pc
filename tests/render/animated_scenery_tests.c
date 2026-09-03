#include "common.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/render.h"
#include "game/track_internal.h"

#include <stdio.h>
#include <string.h>

PlayerCarRuntime g_PlayerCar;
GameRenderState g_RenderState;
ObjectMatrixWork g_ObjectMatrixWork;
s32 g_CourseIndex;
s32 g_CourseModelCount;
s32 g_GrandPrixClass;
s16 g_GrandPrixMode;
s32 g_RacePaused;
Vec4 g_AnimSceneryPos[2];
s16 g_AnimSceneryPitch[2];
s16 g_AnimSceneryFrame;
s32 g_AnimSceneryTint;
s16 g_AnimSceneryRacePosition;
s16 g_AnimSceneryVariant;
s16 g_PresentationSceneryFrame;
s32 g_PresentationSceneryTint;
s16 g_PresentationSceneryVariant;

typedef struct Submission {
    u32 entity;
    s32 model;
    s32 x;
    s32 z;
    s32 tint;
} Submission;

static Submission g_Submissions[4];
static s32 g_SubmissionCount;
static s32 g_Visible = 1;
static s32 g_RandomValue;
static s32 g_RandomCalls;

s32 Random15(void) {
    g_RandomCalls++;
    return g_RandomValue;
}
int TrackCellVisible(s32 x, s32 z) {
    (void)x;
    (void)z;
    return g_Visible;
}
void BuildRotMatrixY(void *matrix, s32 angle) {
    (void)angle;
    memset(matrix, 0, sizeof(Matrix));
}
void BuildRotMatrixX(void *matrix, s32 angle) {
    (void)angle;
    memset(matrix, 0, sizeof(Matrix));
}

#undef MulMatrix
#undef MulMatrix2
MATRIX *MulMatrix(MATRIX *left, MATRIX *right) {
    (void)right;
    return left;
}
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
void GameRenderWorldSubmitDynamicCourseOverlay(
    u32 entity, s32 model, s32 x, s32 y, s32 z,
    const s16 rotation[3][3], int fogged, int mirrorPass) {
    Submission *submission = &g_Submissions[g_SubmissionCount++];

    (void)y;
    (void)rotation;
    (void)fogged;
    (void)mirrorPass;
    *submission = (Submission){entity, model, x, z, g_RenderState.envMode4};
}

static void Reset(void) {
    g_SubmissionCount = 0;
    g_RandomCalls = 0;
    memset(g_Submissions, 0, sizeof(g_Submissions));
}

static int ExpectPair(const char *label, u32 entity, s32 primary,
                      s32 secondary, s32 x, s32 z, s32 tint) {
    if (g_SubmissionCount != 2 ||
        g_Submissions[0].entity != entity ||
        g_Submissions[1].entity != entity + 1 ||
        g_Submissions[0].model != primary ||
        g_Submissions[1].model != secondary ||
        g_Submissions[0].x != x || g_Submissions[0].z != z ||
        g_Submissions[1].tint != tint) {
        printf("FAIL %s: count=%d primary=(%u,%d,%d,%d) secondary=(%u,%d,%d)\n",
               label, g_SubmissionCount,
               g_Submissions[0].entity, g_Submissions[0].model,
               g_Submissions[0].x, g_Submissions[0].z,
               g_Submissions[1].entity, g_Submissions[1].model,
               g_Submissions[1].tint);
        return 0;
    }
    return 1;
}

int main(void) {
    memset(&g_PlayerCar, 0, sizeof(g_PlayerCar));
    g_AnimSceneryPos[0] = (Vec4){100, 200, 300, 400};
    g_AnimSceneryPos[1] = (Vec4){110, 210, 310, 410};
    g_CourseModelCount = 64;
    g_GrandPrixClass = 0;
    g_RandomValue = 5;

    /* The race variant updates its state before the GP-mode draw guard. */
    g_GrandPrixMode = 0;
    g_PlayerCar.drive.racePosition = 2;
    Reset();
    DrawAnimatedScenery(0, 0);
    if (g_SubmissionCount != 0 || g_RandomCalls != 1 ||
        g_AnimSceneryFrame != 0 || g_AnimSceneryRacePosition != 2 ||
        g_AnimSceneryVariant != 1) {
        puts("FAIL: race animation state-only update");
        return 1;
    }

    g_GrandPrixMode = 1;
    g_CourseIndex = 3;
    g_AnimSceneryRacePosition = 2;
    g_AnimSceneryVariant = 1;
    Reset();
    DrawAnimatedScenery(52, 1);
    if (!ExpectPair("race position layers", 0x22, 2, 5,
                    110, 310 + 0x5000, 2 << 16)) {
        return 1;
    }

    g_CourseIndex = 0;
    g_AnimSceneryRacePosition = 0;
    Reset();
    DrawAnimatedScenery(4, 0);
    if (!ExpectPair("generic race layers", 0x20, 25, 8,
                    100, 300, 0)) {
        return 1;
    }

    /* Partially imported courses may not contain the animated overlays. Both
     * layers then use the ordinary course fallback model. */
    g_CourseModelCount = 5;
    Reset();
    DrawAnimatedScenery(4, 0);
    if (!ExpectPair("missing overlay models", 0x20, 1, 1,
                    100, 300, 0)) {
        return 1;
    }
    g_CourseModelCount = 64;

    /* The replay variant returns before changing state when GP mode is off. */
    g_GrandPrixMode = 0;
    g_PresentationSceneryFrame = 9;
    Reset();
    DrawPresentationAnimatedScenery(0, 0, 1, 1);
    if (g_PresentationSceneryFrame != 9 || g_RandomCalls != 0 ||
        g_SubmissionCount != 0) {
        puts("FAIL: replay early GP-mode guard");
        return 1;
    }

    g_GrandPrixMode = 1;
    Reset();
    DrawAnimatedScenery(0, -1);
    DrawPresentationAnimatedScenery(0, 2, 0, 1);
    if (g_SubmissionCount != 0 || g_RandomCalls != 0) {
        puts("FAIL: animated scenery instance bounds");
        return 1;
    }

    g_PresentationSceneryVariant = 1;
    Reset();
    DrawPresentationAnimatedScenery(4, 1, 1, 0);
    if (!ExpectPair("replay layers", 0x32, 11, 5, 110, 310, 0)) {
        return 1;
    }

    g_Visible = 0;
    Reset();
    DrawPresentationAnimatedScenery(8, 0, 0, 1);
    if (g_SubmissionCount != 0 || g_RandomCalls != 0) {
        puts("FAIL: replay visibility culling");
        return 1;
    }

    puts("animated scenery behavior preserved");
    return 0;
}
