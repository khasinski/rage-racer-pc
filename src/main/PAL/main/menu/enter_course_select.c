#include "game/asset.h"
#include "game/audio.h"
#include "game/course_index.h"
#include "game/menu.h"
#include "game/menu_internal.h"
#include "game/player_car_internal.h"
#include "game/save_internal.h"
#include "game/state.h"

enum {
    COURSE_SELECT_VIEW_ANGLE = 500000,
    COURSE_SELECT_INITIAL_VIEW_OFFSET = 250000,
    COURSE_SELECT_INITIAL_CARD_SPIN = 2048000,
};

const char g_NowLoadingText[] = "NOW LOADING";

static void DrawNowLoadingText(void) {
    if (g_SceneTimer & 8) {
        DrawText8x8(0x74, 0xEC, g_NowLoadingText, 0x78CC);
    }
}

static void ResetCourseSelectShowroom(void) {
    s32 course = g_CourseIndex;

    g_MenuViewOffset = COURSE_SELECT_INITIAL_VIEW_OFFSET;
    g_MenuViewSpin = 8;
    g_UiScriptProgress = 0;
    g_PlayerCar.x = 0;
    g_PlayerCar.y = 0;
    g_PlayerCar.z = 0;
    g_PlayerCar.bodyPitch = 0;
    g_PlayerCar.bodyYaw = 0;
    g_PlayerCar.bodyRoll = 0;
    g_PlayerCar.trackProgress = 0;
    g_PlayerCar.steeringAngle = 0;
    g_PlayerCar.wheelRotation = 0;
    g_MenuViewAngleTarget = COURSE_SELECT_VIEW_ANGLE;
    g_MenuViewAngle = COURSE_SELECT_VIEW_ANGLE;
    g_MenuViewOffsetTarget = 0;
    g_CourseCardSpin = COURSE_SELECT_INITIAL_CARD_SPIN;
    g_CourseCardSpinTarget = 0;
    g_CourseCardPendingGrade = g_CourseProgress->bestPlace[course & 3];
    g_TimeAttackPlateStep = course >= 4 ? 1 : -1;
}

/* g_MenuScreenUpdate[MENU_SCREEN_BOOTSTRAP]: wait for the shared car-select
 * assets before exposing the first interactive menu screen. */
void EnterCourseSelectScreen(void) {
    DrawNowLoadingText();
    if (RequestCarSelectAssets() != 0) {
        return;
    }

    PlaySequence();
    g_MenuHandlerIndex = MENU_SCREEN_COURSE_SELECT;
    g_MenuScreen = MENU_SCREEN_COURSE_SELECT;
    DrawBrowseArrows(0, 0, 0, 0);
    ResetCourseSelectShowroom();
    LoadImage(&g_TeamLogoRect.rect, &g_TeamLogoCanvas);
    UploadTeamLogoClut();
    UploadTeamNameTexture(g_TeamNameChars, g_TeamNameLength);
}
