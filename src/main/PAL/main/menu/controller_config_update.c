#include "game/audio.h"
#include "game/menu.h"
#include "game/render.h"
#include "game/input_internal.h"
#include "game/state.h"

enum { CONTROLLER_HALF_TURN = 2048 };

static ControllerMappingIndex *SelectedControllerMapping(void) {
    return g_PadType == PAD_TYPE_NEGCON ? &g_NegconMappingIndex
                                        : &g_PadMappingIndex;
}

static void UpdateControllerMappingSelection(ControllerSetup *setup,
                                             u16 pressed) {
    ControllerMappingIndex *selection = SelectedControllerMapping();

    if ((pressed & PAD_LEFT) && *selection > CONTROLLER_MAPPING_FIRST) {
        PlaySoundCue(8);
        (*selection)--;
        setup->angleY = (s32)((u32)setup->angleY + CONTROLLER_HALF_TURN);
    }
    if ((pressed & PAD_RIGHT) && *selection < CONTROLLER_MAPPING_LAST) {
        PlaySoundCue(8);
        (*selection)++;
        setup->angleY = (s32)((u32)setup->angleY - CONTROLLER_HALF_TURN);
    }
}

void UpdateControllerConfigScreen(void) {
    ControllerSetup *setup = MenuControllerSetup();
    u16 pressed = g_PadPressed;

    g_AnimTimer = (s32)((u32)g_AnimTimer + 1u);
    setup->arrowPhase = (s32)((u32)setup->arrowPhase + 96u);
    if (pressed & PAD_CANCEL) {
        PlaySoundCue(3);
        g_GameMode = OPTION_MODE_ROOT;
        g_PadMappingIndex = setup->savedPadMapping;
        g_NegconMappingIndex = setup->savedNegconMapping;
    } else if (pressed & PAD_CONFIRM) {
        PlaySoundCue(2);
        LoadPadButtonMapping(g_PadMappingIndex, g_NegconMappingIndex);
        if (g_PadType == PAD_TYPE_NEGCON && (pressed & PAD_START)) {
            g_GameMode = OPTION_MODE_NEGCON_BEGIN;
        } else {
            g_GameMode = OPTION_MODE_ROOT;
        }
    } else {
        UpdateControllerMappingSelection(setup, pressed);
    }
    setup->angleY = (s32)(((int64_t)setup->angleY * 15) / 16);
    DrawControllerConfigScreen(setup);
    DrawOptionHintBar(MENU_OPTION_HINT_CONTROLLER);
    DrawControllerSetupScene(setup, 0);
}
