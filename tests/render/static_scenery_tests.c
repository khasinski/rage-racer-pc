#include "common.h"
#include "game/render.h"
#include "game/track.h"

#include <stdio.h>
#include <string.h>

GameRenderState g_RenderState;
ObjectMatrixWork g_ObjectMatrixWork;
StaticSceneryState g_StaticSceneryState;
s32 g_CourseModelCount;
s32 g_IsEnvironmentMode4;

typedef struct Submission {
    u32 entity;
    s32 model;
    s32 x;
    s32 y;
    s32 z;
    s32 fogged;
    s32 environmentMode;
    s32 legacyPath;
} Submission;

static Submission s_submission;
static s32 s_submissionCount;
static s32 s_visible = 1;

int TrackCellVisible(s32 x, s32 z) {
    (void)x;
    (void)z;
    return s_visible;
}

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

void GameRenderWorldSubmitDynamicCourseObject(
    u32 entity, s32 model, s32 x, s32 y, s32 z,
    const s16 rotation[3][3], int fogged, int mirrorPass) {
    (void)rotation;
    (void)mirrorPass;
    s_submission = (Submission){
        entity, model, x, y, z, fogged, g_RenderState.envMode4, 0
    };
    s_submissionCount++;
}

void SubmitCourseModel(void *renderState, s32 model) {
    (void)renderState;
    (void)model;
    s_submission.legacyPath = 1;
}

void SubmitCourseModel2(void *renderState, s32 model) {
    (void)renderState;
    (void)model;
    s_submission.legacyPath = 2;
}

static void Reset(void) {
    memset(&s_submission, 0, sizeof(s_submission));
    s_submissionCount = 0;
}

static int Expect(const char *label, Submission expected) {
    if (s_submissionCount != 1 ||
        memcmp(&s_submission, &expected, sizeof(expected)) != 0) {
        printf("FAIL %s: count=%d entity=%u model=%d pos=(%d,%d,%d) "
               "fogged=%d env=%d path=%d\n",
               label, s_submissionCount, s_submission.entity,
               s_submission.model, s_submission.x, s_submission.y,
               s_submission.z, s_submission.fogged,
               s_submission.environmentMode, s_submission.legacyPath);
        return 0;
    }
    return 1;
}

int main(void) {
    g_StaticSceneryState.standard.position = (LVec){100, 200, 300};
    g_StaticSceneryState.standard.yaw = 0x123;
    g_StaticSceneryState.highClass.position = (LVec){400, 500, 600};
    g_StaticSceneryState.highClass.yaw = 0x456;

    g_IsEnvironmentMode4 = 0;
    g_CourseModelCount = 0x3A;
    Reset();
    DrawStaticScenery(1);
    if (!Expect("shifted standard landmark",
                (Submission){0, 0x39, 100, 200, 300 + 0x5000,
                             1, 0, 2})) {
        return 1;
    }

    s_visible = 0;
    Reset();
    DrawStaticScenery(0);
    if (s_submissionCount != 0) {
        puts("FAIL invisible standard landmark was submitted");
        return 1;
    }
    s_visible = 1;

    g_IsEnvironmentMode4 = 1;
    g_CourseModelCount = 2;
    Reset();
    DrawStaticScenery(0);
    if (!Expect("environment fallback landmark",
                (Submission){0, 1, 100, 200, 300, 0, 0, 1})) {
        return 1;
    }

    g_CourseModelCount = 0x40;
    Reset();
    DrawHighClassScenery();
    if (!Expect("high-class landmark",
                (Submission){1, 0x3F, 400, 500, 600,
                             0, 0x10000, 1})) {
        return 1;
    }

    puts("static scenery behavior preserved");
    return 0;
}
