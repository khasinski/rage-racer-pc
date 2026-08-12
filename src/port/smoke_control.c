#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "game/player_car_internal.h"
#include "game/menu.h"
#include "game/race_internal.h"
#include "game/render_internal.h"
#include "game/render.h"
#include "game/scratchpad.h"
#include "game/state.h"

typedef struct RageSmokeInput {
    long firstFrame;
    long lastFrame;
    unsigned short buttons;
    int held;
} RageSmokeInput;

static uint32_t RageSmokeHashWords(const u32 *words, size_t count) {
    uint32_t hash = 2166136261u;
    size_t index;
    for (index = 0; index < count; index++) {
        u32 value = words[index];
        int byteIndex;
        for (byteIndex = 0; byteIndex < 4; byteIndex++) {
            hash ^= (value >> (byteIndex * 8)) & 0xff;
            hash *= 16777619u;
        }
    }
    return hash;
}

static uint32_t RageSmokeHashVisibleList(const Vec4 *entries) {
    uint32_t hash = 2166136261u;
    size_t entryIndex;
    for (entryIndex = 0; entryIndex < 64; entryIndex++) {
        u32 words[4];
        int wordIndex;
        words[0] = entries[entryIndex].w == -1 ? 0 : (u32)entries[entryIndex].x;
        words[1] = entries[entryIndex].w == -1 ? 0 : (u32)entries[entryIndex].y;
        words[2] = entries[entryIndex].w == -1 ? 0 : (u32)entries[entryIndex].z;
        words[3] = (u32)entries[entryIndex].w;
        for (wordIndex = 0; wordIndex < 4; wordIndex++) {
            int byteIndex;
            for (byteIndex = 0; byteIndex < 4; byteIndex++) {
                hash ^= (words[wordIndex] >> (byteIndex * 8)) & 0xff;
                hash *= 16777619u;
            }
        }
    }
    return hash;
}

typedef struct RageSmokeStateInput {
    long scene;
    long timer;
    long phase;
    int hasPhase;
    unsigned short buttons;
    int armed;
    int fired;
} RageSmokeStateInput;

static RageSmokeInput g_SmokeInputs[64];
static int g_SmokeInputCount;
static long g_SmokeFrameLimit;
static long g_SmokeFinishFrame;
static long g_SmokeAutoConfirmFrame;
static long g_SmokeStopScene;
static long g_SmokeStopSceneTimer;
static long g_SmokeCaptureTimerStride;
static long g_SmokeCaptureTimerMin;
static long g_SmokeCaptureTimerMax;
static const char *g_SmokeCaptureDirectory;
static FILE *g_SmokeCaptureManifest;
static int g_SmokeHasStopScene;
static int g_SmokeHasStopSceneTimer;
static int g_SmokeHasCaptureTimerMin;
static int g_SmokeHasCaptureTimerMax;
static int g_SmokeInitialized;
static int g_SmokeRawPadPath;
static int g_SmokeCaptureAllPhases;
static RageSmokeStateInput g_SmokeStateInputs[32];
static int g_SmokeStateInputCount;

extern int g_SceneId;
extern int g_FrontendState;
extern int g_TrackLength;
extern int g_SkyRowBase;
extern int g_IsEnvironmentMode4;
extern unsigned int g_RandomSeed;
extern int g_AnimTimer;
extern s32 g_MirrorPanelY;
int RageWriteCapturedFrame(const char *path);
void UpdatePadState(void);

static unsigned short RageSmokeButton(const char *name) {
    if (strcmp(name, "START") == 0) return 0x800;
    if (strcmp(name, "CROSS") == 0 || strcmp(name, "CONFIRM") == 0) return 0x40;
    if (strcmp(name, "CIRCLE") == 0 || strcmp(name, "CANCEL") == 0) return 0x20;
    if (strcmp(name, "UP") == 0) return 0x1000;
    if (strcmp(name, "RIGHT") == 0) return 0x2000;
    if (strcmp(name, "DOWN") == 0) return 0x4000;
    if (strcmp(name, "LEFT") == 0) return 0x8000;
    return 0;
}

static void RageSmokeInitialize(void) {
    const char *limit = getenv("RAGE_PORT_SMOKE_FRAMES");
    const char *script = getenv("RAGE_PORT_RAW_INPUT_SCRIPT");
    const char *finish = getenv("RAGE_PORT_SMOKE_FINISH_FRAME");
    const char *autoConfirm = getenv("RAGE_PORT_SMOKE_AUTO_CONFIRM_FRAME");
    const char *stopScene = getenv("RAGE_PORT_SMOKE_STOP_SCENE");
    const char *stopTimer = getenv("RAGE_PORT_SMOKE_STOP_SCENE_TIMER");
    const char *captureDirectory = getenv("RAGE_PORT_SMOKE_CAPTURE_DIR");
    const char *captureStride =
        getenv("RAGE_PORT_SMOKE_CAPTURE_TIMER_STRIDE");
    const char *captureMin = getenv("RAGE_PORT_SMOKE_CAPTURE_TIMER_MIN");
    const char *captureMax = getenv("RAGE_PORT_SMOKE_CAPTURE_TIMER_MAX");
    const char *captureAllPhases =
        getenv("RAGE_PORT_SMOKE_CAPTURE_ALL_PHASES");
    const char *stateScript = getenv("RAGE_PORT_STATE_INPUT_SCRIPT");
    char *copy;
    char *token;

    g_SmokeFrameLimit = limit ? strtol(limit, NULL, 10) : 0;
    g_SmokeFinishFrame = finish ? strtol(finish, NULL, 10) : 0;
    g_SmokeAutoConfirmFrame = autoConfirm ? strtol(autoConfirm, NULL, 10) : 0;
    g_SmokeHasStopScene = stopScene != NULL;
    g_SmokeHasStopSceneTimer = stopTimer != NULL;
    g_SmokeStopScene = stopScene ? strtol(stopScene, NULL, 10) : 0;
    g_SmokeStopSceneTimer = stopTimer ? strtol(stopTimer, NULL, 10) : 0;
    g_SmokeCaptureDirectory = captureDirectory;
    g_SmokeCaptureTimerStride =
        captureStride ? strtol(captureStride, NULL, 10) : 0;
    g_SmokeHasCaptureTimerMin = captureMin != NULL;
    g_SmokeHasCaptureTimerMax = captureMax != NULL;
    g_SmokeCaptureTimerMin = captureMin ? strtol(captureMin, NULL, 10) : 0;
    g_SmokeCaptureTimerMax = captureMax ? strtol(captureMax, NULL, 10) : 0;
    g_SmokeCaptureAllPhases = captureAllPhases != NULL;
    if (g_SmokeCaptureDirectory != NULL &&
        g_SmokeCaptureDirectory[0] != '\0' &&
        g_SmokeCaptureTimerStride > 0) {
        char manifestPath[1024];
        int length = snprintf(manifestPath, sizeof(manifestPath),
                              "%s/capture-manifest.csv",
                              g_SmokeCaptureDirectory);
        if (length > 0 && (size_t)length < sizeof(manifestPath)) {
            g_SmokeCaptureManifest = fopen(manifestPath, "w");
            if (g_SmokeCaptureManifest != NULL) {
                fputs("filename,capture_surface,frame,scene,timer,x,z,speed,progress,lap," \
                      "body_yaw,body_pitch,body_roll,track_lateral," \
                      "model_yaw,mirror_y,view_x,view_y,view_z," \
                      "view_angle_x,view_angle_y,view_angle_z," \
                      "proj_m00,proj_m01,proj_m02,proj_m10,proj_m11," \
                      "proj_m12,proj_m20,proj_m21,proj_m22," \
                      "proj_x0,proj_y0,proj_x1,proj_y1,proj_order," \
                      "mirror_m00,mirror_m01,mirror_m02,mirror_m10,mirror_m11," \
                      "mirror_m12,mirror_m20,mirror_m21,mirror_m22," \
                      "main_visible_hash,mirror_visible_hash," \
                      "main_visible_list_hash,mirror_visible_list_hash," \
                      "environment_mode4,scratch_env_mode4,random_seed," \
                      "anim_timer,tacho_rpm,rival0_x,rival0_z,rival1_x,rival1_z," \
                      "rival2_x,rival2_z,rival3_x,rival3_z," \
                      "rival0_speed,rival0_progress,rival0_yaw," \
                      "rival0_lateral,rival0_collision,rival0_active," \
                      "rival1_speed,rival1_progress,rival1_yaw," \
                      "rival1_lateral,rival1_collision,rival1_active," \
                      "rival2_speed,rival2_progress,rival2_yaw," \
                      "rival2_lateral,rival2_collision,rival2_active," \
                      "rival3_speed,rival3_progress,rival3_yaw," \
                      "rival3_lateral,rival3_collision,rival3_active," \
                      "rival0_raw,rival1_raw,rival2_raw,rival3_raw\n",
                      g_SmokeCaptureManifest);
                fflush(g_SmokeCaptureManifest);
            }
        }
    }
    if (script != NULL && script[0] != '\0') {
        g_SmokeRawPadPath = 1;
    } else {
        script = getenv("RAGE_PORT_INPUT_SCRIPT");
    }
    copy = script != NULL && script[0] != '\0' ? strdup(script) : NULL;
    for (token = copy != NULL ? strtok(copy, ",") : NULL;
         token != NULL && g_SmokeInputCount < 64; token = strtok(NULL, ",")) {
        char *separator = strchr(token, ':');
        char *rangeSeparator;
        unsigned short buttons;
        char *name;
        if (separator == NULL) continue;
        *separator = '\0';
        name = separator + 1;
        while (isspace((unsigned char)*name)) name++;
        buttons = 0;
        while (*name != '\0') {
            char *next = strpbrk(name, "+|");
            char *end;
            if (next != NULL) *next = '\0';
            end = name + strlen(name);
            while (end > name && isspace((unsigned char)end[-1])) *--end = '\0';
            buttons |= RageSmokeButton(name);
            if (next == NULL) break;
            name = next + 1;
            while (isspace((unsigned char)*name)) name++;
        }
        if (buttons == 0) continue;
        rangeSeparator = strchr(token, '-');
        if (rangeSeparator != NULL) {
            *rangeSeparator = '\0';
            g_SmokeInputs[g_SmokeInputCount].lastFrame =
                strtol(rangeSeparator + 1, NULL, 10);
            g_SmokeInputs[g_SmokeInputCount].held = 1;
        }
        g_SmokeInputs[g_SmokeInputCount].firstFrame = strtol(token, NULL, 10);
        if (!g_SmokeInputs[g_SmokeInputCount].held) {
            g_SmokeInputs[g_SmokeInputCount].lastFrame =
                g_SmokeInputs[g_SmokeInputCount].firstFrame;
        }
        g_SmokeInputs[g_SmokeInputCount].buttons = buttons;
        g_SmokeInputCount++;
    }
    free(copy);

    if (stateScript == NULL || stateScript[0] == '\0') return;
    copy = strdup(stateScript);
    if (copy == NULL) return;
    for (token = strtok(copy, ","); token != NULL &&
         g_SmokeStateInputCount < 32; token = strtok(NULL, ",")) {
        char *at = strchr(token, '@');
        char *phaseAt;
        char *colon = strchr(token, ':');
        unsigned short buttons;
        if (at == NULL || colon == NULL || at > colon) continue;
        *at = '\0';
        *colon = '\0';
        phaseAt = strchr(at + 1, '@');
        if (phaseAt != NULL) *phaseAt = '\0';
        buttons = RageSmokeButton(colon + 1);
        if (buttons == 0) continue;
        g_SmokeStateInputs[g_SmokeStateInputCount].scene =
            strtol(token, NULL, 0);
        g_SmokeStateInputs[g_SmokeStateInputCount].timer =
            strtol(at + 1, NULL, 0);
        if (phaseAt != NULL) {
            g_SmokeStateInputs[g_SmokeStateInputCount].phase =
                strtol(phaseAt + 1, NULL, 0);
            g_SmokeStateInputs[g_SmokeStateInputCount].hasPhase = 1;
        }
        g_SmokeStateInputs[g_SmokeStateInputCount].buttons = buttons;
        g_SmokeStateInputCount++;
    }
    free(copy);
}

int RagePortShouldExit(int frame_number) {
    static int lastScene = -1;
    static int lastFrontend = -1;
    static int lastCapturedScene = -1;
    static int lastCapturedTimer = -1;
    int index;
    if (!g_SmokeInitialized) {
        g_SmokeInitialized = 1;
        RageSmokeInitialize();
    }
    if (g_SmokeRawPadPath) {
        u16 buttons = 0;
        for (index = 0; index < g_SmokeInputCount; index++) {
            if ((g_SmokeInputs[index].held &&
                 frame_number >= g_SmokeInputs[index].firstFrame &&
                 frame_number <= g_SmokeInputs[index].lastFrame) ||
                (!g_SmokeInputs[index].held &&
                 frame_number == g_SmokeInputs[index].firstFrame)) {
                buttons |= g_SmokeInputs[index].buttons;
            }
        }
        g_PadBuffers[0] = 0;
        g_PadBuffers[1] = 0x41;
        g_PadBuffers[2] = (u8)~(buttons >> 8);
        g_PadBuffers[3] = (u8)~buttons;
        UpdatePadState();
    }
    for (index = 0; index < g_SmokeInputCount; index++) {
        if (!g_SmokeRawPadPath &&
            g_SmokeInputs[index].firstFrame == frame_number) {
            g_PadPressed |= g_SmokeInputs[index].buttons;
            fprintf(stderr, "smoke input frame=%d buttons=%04x\n",
                    frame_number, g_SmokeInputs[index].buttons);
        }
        if (!g_SmokeRawPadPath && g_SmokeInputs[index].held &&
            frame_number >= g_SmokeInputs[index].firstFrame &&
            frame_number <= g_SmokeInputs[index].lastFrame) {
            /* UpdatePlayerCar selects digital/analog input by controller type. */
            g_PadType = 0x41;
            g_PadHeld |= g_SmokeInputs[index].buttons;
        }
    }
    for (index = 0; index < g_SmokeStateInputCount; index++) {
        RageSmokeStateInput *input = &g_SmokeStateInputs[index];
        int phaseMatches = !input->hasPhase ||
            (input->scene == 8 && g_MenuScreen == input->phase) ||
            (input->scene == 32 && g_PrologueStep == input->phase);
        if (!input->fired && g_SceneId == input->scene && phaseMatches &&
            g_SceneTimer <= input->timer) {
            input->armed = 1;
        }
        if (input->armed && !input->fired && g_SceneId == input->scene &&
            g_SceneTimer >= input->timer && phaseMatches) {
            input->fired = 1;
            g_PadPressed |= input->buttons;
            fprintf(stderr,
                    "smoke state input frame=%d scene=%d timer=%d buttons=%04x\n",
                    frame_number, g_SceneId, g_SceneTimer, input->buttons);
        }
    }
    if (g_SmokeFinishFrame > 0 && frame_number >= g_SmokeFinishFrame &&
        g_PlayerCar.lap > 0 && g_PlayerCar.lap < 257) {
        /* Cross each remaining finish line through the normal race logic. */
        g_PlayerCar.progressA = g_PlayerCar.lap * g_TrackLength;
        g_PlayerCar.progressB = 0;
    }
    if (g_SmokeAutoConfirmFrame > 0 &&
        frame_number >= g_SmokeAutoConfirmFrame) {
        g_PadType = 0x41;
        g_PadHeld |= 0x40;
        if ((frame_number - g_SmokeAutoConfirmFrame) % 60 == 0)
            g_PadPressed |= 0x40;
    }
    if (g_SceneId != lastScene || g_FrontendState != lastFrontend) {
        fprintf(stderr,
                "smoke state frame=%d scene=%d frontend=%d sky_row=%d\n",
                frame_number, g_SceneId, g_FrontendState, g_SkyRowBase);
        lastScene = g_SceneId;
        lastFrontend = g_FrontendState;
    }
    if (g_SmokeCaptureDirectory != NULL &&
        g_SmokeCaptureDirectory[0] != '\0' &&
        g_SmokeCaptureTimerStride > 0 &&
        (!g_SmokeHasStopScene || g_SceneId == g_SmokeStopScene) &&
        g_SceneTimer >= 0 &&
        (!g_SmokeHasCaptureTimerMin ||
         g_SceneTimer >= g_SmokeCaptureTimerMin) &&
        (!g_SmokeHasCaptureTimerMax ||
         g_SceneTimer <= g_SmokeCaptureTimerMax) &&
        g_SceneTimer % g_SmokeCaptureTimerStride == 0 &&
        (g_SmokeCaptureAllPhases || g_SceneId != lastCapturedScene ||
         g_SceneTimer != lastCapturedTimer)) {
        char path[1024];
        int length;
        if (g_SmokeCaptureAllPhases) {
            length = snprintf(path, sizeof(path),
                              "%s/timer-%05d-f%05d-s%02d.ppm",
                              g_SmokeCaptureDirectory, g_SceneTimer,
                              frame_number, g_SceneId);
        } else {
            length = snprintf(path, sizeof(path), "%s/timer-%05d-s%02d.ppm",
                              g_SmokeCaptureDirectory, g_SceneTimer,
                              g_SceneId);
        }
        if (length <= 0 || (size_t)length >= sizeof(path) ||
            !RageWriteCapturedFrame(path)) {
            fprintf(stderr, "failed synchronized series capture: %s\n",
                    length > 0 && (size_t)length < sizeof(path) ? path :
                    "path too long");
        } else {
            fprintf(stderr,
                    "smoke capture=%s frame=%d scene=%d timer=%d\n",
                    path, frame_number, g_SceneId, g_SceneTimer);
            if (g_SmokeCaptureManifest != NULL) {
                const char *filename = strrchr(path, '/');
                filename = filename != NULL ? filename + 1 : path;
                fprintf(g_SmokeCaptureManifest,
                        "%s,%s,%d,%d,%d,%d,%d,%d,%d,%d," \
                        "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d," \
                        "%d,%d,%d,%d,%d,%d,%d,%d,%d," \
                        "%u,%u,%u,%u," \
                        "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d," \
                        "%u,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d," \
                        "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d," \
                        "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
                        filename,
                        getenv("RAGE_PORT_CAPTURE_DRAW_PAGE") != NULL ?
                            "draw" : "display",
                        frame_number, g_SceneId, g_SceneTimer,
                        g_PlayerCar.x, g_PlayerCar.z,
                        g_PlayerCar.speed, g_PlayerCar.trackProgress,
                        g_PlayerCar.lap, g_PlayerCar.bodyYaw,
                        g_PlayerCar.bodyPitch, g_PlayerCar.bodyRoll,
                        g_PlayerCar.trackLateralOffset,
                        g_PlayerCar.modelYaw, g_MirrorPanelY,
                        SCRATCH_VIEW_X, SCRATCH_VIEW_Y, SCRATCH_VIEW_Z,
                        SCRATCH_VIEW_ANGLE_X, SCRATCH_VIEW_ANGLE_Y,
                        SCRATCH_VIEW_ANGLE_Z,
                        SCRATCHPAD->matrix.m[0][0], SCRATCHPAD->matrix.m[0][1],
                        SCRATCHPAD->matrix.m[0][2], SCRATCHPAD->matrix.m[1][0],
                        SCRATCHPAD->matrix.m[1][1], SCRATCHPAD->matrix.m[1][2],
                        SCRATCHPAD->matrix.m[2][0], SCRATCHPAD->matrix.m[2][1],
                        SCRATCHPAD->matrix.m[2][2],
                        SCRATCHPAD->x0, SCRATCHPAD->y0,
                        SCRATCHPAD->x1, SCRATCHPAD->y1,
                        SCRATCHPAD->orderingFlag & 1,
                        g_MirrorViewMatrix.m[0][0], g_MirrorViewMatrix.m[0][1],
                        g_MirrorViewMatrix.m[0][2], g_MirrorViewMatrix.m[1][0],
                        g_MirrorViewMatrix.m[1][1], g_MirrorViewMatrix.m[1][2],
                        g_MirrorViewMatrix.m[2][0], g_MirrorViewMatrix.m[2][1],
                        g_MirrorViewMatrix.m[2][2],
                        RageSmokeHashWords(g_MainVisibleCellMask, 32),
                        RageSmokeHashWords(g_MirrorVisibleCellMask, 32),
                        RageSmokeHashVisibleList(g_MainVisibleCellList),
                        RageSmokeHashVisibleList(g_MirrorVisibleCellList),
                        g_IsEnvironmentMode4,
                        SCRATCH_ENV_MODE4, g_RandomSeed, g_AnimTimer,
                        g_EngineRpm + g_EngineRpmJitter,
                        g_Cars[0].x, g_Cars[0].z,
                        g_Cars[1].x, g_Cars[1].z,
                        g_Cars[2].x, g_Cars[2].z,
                        g_Cars[3].x, g_Cars[3].z,
                        g_Cars[0].speed, g_Cars[0].trackProgress,
                        g_Cars[0].bodyYaw, g_Cars[0].trackLateralOffset,
                        g_Cars[0].collisionFlag, g_Cars[0].activeFlag,
                        g_Cars[1].speed, g_Cars[1].trackProgress,
                        g_Cars[1].bodyYaw, g_Cars[1].trackLateralOffset,
                        g_Cars[1].collisionFlag, g_Cars[1].activeFlag,
                        g_Cars[2].speed, g_Cars[2].trackProgress,
                        g_Cars[2].bodyYaw, g_Cars[2].trackLateralOffset,
                        g_Cars[2].collisionFlag, g_Cars[2].activeFlag,
                        g_Cars[3].speed, g_Cars[3].trackProgress,
                        g_Cars[3].bodyYaw, g_Cars[3].trackLateralOffset,
                        g_Cars[3].collisionFlag, g_Cars[3].activeFlag);
                {
                    int rivalIndex;
                    for (rivalIndex = 0; rivalIndex < 4; rivalIndex++) {
                        const unsigned char *bytes =
                            (const unsigned char *)&g_Cars[rivalIndex];
                        size_t byteIndex;
                        fputc(',', g_SmokeCaptureManifest);
                        for (byteIndex = 0; byteIndex < sizeof(GameCarRuntime);
                             byteIndex++) {
                            fprintf(g_SmokeCaptureManifest, "%02x",
                                    bytes[byteIndex]);
                        }
                    }
                    fputc('\n', g_SmokeCaptureManifest);
                }
                fflush(g_SmokeCaptureManifest);
            }
        }
        lastCapturedScene = g_SceneId;
        lastCapturedTimer = g_SceneTimer;
    }
    if (g_SmokeHasStopScene && g_SmokeHasStopSceneTimer &&
        g_SceneId == g_SmokeStopScene &&
        g_SceneTimer >= g_SmokeStopSceneTimer) {
        fprintf(stderr, "smoke synchronized stop frame=%d scene=%d timer=%d\n",
                frame_number, g_SceneId, g_SceneTimer);
        return 1;
    }
    return g_SmokeFrameLimit > 0 && frame_number >= g_SmokeFrameLimit;
}
