#include "common.h"
#include "game/menu.h"
#include "game/render_state.h"

#include <stdio.h>
#include <limits.h>

s32 g_BrowseArrowsFade;
s32 g_BrowseArrowsPulsePhase;
s32 g_MenuAltLayout;
GameRenderState g_RenderState;

typedef struct DrawRecord {
    s32 x;
    s32 y;
    s32 intensity;
} DrawRecord;

static DrawRecord s_sprites[2];
static DrawRecord s_highlights[2];
static s32 s_spriteCount;
static s32 s_highlightCount;
static s32 s_sineAngle;

s32 rsin(s32 angle) {
    s_sineAngle = angle;
    return 4096;
}

void DrawSprite(GameOrderingTableEntry *ot, s16 x, s16 y, s16 width, u16 height, u16 textureU,
                u16 textureV, u8 r, u8 g, u8 b, u16 clut, s32 shadeTex,
                s32 semiTrans, u32 flags) {
    (void)ot;
    (void)width;
    (void)height;
    (void)textureU;
    (void)textureV;
    (void)r;
    (void)g;
    (void)b;
    (void)clut;
    (void)shadeTex;
    (void)semiTrans;
    (void)flags;
    s_sprites[s_spriteCount++] = (DrawRecord){x, y, 0};
}

void DrawSolidRect(GameOrderingTableEntry *ot, s32 x, s32 y, s32 width, s32 height, s32 r,
                   s32 g, s32 b, s32 alpha) {
    (void)ot;
    (void)width;
    (void)height;
    (void)r;
    (void)b;
    (void)alpha;
    s_highlights[s_highlightCount++] = (DrawRecord){x, y, g};
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "check failed at line %d: %s\n", __LINE__,         \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static void ResetDraws(void) {
    s_spriteCount = 0;
    s_highlightCount = 0;
}

int main(void) {
    GameOrderingTableEntry orderingTable[1] = {0};

    g_RenderState.primData = orderingTable;
    g_BrowseArrowsFade = 9;
    DrawBrowseArrows(0, 0, 1, 1);
    CHECK(g_BrowseArrowsFade == 0 && s_spriteCount == 0);

    DrawBrowseArrows(11, 0, 1, 1);
    CHECK(g_BrowseArrowsFade == 11 && s_spriteCount == 0);
    DrawBrowseArrows(1, 0, 1, 1);
    CHECK(g_BrowseArrowsFade == 12 && s_spriteCount == 2);
    CHECK(s_sprites[0].x == -26 && s_sprites[0].y == 0x119);
    CHECK(s_sprites[1].x == 472 && s_sprites[1].y == 0x119);
    CHECK(s_highlightCount == 2 && s_highlights[0].intensity == 0xFF);
    CHECK(g_BrowseArrowsPulsePhase == 0x60);

    ResetDraws();
    g_BrowseArrowsFade = 25;
    DrawBrowseArrows(1, 1, 1, 0);
    CHECK(g_BrowseArrowsFade == 25);
    CHECK(s_sprites[0].x == 72 && s_sprites[0].y == 0x144);
    CHECK(s_sprites[1].x == 287 && s_sprites[1].y == 0x144);
    CHECK(s_highlightCount == 1 && s_highlights[0].x == 72);

    ResetDraws();
    g_MenuAltLayout = 1;
    g_BrowseArrowsFade = 25;
    DrawBrowseArrows(1, 0, 1, 1);
    CHECK(s_sprites[0].x == 72 && s_sprites[0].y == 0x119);
    CHECK(s_sprites[1].x == 287 && s_sprites[1].y == 0x119);

    ResetDraws();
    g_BrowseArrowsFade = 11;
    DrawBrowseArrows(-1, 0, 1, 1);
    CHECK(g_BrowseArrowsFade == 10 && s_spriteCount == 0);

    ResetDraws();
    g_MenuAltLayout = 0;
    g_BrowseArrowsFade = INT_MAX;
    g_BrowseArrowsPulsePhase = INT_MAX;
    DrawBrowseArrows(INT_MAX, 0, 1, 1);
    CHECK(g_BrowseArrowsFade == 25 && s_spriteCount == 2);
    CHECK(g_BrowseArrowsPulsePhase == (s32)((u32)INT_MAX + 0x60u));
    g_BrowseArrowsFade = INT_MIN;
    DrawBrowseArrows(-1, 0, 1, 1);
    CHECK(g_BrowseArrowsFade == 0);

    ResetDraws();
    g_BrowseArrowsFade = 25;
    g_BrowseArrowsPulsePhase = -1;
    DrawBrowseArrows(1, 0, 1, 1);
    CHECK(s_sineAngle == 0xFFF);

    ResetDraws();
    g_RenderState.primData = NULL;
    g_BrowseArrowsFade = 11;
    g_BrowseArrowsPulsePhase = 123;
    DrawBrowseArrows(1, 0, 1, 1);
    CHECK(g_BrowseArrowsFade == 12 && g_BrowseArrowsPulsePhase == 123);
    CHECK(s_spriteCount == 0 && s_highlightCount == 0);

    puts("browse arrows tests passed");
    return 0;
}
