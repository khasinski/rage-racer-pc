#include "game/asset.h"
#include "game/course_index.h"
#include "game/menu.h"
#include "game/menu_internal.h"
#include "game/race.h"
#include "game/player_car_internal.h"
#include "game/render_internal.h"
#include "game/track.h"

#include <string.h>

enum {
    MENU_VIEW_FIXED_SCALE = 1000,
    SHOWROOM_FLOOR_MODEL = 5,
    SHOWROOM_MODEL_BANK = 14,
    SHOWROOM_OT_DEPTH_BIAS = 30,
};

static PlayerCarRuntime s_Car;
static s32 s_rotationSpeed;

void ResetMenuCar(void) {
    memset(&s_Car, 0, sizeof(s_Car));
    s_rotationSpeed = 0;
}

void StartMenuCarRotation(void) { s_rotationSpeed = 8; }

s32 MenuCarRotationSpeed(void) { return s_rotationSpeed; }

static void SetupMenuViewCamera(s32 pitch, s32 yaw) {
    GameViewWork view = {
        .x = 0, .y = -64, .z = -256,
        .angleX = pitch, .angleY = yaw, .angleZ = 0,
    };
    SetCameraRotMatrix(&g_RenderState, &view);
    ScaleMatrix(&g_RenderState.geometry.matrix, &g_MenuViewScale);

    g_MenuViewOffset = PrepareMenuViewOffset(
        g_MenuViewOffset, g_MenuViewOffsetTarget);
}

static s32 AdvanceMenuViewOffset(void) {
    g_MenuViewOffset = AdvanceMenuViewOffsetValue(
        g_MenuViewOffset, g_MenuViewOffsetTarget);
    return g_MenuViewOffset / 1000;
}

static void DrawShowroomFloor(PlayerCarRuntime *car, Matrix *matrix) {
    GameOrderingTableEntry *originalOt = RENDER_OT_BASE;
    s32 modelIndex;

    if (originalOt == NULL) {
        return;
    }

    car->z = 0;
    SelectModelBank(SHOWROOM_MODEL_BANK);
    RENDER_OT_BASE = originalOt + SHOWROOM_OT_DEPTH_BIAS;
    SetGteObjectMatrix(AsPositionWords(&car->x), matrix);
    g_RenderState.geometry.envMode4 = 0;
    modelIndex = MenuModelIndexOrFallback(SHOWROOM_FLOOR_MODEL,
                                          g_ModelBankCount);
    if (modelIndex >= 0) {
        SubmitModel(&g_RenderState, modelIndex);
    }
    RENDER_OT_BASE = originalOt;
}

void DrawMenuCarView(void) {
    PlayerCarRuntime *car = &s_Car;
    GameCarRuntime *renderObject = AsRivalCar(car);
    Matrix mtxA;
    Matrix mtxB;
    Vec4 out;
    Vec4 vec;
    s32 carIndex;
    s32 viewHeight;
    s32 currentAngle;
    s32 horizontalAngle;
    s32 offset;
    ShowroomCarLoadAction loadAction;

    vec = g_MenuCarPivotOffset;
    SetupMenuViewCamera(0x100, 0);

    currentAngle = g_MenuViewAngle;
    if (ShowroomCarAtSwapPoint(currentAngle, g_MenuViewAngleTarget,
                               g_CarSwapToIndex)) {
        loadAction = ResolveShowroomCarLoadAction(
            AssetLoadCompletedSuccessfully(), AssetLoadHasFailed());
        if (loadAction == SHOWROOM_CAR_LOAD_CANCEL) {
            g_CarSwapToIndex = -1;
            return;
        }
        if (loadAction == SHOWROOM_CAR_LOAD_WAIT) {
            return;
        }
        if (!ActivateShowroomCarModel(g_CarModelSlot < 1)) {
            g_CarSwapToIndex = -1;
            return;
        }
        g_CarSwapFromIndex = g_CarSwapToIndex;
        g_CarSwapToIndex = -1;
        if (g_RaceSession.kind == RACE_SESSION_CUSTOM &&
            MenuCarBrowse()->targetModel >= 0) {
            MenuCarBrowse()->displayedModel = MenuCarBrowse()->targetModel;
            MenuCarBrowse()->targetModel = -1;
        }
    } else if (currentAngle != g_MenuViewAngleTarget) {
        g_MenuViewAngle = AdvanceMenuViewAngleValue(
            currentAngle, g_MenuViewAngleTarget, 24);
    }

    horizontalAngle =
        MenuWrapAngle(g_MenuViewAngle, MENU_CAR_VIEW_REBASE_SPAN) /
        MENU_VIEW_FIXED_SCALE;
    carIndex = g_CarSwapFromIndex;
    viewHeight = AdvanceMenuViewOffset();
    if ((u32)carIndex >= GAME_CAR_COUNT || g_CarTable == NULL ||
        g_CarModelAsset == NULL) {
        return;
    }
    car->modelIndex =
        GetCarAssetIndex(carIndex, g_CarTable[carIndex].modelVariant);
    if (car->modelIndex < 0) {
        return;
    }
    car->showroomTireCompound = g_CarTable[carIndex].tireCompound;
    car->steeringAngle = UpdatedShowroomSteering(car->steeringAngle, g_PadHeld);
    car->drive.manual = g_CarTable[carIndex].transmission;
    car->wheelRotation = ((u32)car->wheelRotation + 68u) & 0xFFFu;

    s_rotationSpeed = UpdatedMenuViewSpin(s_rotationSpeed, g_PadHeld);
    car->bodyRotation.y =
        (s32)((u32)car->bodyRotation.y + (u32)s_rotationSpeed);
    BuildRotMatrixY(&mtxA, car->bodyRotation.y);
    vec.z = (s16)(-((s16)g_CarModelAsset->modelOffsetZ / 2));
    ApplyMatrixLV(&mtxA, AsWords(&vec), AsWords(&out));
    BuildRotMatrixY(&mtxB, 0x800 - car->bodyRotation.y);
    BuildRotMatrixX(&mtxA, car->bodyRotation.x);
    MulMatrix2(&mtxB, &mtxA);
    MulMatrix2(&g_RenderState.geometry.matrix, &mtxA);

    offset = horizontalAngle - 52;
    car->x = out.x - offset;
    car->y = viewHeight + 30;
    car->z = -out.z;
    car->modelRotation = car->bodyRotation;
    car->modelY = car->y;
    if (g_RaceSession.kind == RACE_SESSION_CUSTOM &&
        CustomRaceRivalModelForSelection(
            MenuCarBrowse()->displayedModel) >= 0) {
        DrawCustomRivalPreview(renderObject, MenuCarBrowse()->displayedModel);
    } else {
        SelectModelBank(g_CarModelSlot);
        DrawPlayerCarModel(renderObject);
    }

    car->x = 52 - horizontalAngle;
    car->y = viewHeight + 30;
    DrawShowroomFloor(car, &mtxA);
}

/* The course diorama behind COURSE SELECT and RANKING, with the carousel easing. */
void DrawMenuCourseView(CourseSelectScreen *screen) {
    PlayerCarRuntime *car = &s_Car;
    GameCarRuntime *renderObject = AsRivalCar(car);
    Matrix mtxA;
    Matrix mtxB;
    s32 horizontalAngle;
    s32 courseModelIndex;
    s32 viewHeight;
    CourseCarouselAnimation animation;

    SetupMenuViewCamera(0x100, 0);

    animation = AdvanceCourseCarouselAnimation(
        g_MenuViewAngle, g_MenuViewAngleTarget, screen->swapDelay,
        screen->displayedCourse, screen->pendingCourse);
    g_MenuViewAngle = animation.angle;
    screen->swapDelay = animation.swapDelay;
    screen->displayedCourse = animation.displayedCourse;
    screen->pendingCourse = animation.pendingCourse;

    horizontalAngle =
        MenuWrapAngle(g_MenuViewAngle, MENU_COURSE_VIEW_REBASE_SPAN) /
        MENU_VIEW_FIXED_SCALE;
    courseModelIndex = screen->displayedCourse;
    viewHeight = AdvanceMenuViewOffset();

    car->x = 23 - horizontalAngle;
    car->z = -20;
    car->y = viewHeight + 15;

    s_rotationSpeed = UpdatedMenuViewSpin(s_rotationSpeed, g_PadHeld);
    car->bodyYaw = (s32)((u32)car->bodyYaw + (u32)s_rotationSpeed);
    BuildRotMatrixY(&mtxB, 0x800 - car->bodyYaw);
    BuildRotMatrixX(&mtxA, car->bodyPitch);
    MulMatrix2(&mtxB, &mtxA);
    MulMatrix2(&g_RenderState.geometry.matrix, &mtxA);
    SelectModelBank(SHOWROOM_MODEL_BANK);
    SetGteObjectMatrix(AsPositionWords(&renderObject->x), &mtxA);
    g_RenderState.geometry.envMode4 = 0;
    courseModelIndex = MenuModelIndexOrFallback(
        CourseSlot(courseModelIndex), g_ModelBankCount);
    if (courseModelIndex >= 0) {
        SubmitModel(&g_RenderState, courseModelIndex);
    }
}

/* The 3D character model under the TEAM NAME grid cursor; skips the BS and ED cells. */
void DrawTeamNameCharModel(TeamName *teamName) {
    Matrix mtxA;
    Matrix mtxB;
    Vec4 position;
    Vec4 vcopy;
    s32 viewHeight;
    s32 baseHeight;
    s32 modelIndex;
    s32 rotationY;
    s32 rotationZ;
    TeamNameModelAnimation animation;

    vcopy = g_TeamNameCharScale;

    SetupMenuViewCamera(0, -104);

    animation = AdvanceTeamNameModelAnimation(
        g_MenuViewAngle, g_MenuViewAngleTarget, teamName->charModel,
        GameMenuCursorAnim);
    g_MenuViewAngle = animation.angle;
    teamName->charModel = animation.displayedModel;
    GameMenuCursorAnim = animation.pendingModel;

    viewHeight = AdvanceMenuViewOffset();
    baseHeight = 40;

    position.x = 0;
    position.y =
        (viewHeight - baseHeight) +
        rsin((s32)((u32)g_AnimTimer * 32u & 0xFE0u)) * 12 / 4096;
    position.z = 0;
    position.w = 0;
    rotationY = g_MenuViewAngle / MENU_VIEW_FIXED_SCALE;
    rotationZ =
        rsin((s32)((u32)g_AnimTimer * 20u & 0xFFCu)) * 72 / 4096;

    BuildRotMatrixY(&mtxB, 0x800 - rotationY);
    BuildRotMatrixZ(&mtxA, rotationZ);
    MulMatrix2(&mtxB, &mtxA);
    MulMatrix2(&g_RenderState.geometry.matrix, &mtxA);
    ScaleMatrix(&mtxA, &vcopy);

    modelIndex = TeamNameCharacterModelIndex(teamName->charModel,
                                             g_CourseModelCount);
    if (modelIndex >= 0) {
        SetGteObjectMatrix(AsPosition(&position), &mtxA);
        g_RenderState.geometry.envMode4 = 0;
        SubmitCourseModel(&g_RenderState, modelIndex);
    }
}
