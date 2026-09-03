#include "game/prim.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/state.h"

static s32 BootLogoFade(void) {
    if (g_SceneTimer < 0) return 0;
    if (g_SceneTimer > 0xFF) return 0xFF;
    return g_SceneTimer;
}

void DrawBootLogo(void) {
    GameOrderingTableEntry *ot;
    u8 *next;
    const s32 fade = BootLogoFade();

    if (g_DrawBuffer == NULL || g_RenderState.packetCursor == NULL) return;

    ot = GamePrimaryOrderingTable(0);
    next = RENDER_PRIM_CURSOR_AS(u8);
    next = GameQueueShadedSprite(ot, next, 0x64, 0xEC, 0x7C, 0x18, 0x80, 0,
                                 0x3F97, fade);
    next = GameQueueShadedSprite(ot, next, 0xDC, 0xC4, 8, 0x10, 0, 0x20,
                                 0x3FD7, fade);
    next = GameQueueShadedSprite(ot, next, 0x64, 0xC4, 0x78, 0x20, 0, 0,
                                 0x3FD7, fade);
    g_RenderState.packetCursor = QueueDrawModePrim(ot, next, 5);
}
