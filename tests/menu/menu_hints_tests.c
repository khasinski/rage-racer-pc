#include "game/menu.h"
#include "game/render_internal.h"

#include <stdio.h>

u8 g_LastValidPadType;
u8 g_PadType;
GameFrameContext *g_DrawBuffer;
GameRenderState g_RenderState;
OptionHintCaption g_OptionHintCaptions[7];

typedef struct SpriteCall {
    void *ot;
    s32 x, y, width, height, u, v, clut;
} SpriteCall;

static GameFrameContext s_frame;
static u8 s_packets[32];
static SpriteCall s_calls[4];
static s32 s_callCount;
static void *s_drawModeOt;
static s32 s_drawMode;

u8 *GameQueueSpriteTrans(void *ot, u8 *prim, s32 x, s32 y, s32 width,
                         s32 height, s32 u, s32 v, s32 clut) {
    s_calls[s_callCount++] = (SpriteCall){ot, x, y, width, height, u, v, clut};
    return prim + 1;
}

u8 *QueueDrawModePrim(void *ot, u8 *prim, s32 texturePage) {
    s_drawModeOt = ot;
    s_drawMode = texturePage;
    return prim + 1;
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static void ResetCalls(void) {
    s_callCount = 0;
    s_drawModeOt = NULL;
    s_drawMode = 0;
    g_RenderState.packetCursor = s_packets;
}

int main(void) {
    void *ot0;
    void *ot51;

    g_DrawBuffer = &s_frame;
    ot0 = GamePrimaryOrderingTable(0);
    ot51 = GamePrimaryOrderingTable(51);

    ResetCalls();
    DrawMenuCursorArrow(12, 34);
    CHECK(s_callCount == 1 && s_calls[0].ot == ot51);
    CHECK(s_calls[0].x == 12 && s_calls[0].y == 34);
    CHECK(s_calls[0].width == 0xC && s_calls[0].height == 0x18);
    CHECK(s_calls[0].u == 0xE0 && s_calls[0].v == 0x48);
    CHECK(s_calls[0].clut == 0x7F40 && s_drawMode == 0x3F);
    CHECK(s_drawModeOt == ot51 && g_RenderState.packetCursor == s_packets + 2);

    g_OptionHintCaptions[2] = (OptionHintCaption){0x20, 0x30, 0x40, 0x44};
    ResetCalls();
    DrawOptionHintBar(2);
    CHECK(s_callCount == 3 && s_calls[0].ot == ot0);
    CHECK(s_calls[0].x == 0x70 && s_calls[0].u == 0xE0);
    CHECK(s_calls[1].x == 0x80 && s_calls[1].width == 0x40);
    CHECK(s_calls[1].u == 0x20 && s_calls[1].v == 0x30);
    CHECK(s_calls[2].x == 0xC4 && s_calls[2].u == 0xEC);
    CHECK(g_RenderState.packetCursor == s_packets + 4);

    g_OptionHintCaptions[4] = (OptionHintCaption){4, 5, 6, 8};
    ResetCalls();
    DrawOptionHintBar(4);
    CHECK(s_callCount == 4);
    CHECK(s_calls[0].x == 0x5A && s_calls[1].x == 0x6A);
    CHECK(s_calls[2].x == 0x72 && s_calls[2].width == 0x30);
    CHECK(s_calls[3].x == 0xA6);

    ResetCalls();
    g_PadType = PAD_TYPE_NEGCON;
    g_LastValidPadType = PAD_TYPE_DIGITAL;
    DrawPadTypeHint();
    CHECK(g_LastValidPadType == PAD_TYPE_NEGCON && s_callCount == 3);
    CHECK(s_calls[0].u == 0xA0 && s_calls[1].u == 0xA8);
    CHECK(s_calls[2].x == 0x58 && s_calls[2].width == 0x90);

    ResetCalls();
    g_PadType = 0;
    DrawPadTypeHint();
    CHECK(g_LastValidPadType == PAD_TYPE_NEGCON);
    CHECK(s_calls[0].u == 0xA0 && s_calls[1].u == 0xA8);

    ResetCalls();
    g_PadType = PAD_TYPE_DIGITAL;
    DrawPadTypeHint();
    CHECK(g_LastValidPadType == PAD_TYPE_DIGITAL);
    CHECK(s_calls[0].u == 0x90 && s_calls[1].u == 0x98);
    CHECK(s_drawModeOt == ot0 && s_drawMode == 0x3F);
    CHECK(g_RenderState.packetCursor == s_packets + 4);

    puts("menu hint tests passed");
    return 0;
}
