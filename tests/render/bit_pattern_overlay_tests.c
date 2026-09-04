#include "game/render.h"
#include "game/render_internal.h"

#include <stdio.h>
#include <string.h>

GameRenderState g_RenderState;
MenuOverlayPatternFrame
    g_MenuOverlayPatternTable[MENU_OVERLAY_PATTERN_FRAME_COUNT];
s32 g_MenuOverlayPatternAnimFrame;
s32 g_AnimTimer;

typedef struct SpriteCall {
    s16 x;
    s16 y;
} SpriteCall;

static SpriteCall s_calls[64];
static s32 s_callCount;
static s32 s_drawMode;
static GameOrderingTableEntry *s_drawModeOt;
static GameOrderingTableEntry s_ot[2];
static u8 s_packet[64];

void DrawSprite(GameOrderingTableEntry *ot, s16 x, s16 y, s16 w, u16 h, u16 u, u16 v,
                u8 r, u8 g, u8 b, u16 clut, s32 shade, s32 semi,
                u32 flags) {
    (void)ot; (void)w; (void)h; (void)u; (void)v; (void)r; (void)g;
    (void)b; (void)clut; (void)shade; (void)semi; (void)flags;
    s_calls[s_callCount].x = x;
    s_calls[s_callCount].y = y;
    s_callCount++;
}

u8 *QueueDrawModePrim(GameOrderingTableEntry *ot, u8 *prim, s32 tpage) {
    s_drawModeOt = ot;
    s_drawMode = tpage;
    return prim;
}

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "%s:%d: check failed: %s\n", \
                __FILE__, __LINE__, #condition); \
        return 1; \
    } \
} while (0)

int main(void) {
    memset(&g_RenderState, 0, sizeof(g_RenderState));
    g_RenderState.primData = s_ot;
    g_RenderState.packetCursor = s_packet;
    g_MenuOverlayPatternTable[0].rows[0] = 0x80;
    g_MenuOverlayPatternTable[0].rows[1] = 0x01;

    DrawBitPatternOverlay(0);
    CHECK(s_callCount == 0);

    DrawBitPatternOverlay(3);
    DrawBitPatternOverlay(INT32_MAX);
    CHECK(s_callCount == 0);

    DrawBitPatternOverlay(1);
    CHECK(s_callCount == 18);
    CHECK(s_calls[0].x == 0x22 && s_calls[0].y == 0x150);
    CHECK(s_calls[1].x == 0x3E && s_calls[1].y == 0x158);
    CHECK(s_calls[2].x == 0x4C && s_calls[2].y == 0x33);
    CHECK(s_calls[17].x == 0x97 && s_calls[17].y == 0x33);
    CHECK(s_drawMode == 0x39);
    CHECK(s_drawModeOt == s_ot + 1);

    s_callCount = 0;
    g_MenuOverlayPatternAnimFrame = 2;
    g_AnimTimer = 6;
    g_MenuOverlayPatternTable[3].rows[0] = 0x80;
    DrawBitPatternOverlay(-1);
    CHECK(g_MenuOverlayPatternAnimFrame == 3);
    CHECK(s_callCount == 17);
    CHECK(s_calls[0].x == 0x22 && s_calls[0].y == 0x150);

    s_callCount = 0;
    g_MenuOverlayPatternAnimFrame = 2;
    g_MenuOverlayPatternTable[3].rows[7] = 1;
    DrawBitPatternOverlay(-1);
    CHECK(g_MenuOverlayPatternAnimFrame == 2);
    CHECK(s_callCount == 16);

    s_callCount = 0;
    g_AnimTimer = 1;
    g_MenuOverlayPatternAnimFrame = -1;
    g_MenuOverlayPatternTable[2].rows[0] = 0x80;
    DrawBitPatternOverlay(-1);
    CHECK(g_MenuOverlayPatternAnimFrame == 2);
    CHECK(s_callCount == 17);

    s_callCount = 0;
    g_AnimTimer = 6;
    g_MenuOverlayPatternAnimFrame = MENU_OVERLAY_PATTERN_FRAME_COUNT - 1;
    DrawBitPatternOverlay(-1);
    CHECK(g_MenuOverlayPatternAnimFrame == 2);
    CHECK(s_callCount == 17);

    s_callCount = 0;
    g_AnimTimer = 1;
    g_MenuOverlayPatternAnimFrame = MENU_OVERLAY_PATTERN_FRAME_COUNT;
    DrawBitPatternOverlay(-1);
    CHECK(g_MenuOverlayPatternAnimFrame == 2);
    CHECK(s_callCount == 17);

    puts("bit_pattern_overlay: pattern grid and footer positions ok");
    return 0;
}
