#include "common.h"
#include "game/car.h"
#include "game/frontend_internal.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/render_internal.h"
#include "game/save_internal.h"
#include "game/screens.h"

#include <limits.h>
#include <stdio.h>

CarEntry g_GrandPrixCars[GAME_CAR_COUNT];
CarEntry g_ExtraGrandPrixCars[GAME_CAR_COUNT];
CarEntry g_TimeAttackCars[GAME_CAR_COUNT];
CarEntry *g_CarTable;
CourseProgressState g_GrandPrixCourseProgress;
CourseProgressState g_ExtraGrandPrixCourseProgress;
CourseProgressState *g_CourseProgress;
GameRaceProgress g_GrandPrixSave;
GameRaceProgress g_ExtraGrandPrixSave;
GameRaceProgress g_TimeAttackSave;
GameRaceProgress *g_RaceProgress;
GameFrameContext *g_DrawBuffer;
GameRenderState g_RenderState;
s32 g_CourseIndex;
s16 g_ExtraGrandPrixUnlocked;
u32 g_FrontendIdleTimer;
FrontendState g_FrontendState;
s32 g_GrandPrixClass;
s32 g_MainMenuSlide;
s32 g_OptionMenuCursor;
u16 g_PadPressed;
s16 g_SeriesSelection;
s32 g_TitleMenuSelection;
s32 g_TitlePulse;

static GameFrameContext s_frame;
static s32 s_assetComplete;
static s32 s_courseRequests;
static s32 s_optionRequests;
static s32 s_resetCalls;
static s32 s_saveRequests;
static s32 s_selectBgmRequests;
static s32 s_shuffleCalls;

s32 AssetLoadCompletedSuccessfully(void) { return s_assetComplete; }
void ResetAssetLoader(void) { s_resetCalls++; }
void ShuffleBgmOrder(void) { s_shuffleCalls++; }
s32 RequestCourseTextureAssets(void) {
    s_courseRequests++;
    return 1;
}
s32 RequestOptionScreenAssets(void) {
    s_optionRequests++;
    return 1;
}
s32 RequestSaveScreenAssets(void) {
    s_saveRequests++;
    return 1;
}
s32 RequestSelectBgmAssetsKeepAudioSlots(void) {
    s_selectBgmRequests++;
    return 1;
}
void PlaySoundCue(s32 cue) { (void)cue; }
u8 *GameQueueTexturedRect(GameOrderingTableEntry *ot, u8 *packet, s32 x,
                          s32 y, s32 width, s32 height, s32 u, s32 v,
                          s32 textureWidth, s32 textureHeight, s32 clut,
                          s32 flags) {
    (void)ot;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)u;
    (void)v;
    (void)textureWidth;
    (void)textureHeight;
    (void)clut;
    (void)flags;
    return packet;
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "check failed at line %d: %s\n", __LINE__,       \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static void ResetState(s32 selection) {
    g_DrawBuffer = &s_frame;
    g_RenderState.packetCursor = s_frame.layout.primitiveBuffer;
    g_TitleMenuSelection = selection;
    g_PadPressed = PAD_CONFIRM;
    g_FrontendState = FRONTEND_STATE_MENU_INPUT;
    g_FrontendIdleTimer = 99;
    g_OptionMenuCursor = 7;
    g_GrandPrixClass = 5;
    g_CourseIndex = 2;
    s_assetComplete = 0;
    s_courseRequests = 0;
    s_optionRequests = 0;
    s_resetCalls = 0;
    s_saveRequests = 0;
    s_selectBgmRequests = 0;
    s_shuffleCalls = 0;
}

static int CheckCommonConfirmation(void) {
    CHECK(g_FrontendState == FRONTEND_STATE_MENU_EXIT);
    CHECK(g_FrontendIdleTimer == 0);
    CHECK(s_shuffleCalls == 1 && s_resetCalls == 1);
    return 0;
}

int main(void) {
    g_GrandPrixSave.maxClassReached = 1;
    ResetState(TITLE_MENU_GRAND_PRIX);
    UpdateMainMenuInput();
    if (CheckCommonConfirmation()) return 1;
    CHECK(g_CarTable == g_GrandPrixCars && g_RaceProgress == &g_GrandPrixSave);
    CHECK(g_CourseProgress == &g_GrandPrixCourseProgress);
    CHECK(g_SeriesSelection == 0 && s_selectBgmRequests == 1);

    g_GrandPrixSave.maxClassReached = -1;
    ResetState(TITLE_MENU_GRAND_PRIX);
    UpdateMainMenuInput();
    CHECK(s_courseRequests == 1 && s_selectBgmRequests == 0);
    CHECK(g_GrandPrixClass == 0 && g_CourseIndex == 3);

    g_ExtraGrandPrixSave.maxClassReached = 1;
    ResetState(TITLE_MENU_EXTRA_GRAND_PRIX);
    UpdateMainMenuInput();
    CHECK(g_CarTable == g_ExtraGrandPrixCars &&
          g_RaceProgress == &g_ExtraGrandPrixSave);
    CHECK(g_CourseProgress == &g_ExtraGrandPrixCourseProgress);
    CHECK(g_SeriesSelection == 1 && s_selectBgmRequests == 1);

    g_ExtraGrandPrixSave.maxClassReached = -1;
    ResetState(TITLE_MENU_EXTRA_GRAND_PRIX);
    UpdateMainMenuInput();
    CHECK(s_courseRequests == 1 && s_selectBgmRequests == 0);
    CHECK(g_GrandPrixClass == 0 && g_CourseIndex == 3);

    ResetState(TITLE_MENU_TIME_ATTACK);
    UpdateMainMenuInput();
    CHECK(g_CarTable == g_TimeAttackCars && g_RaceProgress == &g_TimeAttackSave);
    CHECK(g_SeriesSelection == 0 && s_selectBgmRequests == 1);

    ResetState(TITLE_MENU_LOAD_SAVE);
    UpdateMainMenuInput();
    CHECK(s_saveRequests == 1);

    ResetState(TITLE_MENU_OPTIONS);
    UpdateMainMenuInput();
    CHECK(s_optionRequests == 1 && g_OptionMenuCursor == 0);

    ResetState(TITLE_MENU_OPTIONS);
    s_assetComplete = 1;
    UpdateMainMenuInput();
    CHECK(s_resetCalls == 0);

    ResetState(TITLE_MENU_GRAND_PRIX);
    g_PadPressed = 0;
    g_FrontendState = FRONTEND_STATE_MENU_OPENING;
    g_MainMenuSlide = INT_MAX;
    UpdateMainMenuOpen();
    CHECK(g_MainMenuSlide == 0x30);
    CHECK(g_FrontendState == FRONTEND_STATE_MENU_INPUT);

    g_FrontendState = FRONTEND_STATE_MENU_OPENING;
    g_MainMenuSlide = INT_MIN;
    UpdateMainMenuOpen();
    CHECK(g_MainMenuSlide == 1);
    CHECK(g_FrontendState == FRONTEND_STATE_MENU_OPENING);

    puts("main menu state tests passed");
    return 0;
}
