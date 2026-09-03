#include "game/asset.h"
#include "game/audio.h"
#include "game/input_internal.h"
#include "game/menu.h"
#include "game/menu_internal.h"
#include "game/prim.h"
#include "game/race.h"
#include "game/random.h"
#include "game/render_internal.h"

typedef enum OptionRootItem {
    OPTION_ROOT_TROPHIES,
    OPTION_ROOT_CONTROLLER,
    OPTION_ROOT_SOUND,
    OPTION_ROOT_RANDOM_RACE,
    OPTION_ROOT_SCREEN,
    OPTION_ROOT_EXIT,
    OPTION_ROOT_ITEM_COUNT,
} OptionRootItem;

typedef struct OptionRootLabel {
    u8 width;
    u8 textureU;
    u8 textureV;
} OptionRootLabel;

static const OptionRootLabel s_optionRootLabels[OPTION_ROOT_ITEM_COUNT] = {
    {0x3C, 0x00, 0x48}, {0x88, 0x40, 0x48}, {0x74, 0x00, 0x60},
    {0x5C, 0x74, 0x60}, {0x64, 0x00, 0x78}, {0x1C, 0xD0, 0x60},
};

void DrawOptionRootMenu(void) {
    GameOrderingTableEntry *ot = GamePrimaryOrderingTable(51);
    u8 *next = RENDER_PRIM_CURSOR_AS(u8);
    s32 row;

    g_OptionMenuCursor = AddClampedMenuValue(
        g_OptionMenuCursor, 0, 0, OPTION_ROOT_ITEM_COUNT - 1);
    for (row = 0; row < OPTION_ROOT_ITEM_COUNT; row++) {
        const OptionRootLabel *label = &s_optionRootLabels[row];

        next = GameQueueSpriteTrans(ot, next, 0x24, 0x94 + row * 0x20,
                                    label->width, 0x18, label->textureU,
                                    label->textureV, 0x7F40);
    }
    g_RenderState.packetCursor = QueueDrawModePrim(ot, next, 0x3F);

    if (g_GameMode == OPTION_MODE_ROOT) {
        DrawMenuCursorArrow(0x14, g_OptionMenuCursor * 0x20 + 0x94);
    }
}

static void StartRandomOptionRace(void) {
    g_GrandPrixMode = 0;
    g_GrandPrixSeries = 0;
    g_GrandPrixClass = (Random15() & 0xFFF) % 5;
    g_CourseIndex = (Random15() & 0xFFF) % 4;
    if (g_GrandPrixClass < 2 && g_CourseIndex == 3) {
        g_CourseIndex = (Random15() & 0xFFF) % 3;
    }
    RequestCourseTextureAssets();
    StartOptionMenuExit(0x1B);
}

/* OPTION_MODE_ROOT: the six-row root menu and where each row goes. */
void UpdateOptionRootMenu(void) {
    s32 oldCursor;

    DrawOptionRootMenu();
    oldCursor = g_OptionMenuCursor;
    if (g_PadPressed & PAD_UP) {
        g_OptionMenuCursor = WrapMenuIndex(
            g_OptionMenuCursor, -1, OPTION_ROOT_ITEM_COUNT);
    } else if (g_PadPressed & PAD_DOWN) {
        g_OptionMenuCursor = WrapMenuIndex(
            g_OptionMenuCursor, 1, OPTION_ROOT_ITEM_COUNT);
    }
    if (oldCursor != g_OptionMenuCursor) {
        PlaySoundCue(1);
    }

    if (g_PadPressed & PAD_CONFIRM) {
        PlaySoundCue(2);
        switch (g_OptionMenuCursor) {
        case OPTION_ROOT_TROPHIES:
            g_GameMode = OPTION_MODE_CLASS_MENU;
            g_ClassRecordMenuCursor = 0;
            g_ScreenOffsetEditX = 0;
            g_ScreenOffsetEditY = 0;
            break;
        case OPTION_ROOT_CONTROLLER:
            BeginControllerConfig();
            g_GameMode = OPTION_MODE_CONTROLLER_CONFIG;
            break;
        case OPTION_ROOT_SOUND:
            g_GameMode = OPTION_MODE_SOUND_MENU;
            g_SoundOptionCursor = 0;
            break;
        case OPTION_ROOT_RANDOM_RACE:
            StartRandomOptionRace();
            break;
        case OPTION_ROOT_SCREEN:
            g_GameMode = OPTION_MODE_SCREEN_ADJUST;
            g_ScreenOffsetEditX = g_ScreenOffsetX.value;
            g_ScreenOffsetEditY = g_ScreenOffsetY.value;
            break;
        case OPTION_ROOT_EXIT:
            StartOptionMenuExit(2);
            break;
        }
    } else if (g_PadPressed & PAD_CANCEL) {
        PlaySoundCue(3);
        StartOptionMenuExit(2);
    }
}
