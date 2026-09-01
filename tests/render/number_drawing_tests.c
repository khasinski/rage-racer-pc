#include "game/render.h"
#include "game/render_internal.h"

#include <stdio.h>
#include <string.h>

GameRenderState g_RenderState;
FontGlyph g_SmallFontGlyphs[64];
FontGlyph g_LargeFontGlyphs[64];
u8 g_MenuOverlayPatternTable[64];
s32 g_MenuOverlayPatternAnimOffset;
s32 g_AnimTimer;

typedef struct SpriteCall {
    void *ot;
    s16 x;
    s16 y;
    s16 width;
    u16 height;
    u16 textureU;
    u16 textureV;
    u8 red;
    u8 green;
    u8 blue;
    u16 clut;
} SpriteCall;

static SpriteCall s_calls[10];
static s32 s_callCount;
static void *s_drawModeOt;
static s32 s_drawMode;
static OT_TYPE s_ot[2];
static u8 s_packet[64];

void DrawSprite(void *ot, s16 x, s16 y, s16 width, u16 height, u16 textureU,
                u16 textureV, u8 red, u8 green, u8 blue, u16 clut, s32 shade,
                s32 semi, u32 flags) {
    SpriteCall *call = &s_calls[s_callCount++];
    (void)shade;
    (void)semi;
    (void)flags;
    call->ot = ot;
    call->x = x;
    call->y = y;
    call->width = width;
    call->height = height;
    call->textureU = textureU;
    call->textureV = textureV;
    call->red = red;
    call->green = green;
    call->blue = blue;
    call->clut = clut;
}

u8 *QueueDrawModePrim(void *ot, u8 *prim, s32 tpage) {
    s_drawModeOt = ot;
    s_drawMode = tpage;
    return prim;
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__,   \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static void ResetCalls(void) {
    memset(s_calls, 0, sizeof(s_calls));
    s_callCount = 0;
    s_drawModeOt = NULL;
    s_drawMode = -1;
}

int main(void) {
    memset(&g_RenderState, 0, sizeof(g_RenderState));
    g_RenderState.primData = s_ot;
    g_RenderState.packetCursor = s_packet;

    CHECK(GameDrawNumber(10, 20, 0, 407, 1, 2, 3, 4, 5) == 3);
    CHECK(s_callCount == 3);
    CHECK(s_calls[0].ot == s_ot && s_calls[0].x == 10);
    CHECK(s_calls[0].y == 20 && s_calls[0].width == 6);
    CHECK(s_calls[0].height == 12 && s_calls[0].textureU == 24);
    CHECK(s_calls[0].textureV == 0);
    CHECK(s_calls[1].x == 16 && s_calls[1].textureU == 0);
    CHECK(s_calls[2].x == 22 && s_calls[2].textureU == 42);
    CHECK(s_calls[2].red == 1 && s_calls[2].green == 2 && s_calls[2].blue == 3);
    CHECK(s_calls[2].clut == 4);
    CHECK(s_drawModeOt == s_ot && s_drawMode == 32);
    CHECK(g_RenderState.packetCursor == s_packet);

    ResetCalls();
    CHECK(GameDrawNumber(4, 5,
                         DRAW_NUMBER_LARGE_DIGITS | DRAW_NUMBER_OVERLAY_LAYER,
                         0, 6, 7, 8, 9, 10) == 1);
    CHECK(s_callCount == 1);
    CHECK(s_calls[0].ot == s_ot + 1 && s_calls[0].width == 8);
    CHECK(s_calls[0].height == 16 && s_calls[0].textureV == 0x18);
    CHECK(s_drawModeOt == s_ot + 1 && s_drawMode == 37);

    ResetCalls();
    CHECK(GameDrawNumber(0, 0,
                         DRAW_NUMBER_TEN_DIGIT_FIELD |
                             DRAW_NUMBER_ALT_DIGIT_ATLAS,
                         12, 0, 0, 0, 0, 0) == 2);
    CHECK(s_callCount == 2);
    CHECK(s_calls[0].x == 64 && s_calls[0].textureU == 8);
    CHECK(s_calls[0].textureV == 0xDC);
    CHECK(s_calls[1].x == 72 && s_calls[1].textureU == 16);

    puts("number drawing tests passed");
    return 0;
}
