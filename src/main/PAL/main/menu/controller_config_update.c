#include "game/audio.h"
#include "game/menu.h"
#include "game/render.h"
#include "game/input_internal.h"
#include "game/state.h"

enum {
    CONTROLLER_MAPPING_FIRST = 0,
    CONTROLLER_MAPPING_LAST = 7,
    CONTROLLER_FLIP_FRAMES = 30,
    CONTROLLER_HALF_TURN = 2048,
};

static ControllerMappingIndex *SelectedControllerMapping(void) {
    return g_PadType == PAD_TYPE_NEGCON ? &g_NegconMappingIndex
                                        : &g_PadMappingIndex;
}

static void BeginControllerFlip(s32 direction) {
    g_PadConfigFlipDirection = direction;
    g_PadConfigFlipTimer = CONTROLLER_FLIP_FRAMES;
}

static void UpdateControllerMappingSelection(u16 pressed) {
    ControllerMappingIndex *selection = SelectedControllerMapping();

    if ((pressed & PAD_LEFT) && *selection > CONTROLLER_MAPPING_FIRST) {
        PlaySoundCue(8);
        BeginControllerFlip(0);
        (*selection)--;
        g_ControllerSceneAngleY += CONTROLLER_HALF_TURN;
    }
    if ((pressed & PAD_RIGHT) && *selection < CONTROLLER_MAPPING_LAST) {
        PlaySoundCue(8);
        BeginControllerFlip(1);
        (*selection)++;
        g_ControllerSceneAngleY -= CONTROLLER_HALF_TURN;
    }
}

void UpdateControllerConfigScreen(void) {
    u16 pressed = g_PadPressed;

    g_AnimTimer++;
    g_SetupArrowPulse += 96;
    if (pressed & PAD_CANCEL) {
        PlaySoundCue(3);
        g_GameMode = 1;
        g_PadMappingIndex = g_PadMappingIndexSaved;
        g_NegconMappingIndex = g_NegconMappingIndexSaved;
    }
    if (pressed & PAD_CONFIRM) {
        PlaySoundCue(2);
        LoadPadButtonMapping(g_PadMappingIndex, g_NegconMappingIndex);
        if (g_PadType == PAD_TYPE_NEGCON && (pressed & PAD_START)) {
            g_GameMode = 8;
        } else {
            g_GameMode = 1;
        }
    }
    UpdateControllerMappingSelection(pressed);
    if (g_PadConfigFlipTimer > 0) {
        g_PadConfigFlipTimer--;
        g_PadConfigFlipPhase = ((u32)g_PadConfigFlipTimer / 4) & 1;
    }
    g_ControllerSceneAngleY = (g_ControllerSceneAngleY * 15) / 16;
    DrawControllerConfigScreen();
    DrawOptionHintBar(1);
    DrawControllerSetupScene(0);
}
