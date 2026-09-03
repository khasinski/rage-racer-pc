#include "common.h"
#include "game/menu.h"
#include "game/render_state.h"

#include <stdio.h>
#include <string.h>

s32 g_LogoSamplePanelSlide;
s32 g_TeamNameEntrySlide;
s32 g_TeamNameCursorPhase;
u8 g_TeamNameLength;
u8 g_TeamNameChars[16];
u16 g_TeamLogoSwatches[15];
GameRenderState g_RenderState;

typedef struct SpriteRecord {
    s16 x;
    s16 y;
    u16 height;
    u16 u;
    u16 v;
    u32 flags;
} SpriteRecord;

typedef struct RectRecord {
    s32 x;
    s32 y;
    s32 r;
    s32 g;
    s32 b;
} RectRecord;

static SpriteRecord s_sprites[64];
static RectRecord s_rects[16];
static s32 s_spriteCount;
static s32 s_rectCount;
static s32 s_outlineCount;

s32 rsin(s32 angle) {
    return angle == 0 ? 4096 : 0;
}

void DrawSprite(GameOrderingTableEntry *ot, s16 x, s16 y, s16 width, u16 height, u16 u,
                u16 v, u8 r, u8 g, u8 b, u16 clut, s32 shade,
                s32 semiTrans, u32 flags) {
    (void)ot;
    (void)width;
    (void)r;
    (void)g;
    (void)b;
    (void)clut;
    (void)shade;
    (void)semiTrans;
    s_sprites[s_spriteCount++] = (SpriteRecord){x, y, height, u, v, flags};
}

void DrawSolidRect(GameOrderingTableEntry *ot, s32 x, s32 y, s32 width, s32 height, s32 r,
                   s32 g, s32 b, s32 alpha) {
    (void)ot;
    (void)width;
    (void)height;
    (void)alpha;
    s_rects[s_rectCount++] = (RectRecord){x, y, r, g, b};
}

void DrawRectOutline(void *ot, s32 x, s32 y, s32 width, s32 height, u8 r,
                     u8 g, u8 b, u8 alpha) {
    (void)ot;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)r;
    (void)g;
    (void)b;
    (void)alpha;
    s_outlineCount++;
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
    s_rectCount = 0;
    s_outlineCount = 0;
}

int main(void) {
    g_TeamLogoSwatches[0] = 0x7FFF;
    DrawLogoSamplePanel(0, 0);
    CHECK(g_LogoSamplePanelSlide == 0 && s_spriteCount == 0);

    DrawLogoSamplePanel(1, 27);
    CHECK(g_LogoSamplePanelSlide == 1);
    CHECK(s_spriteCount == 5 && s_sprites[0].y == 494);
    CHECK(s_sprites[0].u == 16 && s_sprites[1].u == 56);
    CHECK(s_rectCount == 15 && s_rects[0].x == 0x8B);
    CHECK(s_rects[0].r == 248 && s_rects[0].g == 248 &&
          s_rects[0].b == 248);
    CHECK(s_outlineCount == 1);

    ResetDraws();
    g_LogoSamplePanelSlide = 5;
    DrawLogoSamplePanel(1, 1);
    CHECK(g_LogoSamplePanelSlide == 5 && s_sprites[0].y == 344);

    ResetDraws();
    g_TeamNameLength = 3;
    g_TeamNameChars[0] = 0;
    g_TeamNameChars[1] = 10;
    g_TeamNameChars[2] = 11;
    g_TeamNameEntrySlide = 25;
    g_TeamNameCursorPhase = 0;
    DrawTeamNameEntry(1, 12);

    CHECK(g_TeamNameEntrySlide == 25);
    CHECK(s_spriteCount == 50);
    CHECK(s_rectCount == 1 && s_rects[0].x == 0x60 && s_rects[0].y == 0xFD);
    CHECK(s_rects[0].g == -1);
    CHECK(s_sprites[0].x == 0x77 && s_sprites[0].y == 0x7D);
    CHECK(s_sprites[1].x == 0x62 && s_sprites[1].y == 0x101);
    CHECK(s_sprites[1].u == 0x58 && s_sprites[1].v == 0x18);
    CHECK(s_sprites[2].x == 0x56 && s_sprites[2].u == 0);
    CHECK(s_sprites[12].x == 0x56 && s_sprites[12].y == 0x101);
    CHECK(s_sprites[48].flags == 0x3B && s_sprites[49].flags == 0x3B);
    CHECK(g_TeamNameCursorPhase == 0x60);

    ResetDraws();
    memset(g_TeamNameChars, 1, sizeof(g_TeamNameChars));
    g_TeamNameLength = 0xFF;
    DrawTeamNameEntry(1, -1);
    CHECK(s_rectCount == 1);
    CHECK(s_rects[0].x == 0x54 && s_rects[0].y == 0xE5);
    CHECK(s_spriteCount == 53);

    ResetDraws();
    DrawTeamNameEntry(0, 0);
    CHECK(g_TeamNameEntrySlide == 0 && s_spriteCount == 0);

    puts("logo and team name panels preserve their layout and animation");
    return 0;
}
