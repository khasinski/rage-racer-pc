#include "game/asset.h"
#include "game/audio.h"
#include "game/menu.h"
#include "game/menu_internal.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/scene.h"

#include <stdio.h>

enum {
    CUSTOM_ROW_COURSE,
    CUSTOM_ROW_CLASS,
    CUSTOM_ROW_CAR,
    CUSTOM_ROW_START,
    CUSTOM_ROW_COUNT,
};

static const char *CustomCourseName(s32 selection) {
    const char *name = g_CourseNames[selection % COURSE_SLOT_COUNT];
    return name != NULL ? name : "COURSE";
}

static void DrawCustomRows(void) {
    char text[64];
    s32 row = MenuRuntimeScreenState(MENU_SCREEN_CUSTOM_RACE);
    s32 model = g_RaceSession.model;
    s32 rival = model - GAME_CAR_COUNT;
    DrawSolidRect(GamePrimaryOrderingTable(0), 0x34, 0x58, 0xD8, 0x90,
                  8, 8, 16, 0xFF);
    DrawText8x8(0x48, 0x64, "CUSTOM RACE", 0x78CC);
    snprintf(text, sizeof(text), "COURSE  %s%s",
             CustomCourseName(g_RaceSession.course),
             g_RaceSession.course >= COURSE_SLOT_COUNT ? " REVERSE" : "");
    DrawText8x8(0x48, 0x84, text, row == CUSTOM_ROW_COURSE ? 0x780F : 0x78CC);
    snprintf(text, sizeof(text), "CLASS   %d", g_RaceSession.classIndex + 1);
    DrawText8x8(0x48, 0xA0, text, row == CUSTOM_ROW_CLASS ? 0x780F : 0x78CC);
    if (model < GAME_CAR_COUNT) {
        snprintf(text, sizeof(text), "CAR     %s", g_CarNames[model]);
    } else {
        snprintf(text, sizeof(text), "CAR     RIVAL %02d", rival + 1);
    }
    DrawText8x8(0x48, 0xBC, text, row == CUSTOM_ROW_CAR ? 0x780F : 0x78CC);
    DrawText8x8(0x48, 0xD8, "START", row == CUSTOM_ROW_START ? 0x780F : 0x78CC);
}

s32 DrawCustomRaceScreen(s32 *progress, s32 step) {
    if (progress == NULL) return 0;
    return AdvanceMenuFade(progress, step);
}

static void MoveCustomValue(s32 delta) {
    s32 row = MenuRuntimeScreenState(MENU_SCREEN_CUSTOM_RACE);
    if (row == CUSTOM_ROW_COURSE) {
        g_RaceSession.course = WrapMenuIndex(
            g_RaceSession.course, delta, CUSTOM_RACE_COURSE_COUNT);
    } else if (row == CUSTOM_ROW_CLASS) {
        g_RaceSession.classIndex = WrapMenuIndex(
            g_RaceSession.classIndex, delta, GRAND_PRIX_FINAL_CLASS_INDEX + 1);
    } else if (row == CUSTOM_ROW_CAR) {
        g_RaceSession.model = WrapMenuIndex(
            g_RaceSession.model, delta, CUSTOM_RACE_MODEL_COUNT);
    }
}

void UpdateCustomRaceScreen(void) {
    s32 state = MenuRuntimeScreenState(MENU_SCREEN_CUSTOM_RACE);
    s32 row;

    if (state >= CUSTOM_ROW_COUNT) {
        if (state == CUSTOM_ROW_COUNT) {
            if (AssetLoadCompletedSuccessfully()) {
                RequestRoundAssets();
                MenuRuntimeSetScreenState(MENU_SCREEN_CUSTOM_RACE,
                                          CUSTOM_ROW_COUNT + 1);
            }
        } else if (AssetLoadCompletedSuccessfully()) {
            g_SceneId = GAME_SCENE_ENTER_ROUND;
        }
        DrawCustomRows();
        return;
    }

    row = state;
    if (g_PadPressed & PAD_UP) {
        row = WrapMenuIndex(row, -1, CUSTOM_ROW_COUNT);
        PlaySoundCue(1);
    } else if (g_PadPressed & PAD_DOWN) {
        row = WrapMenuIndex(row, 1, CUSTOM_ROW_COUNT);
        PlaySoundCue(1);
    } else if (g_PadPressed & PAD_LEFT) {
        MoveCustomValue(-1);
        PlaySoundCue(1);
    } else if (g_PadPressed & PAD_RIGHT) {
        MoveCustomValue(1);
        PlaySoundCue(1);
    } else if ((g_PadPressed & PAD_CONFIRM) && row == CUSTOM_ROW_START) {
        ApplyCustomRaceSelection();
        if (RequestCarModel(g_PlayerCarIndex)) {
            MenuRuntimeSetScreenState(MENU_SCREEN_CUSTOM_RACE,
                                      CUSTOM_ROW_COUNT);
            PlaySoundCue(2);
        }
    } else if (g_PadPressed & PAD_CANCEL) {
        g_RaceSession.kind = RACE_SESSION_STANDARD;
        g_SceneId = GAME_SCENE_ENTER_FRONTEND;
        PlaySoundCue(3);
    }
    if (row != state) {
        MenuRuntimeSetScreenState(MENU_SCREEN_CUSTOM_RACE, row);
    }
    DrawCustomRows();
}
