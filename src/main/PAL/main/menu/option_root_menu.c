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
    OPTION_ROOT_EXIT,
    OPTION_ROOT_ITEM_COUNT,
} OptionRootItem;

enum {
    OPTION_RACE_CLASS_COUNT = GRAND_PRIX_FINAL_CLASS_INDEX,
    OPTION_RACE_OVAL_MINIMUM_CLASS = 2,
};

typedef struct OptionRootLabel {
    u8 width;
    u8 textureU;
    u8 textureV;
} OptionRootLabel;

static const OptionRootLabel s_optionRootLabels[OPTION_ROOT_ITEM_COUNT] = {
    {0x3C, 0x00, 0x48}, {0x88, 0x40, 0x48}, {0x74, 0x00, 0x60},
    {0x5C, 0x74, 0x60}, {0x1C, 0xD0, 0x60},
};

void DrawOptionRootMenu(void) {
    OptionMenu *menu = MenuOption();
    GameOrderingTableEntry *ot = GamePrimaryOrderingTable(51);
    u8 *next = RENDER_PRIM_CURSOR_AS(u8);
    s32 row;

    menu->cursor = AddClampedMenuValue(
        menu->cursor, 0, 0, OPTION_ROOT_ITEM_COUNT - 1);
    for (row = 0; row < OPTION_ROOT_ITEM_COUNT; row++) {
        const OptionRootLabel *label = &s_optionRootLabels[row];

        next = GameQueueSpriteTrans(ot, next, 0x24, 0x94 + row * 0x20,
                                    label->width, 0x18, label->textureU,
                                    label->textureV, 0x7F40);
    }
    g_RenderState.draw.packetCursor = QueueDrawModePrim(ot, next, 0x3F);

    if (g_GameMode == OPTION_MODE_ROOT) {
        DrawMenuCursorArrow(0x14, menu->cursor * 0x20 + 0x94);
    }
}

static void StartRandomOptionRace(void) {
    g_GrandPrixMode = 0;
    g_GrandPrixSeries = 0;
    g_GrandPrixClass = RandomIndex(OPTION_RACE_CLASS_COUNT);
    g_CourseIndex = RandomIndex(COURSE_SLOT_COUNT);
    if (g_GrandPrixClass < OPTION_RACE_OVAL_MINIMUM_CLASS &&
        g_CourseIndex == COURSE_LONG_SLOT) {
        g_CourseIndex = RandomIndex(COURSE_SLOT_COUNT - 1);
    }
    RequestCourseTextureAssets();
    StartOptionMenuExit(GAME_SCENE_ENTER_BGM_SELECT);
}

/* OPTION_MODE_ROOT: the root menu and where each row goes. */
void UpdateOptionRootMenu(void) {
    OptionMenu *menu = MenuOption();
    s32 oldCursor;

    DrawOptionRootMenu();
    oldCursor = menu->cursor;
    if (g_PadPressed & PAD_UP) {
        menu->cursor = WrapMenuIndex(menu->cursor, -1, OPTION_ROOT_ITEM_COUNT);
    } else if (g_PadPressed & PAD_DOWN) {
        menu->cursor = WrapMenuIndex(menu->cursor, 1, OPTION_ROOT_ITEM_COUNT);
    }
    if (oldCursor != menu->cursor) {
        PlaySoundCue(1);
    }

    if (g_PadPressed & PAD_CONFIRM) {
        PlaySoundCue(2);
        switch (menu->cursor) {
        case OPTION_ROOT_TROPHIES:
            g_GameMode = OPTION_MODE_CLASS_MENU;
            menu->classRecordCursor = 0;
            menu->classRecordColumn = 0;
            menu->classRecordRow = 0;
            break;
        case OPTION_ROOT_CONTROLLER:
            BeginControllerConfig(MenuControllerSetup());
            g_GameMode = OPTION_MODE_CONTROLLER_CONFIG;
            break;
        case OPTION_ROOT_SOUND:
            EnterSoundOptionMenu();
            g_GameMode = OPTION_MODE_SOUND_MENU;
            break;
        case OPTION_ROOT_RANDOM_RACE:
            StartRandomOptionRace();
            break;
        case OPTION_ROOT_EXIT:
            StartOptionMenuExit(GAME_SCENE_ENTER_FRONTEND);
            break;
        }
    } else if (g_PadPressed & PAD_CANCEL) {
        PlaySoundCue(3);
        StartOptionMenuExit(GAME_SCENE_ENTER_FRONTEND);
    }
}
