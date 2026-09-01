#include "game/menu.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/state.h"

#include <stdio.h>
#include <string.h>

s32 g_SceneTimer;
s32 g_FadeLevel;
s32 g_FadeStep;
s32 g_SceneId;
u16 g_PadPressed;
GameRenderState g_RenderState;
static GameFrameContext s_frame;
u8 *g_DrawBuffer = s_frame.bytes;

static s32 s_displayMask;
static s32 s_bannerFade;
static s32 s_spriteCount;
static s32 s_spriteX[2];
static s32 s_spriteWidth[2];
static s32 s_drawModes[2];

void SetDispMask(s32 enabled) {
    s_displayMask = enabled;
}

void DrawRaceEndBanner(s32 fade) {
    s_bannerFade = fade;
}

u8 *GameQueueSprite(void *ot, u8 *packet, s32 x, s32 y, s32 width,
                    s32 height, s32 u, s32 v, s32 clut) {
    (void)ot;
    (void)y;
    (void)height;
    (void)u;
    (void)v;
    (void)clut;
    s_spriteX[s_spriteCount] = x;
    s_spriteWidth[s_spriteCount] = width;
    s_spriteCount++;
    return packet + 8;
}

u8 *QueueDrawModePrim(void *ot, u8 *packet, s32 tpage) {
    (void)ot;
    s_drawModes[s_spriteCount - 1] = tpage;
    return packet + 4;
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__,   \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

int main(void) {
    u8 packets[64];

    g_SceneTimer = 0;
    g_FadeLevel = 0;
    g_FadeStep = 4;
    s_displayMask = -1;
    UpdateEndingStill();
    CHECK(g_SceneTimer == 1 && g_FadeLevel == 4 && s_bannerFade == 4);
    CHECK(s_displayMask == -1);
    UpdateEndingStill();
    CHECK(s_displayMask == 1);

    g_FadeLevel = 254;
    g_FadeStep = 4;
    UpdateEndingStill();
    CHECK(g_FadeLevel == 256 && g_FadeStep == 0);

    g_SceneTimer = 299;
    g_PadPressed = 0;
    UpdateEndingStill();
    CHECK(g_SceneTimer == 300 && g_FadeLevel == 256 && g_FadeStep == -4);

    g_FadeStep = 0;
    g_FadeLevel = 100;
    g_PadPressed = PAD_CONFIRM;
    UpdateEndingStill();
    CHECK(g_FadeLevel == 256 && g_FadeStep == -4);

    g_PadPressed = 0;
    g_FadeLevel = 4;
    g_FadeStep = -4;
    g_SceneId = 34;
    UpdateEndingStill();
    CHECK(g_FadeLevel == 0 && g_SceneId == 2 && s_bannerFade == 0);

    memset(&s_frame, 0, sizeof(s_frame));
    memset(&g_RenderState, 0, sizeof(g_RenderState));
    g_RenderState.packetCursor = packets;
    s_spriteCount = 0;
    DrawEndingStill();
    CHECK(s_spriteCount == 2);
    CHECK(s_spriteX[0] == 0 && s_spriteWidth[0] == 0x100);
    CHECK(s_spriteX[1] == 0x100 && s_spriteWidth[1] == 0x40);
    CHECK(s_drawModes[0] == 6 && s_drawModes[1] == 7);
    CHECK(g_RenderState.packetCursor == packets + 24);

    puts("ending still tests passed");
    return 0;
}
