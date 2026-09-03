#include "game/menu.h"
#include "game/prim.h"
#include "game/render_internal.h"

enum {
    DISPLAY_WIDTH = 320,
    DISPLAY_240_HEIGHT = 240,
    DISPLAY_480_HEIGHT = 480,
    DISPLAY_SCREEN_Y_OFFSET = 29,
    FRAME_CONTEXT_COUNT = 2,
};

static void ConfigureFrameColors(s32 red, s32 green, s32 blue) {
    s32 i;

    for (i = 0; i < FRAME_CONTEXT_COUNT; i++) {
        GameFrameEnvironmentHeader *environment =
            &g_FrameContexts[i].environment;

        environment->draw.dtd = 1;
        environment->draw.isbg = 1;
        environment->draw.r0 = red;
        environment->draw.g0 = green;
        environment->draw.b0 = blue;
        environment->mirrorDraw.dtd = 1;
        environment->mirrorDraw.isbg = 0;
        environment->mirrorDraw.r0 = red;
        environment->mirrorDraw.g0 = green;
        environment->mirrorDraw.b0 = blue;
        environment->display.screen.x =
            WrapSigned16(g_ScreenOffsetX);
        environment->display.screen.y = WrapSigned16(
            (int64_t)g_ScreenOffsetY + DISPLAY_SCREEN_Y_OFFSET);
    }
}

void SetupDisplay240(s32 red, s32 green, s32 blue) {
    SetGeomOffset(DISPLAY_WIDTH / 2, DISPLAY_240_HEIGHT / 2);
    SetGeomScreen(DISPLAY_WIDTH);

    SetDefDrawEnv(&g_FrameContexts[0].environment.draw, 0, 0, DISPLAY_WIDTH,
                  DISPLAY_240_HEIGHT);
    SetDefDrawEnv(&g_FrameContexts[1].environment.draw, 0,
                  DISPLAY_240_HEIGHT, DISPLAY_WIDTH, DISPLAY_240_HEIGHT);
    SetDefDispEnv(&g_FrameContexts[0].environment.display, 0,
                  DISPLAY_240_HEIGHT, DISPLAY_WIDTH, DISPLAY_240_HEIGHT);
    SetDefDispEnv(&g_FrameContexts[1].environment.display, 0, 0,
                  DISPLAY_WIDTH, DISPLAY_240_HEIGHT);

    SetDefDrawEnv(&g_FrameContexts[0].environment.mirrorDraw, 0x56, 0x12,
                  0x94, 0x24);
    SetDefDrawEnv(&g_FrameContexts[1].environment.mirrorDraw, 0x56, 0x102,
                  0x94, 0x24);
    ConfigureFrameColors(red, green, blue);
    g_RenderState.y1 = DISPLAY_240_HEIGHT;
}

void SetupDisplay480(s32 red, s32 green, s32 blue) {
    SetGeomOffset(DISPLAY_WIDTH / 2, DISPLAY_480_HEIGHT / 2);
    SetGeomScreen(DISPLAY_WIDTH);

    SetDefDrawEnv(&g_FrameContexts[0].environment.draw, 0, 0, DISPLAY_WIDTH,
                  DISPLAY_480_HEIGHT);
    SetDefDrawEnv(&g_FrameContexts[1].environment.draw, 0, 0, DISPLAY_WIDTH,
                  DISPLAY_480_HEIGHT);
    SetDefDispEnv(&g_FrameContexts[0].environment.display, 0, 0,
                  DISPLAY_WIDTH, DISPLAY_480_HEIGHT);
    SetDefDispEnv(&g_FrameContexts[1].environment.display, 0, 0,
                  DISPLAY_WIDTH, DISPLAY_480_HEIGHT);

    ConfigureFrameColors(red, green, blue);
    g_RenderState.y1 = DISPLAY_480_HEIGHT;
}
