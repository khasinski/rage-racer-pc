#include "rage/menu_animation.h"
#include "game/asset.h"
#include "game/audio.h"
#include "game/car.h"
#include "game/frontend_internal.h"
#include "game/menu.h"
#include "game/menu_internal.h"
#include "game/race.h"
#include "game/render_internal.h"
#include "game/save_internal.h"
#include "game/screens.h"
#include <string.h>
#include "main_menu_ui.h"

enum { MAIN_MENU_OPEN_FRAMES = 16 };

/* Menu rows are rendered as native text by the host presentation layer. */
void DrawMainMenuRows(void) {}

void UpdateMainMenuOpen(void) {
    Frontend *frontend = MenuFrontend();

    frontend->menuSlide = AddClampedMenuValue(
        frontend->menuSlide, 0, 0, MAIN_MENU_OPEN_FRAMES);
    frontend->menuSlide = AddClampedMenuValue(
        frontend->menuSlide, PortMenuAnimationStep(1), 0, MAIN_MENU_OPEN_FRAMES);
    if (frontend->menuSlide >= MAIN_MENU_OPEN_FRAMES) {
        frontend->state = FRONTEND_STATE_MENU_INPUT;
    }
    DrawMainMenuRows();
}

static void SelectGrandPrixSave(CarEntry *cars, GameRaceProgress *progress,
                                CourseProgressState *courseProgress,
                                s16 series) {
    g_CarTable = cars;
    g_RaceProgress = progress;
    g_CourseProgress = courseProgress;
    g_SeriesSelection = series;
    if (progress->maxClassReached == -1) {
        g_GrandPrixClass = 0;
        g_CourseIndex = 3;
        RequestCourseTextureAssets();
    } else {
        RequestSelectBgmAssetsKeepAudioSlots();
    }
}

void UpdateMainMenuInput(void) {
    Frontend *frontend = MenuFrontend();
    s32 oldSelection;
    s32 direction = 0;
    u16 pressed = g_PadPressed;

    int nativeInput = PortMainMenuTakeInput(&frontend->selection);
    if (nativeInput & 2) pressed |= PAD_CONFIRM;
    if (pressed != 0 || nativeInput) frontend->idleTimer = 0;
    oldSelection = frontend->selection;
    if (pressed & PAD_UP) {
        direction = -1;
    } else if (pressed & PAD_DOWN) {
        direction = 1;
    }

    frontend->selection = MoveTitleMenuSelection(
        oldSelection, direction, g_ExtraGrandPrixUnlocked != 0);
    if (oldSelection != frontend->selection) PlaySoundCue(1);

    if (pressed & PAD_CONFIRM) {
        PlaySoundCue(2);
        if (frontend->selection == TITLE_MENU_QUIT) {
            PortMainMenuRequestQuit();
            return;
        }
        if (!AssetLoadCompletedSuccessfully()) ResetAssetLoader();
        ShuffleBgmOrder();
        g_RaceSession.kind = RACE_SESSION_STANDARD;
        g_DuelEnabled = g_DuelConfigEnabled;
        switch (frontend->selection) {
        case TITLE_MENU_GRAND_PRIX:
            SelectGrandPrixSave(g_GrandPrixCars, &g_GrandPrixSave,
                                &g_GrandPrixCourseProgress, 0);
            break;
        case TITLE_MENU_EXTRA_GRAND_PRIX:
            SelectGrandPrixSave(g_ExtraGrandPrixCars, &g_ExtraGrandPrixSave,
                                &g_ExtraGrandPrixCourseProgress, 1);
            break;
        case TITLE_MENU_TIME_ATTACK:
            g_DuelEnabled = 0;
            g_CarTable = g_TimeAttackCars;
            g_RaceProgress = &g_TimeAttackSave;
            g_SeriesSelection = 0;
            RequestSelectBgmAssetsKeepAudioSlots();
            break;
        case TITLE_MENU_CUSTOM:
        case TITLE_MENU_DUEL:
            g_RaceSession.kind = RACE_SESSION_CUSTOM;
            g_RaceSession.course = 0;
            g_RaceSession.classIndex = 0;
            g_RaceSession.model = 3;
            memcpy(g_RaceSession.cars, g_TimeAttackCars,
                   sizeof(g_RaceSession.cars));
            for (s32 car = 0; car < GAME_CAR_COUNT; ++car) {
                g_RaceSession.cars[car].enabled = 1;
            }
            g_CarTable = g_RaceSession.cars;
            g_RaceProgress = &g_TimeAttackSave;
            g_CourseProgress = NULL;
            g_SeriesSelection = 0;
            if (frontend->selection == TITLE_MENU_DUEL) {
                g_DuelEnabled = 1;
            }
            RequestSelectBgmAssetsKeepAudioSlots();
            break;
        case TITLE_MENU_LOAD_SAVE:
            RequestSaveScreenAssets();
            break;
        case TITLE_MENU_OPTIONS:
            RequestOptionScreenAssets();
            MenuOption()->cursor = 0;
            break;
        case TITLE_MENU_QUIT:
            break;
        }
        frontend->pulse = 0;
        frontend->state = FRONTEND_STATE_MENU_EXIT;
    }
    DrawMainMenuRows();
}
