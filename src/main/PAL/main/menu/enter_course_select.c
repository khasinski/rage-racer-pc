#include "game/asset.h"
#include "game/audio.h"
#include "game/course_index.h"
#include "game/menu.h"
#include "game/menu_internal.h"
#include "game/player_car_internal.h"
#include "game/save_internal.h"
#include "game/state.h"

enum {
    COURSE_SELECT_INITIAL_VIEW_OFFSET = MENU_VIEW_OFFSET_MAX,
    COURSE_SELECT_INITIAL_CARD_SPIN = 2048000,
};

static void ResetCourseSelectShowroom(CourseSelectScreen *screen) {
    s32 course = AddClampedMenuValue(
        g_CourseIndex, 0, 0, PHYSICAL_COURSE_COUNT - 1);

    g_CourseIndex = course;
    g_MenuViewOffset = COURSE_SELECT_INITIAL_VIEW_OFFSET;
    StartMenuCarRotation();
    g_UiScriptProgress = 0;
    g_MenuViewAngleTarget = MENU_COURSE_VIEW_REBASE_SPAN;
    g_MenuViewAngle = MENU_COURSE_VIEW_REBASE_SPAN;
    g_MenuViewOffsetTarget = 0;
    screen->cardSpin = COURSE_SELECT_INITIAL_CARD_SPIN;
    screen->cardSpinTarget = 0;
    screen->cardPendingGrade =
        g_CourseProgress != NULL
            ? g_CourseProgress->bestPlace[CourseSlot(course)]
            : 0;
    MenuWidgetState()->timeAttackStep = CourseSeries(course) != 0 ? 1 : -1;
}

/* g_MenuScreenUpdate[MENU_SCREEN_BOOTSTRAP]: wait for the shared car-select
 * assets before exposing the first interactive menu screen. */
void EnterCourseSelectScreen(void) {
    CourseSelectScreen *screen = MenuCourseSelect();
    if (RequestCarSelectAssets() != 0) {
        return;
    }

    PlaySequence();
    MenuActivateScreen(MENU_SCREEN_COURSE_SELECT);
    DrawBrowseArrows(MenuBrowseArrows(), 0, 0, 0, 0);
    ResetCourseSelectShowroom(screen);
    LoadImage(&g_TeamLogoRect, &g_TeamLogoCanvas);
    UploadTeamLogoClut();
    UploadTeamNameTexture(g_TeamNameChars, g_TeamNameLength);
}
