#include "game/audio.h"
#include "game/menu.h"
#include "game/prim.h"
#include "game/render_internal.h"

enum {
    SOUND_OPTION_COUNT = 4,
    SOUND_OPTION_BGM = 0,
    SOUND_OPTION_SFX = 1,
    SOUND_OPTION_OUTPUT = 2,
    SOUND_OPTION_EXIT = 3,
    OPTION_MODE_ROOT = 1,
    OPTION_MODE_SOUND_EDIT = 5,
};

static void DrawOutputModeChoice(OT_TYPE *ot, u8 **next, s32 selected,
                                 s32 x, s32 width, s32 textureU,
                                 s32 textureV) {
    s32 intensity = selected ? 0x7F : 0x20;

    *next = GameQueueShadedSpriteTrans(ot, *next, x + 0x20, 0x12A, width,
                                       0xC, textureU, textureV, 0x7F40,
                                       intensity);
    *next = AddTilePrim(ot, *next, x + 1, 0x122, 0x56, 0x1C, 0x85, 0x15,
                        0xE);
    *next = AddTilePrim(ot, *next, x, 0x120, 0x58, 0x20, intensity * 2,
                        intensity * 2, intensity * 2);
}

void DrawSoundOptionScreen(void) {
    OT_TYPE *ot = GamePrimaryOrderingTable(0);
    u8 *next = RENDER_PRIM_CURSOR_AS(u8);

    DrawMenuCursorArrow(0x14, g_SoundOptionCursor * 32 + 56);
    next = GameQueueSpriteTrans(ot, next, 0x24, 0x38, 0x2C, 0x18, 0x9C,
                                0x78, 0x7F40);
    next = GameQueueSpriteTrans(ot, next, 0x24, 0x58, 0x18, 0x18, 0xC8,
                                0x78, 0x7F40);
    next = GameQueueSpriteTrans(ot, next, 0x24, 0x78, 0x38, 0x18, 0, 0x90,
                                0x7F40);
    next = GameQueueSpriteTrans(ot, next, 0x24, 0x98, 0x1C, 0x18, 0xD0,
                                0x60, 0x7F40);
    RENDER_PRIM_CURSOR_AS(u8) = next;

    DrawOptionHintBar(2);
    next = RENDER_PRIM_CURSOR_AS(u8);
    DrawOutputModeChoice(ot, &next, g_MonoOutput == 0, 0x46, 0x18, 0xD4,
                         0xC4);
    DrawOutputModeChoice(ot, &next, g_MonoOutput != 0, 0xA2, 0x28, 0xB4,
                         0xD0);
    RENDER_PRIM_CURSOR_AS(u8) = next;

    DrawVolumeBar(g_BgmVolumeSetting, 0xD0);
    DrawVolumeBar(g_SfxVolumeSetting, 0xF8);

    if (g_GameMode != OPTION_MODE_SOUND_EDIT) {
        return;
    }

    next = RENDER_PRIM_CURSOR_AS(u8);
    switch (g_SoundOptionCursor) {
    case SOUND_OPTION_BGM:
        next = AddTilePrim(ot, next, 0x44, 0xCC, 0xB8, 0x28, 0x89, 0xFF,
                           0x76);
        break;
    case SOUND_OPTION_SFX:
        next = AddTilePrim(ot, next, 0x44, 0xF4, 0xB8, 0x28, 0x89, 0xFF,
                           0x76);
        break;
    case SOUND_OPTION_OUTPUT:
        next = AddTilePrim(ot, next, g_MonoOutput ? 0xA0 : 0x44, 0x11C,
                           0x5C, 0x28, 0x89, 0xFF, 0x76);
        break;
    }
    RENDER_PRIM_CURSOR_AS(u8) = next;
}

/* g_GameModeHandlers[4]: choose a setting, then mode 5 edits it. */
void UpdateSoundOptionMenu(void) {
    s32 oldCursor;

    DrawSoundOptionScreen();
    oldCursor = g_SoundOptionCursor;
    if (g_PadPressed & PAD_UP) {
        g_SoundOptionCursor--;
    }
    if (g_PadPressed & PAD_DOWN) {
        g_SoundOptionCursor++;
    }
    g_SoundOptionCursor =
        (g_SoundOptionCursor + SOUND_OPTION_COUNT) % SOUND_OPTION_COUNT;
    if (oldCursor != g_SoundOptionCursor) {
        PlaySoundCue(1);
    }

    if (g_PadPressed & PAD_CONFIRM) {
        PlaySoundCue(2);
        g_GameMode = OPTION_MODE_SOUND_EDIT;
        switch (g_SoundOptionCursor) {
        case SOUND_OPTION_BGM:
            g_ScreenOffsetEditX = g_BgmVolumeSetting;
            break;
        case SOUND_OPTION_SFX:
            g_ScreenOffsetEditX = g_SfxVolumeSetting;
            break;
        case SOUND_OPTION_OUTPUT:
            g_ScreenOffsetEditX = g_MonoOutput;
            break;
        case SOUND_OPTION_EXIT:
            g_GameMode = OPTION_MODE_ROOT;
            break;
        }
    } else if (g_PadPressed & PAD_CANCEL) {
        PlaySoundCue(3);
        g_GameMode = OPTION_MODE_ROOT;
    }
}
