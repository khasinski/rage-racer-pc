#include "game/asset.h"
#include "game/car.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/state.h"
#include "rage/render_world_game.h"

#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

GameRenderState g_RenderState;
Matrix g_SceneLightMatrix;
GameCarRuntime g_Cars[RACE_CAR_SLOT_COUNT];
PlayerCarRuntime g_PlayerCar;
CarEntry *g_CarTable;
CarModelAsset *g_CarModelAsset;
TrackRenderTable *g_TrackRenderTable;
s16 g_CarModelBankTable[2][2];
u8 g_CarModelByCourse[1][11];
s32 g_CourseIndex;
s32 g_ModelBankCount;
s32 g_PlayerCarIndex;
s32 g_SceneId;
s32 g_SceneTimer;

static s32 s_viewDepth;
static s32 s_zoneBlend;
static s32 s_modernCarCalls;
static s32 s_modernPlayerCalls;
static RageGameCarRenderDetail s_detail;
static s32 s_submitCount;
static s32 s_submittedBanks[8];
static s32 s_materialModes[8];
static s32 s_restoreCalls;
static s32 s_zoneLightCalls;
static s32 s_lightMatrixCalls;

void BuildRotMatrixX(void *matrix, s32 angle) {
    (void)angle;
    memset(matrix, 0, sizeof(Matrix));
}
void BuildRotMatrixY(void *matrix, s32 angle) {
    (void)angle;
    memset(matrix, 0, sizeof(Matrix));
}
void BuildRotMatrixZ(void *matrix, s32 angle) {
    (void)angle;
    memset(matrix, 0, sizeof(Matrix));
}

#undef MulMatrix
#undef MulMatrix2
#undef ApplyMatrix
MATRIX *MulMatrix(MATRIX *left, MATRIX *right) {
    (void)right;
    return left;
}
MATRIX *MulMatrix2(MATRIX *left, MATRIX *right) {
    (void)left;
    return right;
}
MATRIX *MulMatrix0(MATRIX *left, MATRIX *right, MATRIX *result) {
    (void)left;
    (void)right;
    memset(result, 0, sizeof(*result));
    return result;
}
void ApplyMatrix(MATRIX *matrix, SVECTOR *input, VECTOR *output) {
    (void)matrix;
    output->vx = input->vx;
    output->vy = input->vy;
    output->vz = input->vz;
}
void ApplyMatrixLV(const Matrix *matrix, const s32 *input, s32 *output) {
    (void)matrix;
    output[0] = input[0];
    output[1] = input[1];
    output[2] = s_viewDepth;
}
void SetLightMatrix(MATRIX *matrix) {
    (void)matrix;
    s_lightMatrixCalls++;
}
void SetGteObjectMatrix(LVec *position, Matrix *rotation) {
    (void)position;
    (void)rotation;
}
void FlipMatrixXZColumns(Matrix *destination, const Matrix *source) {
    if (destination != source) *destination = *source;
}

void SubmitModel(void *ctx, s32 bank) {
    GameRenderState *state = ctx;
    if (s_submitCount < 8) {
        s_submittedBanks[s_submitCount] = bank;
        s_materialModes[s_submitCount] = state->envMode4;
    }
    s_submitCount++;
}
void GameRenderWorldSubmitCar(const GameRenderObject *object, int mirror,
                              RageGameCarRenderDetail detail) {
    (void)object;
    (void)mirror;
    s_modernCarCalls++;
    s_detail = detail;
}
void GameRenderWorldSubmitPlayerCar(const GameRenderObject *object,
                                    int mirror) {
    (void)object;
    (void)mirror;
    s_modernPlayerCalls++;
}
s32 GetTrackZoneBlend(s32 position) {
    (void)position;
    return s_zoneBlend;
}
void ApplyZoneLighting(s32 blend, Matrix *lightMatrix) {
    (void)blend;
    (void)lightMatrix;
    s_zoneLightCalls++;
}
void RestoreColorMatrix(void) { s_restoreCalls++; }
s32 GetCarAssetIndex(s32 model, s32 grade) {
    (void)model;
    (void)grade;
    return 0;
}
s32 DiagnosticsEnabled(const char *name) {
    (void)name;
    return 0;
}
s32 DiagnosticsIntValue(const char *name, s32 fallback) {
    (void)name;
    return fallback;
}
void Trace(const char *category, const char *format, ...) {
    (void)category;
    (void)format;
}

static void ResetCounters(void) {
    s_modernCarCalls = 0;
    s_modernPlayerCalls = 0;
    s_submitCount = 0;
    s_restoreCalls = 0;
    s_zoneLightCalls = 0;
    s_lightMatrixCalls = 0;
    memset(s_submittedBanks, 0, sizeof(s_submittedBanks));
    memset(s_materialModes, 0, sizeof(s_materialModes));
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "check failed at line %d: %s\n", __LINE__,       \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

int main(void) {
    struct {
        TrackRenderTable header;
    } track = {0};
    CarModelAsset playerAsset = {0};
    GameRenderObject object = {0};
    const s32 originalY = 200;
    const s32 originalModelY = 220;

    g_TrackRenderTable = &track.header;
    g_CarModelAsset = &playerAsset;
    g_CarModelByCourse[0][0] = 0;
    g_CarModelBankTable[0][0] = 20;
    g_CarModelBankTable[0][1] = -2;
    g_ModelBankCount = 64;
    track.header.models[0].axis0 = 10;
    track.header.models[0].axis1 = 20;
    track.header.models[0].axis2 = 30;
    track.header.models[0].horizon = 40;
    object.y = originalY;
    object.modelY = originalModelY;

    ResetCounters();
    object.x = 0x3000;
    s_viewDepth = 100;
    DrawCar(&object);
    CHECK(s_modernCarCalls == 0 && s_submitCount == 0);
    CHECK(object.y == originalY && object.modelY == originalModelY);

    ResetCounters();
    object.x = 100;
    s_viewDepth = -1;
    DrawCar(&object);
    CHECK(s_modernCarCalls == 0 && s_submitCount == 0);
    CHECK(object.y == originalY && object.modelY == originalModelY);

    ResetCounters();
    object.x = 0x1000;
    s_viewDepth = 100;
    s_zoneBlend = 7;
    DrawCar(&object);
    CHECK(s_modernCarCalls == 1 && s_detail == RAGE_GAME_CAR_RENDER_FAR);
    CHECK(s_submitCount == 1 && s_submittedBanks[0] == 24);
    CHECK(s_materialModes[0] == (s32)0xfffe0000u);
    CHECK(s_zoneLightCalls == 1 && s_restoreCalls == 1);
    CHECK(object.y == originalY && object.modelY == originalModelY);

    ResetCounters();
    object.x = 100;
    s_viewDepth = 100;
    DrawCar(&object);
    CHECK(s_modernCarCalls == 1 && s_detail == RAGE_GAME_CAR_RENDER_CLOSE);
    CHECK(s_submitCount == 6);
    CHECK(s_submittedBanks[0] == 21 && s_submittedBanks[1] == 21);
    CHECK(s_submittedBanks[2] == 20 && s_submittedBanks[3] == 23);
    CHECK(s_submittedBanks[4] == 22 && s_submittedBanks[5] == 22);
    CHECK(object.y == originalY && object.modelY == originalModelY);

    ResetCounters();
    playerAsset.horizon = 30;
    playerAsset.modelOffsetX = 10;
    object.x = INT_MAX;
    object.y = INT_MIN;
    object.z = INT_MAX;
    object.modelY = INT_MIN;
    object.bodyYaw = INT_MIN;
    object.modelYaw = INT_MIN;
    object.bodyRoll = INT_MIN;
    object.bodyRollVelocity = 1;
    object.renderDepth = INT_MAX;
    DrawPlayerCarModel(&object);
    CHECK(s_modernPlayerCalls == 1 && s_submitCount == 6);
    CHECK(object.y == INT_MIN && object.modelY == INT_MIN);

    puts("car model drawing tests passed");
    return 0;
}
