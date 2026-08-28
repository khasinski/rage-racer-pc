#include "common.h"
#include "game/prim.h"
#include "game/asset.h"
#include "game/audio.h"
#include "game/menu.h"
#include "game/menu_internal.h"
#include "game/frontend_internal.h"
#include "game/race.h"
#include "game/random.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/scratchpad.h"
#include "game/screens.h"
#include "game/state.h"
#include "psyq/cd.h"
#include "psyq/gpu.h"

void UpdateMainMenuExit(void) {
    s32 value;
    GameRaceProgress *ptr;

    value = g_TitlePulse + 1;
    g_TitlePulse = value;
    DrawFullscreenFadeTile(value * 2, 0x59);

    if (g_TitlePulse >= 0x81) {
        switch (g_TitleMenuSelection) {
        case 0:
        case 1:
            ptr = g_RaceProgress;
            g_GrandPrixMode = 1;
            if (ptr->maxClassReached == -1) {
                ptr->maxClassReached = 0;
                g_SceneId = 0x1F;
                g_GrandPrixSeries = 0;
            } else {
                g_SceneId = 6;
            }
            break;
        case 2:
            g_GrandPrixMode = 0;
            g_SceneId = 6;
            break;
        case 3:
            g_SceneId = 0x19;
            break;
        case 4:
            g_SceneId = 0x16;
            break;
        }
    }

    DrawMainMenuRows();
}


void UpdateTitleAttract(void) {
    s32 alpha;
    void *base;
    s32 color;
    s32 h88;
    RenderBufferAddress scratch;
    s32 hF0;
    s32 clut0;
    void *next;
    s32 tmp;
    s32 clamped;
    s32 x28;
    s32 yA0;

    tmp = g_MainMenuSlide;
    tmp <<= 1;
    clamped = 0x7F;
    alpha = clamped - tmp;
    if (alpha >= 0x30) {
        clamped = alpha;
        if (clamped >= 0x80) {
            clamped = 0x7F;
        }
    } else {
        clamped = 0x30;
    }
    alpha = clamped;

    x28 = 0x28;
    yA0 = 0xA0;
     /* Match note: materialize first-call argument registers before the stack-arg temp. */
    color = 0x7E00;
    scratch.pointerLink = &SCRATCH_PRIM_CURSOR_AS(void);
    hF0 = 0xF0;
     /* Match note: keep scratchpad base and 0xf0 materialized before the first stack-arg temp. */
    tmp = 0x18;
    next = *scratch.pointerLink;
    h88 = 0x88;
    clut0 = 0x7DC0;

    next = GameQueueShadedSprite(base = (u8 *)GamePrimaryOrderingTable(1), next, x28, yA0, hF0, tmp, 0, h88, clut0, alpha);
    next = GameQueueShadedSprite(base, next, 0x20, 0xB8, 0x100, 0x10, 0, hF0, 0x7DC1, alpha);
    next = GameQueueShadedSprite(base, next, 0x11A, 0xAF, 0xC, 8, 0xE0, 0xB0, clut0, alpha);
    next = QueueDrawModePrim(base, next, 0x19);

    if (g_ClassWinCount >= 0xB) {
        color = 0x7D80;
    }

    next = GameQueueShadedSprite(base, next, 0x34, 0x18, 0x6C, h88, 0, 0, color, alpha);
    *scratch.pointerLink = GameQueueShadedTexturedRect(base, next, 0xA0, 0x18, -0x6C, h88, 0, 0, color, 0x99, alpha);
}


void UpdateFrontend(void) {
    u32 state;
    s32 b, m4;

    g_AnimTimer++;
    Random15();

    if (g_TitleAttractTimer > 0) {
        g_TitleAttractTimer--;
    }
    if (g_TitleAttractTimer == 0) {
        if (CdControl(9, 0, 0) == 1) {
            g_TitleAttractTimer--;
        }
    }
    if (g_TitleExitTimer != 0) {
        if (--g_TitleExitTimer == 0) {
            PlaySoundCue(0x1a);
        }
    }

    state = g_SceneTimer;
    if (state < 0x1cc) {
        g_SceneTimer = state + 1;
    } else {
        if (!(g_FrontendState == FRONTEND_STATE_MENU_EXIT) &&
            !(g_AttractCycleCount % 2)) {
        if (state == 0x1cc) {
            g_GrandPrixSeries = 0;
            g_GrandPrixClass = (Random15() & 0xfff) % 5;
            b = Random15() & 0xfff;
            m4 = b % 4;
            g_CourseIndex = m4;
            if (g_GrandPrixClass < 2 && m4 == 3) {
                g_CourseIndex = (Random15() & 0xfff) % 3;
            }
            RequestTrackLoad();
            g_SceneTimer++;
        } else if (state == 0x1cd) {
            if (g_AssetLoadState == 0) {
                RequestRaceStart();
                g_SceneTimer++;
            }
        } else if (state == 0x1ce) {
            if (g_AssetLoadState == 0) {
                g_SceneTimer = 0x1cf;
            }
        }
        }
    }
    state = g_SceneTimer;
    if (state == 0xf) {
        SetDispMask(1);
        state = g_SceneTimer;
    }
    if (state == 1) {
        SetupDisplay240(0, 0, 0);
    }

    g_FrontendDrawHandlers[g_FrontendState]();

    if (g_FrontendIdleTimer < 900) {
        g_FrontendIdleTimer++;
    } else {
        if (g_AttractCycleCount % 2) {
            BeginIntroFmv(3);
            g_AttractCycleCount++;
        } else {
            if (g_SceneTimer == 0x1cf) {
                g_GrandPrixMode = 1;
                g_SceneId = 0x1d;
                g_AttractCycleCount++;
            }
        }
    }

    UpdateTitleAttract();
}

/*
 * Empty stub; SetupDisplay240 and SetupDisplay480 both call it with one argument,
 * so the parameter is declared and ignored.
 */
void ResetFrameContext(int buffer) {
    (void)buffer;
}


void SetupDisplay240(s32 r, s32 g, s32 b) {
    GameFrameContext *context;
    s32 height;
    u16 *src0;
    u16 *src1;
    s32 i;
    s32 offset;
    s32 one;
    s32 stride;
    u16 value;
    u16 value2;

    SetGeomOffset(0xA0, 0x78);
    SetGeomScreen(0x140);

    context = g_FrameContexts;
    height = 0xF0;
    SetDefDrawEnv(&context->environment.draw, 0, 0, 0x140, height);
    SetDefDrawEnv(&g_FrameContexts[1].environment.draw,
                  0, 0xF0, 0x140, height);
    SetDefDispEnv(&context->environment.display, 0, 0xF0, 0x140, height);
    SetDefDispEnv(&g_FrameContexts[1].environment.display, 0, 0, 0x140, height);

    {
        DrawEnv *ptr;
        s32 g;
        s32 b;
        s32 smallWidth;
        s32 small_height;

        ptr = &context->environment.mirrorDraw;
        g = 0x56;
        b = 0x12;
        smallWidth = 0x94;
        small_height = 0x24;
        SetDefDrawEnv(ptr, g, b, smallWidth, small_height);
        SetDefDrawEnv(&g_FrameContexts[1].environment.mirrorDraw,
                      0x56, 0x102, 0x94, small_height);
    }

    i = 0;
    one = 1;
    src0 = &g_ScreenOffsetX.displayValue;
    src1 = &g_ScreenOffsetY.displayValue;
    offset = 0;
    do {
        stride = 0x20000;
        g_FrameContexts[i].environment.draw.dtd = one;
        g_FrameContexts[i].environment.draw.isbg = one;
        g_FrameContexts[i].environment.draw.r0 = r;
        g_FrameContexts[i].environment.draw.g0 = g;
        g_FrameContexts[i].environment.draw.b0 = b;
        value = *src0;
        stride |= 0x37E8;
        g_FrameContexts[i].environment.display.screen.x = value;
        value2 = *src1;
        g_FrameContexts[i].environment.mirrorDraw.dtd = one;
        g_FrameContexts[i].environment.mirrorDraw.isbg = 0;
        g_FrameContexts[i].environment.mirrorDraw.r0 = r;
        g_FrameContexts[i].environment.mirrorDraw.g0 = g;
        g_FrameContexts[i].environment.mirrorDraw.b0 = b;
        g_FrameContexts[i].environment.display.screen.y = value2 + 0x1D;
        i++;
        offset += stride;
    } while (i < 2);

    ResetFrameContext(0);
    ResetFrameContext(1);

}

void SetupDisplay480(s32 mode, s32 x, s32 y) {
    GameFrameContext *context = g_FrameContexts;
    s32 height;
    u16 *src0;
    u16 *src1;
    s32 i;
    s32 offset;
    s32 one;
    s32 stride;
    u16 value;
    u16 value2;

    SetGeomOffset(0xA0, 0xF0);
    SetGeomScreen(0x140);

    height = 0x1E0;
    SetDefDrawEnv(&context->environment.draw, 0, 0, 0x140, height);
    SetDefDrawEnv(&g_FrameContexts[1].environment.draw,
                  0, 0, 0x140, height);
    SetDefDispEnv(&context->environment.display, 0, 0, 0x140, height);
    SetDefDispEnv(&g_FrameContexts[1].environment.display, 0, 0, 0x140, height);

    i = 0;
    one = 1;
    src0 = &g_ScreenOffsetX.displayValue;
    src1 = &g_ScreenOffsetY.displayValue;
    offset = 0;
    do {
        stride = 0x20000;
        g_FrameContexts[i].environment.draw.dtd = one;
        g_FrameContexts[i].environment.draw.isbg = one;
        g_FrameContexts[i].environment.draw.r0 = mode;
        g_FrameContexts[i].environment.draw.g0 = x;
        g_FrameContexts[i].environment.draw.b0 = y;
        value = *src0;
        stride |= 0x37E8;
        g_FrameContexts[i].environment.display.screen.x = value;
        value2 = *src1;
        g_FrameContexts[i].environment.mirrorDraw.dtd = one;
        g_FrameContexts[i].environment.mirrorDraw.isbg = 0;
        g_FrameContexts[i].environment.mirrorDraw.r0 = mode;
        g_FrameContexts[i].environment.mirrorDraw.g0 = x;
        g_FrameContexts[i].environment.mirrorDraw.b0 = y;
        g_FrameContexts[i].environment.display.screen.y = value2 + 0x1D;
        i++;
        offset += stride;
    } while (i < 2);

    ResetFrameContext(0);
    ResetFrameContext(1);

    SCRATCH_CLIP_Y1 = 0x1E0;
}
