#include "game/render_internal.h"

typedef struct Text8x8Style {
    u8 intensity;
    u8 shadeTexture;
    u8 semiTransparent;
    u8 drawMode;
} Text8x8Style;

static void DrawText8x8Styled(s32 x, s32 y, const char *text, s32 clutIndex,
                              Text8x8Style style) {
    u8 *packet = RENDER_PRIM_CURSOR_AS(u8);
    GameOrderingTableEntry *ot = GamePrimaryOrderingTable(0);

    while (*text != '\0') {
        s32 cell = PrintableAsciiGlyph((u8)*text++);

        if (cell != 0) {
            SPRT_8 *sprite = (SPRT_8 *)packet;
            s32 fontIndex = cell * 2;

            SetSprt8(sprite);
            if (style.shadeTexture) SetShadeTex(sprite, 1);
            if (style.semiTransparent) SetSemiTrans(sprite, 1);
            if (!style.shadeTexture) {
                sprite->r0 = style.intensity;
                sprite->g0 = style.intensity;
                sprite->b0 = style.intensity;
            }
            sprite->x0 = (s16)x;
            sprite->y0 = (s16)y;
            sprite->u0 = (u8)(g_Font8x8Cells[fontIndex] * 8);
            sprite->v0 = (u8)(g_Font8x8Cells[fontIndex + 1] * 8);
            sprite->clut = (u16)clutIndex;
            AddPrim(ot, sprite);
            packet = (u8 *)(sprite + 1);
        }
        x += 8;
    }

    SetDrawMode((DrawPacket *)packet, 0, 1, style.drawMode, g_DrawModeEnv);
    AddPrim(ot, packet);
    g_RenderState.packetCursor = (DrawPacket *)packet + 1;
}

void DrawText8x8(s32 x, s32 y, const char *text, s32 clutIndex) {
    Text8x8Style style = {0, 1, 0, 9};
    DrawText8x8Styled(x, y, text, clutIndex, style);
}

void GameDrawText8x8Shaded(s32 x, s32 y, const char *text, s32 clutIndex,
                           u8 intensity) {
    Text8x8Style style = {intensity, 0, 1, 0x29};
    DrawText8x8Styled(x, y, text, clutIndex, style);
}

void DrawText8x8Trans(s32 x, s32 y, const char *text, s32 clutIndex) {
    Text8x8Style style = {0, 1, 1, 0x49};
    DrawText8x8Styled(x, y, text, clutIndex, style);
}
