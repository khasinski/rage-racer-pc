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

static Frontend s_frontend;

static OptionMenu s_optionMenu;

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
s32 g_GrandPrixClass;
u16 g_PadPressed;
s16 g_SeriesSelection;

static GameFrameContext s_frame;
static s32 s_assetComplete;
static s32 s_courseRequests;
static s32 s_optionRequests;
static s32 s_resetCalls;
static s32 s_saveRequests;
static s32 s_selectBgmRequests;
static s32 s_shuffleCalls;
static s32 s_labelCount;
static TitleMenuItem s_labelItem[TITLE_MENU_ITEM_COUNT];
static s32 s_labelY[TITLE_MENU_ITEM_COUNT];
static s32 s_labelHeight[TITLE_MENU_ITEM_COUNT];
static s32 s_labelSelected[TITLE_MENU_ITEM_COUNT];

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
u8 *DrawTitleMenuLabel(GameOrderingTableEntry *ot, u8 *packet,
                       TitleMenuItem item, s32 x, s32 y,
                       s32 visibleHeight, s32 selected) {
    (void)ot;
    (void)x;
    if (s_labelCount < TITLE_MENU_ITEM_COUNT) {
        s_labelItem[s_labelCount] = item;
        s_labelY[s_labelCount] = y;
        s_labelHeight[s_labelCount] = visibleHeight;
        s_labelSelected[s_labelCount] = selected;
    }
    s_labelCount++;
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
    g_RenderState.draw.packetCursor = s_frame.layout.primitiveBuffer;
    s_frontend.selection = selection;
    g_PadPressed = PAD_CONFIRM;
    s_frontend.state = FRONTEND_STATE_MENU_INPUT;
    s_frontend.idleTimer = 99;
    s_optionMenu.cursor = 7;
    g_GrandPrixClass = 5;
    g_CourseIndex = 2;
    s_assetComplete = 0;
    s_courseRequests = 0;
    s_optionRequests = 0;
    s_resetCalls = 0;
    s_saveRequests = 0;
    s_selectBgmRequests = 0;
    s_shuffleCalls = 0;
    s_labelCount = 0;
}

static int CheckCommonConfirmation(void) {
    CHECK(s_frontend.state == FRONTEND_STATE_MENU_EXIT);
    CHECK(s_frontend.idleTimer == 0);
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

    ResetState(TITLE_MENU_CUSTOM);
    UpdateMainMenuInput();
    CHECK(s_frontend.state == FRONTEND_STATE_MENU_INPUT);
    CHECK(s_resetCalls == 0 && s_shuffleCalls == 0);

    ResetState(TITLE_MENU_LOAD_SAVE);
    UpdateMainMenuInput();
    CHECK(s_saveRequests == 1);

    ResetState(TITLE_MENU_OPTIONS);
    UpdateMainMenuInput();
    CHECK(s_optionRequests == 1 && s_optionMenu.cursor == 0);

    ResetState(TITLE_MENU_OPTIONS);
    s_assetComplete = 1;
    UpdateMainMenuInput();
    CHECK(s_resetCalls == 0);

    ResetState(TITLE_MENU_GRAND_PRIX);
    g_PadPressed = 0;
    s_frontend.state = FRONTEND_STATE_MENU_OPENING;
    s_frontend.menuSlide = INT_MAX;
    UpdateMainMenuOpen();
    CHECK(s_frontend.menuSlide == 0x38);
    CHECK(s_frontend.state == FRONTEND_STATE_MENU_INPUT);

    s_frontend.state = FRONTEND_STATE_MENU_OPENING;
    s_frontend.menuSlide = INT_MIN;
    UpdateMainMenuOpen();
    CHECK(s_frontend.menuSlide == 1);
    CHECK(s_frontend.state == FRONTEND_STATE_MENU_OPENING);

    ResetState(TITLE_MENU_CUSTOM);
    g_PadPressed = 0;
    g_ExtraGrandPrixUnlocked = 1;
    s_frontend.menuSlide = 0x38;
    s_frontend.pulse = 0;
    DrawMainMenuRows();
    CHECK(s_labelCount == TITLE_MENU_ITEM_COUNT);
    CHECK(s_labelItem[0] == TITLE_MENU_GRAND_PRIX);
    CHECK(s_labelItem[1] == TITLE_MENU_EXTRA_GRAND_PRIX);
    CHECK(s_labelItem[2] == TITLE_MENU_TIME_ATTACK);
    CHECK(s_labelItem[3] == TITLE_MENU_CUSTOM);
    CHECK(s_labelItem[4] == TITLE_MENU_LOAD_SAVE);
    CHECK(s_labelItem[5] == TITLE_MENU_OPTIONS);
    CHECK(s_labelY[0] == 0x64 && s_labelY[1] == 0x7C);
    CHECK(s_labelY[2] == 0x94 && s_labelY[3] == 0xAC);
    CHECK(s_labelY[4] == 0xC4 && s_labelY[5] == 0xDC);
    CHECK(s_labelHeight[3] == 0x10 && s_labelSelected[3] == 1);

    puts("main menu state tests passed");
    return 0;
}

OptionMenu *MenuOption(void) { return &s_optionMenu; }

Frontend *MenuFrontend(void) { return &s_frontend; }
