#include "game/prim.h"
#include "game/asset.h"
#include "game/audio.h"
#include "game/cd.h"
#include "game/frontend_internal.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/render_internal.h"
#include "game/fmv_internal.h"
#include "game/screens.h"

enum {
    TITLE_RETURN_FADE = 255,
    TITLE_RETURN_ATTRACT_FRAMES = 400,
    TITLE_INITIAL_EXIT_FRAMES = 30,
    TITLE_TIMER_ANGLE_MASK = 0xFE0,
};

void EnterTitleScreen(void) {
    SetupDisplay240(0, 0, 0);
    if (g_StreamReturnScene != 0) {
        g_TitleFadeLevel = TITLE_RETURN_FADE;
        g_TitleAttractTimer = TITLE_RETURN_ATTRACT_FRAMES;
        g_TitleExitTimer = 0;
    } else {
        SetDispMask(0);
        UploadLoadBufferImage();
        g_TitleFadeLevel = 0;
        g_TitleAttractTimer = 0;
        g_TitleExitTimer = TITLE_INITIAL_EXIT_FRAMES;
    }
    g_FrameSyncThreshold = 0x80;
    g_SceneTimer = 0;
    g_SceneId = GAME_SCENE_FRONTEND;
    g_FrontendIdleTimer = 0;
    g_MainMenuSlide = 0;
    g_FrontendState = FRONTEND_STATE_TITLE;
    RefreshClassWinState();
    SetDefaultReverbDepth();
    DrawPressStartPrompt();
}

static void DrawTitleFadeOverlay(s32 brightness) {
    GameOrderingTableEntry *ot = GamePrimaryOrderingTable(0);
    u8 *next;
    s32 color;

    color = (u8)brightness;
    next = GameQueueTileTrans(ot, RENDER_PRIM_CURSOR_AS(u8), 0, 0x18,
                              0x140, 0xC0, color, color, color);
    g_RenderState.packetCursor = QueueDrawModePrim(ot, next, 0x29);
}


void DrawPressStartPrompt(void) {
    GameOrderingTableEntry *ot = GamePrimaryOrderingTable(0);
    u8 *next;
    s32 sinValue;
    s32 frame;

    if (g_TitleFadeLevel > 0) {
        DrawTitleFadeOverlay((u8)g_TitleFadeLevel);
        g_TitleFadeLevel -= 2;
        if (g_TitleFadeLevel < 0) g_TitleFadeLevel = 0;
    }

    sinValue = rsin((s32)(((u32)g_AnimTimer * 96U) &
                          TITLE_TIMER_ANGLE_MASK));
    frame = (sinValue / 64) + 0x80;

    next = GameQueueShadedSprite(ot, RENDER_PRIM_CURSOR_AS(u8), 0x68, 0xC8,
                                 0x70, 0x10, 0x70, 0xA0, 0x7E84, frame);
    g_RenderState.packetCursor = QueueDrawModePrim(ot, next, 0x39);
}


void UpdateTitleScreen(void) {
    if (g_PadPressed & PAD_START) {
        PlaySoundCue(2);
        g_FrontendState = FRONTEND_STATE_MENU_OPENING;
        g_FrontendIdleTimer = 0;
        g_TitleMenuSelection = TITLE_MENU_GRAND_PRIX;
        if (g_TitleAttractTimer > 0) {
            g_TitleAttractTimer = 0;
            StartCdVolumeFade(1);
        }
    }
    DrawPressStartPrompt();
}
