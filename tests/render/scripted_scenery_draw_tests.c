#include "game/race.h"
#include "game/render.h"
#include "game/track_internal.h"

#include <stdio.h>
#include <string.h>

GameRenderState g_RenderState;
FlybySceneryState g_FlybyScenery;
PathSceneryTransform g_PathSceneryTransform;
Vec4 g_RouteSceneryPosition;
s32 g_RouteSceneryRotX;
s32 g_RouteSceneryRotY;
s32 g_RouteSceneryRotZ;
s32 g_ModelBankCount;
s32 g_SceneTimer;

typedef struct Submission {
    s32 bank;
    s32 model;
    LVec position;
} Submission;

static Submission s_submissions[2];
static s32 s_submissionCount;
static s32 s_selectedBank;
static LVec s_position;
static s32 s_matrixAngles[3];
static s32 s_spinAngle;

void BuildSceneryObjectMatrix(Matrix *matrix, s32 rotationX, s32 rotationY,
                              s32 rotationZ) {
    memset(matrix, 0, sizeof(*matrix));
    s_matrixAngles[0] = rotationX;
    s_matrixAngles[1] = rotationY;
    s_matrixAngles[2] = rotationZ;
}

void BuildRotMatrixY(void *matrix, s32 angle) {
    memset(matrix, 0, sizeof(Matrix));
    s_spinAngle = angle;
}

#undef MulMatrix2
MATRIX *MulMatrix2(MATRIX *left, MATRIX *right) {
    (void)left;
    return right;
}

void SelectModelBank(s32 bank) { s_selectedBank = bank; }

void SetGteObjectMatrix(LVec *position, Matrix *rotation) {
    (void)rotation;
    s_position = *position;
}

void SubmitModel(void *renderState, s32 model) {
    (void)renderState;
    s_submissions[s_submissionCount++] =
        (Submission){s_selectedBank, model, s_position};
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static void ResetSubmissions(void) {
    memset(s_submissions, 0, sizeof(s_submissions));
    s_submissionCount = 0;
    g_RenderState.envMode4 = 99;
}

int main(void) {
    g_ModelBankCount = 64;

    g_FlybyScenery.position = (Vec4){10, 20, 30, 40};
    g_FlybyScenery.rotationX = 1;
    g_FlybyScenery.rotationY = 2;
    g_FlybyScenery.rotationZ = 3;
    g_FlybyScenery.timer = 0;
    ResetSubmissions();
    DrawFlybyScenery();
    CHECK(s_submissionCount == 0);

    g_FlybyScenery.timer = 1;
    DrawFlybyScenery();
    CHECK(s_submissionCount == 1 && s_submissions[0].bank == 2 &&
          s_submissions[0].model == 0 && s_submissions[0].position.x == 10 &&
          s_matrixAngles[0] == 1 && s_matrixAngles[1] == 2 &&
          s_matrixAngles[2] == 3 && g_RenderState.envMode4 == 0);

    g_RouteSceneryPosition = (Vec4){100, 200, 300, 400};
    g_RouteSceneryRotX = 4;
    g_RouteSceneryRotY = 5;
    g_RouteSceneryRotZ = 6;
    ResetSubmissions();
    DrawRouteScenery();
    CHECK(s_submissionCount == 1 && s_submissions[0].bank == 1 &&
          s_submissions[0].model == 0x25 &&
          s_submissions[0].position.z == 300 && s_matrixAngles[0] == 4 &&
          s_matrixAngles[1] == 5 && s_matrixAngles[2] == 6 &&
          g_RenderState.envMode4 == 0);

    g_PathSceneryTransform.position = (Block16){{1000, 2000, 3000, 4000}};
    g_PathSceneryTransform.rotation = (SVec){7, 8, 9, 10};
    g_SceneTimer = 13;
    ResetSubmissions();
    DrawPathScenery();
    CHECK(s_submissionCount == 2);
    CHECK(s_submissions[0].bank == 1 && s_submissions[0].model == 0x23);
    CHECK(s_submissions[1].bank == 1 && s_submissions[1].model == 0x24);
    CHECK(s_submissions[0].position.x == 1000 &&
          s_submissions[1].position.z == 3000);
    CHECK(s_matrixAngles[0] == 7 && s_matrixAngles[1] == 8 &&
          s_matrixAngles[2] == 9);
    CHECK(s_spinAngle == ((13 * 331) & 0xFFF));
    CHECK(g_RenderState.envMode4 == 0);

    g_ModelBankCount = 1;
    ResetSubmissions();
    DrawRouteScenery();
    CHECK(s_submissionCount == 1 && s_submissions[0].model == 1);

    puts("scripted scenery draw submissions preserved");
    return 0;
}
