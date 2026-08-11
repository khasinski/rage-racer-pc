#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game/player_car_internal.h"
#include "game/state.h"

typedef struct RageSmokeInput {
    long firstFrame;
    long lastFrame;
    unsigned short buttons;
    int held;
} RageSmokeInput;

static RageSmokeInput g_SmokeInputs[64];
static int g_SmokeInputCount;
static long g_SmokeFrameLimit;
static long g_SmokeFinishFrame;
static long g_SmokeAutoConfirmFrame;
static long g_SmokeStopScene;
static long g_SmokeStopSceneTimer;
static int g_SmokeHasStopScene;
static int g_SmokeHasStopSceneTimer;
static int g_SmokeInitialized;
static int g_SmokeRawPadPath;

extern int g_SceneId;
extern int g_FrontendState;
extern int g_TrackLength;
extern int g_SkyRowBase;
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
    char *copy;
    char *token;

    g_SmokeFrameLimit = limit ? strtol(limit, NULL, 10) : 0;
    g_SmokeFinishFrame = finish ? strtol(finish, NULL, 10) : 0;
    g_SmokeAutoConfirmFrame = autoConfirm ? strtol(autoConfirm, NULL, 10) : 0;
    g_SmokeHasStopScene = stopScene != NULL;
    g_SmokeHasStopSceneTimer = stopTimer != NULL;
    g_SmokeStopScene = stopScene ? strtol(stopScene, NULL, 10) : 0;
    g_SmokeStopSceneTimer = stopTimer ? strtol(stopTimer, NULL, 10) : 0;
    if (script != NULL && script[0] != '\0') {
        g_SmokeRawPadPath = 1;
    } else {
        script = getenv("RAGE_PORT_INPUT_SCRIPT");
    }
    if (script == NULL || script[0] == '\0') return;
    copy = strdup(script);
    if (copy == NULL) return;
    for (token = strtok(copy, ","); token != NULL && g_SmokeInputCount < 64;
         token = strtok(NULL, ",")) {
        char *separator = strchr(token, ':');
        char *rangeSeparator;
        unsigned short buttons;
        char *name;
        if (separator == NULL) continue;
        *separator = '\0';
        name = separator + 1;
        while (isspace((unsigned char)*name)) name++;
        buttons = RageSmokeButton(name);
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
}

int RagePortShouldExit(int frame_number) {
    static int lastScene = -1;
    static int lastFrontend = -1;
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
    if (g_SmokeHasStopScene && g_SmokeHasStopSceneTimer &&
        g_SceneId == g_SmokeStopScene &&
        g_SceneTimer >= g_SmokeStopSceneTimer) {
        fprintf(stderr, "smoke synchronized stop frame=%d scene=%d timer=%d\n",
                frame_number, g_SceneId, g_SceneTimer);
        return 1;
    }
    return g_SmokeFrameLimit > 0 && frame_number >= g_SmokeFrameLimit;
}
