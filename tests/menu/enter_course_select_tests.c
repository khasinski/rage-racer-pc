#include "common.h"
#include "game/menu.h"
#include "game/player_car_internal.h"
#include "game/save_internal.h"
#include "game/state.h"

#include <stdio.h>
#include <string.h>

s32 g_CourseCardPendingGrade;
s32 g_CourseCardSpin;
s32 g_CourseCardSpinTarget;
s32 g_CourseIndex;
CourseProgressState *g_CourseProgress;
s32 g_MenuHandlerIndex;
s32 g_MenuScreen;
s32 g_MenuViewAngle;
s32 g_MenuViewAngleTarget;
s32 g_MenuViewOffset;
s32 g_MenuViewOffsetTarget;
s32 g_MenuViewSpin;
PlayerCarRuntime g_PlayerCar;
s32 g_SceneTimer;
TeamLogoCanvas g_TeamLogoCanvas;
u16 g_TeamLogoClut[16];
Rect g_TeamLogoClutRect;
TeamLogoRect g_TeamLogoRect;
u8 g_TeamNameChars[16];
u8 g_TeamNameLength;
s32 g_TimeAttackPlateStep;
s32 g_UiScriptProgress;

static s32 s_assetRequestResult;
static s32 s_arrowCalls;
static s32 s_imageLoads;
static s32 s_sequenceCalls;
static s32 s_textCalls;
static s32 s_teamNameUploads;
static s32 s_teamLogoClutUploads;

s32 RequestCarSelectAssets(void) { return s_assetRequestResult; }
void PlaySequence(void) { s_sequenceCalls++; }
void DrawBrowseArrows(s32 step, s32 wide, s32 left, s32 right) {
    (void)step;
    (void)wide;
    (void)left;
    (void)right;
    s_arrowCalls++;
}
void DrawText8x8(s32 x, s32 y, const char *text, s32 clut) {
    (void)x;
    (void)y;
    (void)text;
    (void)clut;
    s_textCalls++;
}
#undef LoadImage
int LoadImage(RECT *rect, u_long *data) {
    (void)rect;
    (void)data;
    s_imageLoads++;
    return 0;
}
void UploadTeamNameTexture(const u8 *text, s32 length) {
    (void)text;
    (void)length;
    s_teamNameUploads++;
}
void UploadTeamLogoClut(void) { s_teamLogoClutUploads++; }

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "check failed at line %d: %s\n", __LINE__,       \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static void PoisonState(void) {
    memset(&g_PlayerCar, 0x5A, sizeof(g_PlayerCar));
    g_MenuHandlerIndex = -7;
    g_MenuScreen = -8;
    g_MenuViewAngle = -1;
    g_MenuViewAngleTarget = -1;
    g_MenuViewOffset = -1;
    g_MenuViewOffsetTarget = -1;
    g_MenuViewSpin = -1;
    g_UiScriptProgress = -1;
    g_CourseCardSpin = -1;
    g_CourseCardSpinTarget = -1;
    g_CourseCardPendingGrade = -1;
    g_TimeAttackPlateStep = 0;
    s_arrowCalls = 0;
    s_imageLoads = 0;
    s_sequenceCalls = 0;
    s_teamNameUploads = 0;
    s_teamLogoClutUploads = 0;
}

static int CheckShowroomReset(s32 expectedGrade, s32 expectedPlateStep) {
    CHECK(g_MenuHandlerIndex == MENU_SCREEN_COURSE_SELECT);
    CHECK(g_MenuScreen == MENU_SCREEN_COURSE_SELECT);
    CHECK(g_MenuViewOffset == 250000 && g_MenuViewOffsetTarget == 0);
    CHECK(g_MenuViewAngle == 500000 && g_MenuViewAngleTarget == 500000);
    CHECK(g_MenuViewSpin == 8 && g_UiScriptProgress == 0);
    CHECK(g_CourseCardSpin == 2048000 && g_CourseCardSpinTarget == 0);
    CHECK(g_CourseCardPendingGrade == expectedGrade);
    CHECK(g_TimeAttackPlateStep == expectedPlateStep);
    CHECK(g_PlayerCar.x == 0 && g_PlayerCar.y == 0 && g_PlayerCar.z == 0);
    CHECK(g_PlayerCar.bodyPitch == 0 && g_PlayerCar.bodyYaw == 0 &&
          g_PlayerCar.bodyRoll == 0);
    CHECK(g_PlayerCar.trackProgress == 0 && g_PlayerCar.steeringAngle == 0 &&
          g_PlayerCar.wheelRotation == 0);
    CHECK(s_sequenceCalls == 1 && s_arrowCalls == 1);
    CHECK(s_imageLoads == 1 && s_teamLogoClutUploads == 1 &&
          s_teamNameUploads == 1);
    return 0;
}

int main(void) {
    CourseProgressState progress = {{4, 3, 2, 1}, 0, 0};

    g_CourseProgress = &progress;
    PoisonState();
    g_SceneTimer = 8;
    s_textCalls = 0;
    s_assetRequestResult = 1;
    EnterCourseSelectScreen();
    CHECK(s_textCalls == 1);
    CHECK(g_MenuHandlerIndex == -7 && g_MenuScreen == -8);
    CHECK(s_sequenceCalls == 0 && s_arrowCalls == 0 && s_imageLoads == 0 &&
          s_teamLogoClutUploads == 0 && s_teamNameUploads == 0);

    PoisonState();
    g_CourseIndex = 2;
    g_SceneTimer = 0;
    s_assetRequestResult = 0;
    EnterCourseSelectScreen();
    CHECK(s_textCalls == 1);
    if (CheckShowroomReset(2, -1)) return 1;

    PoisonState();
    g_CourseIndex = 5;
    EnterCourseSelectScreen();
    if (CheckShowroomReset(3, 1)) return 1;

    puts("enter course select tests passed");
    return 0;
}
