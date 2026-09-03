#include "game/prim.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/state.h"

#include <stdio.h>
#include <string.h>

s32 g_SceneTimer;
GameRenderState g_RenderState;
static GameFrameContext s_frame;
GameFrameContext *g_DrawBuffer = &s_frame;

typedef struct SpriteCall {
    s32 x;
    s32 y;
    s32 width;
    s32 height;
    s32 u;
    s32 v;
    s32 clut;
    s32 intensity;
} SpriteCall;

static SpriteCall s_calls[3];
static s32 s_callCount;
static s32 s_drawMode;

u8 *GameQueueShadedSprite(GameOrderingTableEntry *ot, u8 *packet, s32 x, s32 y, s32 width,
                          s32 height, s32 u, s32 v, s32 clut,
                          s32 intensity) {
    SpriteCall *call = &s_calls[s_callCount++];
    (void)ot;
    call->x = x;
    call->y = y;
    call->width = width;
    call->height = height;
    call->u = u;
    call->v = v;
    call->clut = clut;
    call->intensity = intensity;
    return packet + 8;
}

u8 *QueueDrawModePrim(GameOrderingTableEntry *ot, u8 *packet, s32 tpage) {
    (void)ot;
    s_drawMode = tpage;
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

static void DrawAtFade(s32 timer, u8 *packets) {
    memset(&g_RenderState, 0, sizeof(g_RenderState));
    g_RenderState.packetCursor = packets;
    g_SceneTimer = timer;
    s_callCount = 0;
    s_drawMode = -1;
    DrawBootLogo();
}

int main(void) {
    u8 packets[64];

    memset(&s_frame, 0, sizeof(s_frame));
    DrawAtFade(80, packets);
    CHECK(s_callCount == 3 && s_drawMode == 5);
    CHECK(s_calls[0].x == 0x64 && s_calls[0].y == 0xEC);
    CHECK(s_calls[0].width == 0x7C && s_calls[0].height == 0x18);
    CHECK(s_calls[0].u == 0x80 && s_calls[0].v == 0);
    CHECK(s_calls[0].clut == 0x3F97 && s_calls[0].intensity == 80);
    CHECK(s_calls[1].x == 0xDC && s_calls[1].width == 8);
    CHECK(s_calls[2].x == 0x64 && s_calls[2].width == 0x78);
    CHECK(s_calls[1].clut == 0x3FD7 && s_calls[2].clut == 0x3FD7);
    CHECK(g_RenderState.packetCursor == packets + 28);

    DrawAtFade(-1, packets);
    CHECK(s_calls[0].intensity == 0 && s_calls[2].intensity == 0);
    DrawAtFade(256, packets);
    CHECK(s_calls[0].intensity == 255 && s_calls[2].intensity == 255);

    g_RenderState.packetCursor = packets;
    g_DrawBuffer = NULL;
    s_callCount = 0;
    DrawBootLogo();
    CHECK(s_callCount == 0 && g_RenderState.packetCursor == packets);

    g_DrawBuffer = &s_frame;
    g_RenderState.packetCursor = NULL;
    DrawBootLogo();
    CHECK(s_callCount == 0 && g_RenderState.packetCursor == NULL);

    puts("boot logo tests passed");
    return 0;
}
