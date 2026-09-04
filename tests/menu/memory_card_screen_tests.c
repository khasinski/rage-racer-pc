#include "common.h"
#include "game/memcard.h"
#include "game/memcard_internal.h"
#include "game/menu.h"
#include "game/render_internal.h"
#include "game/render_state.h"

#include <stdio.h>
#include <string.h>

GameFrameContext *g_DrawBuffer;
GameRenderState g_RenderState;
s32 g_SceneTimer;
s16 g_McMessageColumnX[8];
MemoryCardMessageRow *g_McMessageRows[MEMORY_CARD_MESSAGE_COUNT];

typedef struct DrawRecord {
    GameOrderingTableEntry *ot;
    s32 x;
    s32 y;
} DrawRecord;

static GameFrameContext s_frame;
static u8 s_packets[64];
static DrawRecord s_sprites[8];
static DrawRecord s_tiles[8];
static s32 s_spriteCount;
static s32 s_tileCount;
static s32 s_shadowY[MEMORY_CARD_SAVE_SLOT_COUNT];
static s32 s_shadowCount;
static s32 s_arrowX;
static s32 s_arrowY;
static s32 s_hint;
static s32 s_padHints;
static DrawRecord s_messageSprite;
static s32 s_messageSpriteV;
static s32 s_messageSpriteCount;
static DrawRecord s_textRows[4];
static s32 s_textCount;
static s32 s_drawMode;
static s32 s_drawModeCount;

u8 *GameQueueSpriteTrans(GameOrderingTableEntry *ot, u8 *prim, s32 x, s32 y, s32 width,
                         s32 height, s32 u, s32 v, s32 clut) {
    (void)width; (void)height; (void)u; (void)v; (void)clut;
    s_sprites[s_spriteCount++] = (DrawRecord){ot, x, y};
    return prim + 1;
}

u8 *AddTilePrim(GameOrderingTableEntry *ot, u8 *prim, s32 x, s32 y,
                s32 width, s32 height,
                s32 r, s32 g, s32 b) {
    (void)width; (void)height; (void)r; (void)g; (void)b;
    s_tiles[s_tileCount++] = (DrawRecord){ot, x, y};
    return prim + 1;
}

u8 *DrawShadowedTile(GameOrderingTableEntry *ot, u8 *prim, s32 x, s32 y) {
    (void)ot; (void)x;
    s_shadowY[s_shadowCount++] = y;
    return prim + 1;
}

/* The arrow, the hints and the text queue packets of their own through the
 * shared cursor. The stubs move it so a drawer that keeps using a stale
 * pointer across them shows up as a short packet count. */
static void QueueOwnPacket(void) {
    g_RenderState.packetCursor = (void *)(RENDER_PRIM_CURSOR_AS(u8) + 1);
}

void DrawMenuCursorArrow(s32 x, s32 y) {
    s_arrowX = x;
    s_arrowY = y;
    QueueOwnPacket();
}
void DrawOptionHintBar(s32 hint) {
    s_hint = hint;
    QueueOwnPacket();
}
void DrawPadTypeHint(void) {
    s_padHints++;
    QueueOwnPacket();
}

u8 *GameQueueSprite(GameOrderingTableEntry *ot, u8 *prim, s32 x, s32 y, s32 w, s32 h,
                    s32 u, s32 v, s32 clut) {
    (void)w; (void)h; (void)u; (void)clut;
    s_messageSprite = (DrawRecord){ot, x, y};
    s_messageSpriteV = v;
    s_messageSpriteCount++;
    return prim + 1;
}
u8 *QueueDrawModePrim(GameOrderingTableEntry *ot, u8 *prim, s32 tpage) {
    (void)ot;
    s_drawMode = tpage;
    s_drawModeCount++;
    return prim + 1;
}
void DrawSpriteString(s32 x, s32 y, const char *text, s32 clut) {
    (void)text; (void)clut;
    s_textRows[s_textCount++] = (DrawRecord){NULL, (s32)x, (s32)y};
    QueueOwnPacket();
}

#define CHECK(condition) do {                                                \
    if (!(condition)) {                                                      \
        fprintf(stderr, "check failed at line %d: %s\n", __LINE__,          \
                #condition);                                                 \
        return 1;                                                            \
    }                                                                        \
} while (0)

static void Reset(void) {
    memset(&s_frame, 0, sizeof(s_frame));
    memset(&g_RenderState, 0, sizeof(g_RenderState));
    memset(s_sprites, 0, sizeof(s_sprites));
    memset(s_tiles, 0, sizeof(s_tiles));
    g_DrawBuffer = &s_frame;
    g_RenderState.packetCursor = s_packets;
    s_spriteCount = 0;
    s_tileCount = 0;
    s_shadowCount = 0;
    s_padHints = 0;
    s_messageSpriteCount = 0;
    s_textCount = 0;
    s_drawModeCount = 0;
}

int main(void) {
    MemoryCardMessageRow textRows[] = {
        {"FIRST", 2, {0}},
        {"SECOND", 0, {0}},
    };
    GameOrderingTableEntry *spriteOt;
    GameOrderingTableEntry *tileOt;

    Reset();
    spriteOt = GamePrimaryOrderingTable(51);
    tileOt = GamePrimaryOrderingTable(54);
    DrawMemoryCardScreen(0, 0, 2, 1);
    CHECK(s_spriteCount == 5);
    CHECK(s_sprites[0].ot == spriteOt && s_sprites[0].x == 0x24 &&
          s_sprites[0].y == 0x38);
    CHECK(s_sprites[1].y == 0x58);
    CHECK(s_tileCount == 3);
    CHECK(s_tiles[0].ot == tileOt && s_tiles[0].x == 0x5D);
    CHECK(s_shadowCount == MEMORY_CARD_SAVE_SLOT_COUNT);
    CHECK(s_shadowY[0] == 0xD0 && s_shadowY[1] == 0x100 &&
          s_shadowY[2] == 0x130);
    CHECK(s_arrowX == 0x14 && s_arrowY == 0x78);
    CHECK(s_hint == 5 && s_padHints == 1);
    /* Five sprites, three hint packets, three tiles, three shadows. */
    CHECK(g_RenderState.packetCursor == s_packets + 14);

    Reset();
    DrawMemoryCardScreen(1, 1, 1, 2);
    CHECK(s_spriteCount == 6);
    CHECK(s_sprites[1].y == 0x58 && s_sprites[2].y == 0x78);
    CHECK(s_tileCount == 4);
    CHECK(s_tiles[2].x == 0x3C && s_tiles[2].y == 0x12C);
    CHECK(s_arrowY == 0x58 && s_hint == 6 && s_padHints == 1);
    CHECK(g_RenderState.packetCursor == s_packets + 16);

    g_McMessageColumnX[2] = 42;
    for (s32 message = 6; message <= 13; message++) {
        g_McMessageRows[message] = textRows;
        Reset();
        DrawMemoryCardMessage(message);
        CHECK(s_textCount == 2);
        CHECK(s_textRows[0].x == 0x60 && s_textRows[0].y == 0x40);
        CHECK(s_textRows[1].x == 42 && s_textRows[1].y == 0x60);
        CHECK(s_messageSpriteCount == 1);
        CHECK(s_messageSprite.x == ((message & 1) != 0 ? 0xAC : 0xDE));
        CHECK(s_drawModeCount == 1 && s_drawMode == 0x3D);
        /* The icon and the draw mode follow the two text packets. */
        CHECK(g_RenderState.packetCursor == s_packets + 4);
    }

    g_McMessageRows[5] = textRows;
    Reset();
    g_SceneTimer = 0;
    DrawMemoryCardMessage(5);
    CHECK(s_messageSpriteCount == 0);
    Reset();
    g_SceneTimer = 0x10;
    DrawMemoryCardMessage(5);
    CHECK(s_messageSpriteCount == 1 && s_messageSprite.x == 0x108);

    for (s32 message = 0x10; message <= 0x12; message++) {
        g_McMessageRows[message] = textRows;
        Reset();
        DrawMemoryCardMessage(message);
        CHECK(s_textCount == 0);
        CHECK(s_messageSpriteCount == 1);
        CHECK(s_messageSpriteV == (message - 0x10) * 0x18);
        CHECK(s_drawModeCount == 1 && s_drawMode == 0x3F);
        CHECK(g_RenderState.packetCursor == s_packets + 2);
    }

    g_McMessageRows[MC_PROMPT_NO_FILE - 1] = textRows;
    Reset();
    DrawMemoryCardMessage(MC_PROMPT_NO_FILE - 1);
    CHECK(s_textCount == 2);
    CHECK(s_textRows[0].x == 0x60 && s_textRows[0].y == 0x40);
    CHECK(s_textRows[1].x == 42 && s_textRows[1].y == 0x60);
    CHECK(s_messageSpriteCount == 0);
    CHECK(s_drawModeCount == 1 && s_drawMode == 0x3D);

    puts("memory card screen and message drawing preserved");
    return 0;
}
