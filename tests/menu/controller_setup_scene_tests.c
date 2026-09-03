#include <stdio.h>
#include <string.h>

#include "game/input_internal.h"
#include "game/render_internal.h"
#include "game/render_state.h"
#include "game/state.h"

#undef MulMatrix2
#undef ScaleMatrix

GameFrameContext *g_DrawBuffer;
GameRenderState g_RenderState;
ObjectMatrixWork g_ObjectMatrixWork;
Matrix g_SceneLightMatrix;
u8 g_PadType;
s32 g_GameMode;
s32 g_AnimTimer;
s32 g_ControllerSceneAngleX;
s32 g_ControllerSceneAngleY;
s32 g_ModelBankCount;
s16 g_NegconSteer;
NegconCalibrationValue g_NegconSteerPlay;
NegconCalibrationValue g_NegconMaxTwist;
s16 g_NegconSteerRange[NEGCON_STEER_RANGE_COUNT];
s32 g_NegconPlayScale[4];

static GameFrameContext s_frame;
static GameOrderingTableEntry s_originalOt[4];
static s32 s_cameraCalls;
static s32 s_pitchAngles[4];
static s32 s_pitchCount;
static s32 s_models[8];
static s32 s_modelCount;
static s32 s_objectMatrixCount;
static s32 s_lightMatrixCount;
static s32 s_failures;

#define CHECK(condition)                                                                  \
    do {                                                                                  \
        if (!(condition)) {                                                               \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #condition); \
            s_failures++;                                                                 \
        }                                                                                 \
    } while (0)

void SetCameraRotMatrix(void) {
    s_cameraCalls++;
}

void BuildRotMatrixX(void *matrix, s32 angle) {
    memset(matrix, 0, sizeof(Matrix));
    s_pitchAngles[s_pitchCount++] = angle;
}

void BuildRotMatrixY(void *matrix, s32 angle) {
    (void)angle;
    memset(matrix, 0, sizeof(Matrix));
}

MATRIX *MulMatrix2(MATRIX *left, MATRIX *right) {
    (void)left;
    return right;
}

MATRIX *ScaleMatrix(MATRIX *matrix, VECTOR *scale) {
    (void)scale;
    return matrix;
}

void SetLightMatrix(MATRIX *matrix) {
    (void)matrix;
    s_lightMatrixCount++;
}

void SetGteObjectMatrix(LVec *position, Matrix *rotation) {
    (void)rotation;
    CHECK(position->x == 0 && position->y == 0 && position->z == 0);
    s_objectMatrixCount++;
}

void SubmitModel(void *context, s32 model) {
    (void)context;
    CHECK(RENDER_OT_BASE == s_frame.layout.orderingTables[1]);
    s_models[s_modelCount++] = model;
}

int rsin(int angle) {
    CHECK(angle == g_AnimTimer * 16);
    return 256;
}

static void Reset(void) {
    memset(&s_frame, 0, sizeof(s_frame));
    memset(&g_RenderState, 0, sizeof(g_RenderState));
    memset(s_pitchAngles, 0, sizeof(s_pitchAngles));
    memset(s_models, 0, sizeof(s_models));
    g_DrawBuffer = &s_frame;
    RENDER_OT_BASE = s_originalOt;
    g_PadType = PAD_TYPE_DIGITAL;
    g_GameMode = OPTION_MODE_CONTROLLER_CONFIG;
    g_AnimTimer = 5;
    g_ControllerSceneAngleX = 100;
    g_ControllerSceneAngleY = 200;
    g_ModelBankCount = 5;
    g_NegconSteer = 4;
    g_NegconSteerPlay = 2;
    g_NegconMaxTwist = 0;
    g_NegconSteerRange[0] = 8;
    g_NegconPlayScale[2] = 3;
    s_cameraCalls = 0;
    s_pitchCount = 0;
    s_modelCount = 0;
    s_objectMatrixCount = 0;
    s_lightMatrixCount = 0;
}

static void TestDigitalPadModels(void) {
    Reset();
    g_ModelBankCount = 0;
    DrawControllerSetupScene(0);
    CHECK(s_cameraCalls == 1);
    CHECK(g_RenderState.viewX == 0 && g_RenderState.viewY == -0x40);
    CHECK(g_RenderState.viewZ == -0x1080);
    CHECK(s_pitchCount == 1 && s_pitchAngles[0] == -0xD0);
    CHECK(s_modelCount == 1 && s_models[0] == 1);
    CHECK(s_objectMatrixCount == 1 && s_lightMatrixCount == 1);
    CHECK(RENDER_OT_BASE == s_originalOt);

    Reset();
    g_ModelBankCount = 1;
    DrawControllerSetupScene(0);
    CHECK(s_modelCount == 1 && s_models[0] == 0);
}

static void TestNegconPartsAndOverlays(void) {
    Reset();
    g_PadType = PAD_TYPE_NEGCON;
    g_GameMode = OPTION_MODE_NEGCON_STEER_PLAY;
    DrawControllerSetupScene(1);
    CHECK(g_RenderState.viewY == 0 && g_RenderState.viewZ == -0xC80);
    CHECK(s_pitchCount == 2);
    CHECK(s_pitchAngles[0] == 39 && s_pitchAngles[1] == 33);
    CHECK(s_modelCount == 4);
    CHECK(s_models[0] == 1 && s_models[1] == 3);
    CHECK(s_models[2] == 2 && s_models[3] == 4);
    CHECK(s_objectMatrixCount == 4 && s_lightMatrixCount == 2);

    Reset();
    g_PadType = PAD_TYPE_NEGCON;
    g_GameMode = OPTION_MODE_NEGCON_MAX_TWIST;
    DrawControllerSetupScene(0);
    CHECK(s_pitchAngles[0] == 40 && s_pitchAngles[1] == 32);
    CHECK(s_modelCount == 2 && s_models[0] == 1 && s_models[1] == 2);
}

static void TestUnknownPadDrawsNoModel(void) {
    Reset();
    g_PadType = 0;
    DrawControllerSetupScene(1);
    CHECK(s_cameraCalls == 1);
    CHECK(s_modelCount == 0 && s_objectMatrixCount == 0);
    CHECK(RENDER_OT_BASE == s_originalOt);
}

int main(void) {
    TestDigitalPadModels();
    TestNegconPartsAndOverlays();
    TestUnknownPadDrawsNoModel();
    return s_failures != 0;
}
