#include "common.h"
#include "game/menu.h"
#include "game/render_internal.h"
#include "game/render_state.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

GameFrameContext g_FrameContexts[2];
ScreenOffset g_ScreenOffsetX;
ScreenOffset g_ScreenOffsetY;
GameRenderState g_RenderState;

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "check failed at line %d: %s\n", __LINE__,         \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static int CheckColors(s32 red, s32 green, s32 blue) {
    s32 i;

    for (i = 0; i < 2; i++) {
        GameFrameEnvironmentHeader *environment =
            &g_FrameContexts[i].environment;

        CHECK(environment->draw.dtd == 1);
        CHECK(environment->draw.isbg == 1);
        CHECK(environment->draw.r0 == red);
        CHECK(environment->draw.g0 == green);
        CHECK(environment->draw.b0 == blue);
        CHECK(environment->mirrorDraw.dtd == 1);
        CHECK(environment->mirrorDraw.isbg == 0);
        CHECK(environment->mirrorDraw.r0 == red);
        CHECK(environment->mirrorDraw.g0 == green);
        CHECK(environment->mirrorDraw.b0 == blue);
        CHECK(environment->display.screen.x == 7);
        CHECK(environment->display.screen.y == 40);
    }
    return 1;
}

int main(void) {
    memset(g_FrameContexts, 0, sizeof(g_FrameContexts));
    g_ScreenOffsetX = 7;
    g_ScreenOffsetY = 11;

    g_RenderState.y1 = -1;
    SetupDisplay240(10, 20, 30);
    CHECK(g_FrameContexts[0].environment.draw.clip.y == 0);
    CHECK(g_FrameContexts[1].environment.draw.clip.y == 240);
    CHECK(g_FrameContexts[0].environment.draw.clip.w == 320);
    CHECK(g_FrameContexts[0].environment.draw.clip.h == 240);
    CHECK(g_FrameContexts[0].environment.display.disp.y == 240);
    CHECK(g_FrameContexts[1].environment.display.disp.y == 0);
    CHECK(g_FrameContexts[0].environment.mirrorDraw.clip.x == 0x56);
    CHECK(g_FrameContexts[0].environment.mirrorDraw.clip.y == 0x12);
    CHECK(g_FrameContexts[1].environment.mirrorDraw.clip.y == 0x102);
    CHECK(g_FrameContexts[0].environment.mirrorDraw.clip.w == 0x94);
    CHECK(g_FrameContexts[0].environment.mirrorDraw.clip.h == 0x24);
    CHECK(CheckColors(10, 20, 30));
    CHECK(g_RenderState.y1 == 240);

    /* 480-line setup intentionally keeps the mirror rectangles established
     * by the 240-line setup; only their flags and clear colours change. */
    SetupDisplay480(40, 50, 60);
    CHECK(g_FrameContexts[0].environment.draw.clip.y == 0);
    CHECK(g_FrameContexts[1].environment.draw.clip.y == 0);
    CHECK(g_FrameContexts[0].environment.draw.clip.h == 480);
    CHECK(g_FrameContexts[1].environment.display.disp.h == 480);
    CHECK(g_FrameContexts[0].environment.mirrorDraw.clip.y == 0x12);
    CHECK(g_FrameContexts[1].environment.mirrorDraw.clip.y == 0x102);
    CHECK(CheckColors(40, 50, 60));
    CHECK(g_RenderState.y1 == 480);

    SetupDisplay240(70, 80, 90);
    CHECK(g_RenderState.y1 == 240);

    g_ScreenOffsetX = -7;
    g_ScreenOffsetY = -11;
    SetupDisplay240(0, 0, 0);
    CHECK(g_FrameContexts[0].environment.display.screen.x == -7);
    CHECK(g_FrameContexts[0].environment.display.screen.y == 18);

    g_ScreenOffsetX = INT_MAX;
    g_ScreenOffsetY = INT_MIN;
    SetupDisplay240(0, 0, 0);
    CHECK(g_FrameContexts[0].environment.display.screen.x == -1);
    CHECK(g_FrameContexts[0].environment.display.screen.y == 29);

    puts("display setup configures both frame environments consistently");
    return 0;
}
