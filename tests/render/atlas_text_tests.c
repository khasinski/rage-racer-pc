#include "game/render.h"
#include "game/render_internal.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

GameRenderState g_RenderState;
FontGlyph g_SmallFontGlyphs[SMALL_FONT_GLYPH_COUNT];
FontGlyph g_LargeFontGlyphs[LARGE_FONT_GLYPH_COUNT];

typedef struct SpriteCall {
    GameOrderingTableEntry *ot;
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

static SpriteCall s_calls[16];
static s32 s_callCount;
static GameOrderingTableEntry *s_drawModeOt;
static s32 s_drawMode;
static GameOrderingTableEntry s_ot[2];
static u8 s_packet[64];

void DrawSprite(GameOrderingTableEntry *ot, s16 x, s16 y, s16 width,
                u16 height, u16 textureU, u16 textureV, u8 red, u8 green,
                u8 blue, u16 clut, s32 shade, s32 semi, u32 flags) {
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

u8 *QueueDrawModePrim(GameOrderingTableEntry *ot, u8 *primitive, s32 tpage) {
    s_drawModeOt = ot;
    s_drawMode = tpage;
    return primitive;
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
    memset(s_calls, 0, sizeof(s_calls));
    s_callCount = 0;
    s_drawModeOt = NULL;
    s_drawMode = -1;
}

static void InitGlyphs(FontGlyph *fontData, s32 glyphCount) {
    s32 index;

    for (index = 0; index < glyphCount; index++) {
        fontData[index] = (FontGlyph){
            .u = (u8)(index + 1),
            .v = (u8)(index + 2),
            .width = (u16)(index % 3 + 3),
        };
    }
}

int main(void) {
    static const char smallText[] = {'0', ' ', 'A', '/', (char)0x81,
                                     (char)0x9b, '#', '\0'};
    static const char truncatedSmallText[] = {'1', (char)0x81, '\0'};

    memset(&g_RenderState, 0, sizeof(g_RenderState));
    g_RenderState.primData = s_ot;
    g_RenderState.packetCursor = s_packet;
    InitGlyphs(g_SmallFontGlyphs, SMALL_FONT_GLYPH_COUNT);
    InitGlyphs(g_LargeFontGlyphs, LARGE_FONT_GLYPH_COUNT);

    DrawSmallText(10, 20, smallText, 1, 2, 3, 4, 5);
    CHECK(s_callCount == 4);
    CHECK(s_calls[0].ot == s_ot + 1 && s_calls[0].x == 10);
    CHECK(s_calls[0].width == 3 && s_calls[0].height == 12);
    CHECK(s_calls[0].textureU == 1 && s_calls[0].textureV == 2);
    CHECK(s_calls[1].x == 19 && s_calls[1].textureU == 11);
    CHECK(s_calls[2].x == 23 && s_calls[2].textureU == 0x25);
    CHECK(s_calls[3].x == 26 && s_calls[3].textureU == 0x2b);
    CHECK(s_calls[3].red == 1 && s_calls[3].green == 2);
    CHECK(s_calls[3].blue == 3 && s_calls[3].clut == 4);
    CHECK(s_drawModeOt == s_ot + 1 && s_drawMode == 32);

    ResetCalls();
    DrawSmallText(0, 0, truncatedSmallText, 0, 0, 0, 0, 0x80 | 7);
    CHECK(s_callCount == 1);
    CHECK(s_calls[0].width == 6 && s_calls[0].textureU == 6);
    CHECK(s_calls[0].textureV == 0);
    CHECK(s_drawMode == 34);

    ResetCalls();
    DrawLargeText(5, 6, "0 A:@#", 7, 8, 9, 10, 0x80 | 2);
    CHECK(s_callCount == 4);
    CHECK(s_calls[0].x == 5 && s_calls[0].width == 8);
    CHECK(s_calls[0].height == 16 && s_calls[0].textureU == 0);
    CHECK(s_calls[0].textureV == 24);
    CHECK(s_calls[1].x == 21 && s_calls[1].textureU == 80);
    CHECK(s_calls[2].x == 29 && s_calls[2].textureU == 128);
    CHECK(s_calls[2].textureV == 40);
    CHECK(s_calls[3].x == 37 && s_calls[3].textureU == 64);
    CHECK(s_drawMode == 29);

    ResetCalls();
    DrawSmallText(INT_MAX, 0, "00", 0, 0, 0, 0,
                  DRAW_ATLAS_TEXT_FIXED_WIDTH);
    CHECK(s_callCount == 2);
    CHECK(s_calls[0].x == -1 && s_calls[1].x == 5);

    ResetCalls();
    DrawLargeText(INT_MAX, 0, "00", 0, 0, 0, 0,
                  DRAW_ATLAS_TEXT_FIXED_WIDTH);
    CHECK(s_callCount == 2);
    CHECK(s_calls[0].x == -1 && s_calls[1].x == 7);

    puts("atlas text tests passed");
    return 0;
}
