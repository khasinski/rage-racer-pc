#include "game/asset.h"
#include "game/audio.h"
#include "game/frontend_internal.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/render_internal.h"
#include "game/save_internal.h"
#include "game/screens.h"

void DrawMainMenuRows(void) {
    GameOrderingTableEntry *ot = GamePrimaryOrderingTable(0);
    u8 *packet = RENDER_PRIM_CURSOR_AS(u8);
    s32 row = 0;
    s32 item = 0;

    while (item < TITLE_MENU_ITEM_COUNT) {
        s32 clut = 0x7E85;
        s32 height;
        s32 delta;

        if (g_ExtraGrandPrixUnlocked == 0 &&
            item == TITLE_MENU_EXTRA_GRAND_PRIX) {
            item = TITLE_MENU_TIME_ATTACK;
        }
        if (item == g_TitleMenuSelection &&
            g_FrontendState != FRONTEND_STATE_MENU_OPENING &&
            (g_TitlePulse & 2) == 0) {
            clut = 0x7E86;
        }

        delta = g_MainMenuSlide - row * 8;
        height = delta > 0x10 ? 0x10 : delta;
        if (height < 0) height = 0;

        packet = GameQueueTexturedRect(
            ot, packet, 0x68, 0x64 + row * 0x18, 0x70, height, 0,
            item * 0x10 + 0xA0, 0x70, 0x10, clut, 0x39);
        item++;
        row++;
    }

    g_RenderState.packetCursor = packet;
}

void UpdateMainMenuOpen(void) {
    if (++g_MainMenuSlide == 0x30) {
        g_FrontendState = FRONTEND_STATE_MENU_INPUT;
    }
    DrawMainMenuRows();
}

void UpdateMainMenuInput(void) {
    s32 oldSelection;
    s32 direction = 0;
    u16 pressed = g_PadPressed;

    if (pressed != 0) g_FrontendIdleTimer = 0;
    oldSelection = g_TitleMenuSelection;
    if (pressed & PAD_UP) {
        direction = -1;
    } else if (pressed & PAD_DOWN) {
        direction = 1;
    }

    g_TitleMenuSelection = MoveTitleMenuSelection(
        oldSelection, direction, g_ExtraGrandPrixUnlocked != 0);
    if (oldSelection != g_TitleMenuSelection) PlaySoundCue(1);

    if (pressed & PAD_CONFIRM) {
        PlaySoundCue(2);
        if (!AssetLoadCompletedSuccessfully()) ResetAssetLoader();
        ShuffleBgmOrder();
        switch (g_TitleMenuSelection) {
        case TITLE_MENU_GRAND_PRIX:
            g_CarTable = g_GrandPrixCars;
            g_RaceProgress = &g_GrandPrixSave;
            g_CourseProgress = &g_GrandPrixCourseProgress;
            g_SeriesSelection = 0;
            if (g_GrandPrixSave.maxClassReached == -1) {
                g_GrandPrixClass = 0;
                g_CourseIndex = 3;
                RequestCourseTextureAssets();
            } else {
                RequestSelectBgmAssetsKeepAudioSlots();
            }
            break;
        case TITLE_MENU_EXTRA_GRAND_PRIX:
            g_CarTable = g_ExtraGrandPrixCars;
            g_RaceProgress = &g_ExtraGrandPrixSave;
            g_CourseProgress = &g_ExtraGrandPrixCourseProgress;
            g_SeriesSelection = 1;
            if (g_ExtraGrandPrixSaveMaxClass == -1) {
                g_GrandPrixClass = 0;
                g_CourseIndex = 3;
                RequestCourseTextureAssets();
            } else {
                RequestSelectBgmAssetsKeepAudioSlots();
            }
            break;
        case TITLE_MENU_TIME_ATTACK:
            g_CarTable = g_TimeAttackCars;
            g_RaceProgress = &g_TimeAttackSave;
            g_SeriesSelection = 0;
            RequestSelectBgmAssetsKeepAudioSlots();
            break;
        case TITLE_MENU_LOAD_SAVE:
            RequestSaveScreenAssets();
            break;
        case TITLE_MENU_OPTIONS:
            RequestOptionScreenAssets();
            g_OptionMenuCursor = 0;
            break;
        }
        g_FrontendState = FRONTEND_STATE_MENU_EXIT;
    }
    DrawMainMenuRows();
}
